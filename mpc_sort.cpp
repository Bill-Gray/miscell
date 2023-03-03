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

/* Based largely on 'fix_obs',  but the _only_ thing it does is to test
out the comparison function to make sure the input file is properly sorted.
See notes from 'fix_obs.cpp'.  */

static int mpc_compare( const void *aptr, const void *bptr)
{
   const char *a = (const char *)aptr;
   const char *b = (const char *)bptr;
   int rval = 0;

   if( a[4] == 'P' && b[4] == 'P'     /* compare permanent period comet desig */
               && memcmp( a, "    ", 4) && memcmp( b, "    ", 4))
      rval = memcmp( a, b, 4);
   else
      rval = memcmp( a, b, 12);        /* compare entire ID */
   if( !rval)              /* same ID,  so compare by year */
      rval = memcmp( a + 15, b + 15, 4);
   if( !rval)              /* same year;  compare month/day */
      rval = memcmp( a + 20, b + 20, 12);
   if( !rval)              /* still the same;  compare by note 2 */
      rval = (int)a[14] - (int)b[14];
   if( !rval)           /* Still the same:  compare by comet component */
      rval = (int)a[11] - (int)b[11];
   if( !rval)
      rval = (int)b[10] - (int)a[10];
   if( !rval)              /* now compare by MPC code */
      rval = memcmp( a + 77, b + 77, 3);
   if( !rval)           /* Still the same:  compare by asteroid component */
      rval = (int)a[19] - (int)b[19];
   return( rval);
}

#ifdef __GNUC__
void err_exit( const char *message, const int error_code)  __attribute__ ((noreturn));
#endif

void err_exit( const char *message, const int error_code)
{
   if( message)
      printf( "%s", message);
   printf( "Hit Enter:\n");
   getchar( );
   exit( error_code);
}

static FILE *err_fopen( const char *filename, const char *permits)
{
   FILE *rval = fopen( filename, permits);

   if( !rval)
      {
      char buff[90];

      snprintf( buff, sizeof( buff), "Couldn't open '%s'\n", filename);
      err_exit( buff, -1);
      }
   return( rval);
}

int main( const int argc, const char **argv)
{
   FILE *ifile = err_fopen( argc == 1 ? "UnnObs.txt" : argv[1], "rb");
   char iline[90], prev[90];

   if( !fgets( prev, sizeof( prev), ifile))
      err_exit( "Read failure (1)\n", -2);

   while( fgets( iline, sizeof( iline), ifile))
      {
      const int compare = mpc_compare( prev, iline);

      if( compare >= 0)
         printf( "Compare = %d\n%s%s\n", compare, prev, iline);
      strcpy( prev, iline);
      }
   fclose( ifile);
   return( 0);
}
