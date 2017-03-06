/* #define CURL_STATICLIB    */
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <curl/curl.h>
#include <curl/easy.h>
#if defined( __linux__) || defined( __unix__) || defined( __APPLE__)
   #include <sys/time.h>         /* these allow resource limiting */
   #include <sys/resource.h>     /* see 'avoid_runaway_process'   */
#endif

/* Some of my programs run on a server where,  if I use too much CPU time,
I get rationed.  The following 'avoid_runaway_process' function helps
ensure that a hung process that shouldn't have taken more than 'max_t'
seconds of CPU time gets killed.

   'rlim_cur' is the "soft" limit,  in seconds.  At that point,  the
process gets a SIGXCPU signal once a second:  sort of a "you're running out
of time here;  put down your pencils and close your books." (At least,
that's how Linux does it.  For  portability,  you should assume you may
just get the first SIGXCPU signal,  and do an orderly shutdown then.)

   After 'rlim_max' seconds (the 'hard limit'),  the process gets a
SIGKILL message.  */


static void avoid_runaway_process( const int max_t)
{
   struct rlimit r;       /* Make sure process ends after rlim_cur seconds, */
                          /* with a "hard limit" of rlim_max seconds        */
   r.rlim_cur = (rlim_t)max_t;
   r.rlim_max = (rlim_t)( max_t + 1);
   setrlimit( RLIMIT_CPU, &r);
}

typedef struct
{
   const char *url, *filename;
   bool is_done;
   size_t bytes_xferred, total_bytes;
   int error_code, flags;
            /* 'offset' = starting location within file to download; */
            /* 'n_bytes' = if >0,  number of bytes to download       */
   long offset, n_bytes;
   size_t ultotal, ulnow;
} file_fetch_t;

/* Older cURL libraries don't have a progress function option  */
#if defined( CURLOPT_XFERINFOFUNCTION) && defined( CURLOPT_XFERINFODATA)
int progress_callback( void *clientp, curl_off_t dltotal, curl_off_t dlnow,
                    curl_off_t ultotal, curl_off_t ulnow)
{
   file_fetch_t *f = (file_fetch_t *)clientp;

   f->bytes_xferred = (size_t)dlnow;
   f->total_bytes = (size_t)dltotal;
   f->ultotal = (size_t)ultotal;
   f->ulnow   = (size_t)ulnow;
   return( 0);       /* return non-zero value to abort xfer */
}
#endif

void *fetch_a_file( void *args)
{
   CURL *curl = curl_easy_init();
   file_fetch_t *f = (file_fetch_t *)args;

   printf( "In working func: %s, %s\n", f->url, f->filename);
   assert( curl);
   if( curl)
      {
      FILE *fp = fopen( f->filename, (f->flags & 1) ? "ab" : "wb");
//    const long starting_loc = ftell( fp);

      if( !fp)
         {
         printf( "Couldn't append %s to %s\n", f->url, f->filename);
         perror( NULL);
         f->error_code = -1;
         return( NULL);
         }
      else
         {
         CURLcode res;

         curl_easy_setopt( curl, CURLOPT_URL, f->url);
         curl_easy_setopt( curl, CURLOPT_WRITEFUNCTION, fwrite);
         curl_easy_setopt( curl, CURLOPT_WRITEDATA, fp);
         curl_easy_setopt( curl, CURLOPT_NOPROGRESS, 0);
#if defined( CURLOPT_XFERINFOFUNCTION) && defined( CURLOPT_XFERINFODATA)
         curl_easy_setopt( curl, CURLOPT_XFERINFOFUNCTION, progress_callback);
         curl_easy_setopt( curl, CURLOPT_XFERINFODATA, args);
#endif
         if( f->n_bytes)
            {
            char tbuff[60];

            snprintf( tbuff, sizeof( tbuff), "%ld-%ld",
                        f->offset, f->offset + f->n_bytes - 1);
            curl_easy_setopt( curl, CURLOPT_RANGE, tbuff);
            }
         else if( f->offset)
            curl_easy_setopt( curl, CURLOPT_RESUME_FROM_LARGE, (curl_off_t)f->offset);
         res = curl_easy_perform( curl);
         if( res)
            {
            printf( "libcurl error %d occurred\n", res);
            printf( "File '%s'; url %s\n", f->filename, f->url);
            f->error_code = res;
            }
         fclose( fp);
         }
      curl_easy_cleanup( curl);
      }
   f->is_done = true;
   return( NULL);
}

static int threaded_fetch_a_file( file_fetch_t *f)
{
   int rval;
   pthread_t unused_pthread_rval;

   f->is_done = false;
   f->bytes_xferred = 0;
   f->total_bytes = 0;
   f->error_code = 0;
   rval = pthread_create( &unused_pthread_rval, NULL, fetch_a_file, f);
   if( rval)         /* failed to create the thread */
      f->is_done = true;
   return( rval);
}

int main( const int argc, const char **argv)
{
   file_fetch_t f;

   avoid_runaway_process( 60);
   assert( argc >= 3);
   f.url = argv[1];
   f.filename = argv[2];
   if( argc > 3)
      f.offset = atol( argv[3]);
   else
      f.offset = 0;
   if( argc > 4)
      f.n_bytes = atol( argv[4]);
   else
      f.n_bytes = 0;
   f.flags = false;
   threaded_fetch_a_file( &f);
   while( !f.is_done)
      {
      printf( "Still here...%ld/%ld\n", (long)f.bytes_xferred, (long)f.total_bytes);
      sleep( 1);
      }
   printf( "Err code %d\n", f.error_code);
   return 0;
}
