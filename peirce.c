#include <math.h>

double find_peirce_limit( const int rayleigh, const int N, const int n,
                                        const int m);    /* peirce.cpp */

#ifndef __GNUC__
/* GNU C provides erfc() ("complementary error function"),  defined as

                         ^ +inf
erfc( x) = (2/sqrt(pi)) /  e^(-t^2) dt
                       v x

   ...the above being an approximate ASCII rendition of "integral from x
to +infinity".  OpenWATCOM and MS compilers don't provide erfc.  The
following approximation is from

https://en.wikipedia.org/wiki/Error_function#Approximation_with_elementary_functions

   ...which in turn comes from Abramowitz & Stegun's _Handbook of
Mathematical Functions with Formulae,  Graphs,  and Mathematical Tables_
(equation 7.1.28).  Except I can't find it in the on-line version;  hence
the above Wikipedia reference.

   This has maximum error 1.5E-7.  Good enough for our present needs,  but
be careful in using this elsewhere. */

static double erfc( const double x)
{
   double rval;

   if( x < 0.)
      rval = 2. -erfc( -x);
   else
      {
      const double a1 = 0.254829592, a2 = -0.284496736, a3 = 1.421413741;
      const double p = 0.3275911,   a4 = -1.453152027, a5 = 1.061405429;
      const double t = 1. / (1. + p * x);
      const double tval = t * (a1 + t * (a2 + t * (a3 + t * (a4 + t * a5))));

      rval = exp( -x * x) * tval;
      }
   return( rval);
}
#endif         // #ifndef __GNUC__

/* Code to implement Peirce's method for rejecting/accepting outlying
   observations, for Gaussian or Rayleigh distributions.  (The latter
   is in what I'm most interested,  for astrometric data... though the
   former may prove handy for photometry.)

   See:

   http://peircescriterion.blogspot.com/
   Gould, B. A.,  AJ 4:81 (1855)
   arXiv:1106.0564 [astro-ph]:  http://arxiv.org/abs/1106.0564

   To do this,  we need to find the positive zero-crossing (I'm
   pretty sure there can only be one) of the following "peirce_func"

f(x) = 1 + a(1 - (P/R(x)^n)^b) - x^2

   ...wherein

a = (N - n - m) / n
b = 2 / (N - n)
P = ((n / (N - n)) ^ n) * (((N - n) / N) ^ N)
R = exp( (x^2-1) / 2) * phi( x)

   ...and phi(x) is dependent on whether we're considering a normal
(Gaussian) or Rayleigh distribution:

phi( x) = erfc( x / sqrt( 2)) for a Gaussian distribution,
phi( x) = exp( -x^2) for a Rayleigh distribution

   N = number obs; n=number rejected; m = number of parameters in our
model.  (See the arXiv paper for details on this.)

   In doing our root search,  it helps that a, b, and P are constant
(do not vary with x). Rather than use pow(),  I've rearranged the
above expression as...

f(x) = 1 + a(1 - exp( b * (log(P) - n * log(R)))) - x^2

   For either distribution,  f(0) is positive,  but as x increases, will
eventually drop below zero.   f(x) will be zero somewhere in the neighborhood
of x=2 for moderate N,  i.e.,  our acceptance/rejection will usually be at
around the two-sigma level (more for high N and low n).  The following code
tries f(0) and f(2);  if that doesn't bracket the zero,  it tries f(4),  then
f(6) (only happens for really high N and really low n),  and so on if needed.
Then it can secant-search to find the zero.  This is all very fast;  the
above efforts at optimization are really overkill.  */

static double peirce_func( const int rayleigh, const double x,
               const double a, const double b,
               const double log_P, const int n)
{
   const double sqrt_2 =
            1.414213562373095048801688724209698078569671875376948073176679737;
   const double log_R = (rayleigh ? -(x * x + 1.) / 2. :
                 (x * x - 1.) / 2. + log( erfc( x / sqrt_2)));
   const double tval = exp( b * (log_P - (double)n * log_R));
   const double func = 1. + a * (1. - tval) - x * x;

   return( func);
}

double find_rayleigh_peirce_limit( const double a, const double b,
                                   const double log_P, const int n)
{
   const double log_R = -.5;
   const double tval = exp( b * (log_P - (double)n * log_R));

   return( sqrt( 1. + a * (1 - tval)));
}

double find_peirce_limit( const int rayleigh, const int N, const int n,
                                              const int m)
{
   const double a = (double)(N - n - m) / (double)n;
   const double b = 2. / (double)(N - n);
   const double part_1 = (double)n / (double)(N - n);
   const double part_2 = (double)( N - n) / (double)N;
   const double log_P = (double)n * log( part_1) + (double)N * log( part_2);

   if( rayleigh == 2)
      return( find_rayleigh_peirce_limit( a, b, log_P, n));
   else
      {
      double x1 = 0., y1 = peirce_func( rayleigh, x1, a, b, log_P, n);
      double x2 = 2., y2 = peirce_func( rayleigh, x2, a, b, log_P, n);
      const double thresh = 1e-8;

      while( y2 > 0.)    /* search along increasing x */
         {               /* until we find the root    */
         x1 = x2;
         y1 = y2;
         x2 += x2;
         y2 = peirce_func( rayleigh, x2, a, b, log_P, n);
         }
      while( fabs( x2 - x1) > thresh)
         {
         const double xnew = x2 + (x1 - x2) * y2 / (y2 - y1);

         x1 = x2;        /* secant-search for the root */
         y1 = y2;
         x2 = xnew;
         y2 = peirce_func( rayleigh, xnew, a, b, log_P, n);
         }
      return( x2);
      }
}

#ifdef TEST_MAIN
#include <stdio.h>
#include <stdlib.h>

int main( const int argc, const char **argv)
{
   int i, show_squared = 0, rayleigh = 0;
   int N, n, m = 1;

   for( i = 1; i < argc; i++)
      switch( argv[i][0])
         {
         case 'r':
            rayleigh = 1;     /* "standard" Rayleigh */
            break;
         case 'R':
            rayleigh = 2;     /* modified and,  I suspect,  broken Rayleigh */
            break;
         case '^':
            show_squared = 1;  /* follow Gould's 1855 paper and show    */
            break;             /* squared values */
         default:
            m = atoi( argv[i]);
            break;
         }

          /* print out a table for a given m: */

   printf( " N    n=1, m=%d (%s)\n", m,
               rayleigh ? "Rayleigh" : "Gaussian");
   for( N = m + 2; N < m + 45; N++)
      {
      printf( "%2d:", N);
      for( n = 1; n <= N - m - 1 && n < 11; n++)
         {
         const double oval = find_peirce_limit( rayleigh, N, n, m);

         printf( "%7.4f", (show_squared ? oval * oval : oval));
         }
      if( n < 11)          /* add a column heading */
         printf( "   n=%d", n);
      printf( "\n");
      }
   return( 0);
}
#endif         /* #ifdef TEST_MAIN */

/* Run the above main( ) with a command-line argument of 1 (i.e.,  m=1)
and you replicate the tables shown in the 'piercescriterion.blogspot'
and Gould paper mentioned above.  (Except that Gould gives the _squares_
of x,  and you'll have to run with m=2 to get his second table;  also note
the blog gives an incorrect value for N=3, n=1.)  Anyway,  here's the table
for m=1,  unsquared:

 N    n=1, m=1 (Gaussian)
 3: 1.2163   n=2
 4: 1.3829 1.0786   n=3
 5: 1.5093 1.1996 0.9893   n=4
 6: 1.6098 1.2990 1.0992 0.9011   n=5
 7: 1.6928 1.3821 1.1870 1.0223 0.8042   n=6
 8: 1.7632 1.4532 1.2606 1.1087 0.9515 0.7027   n=7
 9: 1.8242 1.5151 1.3241 1.1783 1.0444 0.8805 0.6042   n=8
10: 1.8777 1.5698 1.3800 1.2375 1.1142 0.9856 0.8079 0.5140   n=9
11: 1.9254 1.6189 1.4300 1.2896 1.1719 1.0594 0.9285 0.7350 0.4338   n=10
12: 1.9683 1.6632 1.4752 1.3362 1.2219 1.1178 1.0091 0.8714 0.6639 0.3635
13: 2.0073 1.7036 1.5163 1.3784 1.2663 1.1672 1.0702 0.9610 0.8143 0.5963
14: 2.0429 1.7406 1.5542 1.4171 1.3065 1.2105 1.1203 1.0263 0.9136 0.7575
15: 2.0757 1.7749 1.5891 1.4528 1.3433 1.2494 1.1635 1.0783 0.9845 0.8664
16: 2.1061 1.8066 1.6216 1.4859 1.3773 1.2849 1.2018 1.1222 1.0394 0.9436
17: 2.1343 1.8362 1.6519 1.5168 1.4089 1.3176 1.2365 1.1605 1.0847 1.0023
18: 2.1606 1.8639 1.6803 1.5457 1.4384 1.3481 1.2684 1.1949 1.1236 1.0496
19: 2.1853 1.8900 1.7070 1.5729 1.4662 1.3765 1.2979 1.2263 1.1581 1.0897
20: 2.2085 1.9145 1.7322 1.5986 1.4923 1.4032 1.3255 1.2552 1.1893 1.1247
21: 2.2305 1.9377 1.7561 1.6229 1.5170 1.4284 1.3514 1.2822 1.2180 1.1561
22: 2.2512 1.9597 1.7787 1.6460 1.5405 1.4523 1.3758 1.3075 1.2445 1.1847
23: 2.2709 1.9805 1.8002 1.6679 1.5627 1.4749 1.3990 1.3313 1.2694 1.2111
24: 2.2895 2.0004 1.8207 1.6888 1.5840 1.4965 1.4210 1.3539 1.2928 1.2356
25: 2.3074 2.0194 1.8403 1.7088 1.6043 1.5171 1.4420 1.3753 1.3149 1.2587
26: 2.3244 2.0375 1.8590 1.7279 1.6237 1.5368 1.4620 1.3958 1.3358 1.2804
27: 2.3406 2.0548 1.8769 1.7462 1.6423 1.5557 1.4812 1.4153 1.3558 1.3011
28: 2.3562 2.0715 1.8941 1.7637 1.6601 1.5738 1.4995 1.4340 1.3749 1.3207
29: 2.3711 2.0874 1.9106 1.7806 1.6773 1.5912 1.5172 1.4519 1.3932 1.3394
30: 2.3855 2.1028 1.9265 1.7969 1.6938 1.6080 1.5342 1.4692 1.4108 1.3574
31: 2.3993 2.1176 1.9418 1.8126 1.7098 1.6242 1.5506 1.4858 1.4276 1.3746
32: 2.4125 2.1318 1.9566 1.8277 1.7252 1.6398 1.5664 1.5018 1.4439 1.3911
33: 2.4254 2.1456 1.9709 1.8423 1.7400 1.6548 1.5816 1.5172 1.4596 1.4071
34: 2.4377 2.1589 1.9847 1.8564 1.7544 1.6694 1.5964 1.5322 1.4747 1.4224
35: 2.4497 2.1717 1.9980 1.8701 1.7683 1.6835 1.6107 1.5466 1.4893 1.4373
36: 2.4613 2.1841 2.0109 1.8833 1.7818 1.6972 1.6245 1.5606 1.5035 1.4517
37: 2.4725 2.1962 2.0235 1.8962 1.7949 1.7104 1.6379 1.5742 1.5173 1.4656
38: 2.4833 2.2079 2.0356 1.9086 1.8075 1.7233 1.6510 1.5874 1.5306 1.4791
39: 2.4939 2.2192 2.0474 1.9207 1.8199 1.7358 1.6636 1.6002 1.5435 1.4922
40: 2.5041 2.2302 2.0589 1.9325 1.8319 1.7480 1.6759 1.6127 1.5561 1.5049
*/
