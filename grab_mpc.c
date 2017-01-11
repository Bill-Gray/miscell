#define CURL_STATICLIB
#include <stdio.h>
#include <curl/curl.h>
#include <curl/easy.h>
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

#define FETCH_FILESIZE_SHORT               -1
#define FETCH_FOPEN_FAILED                 -2
#define FETCH_CURL_PERFORM_FAILED          -3
#define FETCH_FILESIZE_WRONG               -4
#define FETCH_CURL_INIT_FAILED             -5

static int grab_file( const char *url, const char *outfilename,
                                    const bool append)
{
    CURL *curl = curl_easy_init();

    if (curl) {
        FILE *fp = fopen( outfilename, (append ? "ab" : "wb"));
        const time_t t0 = time( NULL);

        if( !fp)
            return( FETCH_FOPEN_FAILED);
        fprintf( fp, "%ld (%.24s) %s\n", (long)t0, ctime( &t0), url);
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
        CURLcode res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        fclose(fp);
        if( res)
           return( FETCH_CURL_PERFORM_FAILED);
    } else
        return( FETCH_CURL_INIT_FAILED);
    return 0;
}

static int fetch_astrometry_from_mpc( const char *output_filename,
            const char *object_name, const bool append)
{
   char url[300];
   int i, rval;

   sprintf( url, "http://www.minorplanetcenter.net/db_search/show_object?object_id=%s",
               object_name);
   for( i = 65; url[i]; i++)
      if( url[i] == '/')
         {
         memmove( url + i + 3, url + i + 1, strlen( url + i));
         memcpy( url + i, "%2F", 3);
         }
      else if( url[i] == ' ')
         url[i] = '+';
   strcat( url, "&btnG=MPC+Database+Search");
   rval = grab_file( url, "zzzz", 0);
   unlink( "zzzz");
   if( rval)
      return( rval);
   if( total_written < 200)
      return( FETCH_FILESIZE_SHORT);

   sprintf( url, "http://www.minorplanetcenter.net/tmp/%s.txt", object_name);
   for( i = 37; url[i]; i++)
      if( url[i] == ' ' || url[i] == '/')
         url[i] = '_';
#ifdef TEST_MAIN
   printf( "Grabbing '%s'\n", url);
#endif
   total_written = 0;
   rval = grab_file( url, output_filename, append);
   if( rval)
      return( rval - 1000);
   if( total_written % 81)
      rval = FETCH_FILESIZE_WRONG;
   else
      rval = (int)total_written / 81;
   return( rval);
}

#ifdef TEST_MAIN

static void abort_message( void)
{
   printf( "'grab_mpc' requires,  as command-line arguments,  the name of\n"
           "an output file and the name of an object for which astrometry\n"
           "is desired.  The name can be in packed or unpacked form.  You can\n"
           "add '-a' as a command-line option to indicate that astrometry should\n"
           "be appended to an existing file.  Examples:\n\n"
           "./grab_mpc obs.txt 2014 AA\n"
           "./grab_mpc obs.txt J99X11F -a\n");
}

int main( const int argc, char **argv)
{
   char obj_name[300];
   int i, rval;
   bool append = false;
   const char *output_filename = argv[1];

   if( argc < 3)
      {
      abort_message( );
      return( -1);
      }
   strcpy( obj_name, argv[2]);
   for( i = 3; i < argc; i++)
      if( !strcmp( argv[i], "-a"))
         append = true;
      else
         {
         strcat( obj_name, " ");
         strcat( obj_name, argv[i]);
         }

   rval = fetch_astrometry_from_mpc( output_filename, obj_name, append);
   if( rval < 0)
      printf( "Failed: rval %d\n", rval);
   else
      {
      FILE *ifile = fopen( output_filename, "rb");
      char buff[100];

      printf( "%d lines read\n", rval);
      if( ifile)
         {
         if( fgets( buff, sizeof( buff), ifile))
            printf( "%s", buff);    /* show header line */
         if( fgets( buff, sizeof( buff), ifile))
            printf( "%s", buff);    /* show first observation */
         fseek( ifile, -81L, SEEK_END);
         if( fgets( buff, sizeof( buff), ifile))
            printf( "%s", buff);    /* show last observation */
         fclose( ifile);
         }
      else
         printf( "File not opened!\n");
      }
   return( rval < 0 ? rval : 0);
}
#endif
