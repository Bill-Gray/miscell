#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* This reads in UnnObs.txt (MPC file of astrometry for unnumbered objects)
and the list of identifications and list of double designations,  available at

http://www.minorplanetcenter.net/iau/ECS/MPCAT-OBS/MPCAT-OBS.html
http://www.minorplanetcenter.net/iau/ECS/MPCAT/MPCAT.html

   the input files being named 'UnnObs.txt',  'ids.txt',  and 'dbl.txt'.

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
