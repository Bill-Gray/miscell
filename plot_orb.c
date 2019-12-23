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

/* Code to produce quick & dirty PostScript plot from a Find_Orb ephemeris.
Used originally to generate an orbit plot for WT1190F.  */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>

const double pi = 3.1415926535897932384626433832795028841971693993751058209749445923;
const double au_in_km = 1.495978707e+8;
const double earth_r = 6378.140;
const double lunar_orbit = 385000.;


int main( const int argc, const char **argv)
{
   FILE *ifile = fopen( "ephemeri.txt", "rb");
   FILE *ofile = fopen( "z.ps", "wb");
   char buff[200];
   double scale = .01, prev_x = 0., prev_y = 0.;
   double rotation = 0.;
   int line = 0, i, index_freq = 10;
   const double center_x = 72. * 4.25, center_y = 72. * 5.5;

   for( i = 1; i < argc; i++)
      if( argv[i][0] == '-')
         switch( argv[i][1])
            {
            case 's':
               scale = atof( argv[i] + 2);
               break;
            case 'i':
               index_freq = atoi( argv[i] + 2);
               break;
            case 'r':
               rotation = atof( argv[i] + 2) * pi / 180.;
               break;
            default:
               printf( "Switch '%s' not understood\n", argv[i]);
               break;
            }

   assert( ifile);
   assert( ofile);
   fprintf( ofile, "%s",
           "%!PS-Adobe-2.0\n"
           "%%Pages: 1\n"
           "%%PageOrder: Ascend\n"
           "%%Creator: calendar.cpp\n"
           "%%Copyright: none\n"
           "%%Title: Calendar for September 2016\n"
           "%%Version: none\n"
           "%%DocumentData: Clean7Bit\n"
           "%%EndComments\n"
           "%%BeginDefaults\n"
           "%%PageResources: font Times-Roman\n"
           "%%PageResources: font Times-Italic\n"
           "%%PageResources: font Courier-Bold\n"
           "%%EndDefaults\n\n"
           "%%Page: 1 1\n\n"
           "/Times-Roman findfont 12 scalefont setfont\n");

   if( !fgets( buff, sizeof( buff), ifile))
      {
      printf( "Failed to get line from ephemeris\n");
      return( -1);
      }
   while( fgets( buff, sizeof( buff), ifile) && atof( buff))
      {
      double x, y, z, tval;

      sscanf( buff + 14, "%lf %lf %lf", &x, &y, &z);
      tval = y * cos( rotation) + x * sin( rotation);
      x = cos( rotation) * x - y * sin( rotation);
      y = tval;
      x *= 72. / scale;
      y *= 72. / scale;
      x += center_x;
      y = center_y - y;
      fprintf( ofile, "%.1f %.1f %s\n", x, y, (line ? "lineto" : "moveto"));
      if( !(line % index_freq) && line)
         {
         double dx = prev_x - x, dy = prev_y - y;
         double dist = sqrt( dx * dx + dy * dy);

         dx *= 3. / dist;
         dy *= 3. / dist;
/*       fprintf( ofile, "currentpoint\n");     */
         fprintf( ofile, "%.1f %.1f rmoveto\n", dy, -dx);
         fprintf( ofile, "%.1f %.1f rlineto\n", -2. * dy, 2. * dx);
/*       fprintf( ofile, "moveto\n");           */
         fprintf( ofile, "%.1f %.1f rmoveto\n", dy, -dx);
         }
      if( strlen( buff) > 111)
         {
         int j = 111;

         while( buff[j] >= ' ')
            j++;
         buff[j] = '\0';
         fprintf( ofile, "currentpoint\n");
         fprintf( ofile, "(%s) show\nmoveto\n", buff + 111);
         }
      line++;
      prev_x = x;
      prev_y = y;
      }
   for( i = 0; i < 2; i++)
      fprintf( ofile, "stroke\n%s\n %.1f %.1f %.1f 0 360 arc closepath\n",
               (i ? "1 0 0 setrgbcolor\n" : "0 1 0 setrgbcolor"),
               center_x, center_y, 72. * (i ? earth_r : lunar_orbit) / (scale * au_in_km));
   fprintf( ofile, "stroke\nshowpage\n");
   fclose( ifile);
   fclose( ofile);
   return( 0);
}
