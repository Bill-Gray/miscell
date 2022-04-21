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

/* This reads in UnnObs.txt (MPC file of astrometry for unnumbered objects)
and the list of identifications and list of double designations,  available at

http://www.minorplanetcenter.net/iau/ECS/MPCAT-OBS/MPCAT-OBS.html
http://www.minorplanetcenter.net/iau/ECS/MPCAT/MPCAT.html

   the input files being named 'UnnObs.txt',  'ids.txt',  and 'numids.txt'.

   It outputs 'UnnObs2.txt',  in which the ID files are used to "correct"
UnnObs.txt.  For example,  the line in ids.txt reading

J81E35NK14F21N

   tells us that J81E35N = 1981 EN35 and K14F21N = 2014 FN21 are the same
object.  'fix_obs' will find all instances of K14F21N and mark them to
be changed to J81E35N.  After we've gone through all lines in 'ids.txt'
and 'dbl.txt', we go back and actually revise such instances to J81E35N.
(We can't revise them as we go.  The code that finds all instances of
K14F21N uses a binary search -- look at 'fix_desig' below;  if we altered
the input data,  the data wouldn't be sorted anymore and the next binary
search might fail.  So the actual replacement has to be a final step.)

   The result is sorted out by designation and date (i.e.,  same sort
order as the original 'UnnObs.txt') and written out to UnnObs2.txt.

   The resulting file can be used much as UnnObs.txt was,  but objects
will be "correctly" identified instead of being left with their old
designations.

   Compile the program with either g++ or clang :

g++ -Wall -O3 -pedantic -o fix_obs fix_obs.cpp
clang -Wall -O3 -pedantic -o fix_obs fix_obs.cpp

   You can run with the command line argument '-x' to have the old
designations saved in columns 57 to 63 (they're usually blank and
ignored anyway).  */

/* Some notes on the sort order for the MPC files:

   -- Bill Zielenbach pointed out that the comparison goes beyond just the
designation and the date/time.  If those are identical,  you need to look
at the observation type (in column 15);  otherwise,  two-line observations
(radar,  roving observer,  satellite) won't get sorted.  Also,  it does
sometimes happen that the same object is observed simultaneously at two
stations,  so the MPC code has to be compared.

   -- 2001 QW322 is an unusual case.  It's the only binary TNO in the file,
and has an a and a b component,  with the letter stuck between the year and
month.  That gets you input such as

     K01QW2W 5C2003a10 28.01437 20 40 46.15 -18 41 48.2                 k3026309
     K01QW2W 5C2003b10 28.01437 20 40 45.98 -18 41 46.5                 k3026309

   -- This works with UnnObs.txt (I wrote a bit of test code that verified
that compared records looking for incorrect ordering).  It fails with CmtObs.txt
and SatObs.txt.  Those involve some additional rules.  I'll probably fix this --
you'll see that I did some work to deal with comet ordering -- but my priority
was getting it to work for UnnObs.txt.                   */

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

static void fix_desig( const char *obs, const size_t n_lines,
               const char *new_desig, const char *old_desig,
               char *xdesigs)
{
   size_t loc = 0, loc1, step;
   int compare, n_found = 0;
   const char *tptr;

/* printf( "Replacing %.7s with %.7s\n", old_desig, new_desig);   */
   for( step = (size_t)0x80000000; step; step >>= 1)
      if( (loc1 = loc + step) < n_lines)
         if( memcmp( obs + loc1 * 81 + 5, old_desig, 7) < 0)
            loc = loc1;
   tptr = obs + loc * 81 + 5;
   xdesigs += loc * 7;
   while( loc < n_lines && (compare = memcmp( tptr, old_desig, 7)) <= 0)
      {
      if( !compare)
         {
         if( new_desig[5] == ' ')         /* numbered */
            memcpy( xdesigs - 5, new_desig, 5);
         else
            memcpy( xdesigs, new_desig, 7);
         n_found++;
         }
      tptr += 81;
      xdesigs += 7;
      loc++;
      }
   if( !n_found && new_desig[6] != ' ')
      fprintf( stderr, "No fix for %.7s = %.7s\n", old_desig, new_desig);
/* printf( "Replaced %d,  starting at %ld\n", (int)( loc - loc1), (long)loc1); */
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

      sprintf( buff, "Couldn't open '%s'\n", filename);
      err_exit( buff, -1);
      }
   return( rval);
}

int main( const int argc, const char **argv)
{
   FILE *ifile = err_fopen( "UnnObs.txt", "rb");
   FILE *ofile;
   char *obs, *xdesigs;
   char iline[80];
   size_t i, len, n_lines;
   int add_old_desig = 0;

   printf( "Starting fix_obs.  Total runtime should be a few seconds.\n");
   for( i = 1; i < (size_t)argc; i++)
      if( argv[i][0] == '-')
         switch( argv[i][1])
            {
            case 'x':
               add_old_desig = 1;
               break;
            }

   fseek( ifile, 0L, SEEK_END);
   len = (size_t)ftell( ifile);
   printf( "%ld lines of astrometry\n", (long)len / 81L);
   if( len % 81L)
      err_exit( "UnnObs.txt ought to be a multiple of 81 bytes long.  It isn't.\n"
                "It also should have about 10 million lines of astrometry.\n", -2);
   n_lines = len / 81;
   obs = (char *)malloc( len);
   xdesigs = (char *)calloc( n_lines, 7);
   if( !obs || !xdesigs)
      err_exit( "Couldn't allocate memory (should need about a gigabyte).\n", -3);
   printf( "Memory allocated\n");
   fseek( ifile, 0L, SEEK_SET);
   if( fread( obs, 1, len, ifile) != len)
      err_exit( "Couldn't read all data from UnnObs.txt\n", -4);
   printf( "Astrometry read\n");
   fclose( ifile);

   ifile = err_fopen( "ids.txt", "rb");
   printf( "Adding xdesigs from ids.txt\n");
   while( fgets( iline, sizeof( iline), ifile))
      for( i = 7; iline[i] >= ' '; i += 7)
         fix_desig( obs, n_lines, iline, iline + i, xdesigs);
   fclose( ifile);

   ifile = err_fopen( "numids.txt", "rb");
   if( ifile)
      {
      printf( "Adding xdesigs from numids.txt\n");
      while( fgets( iline, sizeof( iline), ifile))
         {
         char numbered_desig[8];

         memcpy( numbered_desig, iline, 6);
         numbered_desig[6] = ' ';
         numbered_desig[7] = '\0';
         for( i = 6; i < strlen( iline) && iline[i] >= ' '; i += 7)
            fix_desig( obs, n_lines, numbered_desig, iline + i, xdesigs);
         }
      fclose( ifile);
      }

   for( i = 0; i < n_lines; i++)
      if( xdesigs[i * 7])
         {
         char *tptr = obs + i * 81;

         if( add_old_desig && tptr[14] == 'C')
            memcpy( tptr + 56, tptr + 5, 7);
         memcpy( tptr + 5, xdesigs + i * 7, 7);
         }
   printf( "Sorting revised astrometry\n");
   qsort( obs, n_lines, 81, mpc_compare);
   ofile = err_fopen( "UnnObs2.txt", "wb");
   printf( "Writing results to UnnObs2.txt\n");
   fwrite( obs, len, 1, ofile);
   fclose( ofile);
   err_exit( "Success!\n", 0);
}
