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
