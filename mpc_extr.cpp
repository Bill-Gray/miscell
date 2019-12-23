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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* Code to extract observations for a specific object from the large
MPC 80-column astrometry files (UnnObs.txt,  CmtObs.txt,  SatObs.txt,
NumObs.txt,  itf.txt).  These files contain only 80-column data,  are
always a multiple of 81 bytes (including the line feed at the end of
each line),  and are sorted by packed ID.  So if you want a particular
object,  you can just binary-search to find the first record,  then
start reading until you've gotten all the records.    */

int mpc_compare( const char *str1, const char *str2)
{
   int rval;

   if( strlen( str2) == 5)       /* numbered MP */
      rval = memcmp( str1, str2, 5);
   else                          /* provisional desig */
      rval = memcmp( str1 + 5, str2, 7);
   return( rval);
}

static char *err_fgets( char *buff, size_t nbytes, FILE *ifile)
{
   char *rval = fgets( buff, nbytes, ifile);

   if( !rval)
      {
      perror( "fgets failed");
      exit( -1);
      }
   return( rval);
}

static int err_exit( void)
{
   printf( "mpc_extr will extract data for a particular object from\n"
           "the 'large' MPC files UnnObs.txt, CmtObs.txt, SatObs.txt,\n"
           "NumObs.txt,  and itf.txt.  For example:\n\n"
           "./mpc_extr UnnObs.txt K14A00A K13YD3F\n\n"
           "would output all records for 2014 AA and 2013 YF133 to stdout.\n");
   return( -1);
}

/* Convert,  e.g.,  '2006te179' to 'K06TG9E'  */

static void convert_to_packed( char *packed, const char *ibuff)
{
   const int num_within_month = atoi( ibuff + 6);

   *packed++ = (ibuff[1] == '9') ? 'J' : 'K';
   *packed++ = ibuff[2];                /* decade */
   *packed++ = ibuff[3];                /* year   */
   *packed++ = toupper( ibuff[4]);      /* half-month */
   if( num_within_month < 100)
      *packed++ = '0' + num_within_month / 10;
   else if( num_within_month < 260)
      *packed++ = 'A' + num_within_month / 10 - 10;
   else
      *packed++ = 'a' + num_within_month / 10 - 36;
   *packed++ = '0' + num_within_month % 10;
   *packed++ = toupper( ibuff[5]);
   *packed = '\0';
}

int main( const int argc, const char **argv)
{
   FILE *ifile;
   unsigned long n_recs, recsize;
   char buff[100];
   int compare, i;
   FILE *ofile = stdout;

   if( argc < 3)
      return( err_exit( ));
   ifile = fopen( argv[1], "rb");
   if( !ifile)
      {
      printf( "%s not opened : ", argv[1]);
      fflush( stdout);
      perror( "");
      return( err_exit( ));
      }
   err_fgets( buff, sizeof( buff), ifile);
   recsize = strlen( buff);
   fseek( ifile, 0L, SEEK_END);
   n_recs = ftell( ifile) / recsize;
   for( i = 2; i < argc; i++)
      if( argv[i][0] == '-')
         switch( argv[i][1])
            {
            case 'o':
               ofile = fopen( argv[i] + 2, "wb");
               break;
            default:
               printf( "Unrecognized command-line option '%s'\n", argv[i]);
               break;
            }
   for( i = 2; i < argc; i++)
      if( argv[i][0] != '-')
         {
         unsigned long loc = 0, step, loc1;
         int n_found = 0;
         char target[50];

         if( atoi( argv[i]) > 1800)
            convert_to_packed( target, argv[i]);
         else
            strcpy( target, argv[i]);
         for( step = 0x8000000; step; step >>= 1)
            if( (loc1 = loc + step) < n_recs)
               {
               fseek( ifile, loc1 * recsize, SEEK_SET);
               err_fgets( buff, sizeof( buff), ifile);
               if( mpc_compare( buff, target) < 0)
                  loc = loc1;
               }
         compare = -1;
         fseek( ifile, loc * recsize, SEEK_SET);
         while( compare <= 0 && fgets( buff, sizeof( buff), ifile))
            {
            compare = mpc_compare( buff, target);
            if( !compare)
               {
               fprintf( ofile, "%s", buff);
               n_found++;
               }
            }
         printf( "%d records found for '%s'\n", n_found, target);
         }
   return( 0);
}
