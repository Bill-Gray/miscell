#define CURL_STATICLIB
#include <stdio.h>
#include <curl/curl.h>
#include <curl/easy.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

/* Code to download MPEC headers for a given year to create an index.  See

http://projectpluto.com/mpecs/2017.htm

   for an example.  This code may be of no real interest to anyone except
me;  everybody else can probably just use the indices generated with the
code as provided at the above URL.  Unless I drop dead and somebody else
wants to update those indices.

   When run with the (four-digit) year as a command line arguments,  the
code looks through the _existing_ 'YYYY.htm' file to find the last MPEC in
it.   Let's say you're running it for 2017,  and the last MPEC listed in
'2017.htm' is 2017-C42;  the code will grab the first 20000 bytes of the
assumed next MPEC,  2017-C43,  and get a summary for it. Then for C44,
and so on.

   Eventually,  this will fail to access anything,  and the code looks for
2017-D01,  D02, ...

   The revised index file is written to 'temp.htm'.  If everything goes
without error,  'yyyy.htm' is unlinked and replaced by 'temp.htm'.  If a
failure occurs (MPC site unavailable,  for example),  the original 'yyyy.htm'
is unharmed.

   If you run the code frequently,  it'll usually just access a few recent
MPECs and fail when it tries to get the first MPEC of the next half-month.
If you haven't run it for four months,  though,  it'll get data for eight
half-months.         */

size_t total_written;

static size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    size_t written;

    written = fwrite(ptr, size, nmemb, stream);
    total_written += written;
    return written;
}

static int grab_file( const char *url, const char *outfilename,
                                    const bool append)
{
    CURL *curl = curl_easy_init();

    assert( curl);
    if (curl) {
        FILE *fp = fopen( outfilename, (append ? "ab" : "wb"));

        assert( fp);
        curl_easy_setopt( curl, CURLOPT_URL, url);
        curl_easy_setopt( curl, CURLOPT_WRITEFUNCTION, write_data);
        curl_easy_setopt( curl, CURLOPT_WRITEDATA, fp);
        curl_easy_setopt( curl, CURLOPT_RANGE, "0-20000");
        CURLcode res = curl_easy_perform(curl);
        if( res)
           printf( "res = %d\n", res);
        assert( !res);
        curl_easy_cleanup(curl);
        fclose(fp);
    }
    return 0;
}


static int grab_mpec( FILE *ofile, const char *year, const char half_month, const int mpec_no)
{
   char buff[200], url[200], *tptr;
   FILE *ifile;
   int i, rval = -1, found_name = 0;
   double semimajor_axis = 0.;
   double eccentricity = 0.;
   double perihelion_dist = 0.;
   double earth_moid = 0.;

   sprintf( url, "http://www.minorplanetcenter.net/mpec/%s/%s%cx%d.html",
                        year, year, half_month, mpec_no % 10);
   if( mpec_no < 100)
      url[46] = '0' + mpec_no / 10;
   else
      url[46] = 'A' + mpec_no / 10 - 10;
   grab_file( url, "zz", 0);
   ifile = fopen( "zz", "rb");
   assert( ifile);
   while( rval && fgets( buff, sizeof( buff), ifile))
      if( !memcmp( buff, "<h2>", 4))
         {
         tptr = strstr( buff, "</h2>");
         assert( tptr);
         assert( !found_name);
         if( mpec_no == 1)
            fprintf( ofile, "<br>\n<a name=\"%c\"> </a>\n", half_month);
         *tptr = '\0';
         fprintf( ofile, "<a href=\"%s\"> %s </a>", url, buff + 4);
         printf( "%s\n", buff + 4);
         found_name = 1;
         }
      else if( (tptr = strstr( buff, "Issued")) != NULL)
         {
         char *ut = strstr( tptr + 6, " UT");

         assert( found_name);
                  /* MPEC 2000-G03 is damaged,  needs this repair : */
         if( !ut && !memcmp( tptr, "Issued 2000 Apr.  2.298, 07:09", 30))
            strcpy( tptr, "Issued 2000 Apr. 2,  07:09");
         else
            {
            assert( ut);
            *ut = '\0';
            }
         fprintf( ofile, "%s", tptr + 6);
         rval = 0;
         }
   if( !rval)
      while( fgets( buff, sizeof( buff), ifile))
         if( !memcmp( buff, "Orbital elements:", 17))
            {
            int n_written = 0, j;

            for( i = 0; i < 9 && fgets( buff, sizeof( buff), ifile); i++)
               {
               tptr = strstr( buff, "MOID</a>");
               if( tptr)
                  earth_moid = atof( tptr + 11);
               if( strchr( "aeq", *buff) && buff[1] == ' ')
                  {
                  j = 1;
                  while( buff[j] == ' ')
                     j++;
                  n_written += fprintf( ofile, " %s%c=%.5s",
                              (n_written ? "" : "("), *buff, buff + j);
                  if( *buff == 'a')
                     semimajor_axis = atof( buff + j);
                  if( *buff == 'e')
                     eccentricity = atof( buff + j);
                  if( *buff == 'q')
                     perihelion_dist = atof( buff + j);
                  }
               if( !memcmp( buff + 19, "Incl.", 5))
                  n_written += fprintf( ofile, " %si=%.5s",
                              (n_written ? "" : "("), buff + 26);
               if( !memcmp( buff + 19, "H   ", 4))
                  n_written += fprintf( ofile, " %sH=%.4s",
                              (n_written ? "" : "("), buff + 23);
               memset( buff, 0, sizeof( buff));
               }
            if( semimajor_axis && eccentricity && !perihelion_dist)
               {
               perihelion_dist = semimajor_axis * (1. - eccentricity);
               fprintf( ofile, " q=%.3f", perihelion_dist);
               }
            if( n_written && earth_moid)
               fprintf( ofile, " MOID=%.4f", earth_moid);
            if( n_written)
               fprintf( ofile, ")");
            fseek( ifile, 0L, SEEK_END);
            }
   if( !rval)
      fprintf( ofile, "<br>\n");
   fclose( ifile);
   return( rval);
}

/* Used in situations where failure to open a file is a fatal error */

static FILE *err_fopen( const char *filename, const char *permits)
{
   FILE *rval = fopen( filename, permits);

   if( !rval)
      {
      printf( "Couldn't open %s\n", filename);
      perror( NULL);
      exit( -1);
      }
   return( rval);
}

int main( const int argc, const char **argv)
{
   int half_month = 'A', mpec_no = 1, year;
   FILE *ifile, *ofile;
   char filename[100];
   char buff[200];
   char mpcized_year[10];
   const char *search_str = "<a href=\"http://www.minorplanetcenter.net/mpec/";
   const char *end_marker = "<a name=\"the_end\"> </a>";
   const char *temp_file_name = "temp.htm";
   bool found_end = false;

   if( argc != 2)
      {
      printf( "'mpecer' needs the (four-digit) year as a command line argument\n");
      return( -1);
      }
   year = atoi( argv[1]);
   if( year < 1993 || year > 2100)
      {
      printf( "Invalid year\n");
      return( -2);
      }
   sprintf( filename, "%s.htm", argv[1]);
   ifile = err_fopen( filename, "rb");
   ofile = err_fopen( temp_file_name, "wb");
   while( fgets( buff, sizeof( buff), ifile)
            && !(found_end = !memcmp( buff, end_marker, strlen( end_marker))))
      {
      fputs( buff, ofile);
      if( !memcmp( buff, search_str, strlen( search_str)))
         {
         half_month = buff[54];
         if( buff[55] >= 'a')
            mpec_no = (buff[55] - 'a' + 36) * 10;
         else if( buff[55] >= 'A')
            mpec_no = (buff[55] - 'A' + 10) * 10;
         else
            mpec_no = (buff[55] - '0') * 10;
         mpec_no += buff[56] - '0';

         mpec_no++;     /* we're looking for the _next_ MPEC */
         }
      }
   assert( found_end);
   sprintf( mpcized_year, "%c%02d", 'A' + year / 100 - 10, year % 100);
   for( ; half_month <= 'Y'; half_month++)
      if( half_month != 'I')
         {
         while( !grab_mpec( ofile, mpcized_year, half_month, mpec_no))
            mpec_no++;
         if( mpec_no == 1)         /* didn't find anything for this  */
            half_month = 'Y';       /* half-month; we're done */
         mpec_no = 1;
         }
   fprintf( ofile, "%s\n", end_marker);
   while( fgets( buff, sizeof( buff), ifile))
      fputs( buff, ofile);
   fclose( ifile);
   fclose( ofile);
   unlink( filename);
   rename( temp_file_name, filename);
   return( 0);
}
