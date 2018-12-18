#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

/* MS only got around to adding 'isfinite',  asinh in VS2013 : */

#if defined( _MSC_VER) && (_MSC_VER < 1800)
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

   If the SI_PRINTF_NO_LOWER_PREFIXES flag is set, the lower prefixes m,
u, n, etc. will not be used.  Numbers that would normally be shown using
those prefixes will instead be shown with leading zeroes,  and eventually
as just zeroes.

   If the SI_PRINTF_EXTENDED_PREFIXES flag is set, the 'usual' set of SI
prefixes is extended to include 17 more letters in each direction, for a
range of 10^51 each way.  Since the 'usual' sets end with z/Z or y/Y,
the 'extended' sets simply continue backward through the alphabet,
omitting letters already used elsewhere.  See the 'lower_prefixes' and
'upper_prefixes' variables.

   If the SI_PRINTF_FORCE_SIGN flag is set,  positive numbers will be
shown with a leading '+'.

   Accurate results should be given for positive values if n_places >= 3,
and for negative values if n_places >= 4,  _and_ you don't go outside the
realm of yocto/yotta.  If you do that,  we try to use "standard" N.NNe+NN
forms.  If there aren't enough places to do that,  it may end up looking
like 'e+28' (see examples at very bottom of this file),  which at least
tells you the order of magnitude of the number.  If there aren't even
enough places to do _that_,  we just show "Huge" or "Low" (again,  examples
shown at bottom of this file.)            */

#define SI_PRINTF_NO_LOWER_PREFIXES                1
#define SI_PRINTF_EXTENDED_PREFIXES                2
#define SI_PRINTF_FORCE_SIGN                       4

void si_sprintf( char *buff, double ival, int n_places, const int flags)
{
   int i;
   char prefix = '#';
   const char *err_msg = NULL;
   const bool use_si_prefixes = !(flags & SI_PRINTF_NO_LOWER_PREFIXES);
   const bool use_extended_prefixes = (flags & SI_PRINTF_EXTENDED_PREFIXES);
   const double extender = (use_extended_prefixes ? 1e+51 : 1);

   assert( n_places > 1);
   assert( n_places < 25);
   if( ival < 0.)
      {
      *buff++ = '-';
      n_places--;
      ival = -ival;
      }
   else if( flags & SI_PRINTF_FORCE_SIGN)
      {
      *buff++ = '+';
      n_places--;
      }
   buff[n_places] = '\0';
   if( !ival)
      sprintf( buff, "%*d", n_places, 0);
   else if( isnan( ival))
      err_msg = "NaN!";
   else if( !isfinite( ival))
      err_msg = "inf!";
   else if( ival > 999.39e+24 * extender
                   || (ival > 99.39e+24 * extender && n_places == 3)
                   || (ival > 9.939e+24 * extender && n_places == 2)
                   || (ival < 1.01e-24 / extender && use_si_prefixes))
      {
      char tbuff[100], *tptr;
      int len;

      sprintf( tbuff, "%.*e", n_places, ival);
      tptr = strchr( tbuff, 'e');
      if( tptr)
         {
         while( tptr[2] == '0')     /* remove leading zero(es) */
            memmove( tptr + 2, tptr + 3, strlen( tptr + 2));
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
   else if( ival < .011 && use_si_prefixes)
      {
      const char *lower_prefixes = " munpfazyxwvtsrqoljihgedcb";
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
         const char *prefixes = " kMGTPEZYXWVUSRQONLJIHFDCBA";
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
   int i, n_places, range = 10, flags = 0;
   const double ival = (argc == 1 ? -3.141592653589793 : atof( argv[1]));

   for( i = 2; i < argc; i++)
      if( argv[i][0] == '-')
         switch( argv[i][1])
            {
            case 'r':
               range = atoi( argv[i] + 2);
               break;
            case 'f':
               flags = atoi( argv[i] + 2);
               break;
            }
   for( i = -range; i <= range; i++)
      {
      memset( buff, '&', sizeof( buff));
      printf( "%3d: ", i);
      for( n_places = 3; n_places < 12; n_places++)
         {
         si_sprintf( buff, ival * pow( 10., (double)i), n_places, flags);
         printf (" %s", buff);
         }
      printf( "  %3d\n", i);
      }
   return( 0);
}
#endif      /* #ifdef TEST_CODE     */

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
3.141M

bill@bill:~/miscell$ ./si_print 3.14159265358 30
-30:  Low e-30 3e-30 3.e-30 3.1e-30 3.14e-30 3.141e-30 3.1415e-30 3.14159e-30  -30
-29:  Low e-29 3e-29 3.e-29 3.1e-29 3.14e-29 3.141e-29 3.1415e-29 3.14159e-29  -29
-28:  Low e-28 3e-28 3.e-28 3.1e-28 3.14e-28 3.141e-28 3.1415e-28 3.14159e-28  -28
-27:  Low e-27 3e-27 3.e-27 3.1e-27 3.14e-27 3.141e-27 3.1415e-27 3.14159e-27  -27
-26:  Low e-26 3e-26 3.e-26 3.1e-26 3.14e-26 3.141e-26 3.1415e-26 3.14159e-26  -26
-25:  Low e-25 3e-25 3.e-25 3.1e-25 3.14e-25 3.141e-25 3.1415e-25 3.14159e-25  -25
-24:  3.y 3.1y 3.14y 3.141y 3.1415y 3.14159y 3.141592y 3.1415926y 3.14159265y  -24
-23:  31y 31.y 31.4y 31.41y 31.415y 31.4159y 31.41592y 31.415926y 31.4159265y  -23
-22:  .3z 314y 314.y 314.1y 314.15y 314.159y 314.1592y 314.15926y 314.159265y  -22
-21:  3.z 3.1z 3.14z 3.141z 3.1415z 3.14159z 3.141592z 3.1415926z 3.14159265z  -21
-20:  31z 31.z 31.4z 31.41z 31.415z 31.4159z 31.41592z 31.415926z 31.4159265z  -20
-19:  .3a 314z 314.z 314.1z 314.15z 314.159z 314.1592z 314.15926z 314.159265z  -19
-18:  3.a 3.1a 3.14a 3.141a 3.1415a 3.14159a 3.141592a 3.1415926a 3.14159265a  -18
-17:  31a 31.a 31.4a 31.41a 31.415a 31.4159a 31.41592a 31.415926a 31.4159265a  -17
-16:  .3f 314a 314.a 314.1a 314.15a 314.159a 314.1592a 314.15926a 314.159265a  -16
-15:  3.f 3.1f 3.14f 3.141f 3.1415f 3.14159f 3.141592f 3.1415926f 3.14159265f  -15
-14:  31f 31.f 31.4f 31.41f 31.415f 31.4159f 31.41592f 31.415926f 31.4159265f  -14
-13:  .3p 314f 314.f 314.1f 314.15f 314.159f 314.1592f 314.15926f 314.159265f  -13
-12:  3.p 3.1p 3.14p 3.141p 3.1415p 3.14159p 3.141592p 3.1415926p 3.14159265p  -12
-11:  31p 31.p 31.4p 31.41p 31.415p 31.4159p 31.41592p 31.415926p 31.4159265p  -11
-10:  .3n 314p 314.p 314.1p 314.15p 314.159p 314.1592p 314.15926p 314.159265p  -10
 -9:  3.n 3.1n 3.14n 3.141n 3.1415n 3.14159n 3.141592n 3.1415926n 3.14159265n   -9
 -8:  31n 31.n 31.4n 31.41n 31.415n 31.4159n 31.41592n 31.415926n 31.4159265n   -8
 -7:  .3u 314n 314.n 314.1n 314.15n 314.159n 314.1592n 314.15926n 314.159265n   -7
 -6:  3.u 3.1u 3.14u 3.141u 3.1415u 3.14159u 3.141592u 3.1415926u 3.14159265u   -6
 -5:  31u 31.u 31.4u 31.41u 31.415u 31.4159u 31.41592u 31.415926u 31.4159265u   -5
 -4:  .3m 314u 314.u 314.1u 314.15u 314.159u 314.1592u 314.15926u 314.159265u   -4
 -3:  3.m 3.1m 3.14m 3.141m 3.1415m 3.14159m 3.141592m 3.1415926m 3.14159265m   -3
 -2:  .03 .031 .0314 .03141 .031415 .0314159 .03141592 .031415926 .0314159265   -2
 -1:  .31 .314 .3141 .31415 .314159 .3141592 .31415926 .314159265 .3141592653   -1
  0:  3.1 3.14 3.141 3.1415 3.14159 3.141592 3.1415926 3.14159265 3.141592653    0
  1:  31. 31.4 31.41 31.415 31.4159 31.41592 31.415926 31.4159265 31.41592653    1
  2:  314 314. 314.1 314.15 314.159 314.1592 314.15926 314.159265 314.1592653    2
  3:  3.k 3141 3141. 3141.5 3141.59 3141.592 3141.5926 3141.59265 3141.592653    3
  4:  31k 31.k 31415 31415. 31415.9 31415.92 31415.926 31415.9265 31415.92653    4
  5:  .3M 314k 314.k 314159 314159. 314159.2 314159.26 314159.265 314159.2653    5
  6:  3.M 3.1M 3.14M 3.141M 3141592 3141592. 3141592.6 3141592.65 3141592.653    6
  7:  31M 31.M 31.4M 31.41M 31.415M 31415926 31415926. 31415926.5 31415926.53    7
  8:  .3G 314M 314.M 314.1M 314.15M 314.159M 314159265 314159265. 314159265.3    8
  9:  3.G 3.1G 3.14G 3.141G 3.1415G 3.14159G 3.141592G 3141592653 3141592653.    9
 10:  31G 31.G 31.4G 31.41G 31.415G 31.4159G 31.41592G 31.415926G 31415926535   10
 11:  .3T 314G 314.G 314.1G 314.15G 314.159G 314.1592G 314.15926G 314.159265G   11
 12:  3.T 3.1T 3.14T 3.141T 3.1415T 3.14159T 3.141592T 3.1415926T 3.14159265T   12
 13:  31T 31.T 31.4T 31.41T 31.415T 31.4159T 31.41592T 31.415926T 31.4159265T   13
 14:  .3P 314T 314.T 314.1T 314.15T 314.159T 314.1592T 314.15926T 314.159265T   14
 15:  3.P 3.1P 3.14P 3.141P 3.1415P 3.14159P 3.141592P 3.1415926P 3.14159265P   15
 16:  31P 31.P 31.4P 31.41P 31.415P 31.4159P 31.41592P 31.415926P 31.4159265P   16
 17:  .3E 314P 314.P 314.1P 314.15P 314.159P 314.1592P 314.15926P 314.159265P   17
 18:  3.E 3.1E 3.14E 3.141E 3.1415E 3.14159E 3.141592E 3.1415926E 3.14159265E   18
 19:  31E 31.E 31.4E 31.41E 31.415E 31.4159E 31.41592E 31.415926E 31.4159265E   19
 20:  .3Z 314E 314.E 314.1E 314.15E 314.159E 314.1592E 314.15926E 314.159265E   20
 21:  3.Z 3.1Z 3.14Z 3.141Z 3.1415Z 3.14159Z 3.141592Z 3.1415926Z 3.14159265Z   21
 22:  31Z 31.Z 31.4Z 31.41Z 31.415Z 31.4159Z 31.41592Z 31.415926Z 31.4159265Z   22
 23:  .3Y 314Z 314.Z 314.1Z 314.15Z 314.159Z 314.1592Z 314.15926Z 314.159265Z   23
 24:  3.Y 3.1Y 3.14Y 3.141Y 3.1415Y 3.14159Y 3.141592Y 3.1415926Y 3.14159265Y   24
 25:  31Y 31.Y 31.4Y 31.41Y 31.415Y 31.4159Y 31.41592Y 31.415926Y 31.4159265Y   25
 26:  Hug 314Y 314.Y 314.1Y 314.15Y 314.159Y 314.1592Y 314.15926Y 314.159265Y   26
 27:  Hug e+27 3e+27 3.e+27 3.1e+27 3.14e+27 3.141e+27 3.1415e+27 3.14159e+27   27
 28:  Hug e+28 3e+28 3.e+28 3.1e+28 3.14e+28 3.141e+28 3.1415e+28 3.14159e+28   28
 29:  Hug e+29 3e+29 3.e+29 3.1e+29 3.14e+29 3.141e+29 3.1415e+29 3.14159e+29   29
 30:  Hug e+30 3e+30 3.e+30 3.1e+30 3.14e+30 3.141e+30 3.1415e+30 3.14159e+30   30 */
