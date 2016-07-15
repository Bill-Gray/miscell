#define CURL_STATICLIB
#include <stdio.h>
#include <curl/curl.h>
#include <curl/easy.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

/* Code to download astrometry for a given object from MPC.  Run as

./grab_mpc filename object name [-a]

-a = append astrometry to file rather than overwriting the file. */

size_t total_written;

size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream) {
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
//      CURLcode res;
        FILE *fp = fopen( outfilename, (append ? "ab" : "wb"));

        assert( fp);
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
        CURLcode res = curl_easy_perform(curl);
        if( res)
           printf( "res = %d\n", res);
        assert( !res);
        curl_easy_cleanup(curl);
        fclose(fp);
    }
    return 0;
}

int main( const int argc, char **argv)
{
   char url[300];
   int i;
   bool append = false;

   sprintf( url, "http://www.minorplanetcenter.net/db_search/show_object?object_id=%s",
               argv[2]);
   for( i = 65; url[i]; i++)
      if( url[i] == '/')
         {
         memmove( url + i + 3, url + i + 1, strlen( url + i));
         memcpy( url + i, "%2F", 3);
         }
   for( i = 3; i < argc; i++)
      if( !strcmp( argv[i], "-a"))
         append = true;
      else
         sprintf( url + strlen( url), "+%s", argv[i]);
   strcat( url, "&btnG=MPC+Database+Search");
   grab_file( url, "zzzz", 0);
   unlink( "zzzz");
   if( total_written < 200)
      {
      printf( "Didn't find that object\n");
      return( -1);
      }

   sprintf( url, "http://www.minorplanetcenter.net/tmp/%s", argv[2]);
   for( i = 3; i < argc; i++)
      if( strcmp( argv[i], "-a"))
         sprintf( url + strlen( url), "_%s", argv[i]);
   strcat( url, ".txt");
   for( i = 37; url[i]; i++)
      if( url[i] == '/')
         url[i] = '_';
   printf( "Grabbing '%s'\n", url);
   total_written = 0;
   grab_file( url, argv[1], append);
   printf( "%ld lines written\n", (long)total_written / 81L);
   if( total_written % 81)
      printf( "!!! Not an even multiple of 81\n");
   else
      {
      FILE *ifile = fopen( argv[1], "rb");
      char buff[100];

      assert( ifile);
      if( fgets( buff, sizeof( buff), ifile))
         printf( "%s", buff);    /* show first observation */
      fseek( ifile, -81L, SEEK_END);
      if( fgets( buff, sizeof( buff), ifile))
         printf( "%s", buff);    /* show last observation */
      fclose( ifile);
      }
   return( 0);
}
