/* plot_els.c: example for PostScript "splot" plotting routines
Copyright (C) 2018, Project Pluto

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
02110-1301, USA.    */

/* This program rummages through ephemerides of eight-line elements
to get data suitable for plotting.  At present,  it plots inclination
in black,  then switches to red to plot perigee distance.  Included
both as an example of how the 'splot' routines work,  and also because
somebody asked about the evolution of the orbit of a particular artsat
and I needed a plot to explain what was going on. */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include "splot.h"

#define JD_TO_YEAR( jd) (2000. + ((jd) - 2451545.) / 365.25)

int main( const int argc, const char **argv)
{
   splot_t splot;
   char buff[200], *tptr;
   FILE *ifile = fopen( "/home/phred/.find_orb/ephemeri.txt", "rb");
   double jd, jd_step;
   double year0, year1;
   double max_q = 0., min_q = 1e+20;
   int line = 0, n_steps;

   assert( ifile);
   if( !fgets( buff, sizeof( buff), ifile))
      return( -1);
   sscanf( buff, "%lf %lf %d", &jd, &jd_step, &n_steps);
   year0 = JD_TO_YEAR( jd);
   year1 = JD_TO_YEAR( jd + jd_step * n_steps);
   if( splot_init( &splot, "z.ps"))
      {
      printf( "Init failed\n");
      return( 0);
      }
   splot_newplot( &splot, 70, 460, 100, 600);
   splot_set_limits( &splot, year0, year1 - year0, 20., 90. - 20.);
   splot_add_ticks_labels( &splot, 60., SPLOT_ALL_EDGES | SPLOT_LIGHT_GRID);
   splot_add_ticks_labels( &splot, 20., SPLOT_HORIZONTAL_EDGES);
   splot_add_ticks_labels( &splot, 60., SPLOT_BOTTOM_EDGE | SPLOT_ADD_LABELS);
   splot_add_ticks_labels( &splot, 15., SPLOT_LEFT_EDGE);
   splot_add_ticks_labels( &splot, 100., SPLOT_LEFT_EDGE | SPLOT_ADD_LABELS);
   splot_label( &splot, SPLOT_BOTTOM_EDGE, 1, "Year");
   splot_label( &splot, SPLOT_LEFT_EDGE, 1, "Incl (deg)");
   splot_label( &splot, SPLOT_TOP_EDGE, 1, "2010-050B inclination/perigee");

   while( fgets( buff, sizeof( buff), ifile))
      if( (tptr = strstr( buff, "Incl.")) != NULL)
         {
         const double year = JD_TO_YEAR( jd);
         const double incl = atof( tptr + 6);

         splot_moveto( &splot, year, incl, (line > 0));
         jd += jd_step;
         line++;
         }
      else if( (tptr = strstr( buff, " q ")) != NULL)
         {
         const double q = atof( tptr + 3) - 6378.;

         if( max_q < q)
            max_q = q;
         if( min_q > q)
            min_q = q;
         }

   fseek( ifile, 0L, SEEK_SET);
   if( !fgets( buff, sizeof( buff), ifile))
      return( -1);
   sscanf( buff, "%lf %lf", &jd, &jd_step);
   splot_setrgbcolor( &splot, 1., 0., 0.);
   splot_set_limits( &splot, year0, year1 - year0, 0., max_q);
   splot_add_ticks_labels( &splot, 15., SPLOT_RIGHT_EDGE);
   splot_label( &splot, SPLOT_RIGHT_EDGE, 1, "q (km)");
   splot_add_ticks_labels( &splot, 100., SPLOT_RIGHT_EDGE | SPLOT_ADD_LABELS);
   line = 0;
   while( fgets( buff, sizeof( buff), ifile))
      if( (tptr = strstr( buff, " q ")) != NULL)
         {
         const double year = JD_TO_YEAR( jd);
         const double q = atof( tptr + 3) - 6378.;

         splot_moveto( &splot, year, q, (line > 0));
         jd += jd_step;
         line++;
         }

   fclose( ifile);
   splot_endplot( &splot);
   splot_display( &splot);
   return( 0);
}
