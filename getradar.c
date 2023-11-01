#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <time.h>
#include <stdbool.h>
#include "mpc_func.h"
#include "stringex.h"

/* Look through the 'radar.ast' file (see 'radar.c') for data for
the specified packed designation.  We have a table of packed desigs
at the top of 'radar.ast',  and we check for the designation in
that table.  If we don't find it,  we don't have to waste time
digging through the rest of the file.

If we _do_ find it,  we keep going until we find a radar observation
matching that designation.  When we do,  we seek back to the previous
blank line and assume that that's where the "real" data (including
headers) for that object is,  and read lines from 'ifile' and output
them to 'ofile' until we encounter another blank line.

Then we repeat the process,  since there may be further observations.

Note that there's code further down that CGI-ifies this for an on-line
"enter a designation and get radar astrometry for it" service.  That
service is available at

https://www.projectpluto.com/radar/radar.htm

and if you just want data for an object now and then,  it may well be
all you really need anyway.           */

int get_radar_data( FILE *ofile, FILE *ifile, const char *packed_desig)
{
   char buff[100], tpacked[13];
   char unpacked_desig[90];
   bool desigs_found = false, object_found = false;
   size_t len;
   long offset;
   bool found_data = false;

   while( *packed_desig == ' ')
      packed_desig++;
   if( 0 > unpack_unaligned_mpc_desig( unpacked_desig, packed_desig))
      return( -2);            /* not a valid packed designation */
   len = strlen( packed_desig);
   while( packed_desig[len - 1] == ' ')
      len--;
   assert( len <= 12);
   memcpy( tpacked, packed_desig, len);
   tpacked[len] = '\0';
   fseek( ifile, 0L, SEEK_SET);
   while( !object_found && fgets( buff, sizeof( buff), ifile))
      if( !memcmp( buff, "COM desigs :", 12))
         {
         char *tptr = strstr( buff + 12, tpacked);

         desigs_found = true;
         if( tptr && tptr[-1] == ' ' && tptr[len] <= ' ')
            object_found = true;
         }
      else if( desigs_found)   /* got to the end of the 'desigs' section, */
         return( -1);          /* didn't find the requested object        */

   offset = ftell( ifile);
   while( fgets( buff, sizeof( buff), ifile))
      if( *buff < ' ')
         offset = ftell( ifile);
      else if( strlen( buff) == 81 && !memcmp( buff + 72, "JPLRS", 5))
         {
         size_t i = 0;

         while( buff[i] == ' ')
            i++;
         if( !memcmp( buff + i, tpacked, len) && buff[i + len] == ' ')
            {
            if( !found_data)  /* first time through : output header data */
               {
               time_t t0 = time( NULL);

               fseek( ifile, 0, SEEK_SET);
               while( fgets( buff, sizeof( buff), ifile) && *buff >= ' ')
                  fputs( buff, ofile);
               fprintf( ofile, "COM radar data for %s = %s, extracted %.24s UTC\n\n",
                               unpacked_desig, packed_desig,
                               asctime( gmtime( &t0)));
               }
            fseek( ifile, offset, SEEK_SET);
            while( fgets( buff, sizeof( buff), ifile) && *buff >= ' ')
               fputs( buff, ofile);
            fputs( "\n", ofile);
            fseek( ifile, -1, SEEK_CUR);
            found_data = true;
            }
         }                       /* if we found the desig in the header, */
   assert( found_data);             /* we ought to find the data    */
   return( found_data ? 0 : -1);    /* in the main part of the file */
}

#ifdef ON_LINE_VERSION
int dummy_main( const int argc, const char **argv)
#else
int main( const int argc, const char **argv)
#endif
{
   const char *ifilename = "radar.ast";
   FILE *ifile;
   char object_name[80];
   int i, rval;

   *object_name = '\0';
   for( i = 1; i < argc; i++)
      if( argv[i][0] != '-')
         {
         if( *object_name)
            strlcat_error( object_name, " ");
         strlcat_error( object_name, argv[i]);
         }
      else
         ifilename = argv[i] + 1;
   ifile = fopen( ifilename, "rb");
   if( !ifile)
      {
      fprintf( stderr, "Couldn't open '%s' : ", ifilename);
      perror( NULL);
      return( -2);
      }
   assert( *object_name);
   rval = get_radar_data( stdout, ifile, object_name);
   if( -2 == rval)         /* maybe an unpacked ID was supplied? */
      {
      char packed[20];

      i = 0;
      while( isdigit( object_name[i]))
         i++;
      if( !object_name[i])    /* numbered object;  desig must be in parens */
         {
         assert( i < 13);
         memmove( object_name + 1, object_name, i);
         object_name[0] = '(';
         object_name[i + 1] = ')';
         object_name[i + 2] = '\0';
         }
      if( -1 < create_mpc_packed_desig( packed, object_name))
         rval = get_radar_data( stdout, ifile, packed);
      }
   return( rval);
}

#ifdef ON_LINE_VERSION
#include "cgi_func.h"

int main( void)
{
   char buff[100];
   char field[30];
   int cgi_status;
   FILE *lock_file = fopen( "lock.txt", "w");

   avoid_runaway_process( 15);
   setbuf( lock_file, NULL);
   fprintf( lock_file, "'getradar' : We're in\n");
   printf( "Content-type: text/html\n\n");
   printf( "<html> <body> <pre>\n");
   cgi_status = initialize_cgi_reading( );
   fprintf( lock_file, "CGI status %d\n", cgi_status);
   if( cgi_status <= 0)
      {
      printf( "<p> <b> CGI data reading failed : error %d </b>", cgi_status);
      printf( "This isn't supposed to happen.</p>\n");
      return( 0);
      }
   while( !get_cgi_data( field, buff, NULL, sizeof( buff)))
      {
      if( !strcmp( field, "desig"))
         {
         const char *argv[4];
         int rval;

         fprintf( lock_file, "desig '%s'\n", buff);
         argv[0] = "getradar";
         argv[1] = buff;
         argv[2] = "-../../radar/radar.ast";
         argv[3] = NULL;
         rval = dummy_main( 3, argv);
         if( rval == -2)
            printf( "'%s' is not a valid designation\n", buff);
         else if( rval == -1)
            printf( "Couldn't find radar data for '%s'\n", buff);
         fprintf( lock_file, "done (1); rval %d\n", rval);
         }
      }
   printf( "</pre> </body> </html>");
   fprintf( lock_file, "done (2)\n");
   return( 0);
}
#endif
