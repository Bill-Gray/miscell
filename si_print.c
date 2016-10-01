#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <math.h>

#ifdef _MSC_VER
   #include <float.h>
   #define isfinite _finite
#endif

/* The following formats a number to fit _without_ SI prefixes if possible
(see examples at bottom of file).  This includes tricks such as removing
leading zeroes to get an extra digit to fit.

   If the number will not fit,  we choose an SI prefix such that the
numerical part is between 1 and 1000.  That works as long as we have three
or more characters in which to display the number.  If there are only two,
the numerical part is kept between .1 and 100,  resulting in "non-standard"
output such as .3G instead of the "correct" 300M.

   Accurate results should be given for positive values if n_places >= 3,
and for negative values if n_places >= 4,  _and_ you don't go outside the
realm of yocto/yotta.  If needed,  '!' characters will be inserted.  */

void si_sprintf( char *buff, double ival, int n_places)
{
   int i;
   char prefix = '#';
   const char *err_msg = NULL;

   assert( n_places > 1);
   assert( n_places < 25);
   if( ival < 0.)
      {
      *buff++ = '-';
      n_places--;
      ival = -ival;
      }
   buff[n_places] = '\0';
   if( !ival)
      sprintf( buff, "%*d", n_places, 0);
   else if( isnan( ival))
      err_msg = "NaN!";
   else if( !isfinite( ival))
      err_msg = "inf!";
   else if( ival > 999.39e+24 || (ival > 99.39e+24 && n_places == 3)
                           || (ival > 9.939e+24 && n_places == 2)
                           || ival < 1.01e-24)
      {
      char tbuff[100], *tptr;
      int len;

      sprintf( tbuff, "%.*e", n_places, ival);
      tptr = strchr( tbuff, 'e');
      if( tptr)
         {
         len = strlen( tptr);
         if( len <= n_places)
            {
            memcpy( buff, tbuff, n_places - len);
            memcpy( buff + n_places - len, tptr, len);
            ival = 0.;
            }
         }
      if( ival)
         err_msg = (ival < 1. ? "Low!" : "Huge");
      }
   else if( ival < .011)
      {
      const char *lower_prefixes = " munpfazy";
/*    const char *lower_prefixes = " munpfazyxwvtsrqoljihgedcb";
               Above include non-standard "extended" SI prefixes */
      const double limit = (n_places > 3 ? .9994 : .0994);

      for( i = 0; lower_prefixes[i] && ival < limit; i++)
         ival *= 1000.;
      prefix = lower_prefixes[i];
      }
   else
      {
      double top_val = 1.;

      for( i = 0; ival > top_val - .6; i++)
         top_val *= 10.;
      if( i > n_places)
         {
         const char *prefixes = " kMGTPEZY";
/*       const char *prefixes = " kMGTPEZYXWVUSRQONLJIHFDCBA";
               Above include non-standard "extended" SI prefixes */
         const double limit = (n_places > 3 ? 999.4 : 99.4);

         for( i = 0; ival > limit && prefixes[i]; i++)
            ival /= 1000.;
         prefix = prefixes[i];
         }
      }
   if( !prefix || err_msg)      /* overflow/underflow */
      {
      memset( buff, '!', n_places);
      if( err_msg)
         {
         const int len = (n_places > 4 ? 4 : n_places);

         memcpy( buff + (n_places - len) / 2, err_msg, len);
         }
      }
   else if( ival)
      {
      char tbuff[100];

      if( prefix != '#')      /* leave room for an SI prefix */
         {
         n_places--;
         buff[n_places] = prefix;
         }
      sprintf( tbuff, "%.*f", n_places, ival);
      if( *tbuff == '0')      /* skip leading zero */
         memcpy( buff, tbuff + 1, n_places);
#ifdef NOT_REALLY_NEEDED
      else if( tbuff[n_places - 1] == '.')
         {
         *buff = ' ';
         memcpy( buff + 1, tbuff, n_places - 1);
         }
#endif
      else
         memcpy( buff, tbuff, n_places);
      if( n_places == 1 && ival > 9.4)
         *buff = '!';
      }
}

#ifdef TEST_CODE
#include <stdlib.h>

int main( const int argc, const char **argv)
{
   char buff[12];
   int i, n_places;
   double ival = (argc == 1 ? -3.141592653589793 : atof( argv[1])) / 1e+10;

   for( i = 0; i < 21; i++, ival *= 10.)
      {
      memset( buff, '&', sizeof( buff));
      printf( "%3d: ", i - 10);
      for( n_places = 3; n_places < 12; n_places++)
         {
         si_sprintf( buff, ival, n_places);
         printf (" %s", buff);
         }
      printf( "  %3d\n", i - 10);
      }
   return( 0);
}
#endif      // #ifdef TEST_CODE

/*
3.141u
31.41u   (31416n)
314.1u
3.141m
.03141   (31.41m)
.31416
3.1416
31.416
314.16
3141.6
 31416
314159
314.2k
 3142k
31416k
314.2M
 3142M
31416M
314.2G
 3142G
31416G



Three places:
3.1
 31
314
 3K
31K
.3M  <- Somewhat "non-standard"-ized to fit
 3M

Four places:

 31m
314m   <- use three-place system plus a suffix
3.14
31.4
 314
3141
 31K  <- again use three-place plus suffix
314K
3.1M
 31M
314M

Six places:

31.41m
.31416
3.1416
31.416
314.16
3141.6
 31416
314159
3.141M      */
