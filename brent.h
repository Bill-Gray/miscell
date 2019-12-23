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

#define BRENT struct brent

BRENT
   {
   double a, b, c, fa, fb, fc;
   double d, fd;
   int mflag, interpolation_used;
   int n_iterations;
   };

   /* At present,  brent_init() just sets n_iterations = 0. */

int brent_init( BRENT *brent);

   /* add_brent_point() returns the new best estimate for the zero point.
      You can keep going until fabs( brent->a - brent->b) is less than
      some desired threshhold,  or until fabs( fs) < threshhold. */

double add_brent_point( BRENT *brent, const double s, const double fs);

   /* "Normally",  add_brent_point() will set the interpolation_used flag
      to one of the following three values,  indicating how the return
      values was computed.  */

#define BRENT_INTERPOLATION_QUADRATIC     2
#define BRENT_INTERPOLATION_SECANT        1
#define BRENT_INTERPOLATION_MIDPOINT      0

   /* If you look in brent.c,  you'll see that it's possible for the "usual"
      Brent method to be extended to do a _cubic_ interpolation.  If enabled,
      this makes the last few steps converge faster.   */
#define BRENT_INTERPOLATION_CUBIC         3

   /* The first time you call add_brent_point(),  no interpolation is
      possible (of course) and 'interpolation_used' is set as follows. */
#define BRENT_INITIALIZING               -1

   /* If you converge beyond all reason,  to the point where the function */
   /* values are zero at both endpoints,  you get the following. */

#define BRENT_FULLY_CONVERGED            -2
