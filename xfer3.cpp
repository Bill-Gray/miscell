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
#include <math.h>

#define PI 3.1415926535897932384626433832795028841971693993751058209749445923


/* We want to determine an orbit that gets us from a point P1 = (x1, y1) to
a point P2 = (x2, y2),  orbiting the origin O = (0, 0),  and taking a time
t0.  There are several ways of doing this,  most of which seem a little
overcomplicated to me,  and/or are not guaranteed to converge in all cases.
With a fair bit of effort,  I've come up with the following (still somewhat
complicated) method of tackling the problem.

   Our first step is to use Euler's equation to determine the time it would
take,  on a parabolic trajectory,  to get from P1 to P2 :

t_parabolic = [(r1 + r2 + s) ^ 1.5 - (r1 + r2 - s) ^ 1.5] / 6k

   (k = Gauss' constant).  If t0 < t_parabolic,  we'll have to take an
hyperbolic orbit to get from P1 to P2 that quickly.  Otherwise,  an
elliptical orbit will suffice.

   Our next step may seem like a complete non-sequitur:  we know that the
orbit will be a conic section with O at one focus (and we even know,  now,
if that conic section is an ellipse or an hyperbola);  where is the other,
unoccupied focus,  F = (xf, yf) ?


          P2_
             \
       F      \
               |
O              P1




   OP1 = r1 = sqrt( x1^2 + y1^2)
   OP2 = r2 = sqrt( x2^2 + y2^2)
   P1P2 = r3 = sqrt( (x1-x2)^2 + (y1-y2)^2)
   FP1 = q1 = sqrt( (xf-x1)^2 + (yf-y1)^2)
   FP2 = q2 = sqrt( (xf-x2)^2 + (yf-y2)^2)

   If the conic section described by the orbit is an ellipse,  then we'll
know that r1 + q1 = r2 + q2 = 2a;  that is,  for any point on the orbit,  the
sum of the distances from that point to the foci will be a constant,  equal
to twice the semimajor axis of the orbit.  A very minor rearrangement gets
us q1 - q2 = r2 - r1;  that is to say,  (still assuming the orbit is
elliptical),  F has to lie on one branch of an hyperbola whose foci are at
P1 and P2.

   If the conic section described by the orbit is an hyperbola,  then we'll
know that r1 - q1 = r2 - q2;  that is,  for any point on the orbit,  the
differences of the distances from that point to the foci will be a constant.
A very minor rearrangement gets us q1 - q2 = r1 - r2;  that puts F on the
_other_ branch of the hyperbola we described in the previous paragraph.

   So we now know that the unoccupied focus is somewhere on (one branch of)
an hyperbola,  and we even know which branch it'll be.  Question remains,
though,  _where_ on that branch?

   I'll state the following without proof.  For the elliptical orbit,
as the other focus is moved out to infinity along the asymptotes,  you
approach parabolic orbits.  Along one asymptote (the one on the same side
of P1P2 as O is),  you get a "short" parabolic orbit,  the one corresponding
to the t_parabolic we got from Euler's equation.  As the focus moves in from
there,  you gradually      */

static void compute_xfer_times( const double r1, const double r2,
                                const double theta0)
{
   const double x1 = r1, y1 = 0.;
   const double x2 = r2 * cos( theta0);
   const double y2 = r2 * sin( theta0);
   const double dx2 = x2 - x1;
   const double dy2 = y2 - y1;         /* = y2,  since y1 = 0 */
   const double r3 = sqrt( dy2 * dy2 + dx2 * dx2);
   const double q = (r3 + r1 - r2) / 2.;
   const double numerator = .5 * (r3 - (r1 - r2) * (r1 - r2) / r3);

   printf( "r3 = %f; q = %f\n", r3, q);
   printf( "critical theta = %f\n", acos( (r1 - r2) / r3));
   for( double theta = 0.; theta < 2 * PI; theta += PI / 10.)
      {
      const double cos_theta = cos( theta);
      const double sin_theta = sin( theta);
      const double r = numerator / (r3 * cos_theta + r2 - r1);
      const double xf = r1 + r * (dx2 * cos_theta + dy2 * sin_theta);
      const double yf =      r * (dy2 * cos_theta - dx2 * sin_theta);
      const double q1 = sqrt( (xf - x1) * (xf - x1) + (yf - y1) * (yf - y1));
      const double q2 = sqrt( (xf - x2) * (xf - x2) + (yf - y2) * (yf - y2));

      printf( "%f: r=%f x=%f y=%f q1-q2=%f  %f\n",
                  theta, r, xf, yf, q1 - q2, q1 / r3);
      }
}


int main( const int argc, const char **argv)
{
   if( argc >= 4)
      {
      const double r1 = atof( argv[1]);
      const double r2 = atof( argv[2]);
      const double theta0 = atof( argv[3]) * PI / 180.;

      compute_xfer_times( r1, r2, theta0);
      }
   return( 0);
}
