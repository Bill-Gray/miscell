#define CURL_STATICLIB
#include <stdio.h>
#include <curl/curl.h>
#include <curl/easy.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>

/* Code to download astrometry for a given object from MPC.  Run as

./grab_mpc filename object name [-a]

-a = append astrometry to file rather than overwriting the file. */

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
   int rval = -1, found_name = 0;

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
         fprintf( ofile, "%s <br>\n", tptr + 6);
         rval = 0;
         }
   fclose( ifile);
   return( rval);
}

const char *month_links =
                  "<TABLE BORDER=1 >\n"
                  "<TR>\n"
                  "<TD><a href=\"#A\">  Jan </a></TD>\n"
                  "<TD><a href=\"#C\">  Feb </a></TD>\n"
                  "<TD><a href=\"#E\">  Mar </a></TD>\n"
                  "<TD><a href=\"#G\">  Apr </a></TD>\n"
                  "<TD><a href=\"#J\">  May </a></TD>\n"
                  "<TD><a href=\"#L\">  Jun </a></TD>\n"
                  "</TR>\n"
                  "<TR>\n"
                  "<TD><a href=\"#N\">  Jul </a></TD>\n"
                  "<TD><a href=\"#P\">  Aug </a></TD>\n"
                  "<TD><a href=\"#R\">  Sep </a></TD>\n"
                  "<TD><a href=\"#T\">  Oct </a></TD>\n"
                  "<TD><a href=\"#V\">  Nov </a></TD>\n"
                  "<TD><a href=\"#X\">  Dec </a></TD>\n"
                  "</TR>\n"
                  "\n"
                  "</TABLE>\n"
                  "\n"
                  "<p>\n";

int main( const int argc, char **argv)
{
   int half_month;
   FILE *ofile = fopen( argv[1], "wb");
   int start = 'A', end = 'Y';

   if( argc > 3)
      {
      start = argv[3][0];
      end   = argv[3][1];
      }

   fprintf( ofile, "%s", month_links);
   for( half_month = start; half_month <= end; half_month++)
      if( half_month != 'I')
         {
         int i = 1;

         fprintf( ofile, "<a name=\"%c\"> </a>\n", half_month);
         while( !grab_mpec( ofile, argv[2], half_month, i))
            i++;
         if( i > 1)
            fprintf( ofile, "<br>\n");
         }
   fprintf( ofile, "</p> </body> </html>\n");
   fclose( ofile);
   return( 0);
}
