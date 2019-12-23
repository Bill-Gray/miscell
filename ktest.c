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

#include <math.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#define PI 3.141592653589793238462643383279502884197169399375105
#define THRESH 1.e-12
#define MIN_THRESH 1.e-15
#define CUBE_ROOT( X)  (exp( log( X) / 3.))

/* Code to test solution to Kepler's equation,  to ensure that it
converges and gets a correct result.  Both are surprisingly more
difficult than you might expect if you want to handle all cases
(highly hyperbolic and nearly parabolic included).  See comments at
the bottom on how the 'test' program works.   */

/* If the eccentricity is very close to parabolic,  and the eccentric
anomaly is quite low,  you can get an unfortunate situation where
roundoff error keeps you from converging.  Consider the just-barely-
elliptical case,  where in Kepler's equation,

M = E - e sin( E)

   E and e sin( E) can be almost identical quantities.  To
around this,  near_parabolic( ) computes E - e sin( E) by expanding
the sine function as a power series:

E - e sin( E) = E - e( E - E^3/3! + E^5/5! - ...)
= (1-e)E + e( -E^3/3! + E^5/5! - ...)

   It's a little bit expensive to do this,  and you only need do it
quite rarely.  (I only encountered the problem because I had orbits
that were supposed to be 'pure parabolic',  but due to roundoff,
they had e = 1+/- epsilon,  with epsilon _very_ small.)  So 'near_parabolic'
is only called if we've gone seven iterations without converging. For
an example where this happens (and where convergence would never occur
if we didn't use this function),  try e=1.000001 or e=.999999 with
mean_anomaly=1e-7.         */

static double near_parabolic( const double ecc_anom, const double e)
{
   const double anom2 = (e > 1. ? ecc_anom * ecc_anom : -ecc_anom * ecc_anom);
   double term = e * anom2 * ecc_anom / 6.;
   double rval = (1. - e) * ecc_anom - term;
   unsigned n = 4;

   while( fabs( term) > 1e-15)
      {
      term *= anom2 / (double)(n * (n + 1));
      rval -= term;
      n += 2;
      }
   return( rval);
}

int verbose = 0;
bool meeus_approx = 1;
unsigned n_iter = 0;

/* For a full description of this function,  see KEPLER.HTM on the Guide
Web site,  http://www.projectpluto.com.  There was a long thread about
solutions to Kepler's equation on sci.astro.amateur,  and I decided to
go into excruciating detail as to how it's done below. */

#define MAX_ITERATIONS 7

static double kepler( const double ecc, double mean_anom)
{
   double curr, err, thresh, offset = 0.;
   double delta_curr = 1.;
   bool is_negative = false;

   n_iter = 0;
   if( !mean_anom)
      return( 0.);

   if( ecc < 1.)
      {
      if( mean_anom < -PI || mean_anom > PI)
         {
         double tmod = fmod( mean_anom, PI * 2.);

         if( tmod > PI)             /* bring mean anom within -pi to +pi */
            tmod -= 2. * PI;
         else if( tmod < -PI)
            tmod += 2. * PI;
         offset = mean_anom - tmod;
         mean_anom = tmod;
         }

      if( ecc < .9)     /* low-eccentricity formula from Meeus,  p. 195 */
         {
         curr = (meeus_approx ? atan2( sin( mean_anom), cos( mean_anom) - ecc) :
                     mean_anom - ecc * .85);
               /* (usually) one or two correction steps,  and we're done */
         do
            {
            err = (curr - ecc * sin( curr) - mean_anom) / (1. - ecc * cos( curr));
            curr -= err;
            n_iter++;
            if( verbose)
               printf( "curr %.13f, err %.13f %u\n",
                   curr * 180. / PI, err * 180. / PI, n_iter);
            }
            while( fabs( err) > THRESH);
         return( curr + offset);
         }
      }

   if( mean_anom < 0.)
      {
      mean_anom = -mean_anom;
      is_negative = true;
      }

   curr = mean_anom;
   thresh = THRESH * fabs( 1. - ecc);
               /* Due to roundoff error,  there's no way we can hope to */
               /* get below a certain minimum threshhold anyway:        */
   if( thresh < MIN_THRESH)
      thresh = MIN_THRESH;
   if( ecc > 1. && mean_anom / ecc > 3.)    /* hyperbolic, large-mean-anomaly case */
      {
      curr = log( mean_anom / ecc) + 0.85;
/*    curr = log( mean_anom / ecc) + (mean_anom < 4. ? 1. : 0.85);   */
      if( verbose)
         printf( "Highly hyperbolic: %f %f %f\n", ecc, mean_anom, curr);
      }
   else if( (ecc > .8 && mean_anom < PI / 3.) || ecc > 1.)    /* up to 60 degrees */
      {
      double trial = mean_anom / fabs( 1. - ecc);

      if( trial * trial > 6. * fabs(1. - ecc))   /* cubic term is dominant */
         trial = CUBE_ROOT( 6. * mean_anom);
      curr = trial;
      if( thresh > THRESH)       /* happens if e > 2. */
         thresh = THRESH;
      }

   if( verbose)
      printf( "Starting with %.15f\n", curr);
   if( ecc < 1.)
      while( fabs( delta_curr) > thresh)
         {
         if( n_iter++ > MAX_ITERATIONS)
            err = near_parabolic( curr, ecc) - mean_anom;
         else
            err = curr - ecc * sin( curr) - mean_anom;
         delta_curr = -err / (1. - ecc * cos( curr));
         curr += delta_curr;
         if( verbose)
            printf( "iter %d: curr = %.15f, delta %.15f\n", n_iter, curr, delta_curr);
         }
   else
      while( fabs( delta_curr) > thresh)
         {
         if( n_iter++ > MAX_ITERATIONS && ecc < 1.01)
            err = -near_parabolic( curr, ecc) - mean_anom;
         else
            err = ecc * sinh( curr) - curr - mean_anom;
         delta_curr = -err / (ecc * cosh( curr) - 1.);
         curr += delta_curr;
         if( verbose)
            printf( "iter %d: curr = %.15f, delta %.15f\n", n_iter, curr, delta_curr);
         }
   return( is_negative ? offset - curr : offset + curr);
}

int main( const int argc, const char **argv)
{
   const double ecc = atof( argv[1]);
   const double mean_anom = atof( argv[2]);
   int i, j;

   for( i = 2; i < argc; i++)
      if( argv[i][0] == '-')
         switch( argv[i][1])
            {
            case 'm':
               meeus_approx = 0;
               printf( "Not using Meeus initial approximation\n");
               break;
            case 'v':
               verbose = 1;
               break;
            }
   if( verbose)      /* show full info on one data point */
      {
      const double ecc_anom = kepler( ecc, mean_anom);

      printf( "%.12f radians = %.12f degrees\n", ecc_anom, ecc_anom * 180. / PI);
      printf( "%d iterations\n", n_iter);
      }
   else
      {
      double worst_ecc = 0., worst_ma = 0.;
      unsigned highest_iter = 0;

      printf( "     +         +         +         +         +         +         +\n");
      for( i = 0; i < 30; i++)
         {
         const double curr_ecc = ecc * pow( 10., (double)i / 10.);
         char hdr[6];

         if( !(i % 10))
            snprintf( hdr, sizeof( hdr), "%-5d", i);
         else
            strcpy( hdr, "     ");
         printf( "%s", hdr);
         for( j = 0; j <= 60; j++)
            {
            const double ma = mean_anom * pow( 10., (double)j / 10.);

            kepler( curr_ecc, ma);
            printf( "%c", (n_iter < 10 ? '0' + n_iter : 'a' + n_iter - 10));
            if( highest_iter < n_iter)
               {
               highest_iter = n_iter;
               worst_ecc = curr_ecc;
               worst_ma = ma;
               }
            }
         printf( " %s\n", hdr);
         }
      printf( "     +         +         +         +         +         +         +\n");
      printf( "Highest number of iter = %u at ecc %f, MA %f\n",
                     highest_iter, worst_ecc, worst_ma);
      }
   return( 0);
}

/* Example output of this program.  The following shows the number of
iterations required,  starting with e=.1000001 and mean anomaly=.0001
radians at upper left.  Every ten columns,  the mean anomaly increases
tenfold (increasing by a factor of a million at the right edge,  MA=100)
and the eccentricity tenfold every ten lines (by a factor of a thousand,
to e=100.0001,  at the bottom line.)  You'll see that the number of
iterations is usually quite low,  peaking at the tenth line with
e=1.000001;  near-parabolic cases are challenging.

phred@phred:~/miscell$ ./ktest .1000001 .0001
     +         +         +         +         +         +         +
0    1111111111112222222222222222222222233333333332332332232232233 0
     1111111111122222222222222222222222333333333332332332232232333
     1111111111222222222222222222222223333333333332332332232332333
     1111111112222222222222222222222233333333333332332332232333333
     1111111222222222222222222222223333333333333332332332232333333
     1111112222222222222222222222233333333334443332342332242333333
     1111222222222222222222222223333333333444444332342432242333334
     1122222222222222222222222333333333444444444432343432342333434
     2222222222222222222222333333333444444444444432343433343444434
     2222222222222222233333333344444455555555544432343433343545545
10   4a45366455444444444444444444444555555555555566666555544555555 10
     2222222222222222333333333344444555666666666666655555444455555
     2222222222222222222223333333333444455556666666655555434455555
     1122222222222222222222222333333333444455556677775554444455555
     1111222222222222222222222223333333334444455566777554444455555
     1111112222222222222222222222233333333344445555667844444455555
     1111111222222222222222222222223333333334444455566774444555555
     1111111112222222222222222222222233333333344445556667444555555
     1111111111222222222222222222222223333333334444455566744555555
     1111111111122222222222222222222222333333333444445556674555555
20   1111111111112222222222222222222222233333333334444555667455555 20
     1111111111111122222222222222222222223333333333444455566755555
     1111111111111112222222222222222222222233333333344445556665555
     1111111111111111222222222222222222222223333333334444455566555
     1111111111111111122222222222222222222222333333333444445556655
     1111111111111111112222222222222222222222233333333344444555665
     1111111111111111111222222222222222222222223333333334444455566
     1111111111111111111122222222222222222222222333333333444445556
     1111111111111111111112222222222222222222222233333333344444555
     1111111111111111111111222222222222222222222223333333334444455
     +         +         +         +         +         +         +
Highest number of iter = 10 at ecc 1.000001, MA 0.000126

   The above sort of plot should work Just Fine for n_iter=35,
printing a 'z',  but I've not been able to find a case with more
than ten iterations.
*/
