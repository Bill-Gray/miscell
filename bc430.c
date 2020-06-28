#include <stdio.h>
#include <stdlib.h>

/* Example code to extract elements from BC430.  Following is what
you ought to get for (980) Anacostia for JD 2459000.5 = 2020 May 31.
For comparison,  see elems at https://ssd.jpl.nasa.gov/sbdb.cgi?ID=a0000980 .

Compile with

gcc -Wall -Wextra -pedantic -o bc430 bc430.c

BC430 itself is available at

https://docs.google.com/document/d/1bZIpK99YNwYnxNLsaMsxfrY6fJ-wGRNWNcDzWmGvP9s/edit

Run the following as a test :

phred@phreddie:~/bc430$ ./bc430 980 2459000.5
Epoch = JD 2459015
Semimajor axis (AU) 2.7393215869271588
Eccentricity 0.20319271032296
Perihelion dist (AU) 2.18271141
Incl (deg)       15.90833532
Arg per (deg)    69.87707334
Asc node (deg)  285.81102290
Mean anom (deg) 265.29725955

BC430 has (I think) the same 300 asteroids as its predecessor,  BC405.
The new version orders the asteroids by decreasing mass (BC405 used
a nearly,  but not entirely,  unordered list... maybe ordered by
the initial masses before they were re-determined?)  Actual elements
are very slightly different,  due to improvements between DE405 and
DE430.
*/

#define N_ASTEROIDS 300
#define START_JD 2378495
#define END_JD   2524615
#define JD_STEP      40
#define PI 3.1415926535897932384626433832795028841971693993751058209749445923

static void err_exit( void)
{
   fprintf( stderr, "'bc430' takes the asteroid number and desired epoch as\n"
                    "command line arguments,  and outputs the elements.\n"
                    "Example usage (for (449) Hamburga,  JD 2451545) :\n"
                    "\n./bc430 449 2451545\n");
   exit( -1);
}

int main( const int argc, const char **argv)
{
   long epoch, idx = -1, line_no = 0, obj_number;
   char buff[300];
   FILE *ifile = fopen( "asteroid_indices.txt", "rb");
   double a, ecc;

   if( !ifile)
      {
      fprintf( stderr, "Couldn't open 'asteroid_indices.txt'\n");
      err_exit( );
      }
   if( argc < 3)
      {
      fprintf( stderr, "Need object number and epoch on command line\n");
      err_exit( );
      }
   obj_number = atol( argv[1]);
   while( idx == -1 && fgets( buff, sizeof( buff), ifile))
      {
      if( obj_number == atol( buff))
         idx = line_no;
      line_no++;
      }
   fclose( ifile);
   if( idx == -1)
      {
      fprintf( stderr, "'%s' is not a valid object index\n", argv[1]);
      fprintf( stderr, "(Valid asteroid numbers are listed in 'asteroid_indices.txt')\n");
      err_exit( );
      }
   epoch = atol( argv[2]) - START_JD;
   if( epoch > END_JD - START_JD || epoch < 0)
      {
      fprintf( stderr, "'%s' is not a valid epoch\n", argv[2]);
      fprintf( stderr, "(BC430 covers JD %d to %d)\n",
                     START_JD, END_JD);
      err_exit( );
      }
   epoch += JD_STEP / 2;         /* round,  not truncate */
   printf( "Epoch = JD %ld\n", START_JD + epoch - (epoch % JD_STEP));
   idx = (idx + (epoch / JD_STEP) * N_ASTEROIDS) * 6;
   ifile = fopen( "asteroid_ephemeris.txt", "rb");

   if( !ifile)
      {
      fprintf( stderr, "Couldn't open 'asteroid_ephemeris.txt'\n");
      err_exit( );
      }
   while( idx--)
      fgets( buff, sizeof( buff), ifile);
   fgets( buff, sizeof( buff), ifile);
   a = atof( buff);
   printf( "Semimajor axis (AU) %s", buff);
   fgets( buff, sizeof( buff), ifile);
   ecc = atof( buff);
   printf( "Eccentricity %s", buff);
   printf( "Perihelion dist (AU) %.8f\n", a * (1. - ecc));
   fgets( buff, sizeof( buff), ifile);
   printf( "Incl (deg)      %12.8f\n", atof( buff) * 180. / PI);
   fgets( buff, sizeof( buff), ifile);
   printf( "Arg per (deg)   %12.8f\n", atof( buff) * 180. / PI);
   fgets( buff, sizeof( buff), ifile);
   printf( "Asc node (deg)  %12.8f\n", atof( buff) * 180. / PI);
   fgets( buff, sizeof( buff), ifile);
   printf( "Mean anom (deg) %12.8f\n", atof( buff) * 180. / PI);
   fclose( ifile);
   return( 0);
}
