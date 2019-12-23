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
#include <string.h>
#include <assert.h>
#include "splot.h"

static int create_sr_plot( const char *filename)
{
   char buff[80], *ext, header[80], obj_name[80];
   splot_t splot;
   double x, y, lim;
   FILE *ifile = fopen( filename, "rb");
   unsigned n_points;
   int n_scanned;

   assert( ifile);
   strcpy( buff, filename);
   ext = strstr( buff, ".off");
   assert( ext);
   strcpy( ext, ".ps");
   if( splot_init( &splot, buff))
      {
      printf( "Init failed\n");
      return( -1);
      }
   if( !fgets( header, sizeof( header), ifile)
      || !fgets( obj_name, sizeof( obj_name), ifile)
      || !fgets( buff, sizeof( buff), ifile))
      {
      printf( "Read failed\n");
      return( -1);
      }
   n_scanned = sscanf( buff + 2, "%u points; %lf x %lf", &n_points, &x, &y);
   if( n_scanned != 3)
      {
      printf( "Only got %d fields\n%s", n_scanned, buff);
      return( -2);
      }
   lim = x * 3;
   splot_newplot( &splot, 100, 400, 100, 400);
   splot_set_limits( &splot, lim, -2. * lim, -lim, 2. * lim);
   splot_add_ticks_labels( &splot, 60., SPLOT_ALL_EDGES | SPLOT_LIGHT_GRID);
   splot_add_ticks_labels( &splot, 30., SPLOT_BOTTOM_EDGE | SPLOT_LEFT_EDGE | SPLOT_ADD_LABELS);
   splot_label( &splot, SPLOT_BOTTOM_EDGE, 1, "RA offset (arcsec)");
   splot_label( &splot, SPLOT_LEFT_EDGE, 1, "Dec offset (arcsec)");
   splot_label( &splot, SPLOT_TOP_EDGE, 1, header + 2);
   splot_label( &splot, SPLOT_TOP_EDGE, 2, obj_name + 2);
   while( fgets( buff, sizeof( buff), ifile))
      {
      sscanf( buff, "%lf %lf", &x, &y);
      if( x < lim && x > -lim && y < lim && y > -lim)
         {
         splot_moveto( &splot, x, y, 0);
         splot_symbol( &splot, 1, "");
         }
      }
   splot_endplot( &splot);
   splot_display( &splot);
   return( 0);
}

int main( const int argc, const char **argv)
{
   int i;

   for( i = 1; i < argc; i++)
      create_sr_plot( argv[i]);
   return( 0);
}
