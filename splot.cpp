#include <stdio.h>
#include <math.h>
#include <assert.h>
#include "splot.h"

int splot_init( splot_t *splot, const char *output_filename)
{
   splot->ofile = (void *)fopen( output_filename, "w");
   if( splot->ofile)
      {
      const char *header =
                  "%!PS-Adobe-2.0\n"
                  "%%Pages: 1\n"
                  "%%PageOrder: Ascend\n"
                  "%%Orientation: Portrait\n"
                  "%%Creator: splot\n"
                  "%%Copyright: none\n"
                  "%%Title: Splot plot\n"
                  "%%Version: none\n"
                  "%%DocumentData: Clean7Bit\n"
                  "%%EndComments\n"
                  "%%BeginDefaults\n"
                  "%%PageResources: font Times-Roman\n"
                  "%%EndDefaults\n"
                  "%%Page: 1 1\n"
                  "\n"
                  "/centershow {\n"
                  "  dup stringwidth pop\n"
                  "  0 exch sub\n"
                  "  2 div\n"
                  "  -5 rmoveto show } def\n"
                  "\n"
                  "/vcentershow {\n"
                  "  gsave currentpoint translate 0 0 moveto\n"
                  "  -90 rotate centershow grestore } def\n"
                  "\n"
                  "/lline {\n"
                  "  currentpoint stroke gsave .05 setlinewidth\n"
                  "  [1 4] 0 setdash\n"
                  "  moveto rlineto stroke grestore } def\n"
                  "\n"
                  "/Times-Roman findfont 12 scalefont setfont\n";

      fprintf( (FILE *)splot->ofile, "%s", header);
      }
   return( splot->ofile ? 0 : -1);
}

void splot_newplot( splot_t *splot, const double x1, const double xsize,
                                    const double y1, const double ysize)
{
   FILE *ofile = (FILE *)splot->ofile;

   fprintf( (FILE *)splot->ofile, "%d %d translate\n", (int)x1, (int)y1);
   fprintf( ofile, "newpath 0 0 moveto %d 0 rlineto 0 %d rlineto\n",
                                 (int)xsize, (int)ysize);
   fprintf( ofile, "-%d 0 rlineto closepath stroke\n", (int)xsize);
   splot->x1 = x1;
   splot->y1 = y1;
   splot->xsize = xsize;
   splot->ysize = ysize;
}

void splot_set_limits( splot_t *splot, const double u1, const double usize,
                                       const double v1, const double vsize)
{
   splot->u1 = u1;
   splot->v1 = v1;
   splot->usize = usize;
   splot->vsize = vsize;
}

/* The user may want ticks or labels at,  say,  every half inch = 36 points
_at most_.  Half an inch on the page may correspond to,  say,  17 "units" on
that axis.  In such a case,  we want to move up to 20 units per tick,  making
the ticks a hair more than half an inch apart.

   To determine this,  we first compute rval as the power of ten _less_ than
17,  getting 10.  Then we try "desirable" multiples (2,  2.5,  5.,  and 10.)
until one of them is greater than 17.  */

static double find_spacing( const double min_spacing)
{
   const double rval = pow( 10., floor( log10( min_spacing)));
   const double multiples[4] = { 2., 2.5, 5., 10. };
   double real_rval;
   size_t i;

   for( i = 0; i < sizeof( multiples) / sizeof( multiples[0]); i++)
      if( (real_rval = rval * multiples[i]) >= min_spacing)
         return( real_rval);
   assert( 1);       /* shouldn't get here */
   return( real_rval);     /* ...just to avoid warning message */
}

static bool is_between( const double x, const double lim1, const double lim2)
{
   const bool rval = ((x > lim1 && x < lim2) || (x > lim2 && x < lim1));

   return( rval);
}

static void add_a_side( splot_t *splot, const double spacing, const int flags,
                  const double u1, const double usize,
                   const double xsize, const double ysize)
{
   const double u_per_x = usize / xsize;
   double u, uspacing = find_spacing( spacing * fabs( u_per_x));
   FILE *ofile = (FILE *)splot->ofile;

   if( usize < 0.)      /* descending axes */
      uspacing = -uspacing;
   u = ceil( u1 / uspacing) * uspacing;
   while( is_between( u, u1, u1 + usize))
      {
      const double x = (u - u1) / u_per_x;

      if( flags & SPLOT_LIGHT_GRID)
         fprintf( ofile, "%d 0 moveto 0 %d lline\n", (int)x,
                     (int)ysize);
      else
         {
         if( flags & SPLOT_BOTTOM_EDGE)
            {
            fprintf( ofile, "%d 0 moveto 0 10 rlineto\n", (int)x);
            if( flags & SPLOT_ADD_LABELS)
               fprintf( ofile, "0 -20 rmoveto (%g) centershow\n", u);
            }
         if( flags & SPLOT_TOP_EDGE)
            {
            fprintf( ofile, "%d %d moveto 0 -10 rlineto\n", (int)x,
                                (int)ysize);
            if( flags & SPLOT_ADD_LABELS)
               fprintf( ofile, "0 20 rmoveto (%g) centershow\n", u);
            }
         }
      u += uspacing;
      }
}

void splot_add_ticks_labels( splot_t *splot, const double spacing,
                                             const int flags)
{
   FILE *ofile = (FILE *)splot->ofile;

   if( flags & SPLOT_HORIZONTAL_EDGES)
      add_a_side( splot, spacing, flags, splot->u1, splot->usize,
                                         splot->xsize, splot->ysize);
   if( flags & SPLOT_VERTICAL_EDGES)
      {
      const int new_flags = (flags >> 2) | (flags & ~15);

#ifdef PREVIOUS_METHOD
      fprintf( ofile, "0 %d translate -90 rotate\n", (int)splot->ysize);
      add_a_side( splot, spacing, new_flags, splot->v1, splot->vsize,
                                         splot->ysize, splot->xsize);
      fprintf( ofile, "90 rotate 0 %d translate\n", -(int)splot->ysize);
#endif
      fprintf( ofile, " -90 rotate\n");
      add_a_side( splot, spacing, new_flags, splot->v1, splot->vsize,
                                        -splot->ysize, splot->xsize);
      fprintf( ofile, "90 rotate\n");
      }
}

void splot_label( splot_t *splot, const unsigned flags,
                   const int line, const char *text)
{
   double x = splot->xsize / 2.;
   double y = splot->ysize / 2.;
   const double line_spacing = 12.;
   const double offset = (double)(line + 1) * line_spacing;

   if( flags & SPLOT_BOTTOM_EDGE)
      y =  -offset;
   if( flags & SPLOT_TOP_EDGE)
      y =  offset + splot->ysize;
   if( flags & SPLOT_LEFT_EDGE)
      x =  -offset;
   if( flags & SPLOT_RIGHT_EDGE)
      x =  offset + splot->xsize;
   fprintf( (FILE *)splot->ofile, "%d %d moveto (%s) %ccentershow\n",
            (int)x, (int)y, text, (flags & SPLOT_VERTICAL_EDGES) ? 'v' : ' ');
}

void splot_moveto( splot_t *splot, const double u, const double v, const bool pen_down)
{
   FILE *ofile = (FILE *)splot->ofile;
   const double x =  (u - splot->u1) * splot->xsize / splot->usize;
   const double y =  (v - splot->v1) * splot->ysize / splot->vsize;

   fprintf( ofile, "%d %d %s\n", (int)x, (int)y, pen_down ? "lineto" : "moveto");
}

void splot_symbol( splot_t *splot, const int symbol_id, const char *text)
{
   FILE *ofile = (FILE *)splot->ofile;

   if( symbol_id)
      fprintf( ofile, "currentpoint 0 5 rmoveto 0 -10 rlineto -5 5 rmoveto 10 0 rlineto\n");
   else
      fprintf( ofile, "currentpoint 5 5 rmoveto -10 -10 rlineto 10 0 rmoveto -10 10 rlineto\n");
   if( text)
      fprintf( ofile, "(%s) show moveto\n", text);
   else
      fprintf( ofile, "moveto\n");
}

void splot_endplot( splot_t *splot)
{
   FILE *ofile = (FILE *)splot->ofile;

   fprintf( ofile, "stroke\n");
}

void splot_display( splot_t *splot)
{
   FILE *ofile = (FILE *)splot->ofile;

   fprintf( ofile, "showpage\n");
   fclose( ofile);
}

