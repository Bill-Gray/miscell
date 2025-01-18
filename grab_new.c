/* Code to download astrometric data from MPC Explorer

Copyright (C) 2018, Project Pluto

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
02110-1301, USA.

Code to download ADES-formatted astrometry from the MPC observations
API,  using the methods described at

https://minorplanetcenter.net/mpcops/documentation/neocp-observations-api/
https://minorplanetcenter.net/mpcops/documentation/observations-api/

(note that for some reason,  there are slightly different interfaces
for NEOCP observations and non-NEOCP observations;  currently,  this
just handles the latter).  This will be a replacement for 'grab_mpc.c'
(q.v.),  which used the older db_search capability. MPC Explorer should
offer several advantages (access to ADES data, observations available
quicker,  etc.)  Compile with

gcc -Wall -Wextra -pedantic -I../include -o grab_mpc grab_new.c ../lunar/snprintf.cpp

   to generate a 'grab_mpc' executable;  you should be able to just
drop it in as a replacement.

   This code assembles a curl command resembling those shown at the
above URLs (look under "Curl Example - XML format").  I tried using
libcURL instead of system( curl command),  but without success;  I
didn't find the right invocation of the library.  So this won't work
on systems lacking the 'curl' command.

   The resulting file has the line feed (character 10) converted into
a backslash and an 'n'.  So we have to go through and convert all
of those.  The file also has some header and trailer data not needed
for our purposes;  those are stripped,  and the output written back
to the file.

   In order to avoid hammering MPC's servers,  we do a bit of checking:
if the file exists,  and has data for the object we're looking for
(based on the file names in the URLs matching),  _and_ no more than
delay_between_reloads seconds have elapsed,  then we can recycle the
existing file.   Currently,  that means we try again if three hours
have elapsed.  With modifications,  this program could work with
NEOCP as well;  we'd presumably use a shorter delay there.     */

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "stringex.h"


static int delay_between_reloads = 10800;    /* = three-hour delay */

static int grab_file( const char *url, const char *outfilename)
{
   const size_t buffsize = strlen( url) + strlen( outfilename) + 16;
   char *command = (char *)malloc( buffsize + 1);
   int err_code;

   snprintf_err( command, buffsize, "curl -s %s -o%s", url, outfilename);
   err_code = system( command);
   if( err_code)
      fprintf( stderr, "Error %d for grab_file\n'%s'\n", err_code, command);
   return( err_code);
}

int main( const int argc, const char **argv)
{
   int err_code, verbose = 0, is_neocp = 0;
   size_t i;
   FILE *fp;
   char object_desig[30], command[250];
   const char *filename = argv[1];
   const time_t t0 = time( NULL);
   const char *format = "curl -X GET -H \"Content-Type: application/json\" " \
              "-d '{ \"%s\": [\"%s\"], \"output_format\":[\"XML\"]}' "  \
              "https://data.minorplanetcenter.net/api/get-obs%s -o%s -s";

   assert( argc > 2);
   if( 3 == argc)                /* simply downloading a file */
      if( !memcmp( argv[2], "http", 4) || !memcmp( argv[2], "ftp", 3))
         return( grab_file( argv[2], argv[1]));
   *object_desig = '\0';
   for( i = 2; i < (size_t)argc; i++)
      if( argv[i][0] != '-')    /* not a command-line option; */
         {                      /* presumed to be part of object name */
         if( *object_desig)
            strlcat_error( object_desig, " ");
         strlcat_error( object_desig, argv[i]);
         }
      else switch( argv[i][1])
         {
         case 'v':
            verbose = 1;
            break;
         case 'n':
            is_neocp = 1;
            break;
         case 't':
            delay_between_reloads = atoi( argv[i] + 2);
            break;
         default:
            fprintf( stderr, "Argument '%s' not recognized\n", argv[i]);
            return( -1);
         }
   object_desig[sizeof( object_desig) - 1] = '\0';
   assert( strlen( object_desig) < sizeof( object_desig) - 1);
   if( verbose)
      printf( "Object desig '%s'\n", object_desig);
   fp = fopen( filename, "rb");
   if( fp)
      {
      char tbuff[90];

      if( verbose)
         printf( "'%s' opened\n", filename);
      if( fgets( tbuff, sizeof( tbuff), fp)
                  && !memcmp( tbuff, "COM UNIX time ", 14)
                  && atol( tbuff + 14) + delay_between_reloads > t0
                  && fgets( tbuff, sizeof( tbuff), fp)
                  && !memcmp( tbuff, "COM Obj ", 8)
                  && !memcmp( tbuff + 8, object_desig, strlen( object_desig))
                  && tbuff[strlen( object_desig) + 8] < ' ')
         {
         if( verbose)
            printf( "Previous download isn't stale yet\n");
         return( 0);
         }
      if( verbose)
         printf( "Got to '%s'\n", tbuff);
      fclose( fp);
      }


   snprintf( command, sizeof( command), format,
                  (is_neocp ? "trksubs" : "desigs"),
                  object_desig,
                  (is_neocp ? "-neocp" : ""),
                  filename);
   if( verbose)
      printf( "%s\n", command);
   err_code = system( command);
   if( err_code)
      fprintf( stderr, "Error %d\n", err_code);
   else
      {
      size_t j, len, bytes_read;
      char *buff, *tptr;

      fp = fopen( filename, "rb");
      assert( fp);
      fseek( fp, 0L, SEEK_END);
      len = (size_t)ftell( fp);
      fseek( fp, 0L, SEEK_SET);
      buff = (char *)malloc( len + 1);
      buff[len] = '\0';
      bytes_read = fread( buff, 1, len, fp);
      assert( bytes_read == len);
      fclose( fp);
      for( i = j = 0; i < len; i++, j++)
         if( buff[i] == '\\' && buff[i + 1] == 'n')
            {
            buff[j] = '\n';
            i++;
            }
         else
            buff[j] = buff[i];
      buff[j] = '\0';
      tptr = strstr( buff, "</ades>");
      assert( tptr);
      tptr[7] = '\n';
      tptr[8] = '\0';
      tptr = strstr( buff, "<ades version=");
      assert( tptr);
      fp = fopen( filename, "wb");
      assert( fp);

      fprintf( fp, "COM UNIX time %ld (%.24s)\n", (long)t0, asctime( gmtime( &t0)));
      fprintf( fp, "COM Obj %s\n", object_desig);
      fwrite( tptr, 1, strlen( tptr), fp);
      fclose( fp);
      free( buff);
      }
   return( 0);
}
