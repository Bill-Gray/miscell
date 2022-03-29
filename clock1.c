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
#include <stdint.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <time.h>
#include <sys/time.h>
#endif

/* The NTP code should,  in theory,  work on other POSIX systems.
But I've had a report of failure on MacOS and haven't tested it on
BSD yet.  So we'll only exercise this on Linux for the nonce. */

#ifdef __linux
   #define USE_NTP 1
#endif

#ifdef USE_NTP
#include <sys/timex.h>
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
/* this function to work until Friday, 2262 Apr 11 23:47:16.854775808.    */
/* We can get another 292 years by using unsigned 64-bit integers,  but   */
/* it may be better to switch to 128-bit integers.                        */
/*    Note that the usual limitations apply:  no leap seconds,  and if    */
/* the computer's time is adjusted by NTP or the user,  the result may    */
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

/* Getting time via ntp_gettime( ) appears to be straightforward.
However,  the man pages appear to be in error on several counts.

A distinction is made between ntp_gettime( ) and ntp_gettimex( ).
The latter is supposed to set the TAI field.  At least on my
Xubuntu 18.04 box,  the ntp_gettimex( ) function is unavailable,
but the "old" ntp_gettime( ) function gives you TAI.

Also,  the time is returned in the 'tv_sec' and 'tv_usec' members.
The latter is supposed to be in microseconds.  It actually returns
_nanoseconds_,  which is why it's not multiplied by a thousand
in the following function.          */

#ifdef USE_NTP
int64_t ntp_nanoseconds_since_1970( long *tai)
{
   struct ntptimeval ntv;

   ntp_gettime( &ntv);
/* ntp_gettimex( &ntv);    */
   if( tai)
      *tai = ntv.tai;
   return( ntv.time.tv_sec * (int64_t)1000000000 + ntv.time.tv_usec);
}
#endif

/* clock_gettime( ) appears to be the way to go.  It has nanosecond
resolution and responds in about a microsecond.    */

#ifndef _WIN32

static void try_clock_gettime( void)
{
   struct timespec t[4];
   size_t i;
   const int flags[] = { CLOCK_MONOTONIC, CLOCK_REALTIME
#ifdef CLOCK_TAI
              , CLOCK_TAI
#endif
#ifdef CLOCK_BOOTTIME
             , CLOCK_BOOTTIME
#endif
             };

   const char *hdr[] = { "Monotonic", "Realtime"
#ifdef CLOCK_TAI
              , "TAI"
#endif
#ifdef CLOCK_BOOTTIME
             , "Boot-time"
#endif
             };
   const size_t n_clocks = sizeof( hdr) / sizeof( hdr[0]);

   for( i = 0; i < n_clocks; i++)
      clock_gettime( flags[i], &t[i]);

   for( i = 0; i < n_clocks; i++)
      printf( "%02d:%02d:%02d.%09ld %s\n",
               (int)( t[i].tv_sec / 3600) % 24,
               (int)( t[i].tv_sec / 60) % 60,
               (int)( t[i].tv_sec % 60), t[i].tv_nsec, hdr[i]);
   printf( "%ld clocks/s\n", (long)CLOCKS_PER_SEC);
}
#endif

/* In the following test code,  the time is found _twice_ in quick
succession.  I was wondering just how fast these functions are.
At least on my box,  it seems to take about two microseconds.  That's
just slow enough that I wouldn't want to call ntp_nanoseconds_since_1970
in a really short loop.

   nanoseconds_since_1970( ),  on the same machine,  almost always
returns the same time (rounded to a microsecond) for both calls,
except maybe one time in five.  Which would suggest that it takes
about 0.2 microseconds to run,  about ten times faster than the
NTP function.  (But the latter gives nanosecond precision and
TAI-UTC.  I don't know if that last bit is to be trusted,  though.) */

int main( void)
{
    int c;

    printf( "Shows current UTC time to nanosecond precision (though not\n"
            "necessarily nanosecond accuracy!).  Hit Enter for updated\n"
            "times,  ^C to end program.\n");
    do
       {
       long tai = 0;
       const uint64_t one_billion = (uint64_t)1000000000;
#ifdef USE_NTP
       const uint64_t t1 = ntp_nanoseconds_since_1970( &tai);
       const uint64_t t2 = ntp_nanoseconds_since_1970( &tai);
#else
       const uint64_t t1 = nanoseconds_since_1970( );
       const uint64_t t2 = nanoseconds_since_1970( );
#endif
       const unsigned sec =     (unsigned)( t1 / one_billion);
       const unsigned nanosec = (unsigned)( t1 % one_billion);
       const unsigned sec2 =     (unsigned)( t2 / one_billion);
       const unsigned nanosec2 = (unsigned)( t2 % one_billion);

       printf(  " %02u:%02u:%02u.%09u\n", (sec / 3600) % 24,
                           (sec / 60) % 60, sec % 60, nanosec);
       printf(  " %02u:%02u:%02u.%09u\n", (sec2 / 3600) % 24,
                           (sec2 / 60) % 60, sec2 % 60, nanosec2);
       printf(  "TAI offset %ld\n", tai);
#ifndef _WIN32
       try_clock_gettime( );
#endif
       c = getchar( );
       }
       while( c != 1);

    return( 0);
}
