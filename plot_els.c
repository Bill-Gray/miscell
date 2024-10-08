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
and I needed a plot to explain what was going on.

Add a command-line argument to cause the red line to plot orbital
period instead of inclination.  The argument should be the maximum
orbital period,  in minutes.   */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include "splot.h"

#define JD_TO_YEAR( jd) (2000. + ((jd) - 2451545.) / 365.25)

static double get_q( const char *buff)
{
   double rval = 0.;
#ifndef TEMP_REMOVE_SO_CAN_SHOW_APOGEE
   const char *tptr = strstr( buff, " q ");

   if( *buff == 'q')
      rval = atof( buff + 1);
   else if( tptr)
      rval = atof( tptr + 3);
#else
   const char *tptr = strstr( buff, " Q ");

   if( tptr)
      rval = atof( tptr + 3);
#endif
   if( rval < 1.)          /* assume AU */
      rval *= 149597870.7;       /* AU in km */
   return( rval);
}

int main( const int argc, const char **argv)
{
   splot_t splot;
   char buff[200], *tptr;
   FILE *ifile = fopen( "/home/phred/output/ephemeri.txt", "rb");
   double jd, jd_step;
   double year0, year1;
   double max_q = 0., min_q = 1e+20;
   double center_radius = 6378.140;      /* assume earth */
   int line = 0, n_steps;
   const double max_period = (argc < 2 ? 0. : atof( argv[1]));
   const double min_period = (argc < 3 ? 0. : atof( argv[2]));
   const double max_incl = 90., min_incl = 0.;

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
   if( year0 < year1)
      splot_set_limits( &splot, year0, year1 - year0,
                  (min_period ? min_period : min_incl),
                  (max_period ? max_period - min_period : max_incl - min_incl));
   else
      splot_set_limits( &splot, year1, year0 - year1,
                  (min_period ? min_period : min_incl),
                  (max_period ? max_period - min_period : max_incl - min_incl));
   splot_add_ticks_labels( &splot, 60., SPLOT_ALL_EDGES | SPLOT_LIGHT_GRID);
   splot_add_ticks_labels( &splot, 20., SPLOT_HORIZONTAL_EDGES);
   splot_add_ticks_labels( &splot, 60., SPLOT_BOTTOM_EDGE | SPLOT_ADD_LABELS);
   splot_add_ticks_labels( &splot, 15., SPLOT_LEFT_EDGE);
   splot_add_ticks_labels( &splot, 100., SPLOT_LEFT_EDGE | SPLOT_ADD_LABELS);
   splot_label( &splot, SPLOT_BOTTOM_EDGE, 1, "Year");
   splot_label( &splot, SPLOT_LEFT_EDGE, 1,
               (max_period ? "Period (minutes)" : "Incl (deg)"));
   tptr = strstr( buff, ": ");      /* top line also gives object name, */
   assert( tptr);                   /* which we use to label top of plot */
   splot_label( &splot, SPLOT_TOP_EDGE, 1, tptr + 1);

   while( fgets( buff, sizeof( buff), ifile))
      {
      double q = get_q( buff);

      if( !memcmp( buff, "   Perilune", 9))
         center_radius = 1737.;
      if( max_period && *buff == 'P' && atof( buff + 1))
         {
         const double year = JD_TO_YEAR( jd);
         const double period_in_minutes = atof( buff + 1);

         splot_moveto( &splot, year, period_in_minutes, (line > 0));
         jd += jd_step;
         line++;
         }
      else if( !max_period && NULL != (tptr = strstr( buff, "Incl.")))
         {
         const double year = JD_TO_YEAR( jd);
         const double incl = atof( tptr + 6);

         splot_moveto( &splot, year, incl, (line > 0));
         jd += jd_step;
         line++;
         }
      if( q)
         {
         q -= center_radius;
         if( max_q < q)
            max_q = q;
         if( min_q > q)
            min_q = q;
         if( max_q > 990000.)
            max_q = 990000.;
         }
      }
   fseek( ifile, 0L, SEEK_SET);
   if( !fgets( buff, sizeof( buff), ifile))
      return( -1);
   sscanf( buff, "%lf %lf", &jd, &jd_step);
   splot_setrgbcolor( &splot, 1., 0., 0.);
   if( year0 < year1)
      splot_set_limits( &splot, year0, year1 - year0, 0., max_q);
   else
      splot_set_limits( &splot, year1, year0 - year1, 0., max_q);
   splot_add_ticks_labels( &splot, 15., SPLOT_RIGHT_EDGE);
   splot_label( &splot, SPLOT_RIGHT_EDGE, 1, "q (km)");
   splot_add_ticks_labels( &splot, 100., SPLOT_RIGHT_EDGE | SPLOT_ADD_LABELS);
   line = 0;
   while( fgets( buff, sizeof( buff), ifile))
      if( get_q( buff))
         {
         const double year = JD_TO_YEAR( jd);
         const double q = get_q( buff) - center_radius;

         splot_moveto( &splot, year, q, (line > 0));
         jd += jd_step;
         line++;
         }

   fclose( ifile);
   splot_endplot( &splot);
   splot_display( &splot);
   return( 0);
}
