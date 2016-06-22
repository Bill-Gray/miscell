/* Assume we need to solve for a transfer orbit wherein the object
starts out at distance r1 from the sun,  and gets to a distance r2
at an angle theta0 (as seen from the sun) at some time delta_t later.
If we set axes such that the object starts out along the x-axis,
then we've gone from (r1, 0) to (r2 * cos(theta0), r2 * sin(theta0)).

   In this system,  the orbit presumably has a periapsis at some
angle theta.  Thus,

(1) r1 = k / (1 + ecc * cos( theta))
(2) r2 = k / (1 + ecc * cos( theta - theta0))

   where k = q * (1 + ecc).

   Guess a given theta,  and we can determine k and ecc,  because

(3) k = r1 + ecc * r1 * cos( theta)
(4) k = r2 + ecc * r2 * cos( theta - theta0)

(5) (r2 - r1) = ecc * (r1 * cos( theta) - r2 * cos( theta - theta0))
(6) ecc = (r2 - r1) / (r1 * cos( theta) - r2 * cos( theta - theta0))

   Once we have ecc,  substituting into (3) or (4) will get us k.

   There is a range of possible orbits running from "zip right across
in a straight line at infinite speed" to "go out in a parabolic orbit,
hang out near infinity,  and come back after the heat death of the
universe".  These correspond to delta_t = 0 and delta_t = +infinity.
For the first case,  the path defines a straight line passing through
both points,  and theta can be determined by the slope of that line:

(7) tan( theta) = -(r1 - r2 * cos( theta0)) / (r2 * sin( theta0))

   (best handled with the atan2( ) function,  of course.)

   In the second case,  the eccentricity is essentially 1,  and (5)
above becomes

 (r2 - r1) = r1 * cos( theta) - r2 * cos( theta - theta0)
           = r1 * cos( theta) - r2 * cos( theta) * cos( theta0)
                              - r2 * sin( theta) * sin( theta0)
           = C cos( theta) + S sin( theta)

   where C = r1 - r2 * cos( theta0) and S = -r2 * sin( theta0).
Set phi = atan( S / C), A = sqrt( C * C + S * S),  so that
C = A * cos( phi) and S = A * sin( phi),  and one gets

   (r2 - r1 / A) = cos( phi) * cos( theta) + sin( phi) * sin( theta)
                 = cos( theta - phi)

   and we have the other extremum for theta.  (Note that since the
arccosine function is double-valued,  we can determine _two_ values
of theta for which e=1.  The "other" theta corresponds to a finite-time
transfer,  and might be useful for some purpose or another... it's
computed below just for testing purposes/future use.)
*/

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef _MSC_VER
static double atanh( const double arg)
{
   return( .5 * log( (arg + 1.) / (1. - arg)));
}
#endif

const double PI = 3.1415926535897932384626433832795028841971693993751058209749445923;
const double gauss_k = .01720209895;

static double conic_area( const double theta1, const double theta2,
                          const double r1, const double r2,
                          double *ecc_out, double *q_out)
{
   const double cos_theta1 = cos( theta1);
   const double cos_theta2 = cos( theta2);
   const double ecc =               /* see eqn (6) above */
             (r2 - r1) / (r1 * cos_theta1 - r2 * cos_theta2);
   const double q = r1 * (1. + ecc * cos_theta1) / (1. + ecc);
   const double a = q / (1. - ecc);
   const double temp = 1. - ecc * ecc;
   const double t0 = a * sqrt( fabs( a));
   const double sqrt_1_minus_ecc2 = sqrt( fabs( temp));
   int pass;
   double rval = 0.;

   for( pass = 0; pass < 2; pass++)
      {
      const double theta = (pass ? theta2 : theta1);
      const double cos_theta = (pass ? cos_theta2 : cos_theta1);
      const double r = temp / (1. + ecc * cos_theta);
      const double x = ecc + r * cos_theta;
      const double y = r * sin( theta) / sqrt_1_minus_ecc2;
      double piece;

      if( temp > 0.)       /* elliptical case */
         {
         piece = atan2( y, x) - y * ecc;
         if( theta > PI)
            piece += PI + PI;
         else if( theta < -PI)
            piece -= PI + PI;
         }
      else                 /* hyperbolic case */
         piece = y * ecc - atanh( y / x);
      if( pass)
         rval -= piece;
      else
         rval = piece;
      }
   if( q_out)
      *q_out = q;
   if( ecc_out)
      *ecc_out = ecc;
   return( rval * t0 / gauss_k);
}

/* Euler found that for a parabolic orbit starting at distance r1 from
the sun,  ending up at a distance r2,  with a straight-line distance s
between them,  the time required can be found from

6kt = (r1 + r2 - s) ^ 1.5 +/- (r1 + r2 - s) ^ 1.5

   ...taking the negative sign for the 'direct' route (you go less than
180 degrees around the sun) and the positive one for the 'indirect' route
(going the long way,  more than 180 degrees around the sun).  I don't
think the latter will be useful for a while,  but perhaps eventually if
we're computing paths that go more than half an orbit.         */

static double compute_parabolic_times( const double r1, const double r2,
                                 const double theta0)
{
   const double s = sqrt( r1 * r1 + r2 * r2 - 2. * r1 * r2 * cos( theta0));
   double z1 = r1 + r2 - s;
   double z2 = r1 + r2 + s;

   z1 *= sqrt( z1) / (6 * gauss_k);
   z2 *= sqrt( z2) / (6 * gauss_k);
   printf( "Parabolic times: %f %f\n", z2 - z1, z2 + z1);
   return( z2 - z1);
}

static void compute_ranges( const double r1, const double r2,
         const double theta0, double *min_theta, double *max_theta,
         double *parabolic_theta)
{
   const double cos_theta0 = cos( theta0);
   const double sin_theta0 = sin( theta0);
   const double big_c = r1 - r2 * cos_theta0;
   const double big_s = -r2 * sin_theta0;
   const double phi = atan2( big_s, big_c);
   const double big_a = sqrt( big_c * big_c + big_s * big_s);
   const double cos_diff = (r1 - r2) / big_a;

   *min_theta = phi + PI / 2.;
   *max_theta = acos( cos_diff) + phi + (r1 > r2 ? PI : -PI);
   *parabolic_theta = PI - acos( cos_diff) + phi;
}

int main( const int argc, const char **argv)
{
   const double r1 = atof( argv[1]);
   const double r2 = atof( argv[2]);
   const double theta0 = atof( argv[3]) * PI / 180.;
   unsigned i;
   unsigned n_splits = (argc >= 5 ? (unsigned)atoi( argv[4]) : 45);
   const char *header_trailer = "angle       ecc        q         time    time^2\n";
   const double t_parabolic = compute_parabolic_times( r1, r2, theta0);
   double max_theta, min_theta, parabolic_theta;

   compute_ranges( r1, r2, theta0, &min_theta, &max_theta, &parabolic_theta);
   printf( "min_theta = %f deg\nmax_theta = %f deg\n",
               min_theta * 180. / PI,
               max_theta * 180. / PI);
   printf( "other_max_theta = %f deg\n",
               parabolic_theta * 180. / PI);

   printf( "%s", header_trailer);
   for( i = 0; i <= n_splits; i++)
      {
      const double theta = min_theta +
            (double)i * (max_theta - min_theta) / (double)n_splits;
      double ecc, q;
      const double dt = conic_area( theta, theta - theta0, r1, r2, &ecc, &q);

      printf( "%9.4f: %f %f %f %f\n", theta * 180. / PI, ecc, q, dt, dt * dt);
      }
   printf( "%s", header_trailer);
   if( argc >= 6)
      {
      const double dt0 = atof( argv[5]);  /* transfer time in days */
      double y0 = -dt0 * dt0, y1 = 1e+30;
      const double tolerance = 1e-5 * dt0;     /* about 0.864 seconds */
      double x[3], y[3];

      if( dt0 > t_parabolic)
         {
         min_theta = parabolic_theta;
         y0 += t_parabolic * t_parabolic;
         }
      i = 0;
      x[0] = min_theta;
      y[0] = y0;
      y[1] = y[2] = 1e+30;
      while( y0 < -tolerance && y1 > tolerance)
         {
         double ecc, q, dt, new_y;
         double theta = (min_theta + max_theta) * .5;
         unsigned j, k;

         if( i >= 2 && i % 3 != 1)
            {
            const double fa_fb = y[0] - y[1];
            const double fa_fc = y[0] - y[2];
            const double fb_fc = y[1] - y[2];
            const double s = -(x[1] - x[0]) * y[2] / fa_fb
                           + (x[2] - x[0]) * y[1] / fa_fc;
            const double quad_theta = x[0] + s * y[0] / fb_fc;

            printf( "Range: %f to %f\n",
                     min_theta * 180. / PI, max_theta * 180. / PI);
            if( (quad_theta - min_theta) * (quad_theta - max_theta) < 0)
               {
               theta = quad_theta;
               printf( "quad theta %f\n", quad_theta * 180. / PI);
               }
            else
               printf( "theta=%f rejected\n", quad_theta * 180. / PI);
            }

         dt = conic_area( theta, theta - theta0, r1, r2, &ecc, &q);
         new_y = dt * dt - dt0 * dt0;
         printf( "%9.4f: %f %f %f %f\n", theta * 180. / PI, ecc, q, dt, dt * dt);
         if( new_y > 0.)
            {
            max_theta = theta;
            y1 = new_y;
            }
         else
            {
            min_theta = theta;
            y0 = new_y;
            }
         i++;
         for( j = k = 0; j < 3; j++)
            if( fabs( y[j]) > fabs( y[k]))
               k = j;
         if( fabs( y[k]) > fabs( new_y))
            {
            y[k] = new_y;
            x[k] = theta;
            }
         }
      }
   return( 0);
}
