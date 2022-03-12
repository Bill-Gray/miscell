#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

/* Code to read in MPCORB.DAT and create the incl_vs_a_scattergram[] used
in Find_Orb's (q.v.) orbit evaluation scheme.  Compiled with :

gcc -Wall -Wextra -pedantic -o incl_a incl_a.c -lm

   Previously,  the chart had values from 0 to 9.  This wasn't granular
enough,  and there were artifacts.  Run with a scale of 8 in March 2022,
the 'assert( ochar < 127)' line didn't trigger.  A smaller scale may
be needed for future runs.  Or,  more likely,  I should switch to a more
Digest2-like scheme in which the histogram involves quantities other than
just inclination and semimajor axis.   */

int main( const int argc, const char **argv)
{
   FILE *ifile = fopen( "MPCORB.DAT", "rb");
   char buff[300];
   int counts[30][60];
   int i, j;
   const double scale = (argc == 1 ? 1. : atof( argv[1]));
   const char *header_line =
     "/*           0 AU      1         2         3         4         5         6 */";

   printf( "%s\n", header_line);
   printf( "static const char *incl_vs_a_scattergram[30] = {\n");
   assert( ifile);
   for( i = 0; i < 30; i++)
      for( j = 0; j < 60; j++)
         counts[i][j] = 0;
   while( fgets( buff, sizeof( buff), ifile))
      if( strlen( buff) > 200)
         {
         const int incl_bin = atoi( buff + 59) / 2;
         const int a_bin = (int)(atof( buff + 92) * 10.);

         if( incl_bin < 30 && a_bin < 60)
            counts[incl_bin][a_bin]++;
         }
   fclose( ifile);
   for( i = 0; i < 30; i++)
      {
      if( !i)
         printf( "/* incl=0*/ ");
      else if( !(i % 5))
         printf( "/* %d deg*/ ", i * 2);
      else
         printf( "            ");
      printf( "\"");
      for( j = 0; j < 60; j++)
         {
         double zval = log( (double)counts[i][j] + 1.);
         int ochar = ' ' + (int)( scale * zval);

         assert( ochar < 127);
         if( ochar == '"' || ochar == '\\')      /* sidestep disallowed values */
            ochar++;
         printf( "%c", ochar);
         }
      if( i == 29)
         printf( "\" };\n");
      else
         printf( "\",\n");
      }
   return( 0);
}
