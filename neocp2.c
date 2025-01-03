/* Copyright (C) 2018, Project Pluto

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
02110-1301, USA. */

/* #define CURL_STATICLIB  */
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>
#include <curl/curl.h>
#include <curl/easy.h>
#if defined( __linux__) || defined( __unix__) || defined( __APPLE__)
   #include <sys/time.h>         /* these allow resource limiting */
   #include <sys/resource.h>     /* see 'avoid_runaway_process'   */
#endif

/* Code to "scrape" astrometry from NEOCP,  in as efficient and
error-proof a manner as reasonably possible.

   We begin by downloading the current NEOCP astrometry :

https://www.minorplanetcenter.net//cgi-bin/bulk_neocp.cgi?what=obs

   and storing it as 'newneocp.txt'.

   We then load both it and the existing 'neocp.txt'.  For each
line in the new file,  we look for a corresponding line in the
old file.  If we find it,  we remove it from the old file and
transfer the date/time to the 'new' file.

   After this,  any 'new' lines that weren't found in the
previous version can be considered to be genuinely new.  We
mark them with the current time and write whatever we've got
(should exactly resemble the file from NEOCP,  except for
date/times added in) to 'neocp.txt'.  Any lines from the existing
file that we didn't match get added to 'neocp.old'.

   We also write out an 'neocp.new' that contains the data
from the new 'neocp.txt' for any object that changed,  i.e.,
has the current time stamp on it.            */

   /* Limit the program to a certain amount of CPU time.  In this
      case,  if it's taking more than 200 seconds,  something
      has gone wrong,  and the process should die.       */

static void avoid_runaway_process( void)
{
   struct rlimit r;       /* Make sure process ends after 200 seconds,  with */
                          /* a "hard limit" of 220 seconds                   */
   r.rlim_cur = 200;
   r.rlim_max = 220;
   setrlimit( RLIMIT_CPU, &r);
}

   /* We have several files that should,  without question,  be
      openable.  If they aren't,  we should just fail.        */

static FILE *err_fopen( const char *filename, const char *permits)
{
   FILE *rval = fopen( filename, permits);

   if( !rval)
      {
      fprintf( stderr, "Couldn't open %s\n", filename);
      perror( NULL);
      exit( -1);
      }
   return( rval);
}

/* Like MPC,  we make use of 'mutant hex' (base 62).  */

static char int_to_mutant_hex_char( const int ival)
{
   const char *buff =
        "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

   assert( ival >=0 && ival <= 61);
   return( buff[ival]);
}

/* When we download 'new' NEOCP data,  we check to see if it exists in
the data we've already got.  If it doesn't,  we put a time tag in the
normally unused bytes 58-62 so that in the future,  we'll know when we
first saw it.  This is to help in situations (which happen a fair bit)
where I'm wondering when something first showed up on NEOCP.

If it didn't already exist,  and we've already got that particular
observation,  we copy the time tag into the 'new' data.  */

static void time_tag( char *tag)
{
   const time_t t0 = time( NULL);
   struct tm tm;

#ifdef _WIN32
   memcpy( &tm, gmtime( &t0), sizeof( tm));
#else
   gmtime_r( &t0, &tm);
#endif
   tag[0] = '~';
   tag[1] = int_to_mutant_hex_char( tm.tm_mon + 1);
   tag[2] = int_to_mutant_hex_char( tm.tm_mday);
   tag[3] = int_to_mutant_hex_char( tm.tm_hour);
   tag[4] = int_to_mutant_hex_char( tm.tm_min);
}

typedef struct
{
   char *obuff;
   size_t loc, max_len;
} curl_buff_t;

size_t curl_buff_write( char *ptr, size_t size, size_t nmemb, void *context_ptr)
{
   curl_buff_t *context = (curl_buff_t *)context_ptr;
   size_t bytes_to_write = size * nmemb;

   if( bytes_to_write > context->max_len - context->loc)
      bytes_to_write = context->max_len - context->loc;
   memcpy( context->obuff + context->loc, ptr, bytes_to_write);
   context->loc += bytes_to_write;
   return( bytes_to_write);
}

static unsigned fetch_a_file( const char *url, char *obuff,
                              const size_t max_len)
{
   CURL *curl = curl_easy_init();
   unsigned rval = 0;

   assert( curl);
   if( curl)
      {
      CURLcode res;
      curl_buff_t context;
      char errbuf[CURL_ERROR_SIZE];

      context.loc = 0;
      context.obuff = obuff;
      context.max_len = max_len;
      curl_easy_setopt( curl, CURLOPT_URL, url);
      curl_easy_setopt( curl, CURLOPT_WRITEFUNCTION, curl_buff_write);
      curl_easy_setopt( curl, CURLOPT_WRITEDATA, &context);
      curl_easy_setopt( curl, CURLOPT_ERRORBUFFER, errbuf);
      *errbuf = '\0';
#ifdef NOT_CURRENTLY_USED
      if( flags & 2)
         {
         curl_easy_setopt( curl, CURLOPT_NOBODY, 1);
         curl_easy_setopt( curl, CURLOPT_HEADER, 1);
         }
#endif
      res = curl_easy_perform( curl);
      if( res)
         {
         fprintf( stderr, "libcurl error %d occurred\n", res);
         fprintf( stderr, "%s\n",
                       (*errbuf ? errbuf : curl_easy_strerror( res)));
         printf( "url %s\n", url);
         exit( -1);
         }
      curl_easy_cleanup( curl);
      rval = (unsigned)context.loc;
      }
   return( rval);
}

static bool is_valid_astrometry_line( const char *buff)
{
   bool is_mpc_line = true;
   const size_t len = strlen( buff);

   if( len > 80 && len < 83 && (buff[80] == 10 || buff[80] == 13))
      {
      size_t j;
      const char *example_line =
                  "0000 00 00.00000 00 00 00.0   00 00 00.0";
               /*  YYYY MM DD.ddddd HH MM SS.s   dd mm ss.s   */

      for( j = 0; example_line[j] && is_mpc_line; j++)
         if( example_line[j] == '0' && !isdigit( buff[j + 15]))
            is_mpc_line = false;
         else if( example_line[j] == '.' && buff[j + 15] != '.')
            is_mpc_line = false;
      }
   else
      is_mpc_line = false;
   return( is_mpc_line);
}

#define MAX_ILEN 81000000

int main( const int argc, const char **argv)
{
   unsigned bytes_read, i, j, n_new_lines = 0, n_lines = 0;
   int n_to_old = 0;
   FILE *ofile, *ifile;
   char *tbuff, buff[100], tag[6], old_neocp[12];
   char **ilines;
   const char *bulk_neocp_url =
           "https://www.minorplanetcenter.net//cgi-bin/bulk_neocp.cgi?what=obs";

   printf( "Content-type: text/html\n\n");
   avoid_runaway_process( );
   for( i = 1; i < (unsigned)argc; i++)
      if( argv[i][0] == '-')
         switch( argv[i][1])
            {
            case 'n':
               bulk_neocp_url = NULL;
               printf( "Working offline\n");
               break;
            default:
               printf( "Command-line option '%s' unknown\n", argv[i]);
               return( 0);
            }

   tbuff = (char *)malloc( MAX_ILEN);
   if( bulk_neocp_url)
      {
      bytes_read = fetch_a_file( bulk_neocp_url, tbuff, MAX_ILEN);
      ofile = err_fopen( "neocpnew.txt", "wb");
      fwrite( tbuff, bytes_read, 1, ofile);
      fclose( ofile);
      }
   else
      {
      ifile = err_fopen( "neocpnew.txt", "rb");
      bytes_read = fread( tbuff, 1, MAX_ILEN, ifile);
      fclose( ifile);
      }
   printf( "%u bytes read; %u lines\n", bytes_read, bytes_read / 81U);
   if( bytes_read % 81)
      {
      printf( "NOT A MULTIPLE OF 81\n");
      return( -1);
      }
   for( i = n_lines = 0; i < bytes_read; i++)
      if( !i || tbuff[i - 1] == 10)
         n_lines++;
   ilines = (char **)calloc( n_lines + 1, sizeof( char *));
   for( i = n_lines = 0; i < bytes_read; i++)
      if( !i || tbuff[i - 1] == 10)
         ilines[n_lines++] = tbuff + i;

   ifile = err_fopen( "neocp.txt", "rb");
   ofile = err_fopen( "neocp.old", "ab");
   memset( old_neocp, ' ', 12);
   while( fgets( buff, sizeof( buff), ifile))
      if( is_valid_astrometry_line( buff))
         {
         bool match_found = false;

         for( i = 0; !match_found && ilines[i]; i++)
            if( !memcmp( ilines[i], buff, 59) &&
                           !memcmp( ilines[i] + 64, buff + 64, 16))
               {
               match_found = true;
               memcpy( ilines[i] + 59, buff + 59, 5);
               }
         if( !match_found)
            {
            if( !n_to_old)
               {
               const time_t t0 = time( NULL);

               fprintf( ofile, "# New objs added %.24s UTC\n", asctime( gmtime( &t0)));
               }
            fprintf( ofile, "%s", buff);
            if( memcmp( old_neocp, buff, 12))
               {
               printf( "%.12s removed\n", buff);
               memcpy( old_neocp, buff, 12);
               }
            n_to_old++;
            }
         }
   printf( "%u lines added to neocp.old\n", n_to_old);
   fclose( ifile);
   fclose( ofile);
   time_tag( tag);
   tag[5] = '\0';
   printf( "Tag for new lines '%s'\n", tag);
   for( i = 0; ilines[i]; i++)
      if( !memcmp( ilines[i] + 59, "     ", 5))
         {
         memcpy( ilines[i] + 59, tag, 5);
         n_new_lines++;
         }
   printf( "%u new lines found\n", n_new_lines);
   ofile = err_fopen( "neocp.txt", "wb");
   fwrite( tbuff, bytes_read, 1, ofile);
   fclose( ofile);

   ofile = NULL;
   j = (unsigned)-1;
   for( i = 0; ilines[i]; i++)
      if( !memcmp( ilines[i] + 59, tag, 5))
         if( j == (unsigned)-1 || memcmp( ilines[i], ilines[j], 12))
            {
            unsigned n_lines_out = 0, n_prev = 0;

            if( !ofile)
               {
               printf( "New/updated objects\n");
               ofile = err_fopen( "neocp.new", "wb");
               }
            for( j = 0; ilines[j]; j++)
               if( !memcmp( ilines[i], ilines[j], 12))
                  {
                  fprintf( ofile, "%.80s\n", ilines[j]);
                  n_lines_out++;
                  if( memcmp( ilines[j] + 59, tag, 5))
                     n_prev++;
                  }
            j = i;
            printf( "%.12s  %u obs written (was %u)\n", ilines[i], n_lines_out, n_prev);
            }
   if( ofile)
      fclose( ofile);
   free( tbuff);
   free( ilines);
   return 0;
}
