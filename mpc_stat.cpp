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

/* NOTE: This code to read the MPC observatory file and output it with
atitudes,  altitudes,  and country/area names is essentially obsolete.
Use 'mpc_code.cpp' from the 'lunar' repository instead.  It replaces
and improves upon the following. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <math.h>

#define PI 3.1415926535897932384626433832795028841971693993751058209749445923
#define EARTH_MAJOR_AXIS 6378137.
#define EARTH_MINOR_AXIS 6356752.314245
         /* above are WGS84 ellipsoid constants */
#define EARTH_AXIS_RATIO (EARTH_MINOR_AXIS / EARTH_MAJOR_AXIS)
#define SWAP( A, B, TEMP) { TEMP = A; A = B; B = TEMP; }

int lat_alt_to_parallax( const double lat, const double ht_in_meters,
            double *rho_cos_phi, double *rho_sin_phi)
{
   const double u = atan( sin( lat) * EARTH_AXIS_RATIO / cos( lat));

   *rho_sin_phi = EARTH_AXIS_RATIO * sin( u) +
                            (ht_in_meters / EARTH_MAJOR_AXIS) * sin( lat);
   *rho_cos_phi = cos( u) + (ht_in_meters / EARTH_MAJOR_AXIS) * cos( lat);
   return( 0);
}

/* Takes parallax constants (rho_cos_phi, rho_sin_phi) in units of the
equatorial radius,  and returns the geographical latitude/altitude.  The
previous (non-iterative) version of this function,  used before 2016 Oct
1,  was derived by me as an approximate inversion of the above function
to convert lat/altitude to parallax. It could be off by as much as 16
meters in altitude and wrong in latitude by about three meters for each
km of altitude (i.e.,  about 25 meters at the top of Everest).  It's
presumably much worse for more oblate planets than the earth.

   An exact non-iterative solution is possible,  but it involves solving a
fourth-degree (quartic) polynomial,  and I don't recommend it.  The
iterations in the following function start out with a laughably poor guess,
but convergence is fast;  eight iterations gets sub-micron accuracy.

   For a solution for truly oblate objects,  and for points near the
center of the object,  see 'ellip_pt.c'.  (Since MPC stations are on
the only mildly oblate earth,  and none of them are near the geocenter,
the following code is perfectly good for the current purposes.)  */

int parallax_to_lat_alt( const double rho_cos_phi, const double rho_sin_phi,
               double *lat, double *ht_in_meters)
{
   double lat0 = atan2( rho_sin_phi, rho_cos_phi);
   double rho0 = sqrt( rho_sin_phi * rho_sin_phi + rho_cos_phi * rho_cos_phi);
   double tlat = lat0, talt = 0.;
   int iter;

   for( iter = 0; iter < 8; iter++)
      {
      double rc2, rs2;

      lat_alt_to_parallax( tlat, talt, &rc2, &rs2);
      talt -= (sqrt( rs2 * rs2 + rc2 * rc2) - rho0) * EARTH_MAJOR_AXIS;
      tlat -= atan2( rs2, rc2) - lat0;
      }
   if( lat)
      *lat = tlat;
   if( ht_in_meters)
      *ht_in_meters = talt;
   return( 0);
}


/* The following code reads in the MPC file ObsCodes.htm,  which gives
longitudes and parallax constants;  and outputs the same file with new
columns for latitude and altitude.  Be warned that the parallax
constants are usually given to five places,  or a precision of about
64 meters.  Therefore,  the latitude and longitude are good to a
similar precision.  The longitude is given to .0001 degree precision,
and is therefore good to about 11 meters.  Some stations give one more
or one fewer decimal place,  with corresponding change in precision
(though not necessarily accuracy).  Also,  note that the altitudes that
are computed are relative to the ellipsoid,  not to the geoid/mean sea level.

   It's been modified a bit so that it can read in lat/lon rectangles
from the 'geo_rect.txt' file.  Each rectangle covers part of the interior
of a state or nation;  if a given lat/lon from 'ObsCodes.htm' falls within
that rectangle,  you can assign that state/nation data to it.  I first made
a slew of rectangles covering most MPC codes,  ran this program,  found
some codes without state/nation info,  and made some more rectangles to
cover those codes.  Every now and then,  I have to add another rectangle
to cover a new code.

   Note that the MPC list of observatory codes is usually 'ObsCodes.html';
the code now checks for both possibilities.
*/


#define GEO_RECT struct geo_rect

GEO_RECT
   {
   double lat1, lon1, lat2, lon2;
   char name[60];
   };

int load_geo_rects( GEO_RECT *rects)
{
   FILE *ifile = fopen( "geo_rect.txt", "rb");
   int rval = 0;
   char buff[100];

   if( ifile)
      {
      while( fgets( buff, sizeof( buff), ifile))
         if( sscanf( buff, "%lf %lf %lf %lf",
                  &rects[rval].lon1, &rects[rval].lat1,
                  &rects[rval].lon2, &rects[rval].lat2) == 4)
            {
            double temp_double;
            int i;

            if( rects[rval].lat1 > rects[rval].lat2)
               SWAP( rects[rval].lat1, rects[rval].lat2, temp_double);
            if( rects[rval].lon1 > rects[rval].lon2)
               SWAP( rects[rval].lon1, rects[rval].lon2, temp_double);
            for( i = 0; buff[i] >= ' '; i++)
               ;
            while( i < 56)          /* pad the string */
               buff[i++] = ' ';
            buff[i] = '\0';
            strcpy( rects[rval].name, buff + 42);
                     /* Crossing the prime meridian is a problem,  since
                        you go from longitude 359.9999 to .0001.  This is
                        handled by making two rectangles,  one on each side
                        of the prime meridian.  */
            if( rects[rval].lon2 - rects[rval].lon1 > 180.)
               {
               SWAP( rects[rval].lon1, rects[rval].lon2, temp_double);
               rects[rval + 1] = rects[rval];
               rects[rval].lon2 = 361.;
               rects[rval + 1].lon1 = -1.;
               rval++;
               }
            rval++;
            }
      fclose( ifile);
      }
   else
      perror( "geo_rect.txt not opened");
   return( rval);
}

int code_compare( const void *a, const void *b)
{
   const char *a1 = (const char *)(*(const char **)a);
   const char *b1 = (const char *)(*(const char **)b);
   int rval = strcmp( a1 + 4, b1 + 4);

   if( !rval)        /* same country */
      rval = strcmp( a1, b1);
   return( rval);
}

#define MAX_RECTS 1000
#define MAX_CODES 8000

int main( const int argc, const char **argv)
{
   char buff[190];
   int in_code_section = 0, n_geo_rects;
   const char *ifilename = (argc > 1 ? argv[1] : "ObsCodes.htm");
   FILE *ifile = fopen( ifilename, "rb");
   FILE *ofile = (argc > 2 ? fopen( argv[2], "wb") : NULL);
   GEO_RECT *rects = (GEO_RECT *)calloc( MAX_RECTS, sizeof( GEO_RECT));

   assert( rects);
   if( !ifile)
      {
      fprintf( stderr, "Input %s not opened: ", ifilename);
      perror( "");
      exit( 0);
      }
   n_geo_rects = load_geo_rects( rects);
   while( fgets( buff, sizeof( buff), ifile))
      {
      if( !memcmp( buff, "000  ", 5))
         in_code_section = 1;
      if( !memcmp( buff, "</pre>", 6))
         in_code_section = 0;
      if( in_code_section && strlen( buff) > 30 && buff[3] == ' ')
         {
         memmove( buff + 68, buff + 30, strlen( buff + 29));
         memmove( buff + 25, buff + 21, 9);  /* move rho sin phi */
         memmove( buff + 15, buff + 13, 8);  /* move rho cos phi */
         buff[13] = buff[14] = buff[23] = buff[24] = ' ';
         memset( buff + 34, ' ', 34);
         if( buff[7] == '.' && buff[16] == '.' && buff[27] == '.' &&
                       atof( buff + 15))
            {
            const double x = atof( buff + 15);
            const double y = atof( buff + 25);
            const double lon = atof( buff + 4);
            char alt_buff[7];
            int rect_no = -1, i;
            double lat, ht_in_meters;

            parallax_to_lat_alt( x, y, &lat, &ht_in_meters);
            if( buff[21] != ' ' && buff[32] != ' ')
               {        /* make sure there's precision for altitude */
               sprintf( alt_buff, "%5.0f", ht_in_meters);
               }
            else        /* put blanks in for altitude */
               strcpy( alt_buff, "     ");

            lat *= 180. / PI;
            for( i = 0; i < n_geo_rects; i++)
               if( rects[i].lon1 < lon && rects[i].lon2 > lon)
                  if( rects[i].lat1 < lat && rects[i].lat2 > lat)
                     rect_no = i;
            sprintf( buff + 34, "  %+09.5f  %s ", lat, alt_buff);
            if( rect_no >= 0)
               memcpy( buff + 53, rects[rect_no].name, 15);
            else
               memset( buff + 53, ' ', 15);
            buff[67] = ' ';
            if( buff[22] == ' ')   /* parallax given to 5 or fewer digits */
               buff[44] = ' ';     /* means lat's good to .0001 deg at best */
            if( buff[21] == ' ')   /* parallax given to 4 or fewer digits */
               buff[43] = ' ';     /* means lat's good to .001 deg at best */
            if( buff[20] == ' ')   /* parallax given to 3 or fewer digits */
               buff[42] = ' ';     /* means lat's good to .01 deg at best */
            }
         memmove( buff + 4, buff + 3, strlen( buff + 2));
         buff[3] = ' ';    /* insert room for four-digit MPC codes */
         if( in_code_section)
            {
            if( ofile)
               fprintf( ofile, "%s", buff);
            else
               printf( "%s", buff);
            }
         }
      }
   if( ofile)
      fclose( ofile);
   free( rects);
}
