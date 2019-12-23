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
#include "splot.h"

int main( void)
{
   splot_t splot;

   if( splot_init( &splot, "z.ps"))
      {
      printf( "Init failed\n");
      return( 0);
      }
   splot_newplot( &splot, 100, 300, 100, 400);
   splot_set_limits( &splot, 1988., 33., 60., 20.);
   splot_add_ticks_labels( &splot, 60., SPLOT_ALL_EDGES | SPLOT_LIGHT_GRID);
   splot_add_ticks_labels( &splot, 30., SPLOT_BOTTOM_EDGE | SPLOT_LEFT_EDGE | SPLOT_ADD_LABELS);
   splot_label( &splot, SPLOT_BOTTOM_EDGE, 1, "Year");
   splot_label( &splot, SPLOT_LEFT_EDGE, 1, "Delta-T");
   splot_label( &splot, SPLOT_LEFT_EDGE, 2, "(seconds TDT-UT1)");
   splot_label( &splot, SPLOT_TOP_EDGE, 1, "Insert title here");
   splot_moveto( &splot, 1995., 62., 0);
   splot_moveto( &splot, 2018., 75., 1);

   splot_moveto( &splot, 2006, 68., 0);
   splot_symbol( &splot, 1, "2006 value");
   splot_endplot( &splot);
   splot_display( &splot);
   return( 0);
}
