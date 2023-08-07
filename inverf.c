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

#define SQRT_PI 1.7724538509055160272981674833411451827975494561223871282138077898529113

/* In computing the inverse of the error function,  it helps that the
derivative of erf( ) is easily computed.  Easy derivatives mean easy
Newton-Raphson root-finding.  An easy second derivative,  in this case,
means an easy Halley's root-finder,  with cubic convergence.

d erf(x)
-------- = (2/sqrt(pi)) exp( -x^2)
  dx

d2 erf(x)      d erf(x)
-------- = -2x --------
  d2x            dx

   Some cancelling out of terms happens with the second derivative
that make it particularly well-suited to Halley's method.

   We start out with a rough approximation for inverf( y),  and
then do a root search for y = erf( x) with Halley's method.  At
most,  three iterations are required,  near y = +/- erf_limit.   */

#ifdef DEBUGGING
#include <stdio.h>
#endif

double inverf( const double y)
{
   double x, diff;
   const double erf_limit = .915;
   const double t = sqrt( -2. * log( (1. - y) / 2.));
   const double guess = -0.70711*((2.30753+t*0.27061)/(1.+t*(0.99229+t*0.04481)) - t);

   printf( "NR guess %.8f\n", guess);
   if( y < -erf_limit)
      return( -inverf( -y));
   else if( y > erf_limit)
      x = sqrt( -log( 1. - y)) - .34;
   else         /* a passable cubic approximation for -.915 < y < .915 */
      x = y * (.8963 + y * y * (.0889 + .494 * y * y));
   do
      {
      const double dy = erf( x) - y;
      const double slope = (2. / SQRT_PI) * exp( -x * x);

      diff = -dy / slope;   /* Just doing this would be Newton-Raphson */
      diff = -dy / (slope + x * dy);   /* This gets us Halley's method */
#ifdef DEBUGGING
      printf( "x: %.15f  diff %.15f erf( x): %.15f\n", x, diff, dy);
#endif
      x += diff;
      }
      while( fabs( diff) > 1e-14);
   return( x);
}

#include <stdlib.h>
#include <stdio.h>

int main( const int argc, const char **argv)
{
   const double y = (argc > 1 ? atof( argv[1]) : 0.6);
   const double x = inverf( y);

   printf( "x: %.15f   erf( x): %.15f\n", x, erf( x));
   return( 0);
}
