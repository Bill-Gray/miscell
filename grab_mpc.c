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

#ifdef _WIN32
#include "windows.h"
#else
#define CURL_STATICLIB
#include <curl/curl.h>
#include <curl/easy.h>
#endif
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <time.h>

/* Code to download astrometry for a given object from MPC.  Run as

./grab_mpc filename object name [-a]

-a = append astrometry to file rather than overwriting the file. */

size_t total_written;


#define FETCH_FILESIZE_SHORT               -1
#define FETCH_FOPEN_FAILED                 -2
#define FETCH_CURL_PERFORM_FAILED          -3
#define FETCH_CURL_INIT_FAILED             -5

/* In order to avoid hammering MPC's servers,  we do a bit of checking:
if the file exists,  and has data for the object we're looking for (based
on the URLs matching),  _and_ no more than DELAY_BETWEEN_RELOADS seconds
have elapsed,  then we can recycle the existing file.

   At least for the nonce,  let's say we don't ask for astrometry any more
often than once every three hours.  */

#define DELAY_BETWEEN_RELOADS 10800

int verbose;

static int check_for_existing( const char *url, const char *outfilename)
{
   FILE *fp = fopen( outfilename, "rb");
   int rval = 0;

   if( fp)
      {
      char buff[200];

      if( fgets( buff, sizeof( buff), fp))
         {
         int i = strlen( buff);
         const time_t t0 = time( NULL);

         while( i && (buff[i - 1] == 10 || buff[i - 1] == 13))
            i--;
         buff[i] = '\0';
         if( verbose)
            {
            printf( "Old file read\n%s\n", buff);
            printf( "Delay is %d seconds\n", (int)( t0 - atoi( buff + 14)));
            }
         if( !strcmp( buff + 52, url) &&
                  t0 < atoi( buff + 14) + DELAY_BETWEEN_RELOADS)
            rval = 1;   /* yup,  just reuse the existing file */
         }
      fclose( fp);
      }
   return( rval);
}

static FILE *init_output_file( const char *url, const char *object_name,
                               const char *outfilename, const bool append)
{
   FILE *ofile = fopen( outfilename, (append ? "ab" : "wb"));

   if( ofile)
      {
      const time_t t0 = time( NULL);

      fprintf( ofile, "COM UNIX time %ld (%.24s) %s\n", (long)t0, ctime( &t0), url);
      fprintf( ofile, "COM Obj %s\n", object_name);
      }
   return( ofile);
}

#ifdef _WIN32
static int grab_file( const char *url, const char *object_name,
                  const char *outfilename, const bool append)
{
   const char *tname = "zzz1";
   HRESULT rval;
   FILE *ifile;
   FILE *ofile = init_output_file( url, object_name, outfilename, append);

   if( !ofile)
      return( FETCH_FOPEN_FAILED);
   unlink( tname);
   rval = URLDownloadToFile( NULL, url, tname, 0, NULL);
   fprintf( ofile, "COM rval = %d\n", (int)rval);
   ifile = fopen( tname, "rb");
   if( ifile)
      {
      char buff[200];

      while( fgets( buff, sizeof( buff), ifile))
         {
         fputs( buff, ofile);
         total_written += strlen( buff);
         }
      fclose( ifile);
      }
   fclose( ofile);
   return( rval != S_OK);
}
#else             /* non-Win32 file grabbing */

static size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    size_t written;

    written = fwrite(ptr, size, nmemb, stream);
    total_written += written;
    return written;
}

static int grab_file( const char *url, const char *object_name,
                  const char *outfilename, const bool append)
{
    CURL *curl;
    int rval = 0;
    FILE *fp = init_output_file( url, object_name, outfilename, append);

    if( !fp)
        return( FETCH_FOPEN_FAILED);
    curl = curl_easy_init();
    if (curl)
        {
        CURLcode res;
        char err_buff[CURL_ERROR_SIZE];

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, err_buff);
        curl_easy_setopt(curl, CURLOPT_USERAGENT,
            "Mozilla/5.0 (Windows NT 6.1; Win64; x64; rv:61.0) Gecko/20100101 Firefox/61.0");
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        if( res)
           {
           fprintf( fp, "Error '%s'\n", err_buff);
           rval = FETCH_CURL_PERFORM_FAILED;
           }
        }
    else
        {
        fprintf( fp, "Error: couldn't initialize curl\n");
        rval = FETCH_CURL_INIT_FAILED;
        }
    fclose(fp);
    return rval;
}
#endif

#define BASE_MPC_URL "https://www.minorplanetcenter.net"

/* After getting the MPC database search summary for an object,  we can dig
through it and (we hope) find the URL for the file containing the actual
astrometry.  I used to just generate this from the object name.  The problem
is that if the object has been redesignated,  we can get the summary through
HTML redirection;  for the astrometry,  we need the actual name as it
appears on MPC's servers.     */

static bool look_for_link_to_astrometry( const char *filename, char *url)
{
   FILE *ifile = fopen( filename, "rb");
   bool got_it = false;
   char buff[200], *tptr;

   if( ifile)
      {
      while( !got_it &&  fgets( buff, sizeof( buff), ifile))
         if( (tptr = strstr( buff, "../tmp/")) != NULL)
            {
            tptr += 2;
            strcpy( url, BASE_MPC_URL);
            url += strlen( url);
            while( *tptr && *tptr != '"')
               *url++ = *tptr++;
            *url = '\0';
            got_it = true;
            }
      fclose( ifile);
      }
   return( got_it);
}

static int fetch_astrometry_from_mpc( const char *output_filename,
            const char *object_name, const bool append)
{
   char url[300], url2[300];
   int i, rval;

   snprintf( url2, sizeof( url2), BASE_MPC_URL "/tmp/%s.txt", object_name);
   for( i = 38; url2[i]; i++)
      if( url2[i] == ' ' || url2[i] == '/')
         url2[i] = '_';
   if( verbose)
      printf( "Temp file name '%s'\n", url2);
   if( check_for_existing( url2, output_filename))
      return( 0);

   snprintf( url, sizeof( url), BASE_MPC_URL "/db_search/show_object?object_id=%s",
               object_name);
   for( i = 66; url[i]; i++)
      if( url[i] == '/')
         {
         memmove( url + i + 3, url + i + 1, strlen( url + i));
         memcpy( url + i, "%2F", 3);
         }
      else if( url[i] == ' ')
         url[i] = '+';
   strcat( url, "&btnG=MPC+Database+Search");
   if( verbose)
      printf( "Grabbing '%s''\n", url);
   unlink( output_filename);
   rval = grab_file( url, object_name, output_filename, 0);
   if( !rval && !look_for_link_to_astrometry( output_filename, url2))
      rval = -314159;
   if( rval)
      return( rval);
   if( verbose)
      printf( "Revised URL: '%s'\n", url2);
   if( rval)
      return( rval);
   if( total_written < 200)
      return( FETCH_FILESIZE_SHORT);

#ifdef TEST_MAIN
   if( verbose)
      printf( "Grabbing '%s'\n", url2);
#endif
   total_written = 0;
   rval = grab_file( url2, object_name, output_filename, append);
   if( rval)
      rval -= 1000;
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
   char obj_name[100];
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
      if( argv[i][0] != '-')
         {
         strcat( obj_name, " ");
         strcat( obj_name, argv[i]);
         }
      else switch( argv[i][1])
         {
         case 'a':
            append = true;
            break;
         case 'v':
            verbose = 1;
            break;
         default:
            fprintf( stderr, "Option %s ignored\n", argv[i]);
            break;
         }

   rval = fetch_astrometry_from_mpc( output_filename, obj_name, append);
   if( !rval && verbose)
      {
      FILE *ifile = fopen( output_filename, "rb");
      char buff[200];

      assert( ifile);
      printf( "%d lines read\n", rval);
      if( ifile)
         {
         for( i = 0; i < 4 && fgets( buff, sizeof( buff), ifile); i++)
            printf( "%s", buff);    /* show header lines and a couple of obs */
         fseek( ifile, -81L, SEEK_END);
         if( fgets( buff, sizeof( buff), ifile))
            printf( "%s", buff);    /* show last observation */
         fclose( ifile);
         }
      else
         fprintf( stderr, "File not opened!\n");
      }
   return( rval < 0 ? rval : 0);
}
#endif

