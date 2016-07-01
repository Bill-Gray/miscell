/* Simple plot */

typedef struct
   {
   void *ofile;
   double x1, xsize, y1, ysize;     /* rectangle in points on page */
   double u1, usize, v1, vsize;     /* rectangle in "user" units */
   } splot_t;

int splot_init( splot_t *splot, const char *output_filename);
void splot_newplot( splot_t *splot, const double x1, const double xsize,
                                    const double y1, const double ysize);
void splot_set_limits( splot_t *splot, const double u1, const double usize,
                                       const double v1, const double vsize);
void splot_add_ticks_labels( splot_t *splot,
                             const double spacing, const int flags);
void splot_label( splot_t *splot, const unsigned flags,
                   const int line, const char *text);
void splot_moveto( splot_t *splot, const double u, const double v, const bool pen_down);
void splot_symbol( splot_t *splot, const int symbol_id, const char *text);
void splot_endplot( splot_t *splot);
void splot_display( splot_t *splot);

#define SPLOT_TOP_EDGE          0x01
#define SPLOT_BOTTOM_EDGE       0x02
#define SPLOT_HORIZONTAL_EDGES   (SPLOT_TOP_EDGE | SPLOT_BOTTOM_EDGE)
#define SPLOT_RIGHT_EDGE        0x04
#define SPLOT_LEFT_EDGE         0x08
#define SPLOT_VERTICAL_EDGES     (SPLOT_RIGHT_EDGE | SPLOT_LEFT_EDGE)
#define SPLOT_ALL_EDGES          (SPLOT_VERTICAL_EDGES | SPLOT_HORIZONTAL_EDGES)
#define SPLOT_ADD_LABELS        0x10
#define SPLOT_LIGHT_GRID        0x20


#define SPLOT_MOVETO               0
#define SPLOT_LINETO               1
#define SPLOT_DASHED               2
