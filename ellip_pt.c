#include <math.h>
#include <stdbool.h>
#include <stdio.h>

#define AU_IN_METERS 1.495978707e+11
#define EARTH_MAJOR_AXIS 6378140.
#define EARTH_MINOR_AXIS 6356755.

int lat_alt_to_parallax( const double lat, const double ht_in_meters,
            double *rho_cos_phi, double *rho_sin_phi, const int planet_idx);
double parallax_to_lat_alt( const double rho_cos_phi, const double rho_sin_phi,
               double *lat_out, const int planet_idx);
double approx_parallax_to_lat_alt( const double x, const double y, double *lat);

/* Revise these next two functions if you're handling planets
other than the earth.  I do that in much of my software,  but
not for this little demonstration code : */

#pragma GCC diagnostic ignored "-Wunused-parameter"

double axis_ratio = EARTH_MINOR_AXIS / EARTH_MAJOR_AXIS;

static double planet_radius_in_meters( const int planet_idx)
{
   return( EARTH_MAJOR_AXIS);
}

static double planet_axis_ratio( const int planet_idx)
{
   return( axis_ratio);
}

#pragma GCC diagnostic warning "-Wunused-parameter"

int lat_alt_to_parallax( const double lat, const double ht_in_meters,
            double *rho_cos_phi, double *rho_sin_phi, const int planet_idx)
{
   const double axis_ratio = planet_axis_ratio( planet_idx);
   const double major_axis_in_meters = planet_radius_in_meters( planet_idx);
   const double u = atan2( sin( lat) * axis_ratio, cos( lat));

   *rho_sin_phi = axis_ratio * sin( u) +
                            (ht_in_meters / major_axis_in_meters) * sin( lat);
   *rho_cos_phi = cos( u) + (ht_in_meters / major_axis_in_meters) * cos( lat);
   return( 0);
}

/* Code to determine the point on the ellipse

P = (a cos(u), b sin(u))

   i.e.,  ellipse centered on the origin with semimajor axes a and b, that
is closest to a given point (x, y). There is an analytic solution to this
problem,  resulting in a quartic polynomial.  But for practical use,  the
following root-finding method is considerably more efficient, and also
avoids some loss of precision issues you get with the analytic method.

   The problem arises in converting parallax constants (rho sin phi,
rho cos phi) to geographical latitude and altitude.  (In that context,
phi is the geocentric latitude and u is the reduced,  or parametric,
latitude.)  It also can crop up in determining the MOID (Minimum Orbit
Intersection Distance,  closest point between two non-coplanar ellipses.)

   There can be up to four extrema.  So we put (x, y) in the first
quadrant and look for a closest point in that quadrant;  that'll get us
the global minimum.  Also,  just to simplify the math very slightly,  we
scale everything by 1/a,  so that the ellipse has a semimajor axis of 1.
(Note that for parallax constants,  where the earth's semimajor axis is
the unit of length,  a=1 anyway,  and x is always positive.  But this
has been written with an eye toward general-purpose use.)

   OK.  We now have P = (cos( u), b sin(u)),  and
dP/du = (-sin( u), b cos( u)). We want that vector to run at right angles
to the one connecting P to (x, y), i.e,  a zero dot product. Call that dot
product 'z',  a function of u :

z = (x - cos u)(-sin u) + (y-b sin u)(b cos u)
  = (1 - b^2) sin u cos u - x sin u + by cos u

   [Set c = 1 - b^2,  and note that for y=0, x=c cos( u)    ]

   = (c cos u - x)sin u + by cos u

dz/du = c(cos^2 u -sin^2 u) - x cos u - by sin u
z(u=0) = by
z(u=90) = -x

   Which leads to another reason for doing things in the first quadrant:
it means we have z(u) = 0 bracked between u=0 and u=90.  The root search
used below starts with those two values and uses the secant method,  which
can shoot off for this function;  the brackets are used to guarantee that
convergence will occur.       */

#define PI 3.1415926535897932384626433832795028841971693993751058209749445923

double find_closest_point_on_ellipse( double x, double y, const double a,
                double b, double *lat)
{
   double min_u = 0., max_u = PI / 2.;
   double u1 = PI / 2., z1;
   double u2 = 0., z2;
   const double tolerance = 1e-13;
   double dist, c;
   const bool x_positive = (x > 0.);
   const bool y_positive = (y > 0.);

   if( !y_positive)     /* Easiest if we work in first quadrant */
      y = -y;
   if( !x_positive)
      x = -x;
   b /= a;
   x /= a;
   y /= a;
   c = 1. - b * b;
   z2 = b * y;
   z1 = -x;
   do
      {
      const double u0 = (u1 * z2 - u2 * z1) / (z2 - z1);
      const double u = (u0 >= min_u && u0 <= max_u) ?
                  u0 : (min_u + max_u) / 2.;
      const double sin_u = sin( u), cos_u = cos( u);
      const double z = (c * cos_u - x) * sin_u + b * y * cos_u;

#ifdef DEBUG_PRINTFS
      if( u0 < min_u || u0 > max_u)
         printf( "Secant method failed; bisecting\n");
      printf( " u %.12f, dist %.12f\n", u * 180. / PI, z);
#endif
      u2 = u1;
      z2 = z1;
      u1 = u;
      z1 = z;
      if( z >= 0.)
         min_u = u;
      else
         max_u = u;
      }
      while( fabs( z1) > tolerance);

   *lat = atan2( sin( u1), cos( u1) * b);
   dist = cos( *lat) * (x - cos( u1)) + sin( *lat) * (y - b * sin( u1));
   if( !y_positive)
      *lat = -*lat;
   if( !x_positive)
      *lat = PI - *lat;
   return( a * dist);
}

double parallax_to_lat_alt( const double rho_cos_phi, const double rho_sin_phi,
               double *lat_out, const int planet_idx)
{
   const double axis_ratio = planet_axis_ratio( planet_idx);
   const double major_axis_in_meters = planet_radius_in_meters( planet_idx);
   const double rval = find_closest_point_on_ellipse( rho_cos_phi, rho_sin_phi,
                           1., axis_ratio, lat_out);

   return( rval * major_axis_in_meters);
}

/* The following might be of interest if you're only handling the problem
of converting parallax constants to latitude/altitude.

   Quoting from

http://www.spaceroots.org/documents/distance/distance-to-ellipse.pdf

   "The technical manual for _Geocentric Datum of Australia_ provides direct
transformation formulae reproduced here... some numerical checks show that
these expressions are very good approximations,  but can be applied only for
almost spherical bodies.  For the earth,  maximum errors are about 1e-11
degree in latitude [about one micron] and 2e-9 metres in altitude!  However,
when we consider very flat ellipses (for example f=0.9),  the errors become
more than 100 degrees."  [To which I'd add:  even with nearly circular
ellipses,  this formula fails badly for points near the centre of the
ellipse.  So this is fine if you're just dealing with geodetic computations,
but isn't really useful for me;  computing MOIDs can involve higher
eccentricities and points near the center.]

   The technical manual in question can be found at

http://www.icsm.gov.au/gda/gda-v_2.4.pdf

   See page 33 ("Conversion between Geographical and Cartesian Coordinates")
for the formula.   */

double approx_parallax_to_lat_alt( const double x, const double y, double *lat)
{
   const double flat = planet_axis_ratio( 3);
   const double f = 1. - flat, k = f * (2. - f);
   const double tan_u = y * (flat + k / sqrt( x * x + y * y)) / x;
   const double sec_u = sqrt( 1. + tan_u * tan_u);
   const double sin_u = tan_u / sec_u, cos_u = 1. / sec_u;
   const double tan_phi = (y * flat + k * sin_u * sin_u * sin_u)
                       / (flat * (x - k * cos_u * cos_u * cos_u));
   const double sec_phi = sqrt( 1. + tan_phi * tan_phi);
   const double sin_phi = tan_phi / sec_phi, cos_phi = 1. / sec_phi;
   const double alt = x * cos_phi + y * sin_phi - sqrt( 1. - k * sin_phi * sin_phi);

   *lat = atan( tan_phi);
   return( alt * planet_radius_in_meters( 3));
}

/* An approximation from p. 83,  Meeus' _Astronomical Algorithms_,
relying on a series conversion from geocentric to geographic
latitude.  (I've generalized it to take the axis ratio as a
parameter.) On the ellipsoid (altitude = 0),  this is good to
better than a meter.  At altitudes of several kilometers,  it's
still good to about ten meters. */

double approx_lat_from_parallax( const double rho_cos_phi,
                  const double rho_sin_phi, const double axis_ratio)
{
   const double flattening = 1. - axis_ratio;
   const double phi = atan2( rho_sin_phi, rho_cos_phi);
   const double lat = phi + flattening * (sin( phi * 2.)
                                 + .5 * flattening * sin( phi * 4.));

   return( lat);
}

static void show_error_msg( void)
{
   printf( "'ellip_pt' tests various methods of converting parallax\n"
       "constants to/from latitude/altitude.  Run it as either:\n"
       "\n"
       "ellip_pt (rho cos phi) (rho sin phi) p\n"
       "ellip_pt (latitude) (altitude in meters) p\n"
       "\n"
       "To this can be added 'e(axis ratio).  The default axis ratio\n"
       "is that for the WGS-84 ellipsoid.\n");
}

#include <stdlib.h>

int main( int argc, const char **argv)
{
   double lat, ht_in_meters;
   double rho_cos_phi, rho_sin_phi;

   if( argc < 3)
      {
      show_error_msg( );
      return( -1);
      }
   if( argc > 3 && argv[argc - 1][0] == 'e')
      {
      axis_ratio = atof( argv[argc - 1] + 1);
      printf( "Reset axis ratio to %f\n", axis_ratio);
      argc--;
      }
   if( argc == 3)
      {
      lat = atof( argv[1]) * PI / 180.;
      ht_in_meters = atof( argv[2]);
      lat_alt_to_parallax( lat, ht_in_meters, &rho_cos_phi, &rho_sin_phi, 3);
      printf( "Parallax constants %.12f, %.12f\n", rho_cos_phi, rho_sin_phi);
      }
   else
      {
      rho_cos_phi = atof( argv[1]);
      rho_sin_phi = atof( argv[2]);
      }
   ht_in_meters = parallax_to_lat_alt( rho_cos_phi, rho_sin_phi, &lat, 3);
   printf( "Lat %.12f   alt %.12f\n", lat * 180. / PI, ht_in_meters);
   ht_in_meters = approx_parallax_to_lat_alt( rho_cos_phi, rho_sin_phi, &lat);
   printf( "Lat %.12f   alt %.12f (Australian approximation)\n",
               lat * 180. / PI, ht_in_meters);
   printf( "Lat %.12f (Meeus approximation)\n",
            approx_lat_from_parallax( rho_cos_phi, rho_sin_phi, axis_ratio) * 180. / PI);

         /* From Wolfgang Wepner, _Mathematisches Hilfsbuch für Studierende
            und Freunde der Astronomie, Dr. Vehrenberg KG, Düsseldorf 1982.
            Good to within 2 meters on the surface of the ellipsoid,  but off
            by 20 meters at the altitude of Everest.  Again,  I've generalized
            the method to apply to any ellipsoid. */
   printf( "Lat %.12f (Wepner 1982)\n",
            atan( (rho_sin_phi / rho_cos_phi) / (2 * axis_ratio - 1.)) * 180. / PI);
   return( 0);
}
