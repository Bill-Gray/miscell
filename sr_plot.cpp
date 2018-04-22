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
