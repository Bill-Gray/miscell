#include <stdio.h>
#include <stdint.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <time.h>
#include <sys/time.h>
#endif

/* 'nanoseconds_since_1970( )' returns something close to the result of   */
/* ctime( ),  except a billion times larger and with added precision.     */
/* Naturally,  DOS/Windows,  OS/X,  and POSIX systems each use different  */
/* methods.                                                               */
/*    As currently coded,  the actual precision provided is 10^-7 second  */
/* in Windows;  a millisecond with the WATCOM compiler;  and 10^-6 second */
/* in everything else.  But note that "true" nanosecond precision is      */
/* possible,  if actually desired,  on Linux,  OS/X,  and BSD.            */
/*    The range of a 64-bit signed integer is large enough to enable      */
/* this function to work until Friday, 2262 Apr 11 23:47:16.  We can get  */
/* another 292 years by using unsigned 64-bit integers,  but it may be    */
/* better to switch to 128-bit integers.                                  */
/*    Note that the usual limitations apply:  no leap seconds,  and if    */
/* he computer's time is adjusted by NTP or the user,  the result may     */
/* actually go backward.                                  */

#ifdef _WIN32
int64_t nanoseconds_since_1970( void)
{
   FILETIME ft;
   const uint64_t jd_1601 = 2305813;  /* actually 2305813.5 */
   const uint64_t jd_1970 = 2440587;  /* actually 2440587.5 */
   const uint64_t ten_million = 10000000;
   const uint64_t seconds_per_day = 24 * 60 * 60;     /* = 86400 */
   const uint64_t diff = (jd_1970 - jd_1601) * ten_million * seconds_per_day;
   uint64_t decimicroseconds_since_1970;   /* i.e.,  time in units of 1e-7 seconds */

   GetSystemTimeAsFileTime( &ft);
   decimicroseconds_since_1970 = ((uint64_t)ft.dwLowDateTime |
                                ((uint64_t)ft.dwHighDateTime << 32)) - diff;
   return( decimicroseconds_since_1970 * (int64_t)100);
}
#else
#ifdef __WATCOMC__
int64_t nanoseconds_since_1970( void)
{
   struct timeb t;
   const int64_t one_million = 1000000;
   int64_t millisec;

   ftime( &t);
   millisec = (int64_t)t.millitm + (int64_t)1000 * (int64_t)t.time;
   return( millisec * (int64_t)one_million);
}
#else      /* OS/X,  BSD,  and Linux */
int64_t nanoseconds_since_1970( void)
{
   struct timeval now;
   const int rv = gettimeofday( &now, NULL);
   int64_t rval;
   const int64_t one_billion = (int64_t)1000000000;

   if( !rv)
      rval = (int64_t)now.tv_sec * one_billion
           + (int64_t)now.tv_usec * (int64_t)1000;
   else
      rval = 0;
   return( rval);
}
#endif
#endif

/* At one time,  I was using the following in Linux.  It gives a
"real" precision of nanoseconds,  instead of getting microseconds
and multiplying by 1000 (or decimicroseconds and multiplying by 100).
However,  it does require the realtime library to be linked in...
I leave it here in case we someday need nanosecond precision.  */

#ifdef NOT_CURRENTLY_IN_USE
int64_t nanoseconds_since_1970( void)
{
   struct timespec t;

   clock_gettime( CLOCK_REALTIME, &t);
   return( t.tv_sec * (int64_t)1000000000 + t.tv_nsec);
}
#endif    /* NOT_CURRENTLY_IN_USE */

int main( void)
{
    int c;

    printf( "Shows current UTC time to nanosecond precision (though not\n"
            "necessarily nanosecond accuracy!).  Hit Enter for updated\n"
            "times,  ^C to end program.\n");
    do
       {
       const uint64_t one_billion = (uint64_t)1000000000;
       const uint64_t t1 = nanoseconds_since_1970( );
       const unsigned sec =     (unsigned)( t1 / one_billion);
       const unsigned nanosec = (unsigned)( t1 % one_billion);

       printf(  " %02u:%02u:%02u.%09u\n", (sec / 3600) % 24,
                           (sec / 60) % 60, sec % 60, nanosec);
       c = getchar( );
       }
       while( c != 1);

    return( 0);
}
