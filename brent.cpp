#include <math.h>
#include "brent.h"

static double brent_iterate( BRENT *brent)
{
   double s;
   const double fa_fb = brent->fa - brent->fb;
   const double limit_val = .75 * brent->a + .25 * brent->b;
   int reject_step = 0;

   if( brent->fa != brent->fc && brent->fb != brent->fc)
      {
      const double fa_fc = brent->fa - brent->fc;
      const double fb_fc = brent->fb - brent->fc;

#ifdef BRENT_INTERPOLATION_CUBIC
      if( brent->fd != brent->fc && brent->fd != brent->fb &&
                                    brent->fd != brent->fa)
         {
         const double fa_fd = brent->fa - brent->fd;
         const double fb_fd = brent->fb - brent->fd;
         const double fc_fd = brent->fc - brent->fd;
         const double db = brent->b - brent->a;
         const double dc = brent->c - brent->a;
         const double dd = brent->d - brent->a;

         s =   db * brent->fc * brent->fd / (fa_fb * fb_fc * fb_fd)
             - dc * brent->fb * brent->fd / (fa_fc * fb_fc * fc_fd)
             + dd * brent->fb * brent->fc / (fa_fd * fb_fd * fc_fd);
         s = brent->a - brent->fa * s;
         brent->interpolation_used = BRENT_INTERPOLATION_CUBIC;
         }
      else
#endif    /* #ifdef BRENT_INTERPOLATION_CUBIC */
         {
         s = -(brent->b - brent->a) * brent->fc / fa_fb
            + (brent->c - brent->a) * brent->fb / fa_fc;
         s = brent->a + s * brent->fa / fb_fc;
         brent->interpolation_used = BRENT_INTERPOLATION_QUADRATIC;
         }
      }
   else if( fa_fb)      /* can't do a quadratic or cubic  */
      {                 /* interpolation, so use secant rule: */
      s = brent->b - brent->fb * (brent->a - brent->b) / fa_fb;
      brent->interpolation_used = BRENT_INTERPOLATION_SECANT;
      }
   else                 /* we're fully converged */
      {
      s = brent->b;
      brent->interpolation_used = BRENT_FULLY_CONVERGED;
      }

            /* The new value must be between certain limits to be accepted.
               If it's not,  we set 'mflag' and do a median step instead. */
   if( (s > limit_val && s > brent->b) || (s < limit_val && s < brent->b))
      reject_step = 1;
   if( brent->mflag && fabs( s - brent->b) >= fabs( brent->b - brent->c) * .5)
      reject_step = 2;
   if( !brent->mflag && fabs( s - brent->b) >= fabs( brent->c - brent->d) * .5)
      reject_step = 3;
   if( brent->mflag && brent->b == brent->c)
      reject_step = 4;
   if( !brent->mflag && brent->c == brent->d)
      reject_step = 5;

   if( reject_step)
      {
      s = (brent->a + brent->b) / 2.;
      brent->interpolation_used = BRENT_INTERPOLATION_MIDPOINT;
      }
   brent->mflag = reject_step;
   brent->d = brent->c;
   brent->c = brent->b;
   brent->fd = brent->fc;
   brent->fc = brent->fb;
   return( s);
}

            /* At various points,  we require that |fb| < |fa|.  If it  */
            /* isn't,  we have to swap fa with fb,  and a with b.       */
static void keep_fb_less_than_fa( BRENT *brent)
{
   if( fabs( brent->fa) < fabs( brent->fb))
      {
      double tval = brent->a;

      brent->a = brent->b;
      brent->b = tval;
      tval = brent->fa;
      brent->fa = brent->fb;
      brent->fb = tval;
      }
}

int brent_init( BRENT *brent)
{
   brent->n_iterations = 0;
   return( 0);
}

double add_brent_point( BRENT *brent, const double s, const double fs)
{
   double rval = -1.;

   if( !brent->n_iterations)
      {
      brent->a = s;
      brent->fa = fs;
      }
   else if( brent->n_iterations == 1)
      {
      brent->b = s;
      brent->fb = fs;
      keep_fb_less_than_fa( brent);
      brent->c = brent->d = brent->a;
      brent->fc = brent->fd = brent->fa;
      brent->mflag = 1;
      }
   else
      {
      keep_fb_less_than_fa( brent);
      if( brent->fa * fs < 0.)
         {
         brent->b = s;
         brent->fb = fs;
         }
      else
         {
         brent->a = s;
         brent->fa = fs;
         }
      keep_fb_less_than_fa( brent);
      }
   brent->n_iterations++;
   if( brent->n_iterations == 1) /* first point,  can't interpolate */
      brent->interpolation_used = BRENT_INITIALIZING;
   else
      rval = brent_iterate( brent);
   return( rval);
}

#ifdef TEST_CODE
#include <stdio.h>
#include <stdlib.h>


   /* This function is used in the Wikipaedia page about Brent's method.
      Functions of the form y = a * sqrt( x - b) + c,  such as the one
      commented out below,  will cause the first quadratic step to find
      the root exactly.   */

static double example_func( const double x)
{
   return( (x + 3.) * (x - 1.) * (x - 1.));
/* return( 3.7 * sqrt( x + 5.1) - 13.4);  */
}

int main( const int argc, const char **argv)
{
   BRENT brent;
   double new_val, new_func;
   const double low_end = (argc > 1 ? atof( argv[1]) : -4.);
   const double high_end = (argc > 2 ? atof( argv[2]) : 4. / 3.);
   double thresh = 1e-9;

   brent_init( &brent);
   add_brent_point( &brent, low_end, example_func( low_end));
   new_val = add_brent_point( &brent, high_end, example_func( high_end));
   if( argc > 3)
      thresh = atof( argv[3]);
   do
      {
      static const char *interp_text[4] = {
                  "Midpoint", "Interpol", "Quadrati", "Cubic   " };

      printf( "Bracketed within %.10f to %.10f\n", brent.a, brent.b);
      printf( "%d: Value: %.13f (%s, median flag %d)\n",
                 brent.n_iterations, new_val,
                 interp_text[brent.interpolation_used], brent.mflag);
      new_func = example_func( new_val);
      new_val = add_brent_point( &brent, new_val, new_func);
      }
      while( fabs( new_func) > thresh);
   printf( "Root = %.10f (%.10f)\n", new_val, new_func);
   return( 0);
}
#endif      /* #ifdef TEST_CODE */
