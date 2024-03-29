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
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <stdint.h>
#include <assert.h>
#include "jpl_xref.h"

/* Code to convert an ephemeris gathered from JPL's _Horizons_ system into
the format used by MPC's DASO service,  or for generating TLEs with
my 'eph2tle' program.  Based largely on code I'd already written to
import _Horizons_ data to the .b32 format used in Guide.

   It can convert either position-only ephemerides (which is what are
used for MPC's DASO service) or state vector ephems (which is the
input needed for my 'eph2tle' software to fit TLEs.)  The output
is in equatorial J2000,  AU,  and AU/day (if the input is in ecliptic
coordinates and/or km and km/s,  the vectors are rotated and scaled
accordingly;  MPC and eph2tle are both particular about taking only
equatorial AU/day data.)                  */

static int look_up_name( char *obuff, const size_t buffsize, const int idx)
{
   size_t i;

   for( i = 0; i < sizeof( jpl_xrefs) / sizeof( jpl_xrefs[0]); i++)
      if( idx == jpl_xrefs[i].jpl_desig)
         {
         snprintf( obuff, (int)buffsize, "%s = %s = NORAD %d",
                  jpl_xrefs[i].name, jpl_xrefs[i].intl_desig,
                  jpl_xrefs[i].norad_number);
         return( i);
         }
   fprintf( stderr, "Couldn't find JPL ID %d\n"
             "This object's JPL object number isn't in the list built into\n"
             "this program.  It either isn't an artsat,  or just isn't in the\n"
             "list.  That may just mean the list needs updating;  please contact\n"
             "the author of this program if you think that's happening here.\n", idx);
   exit( -1);
}

static int get_coords_from_buff( double *coords, const char *buff, const bool is_ecliptical)
{
   int xloc = 1, yloc = 24, zloc = 47;

   if( buff[1] == 'X' || buff[2] == 'X')     /* quantities are labelled */
      {
      xloc = 4;
      yloc = 30;
      zloc = 56;
      }
   coords[0] = atof( buff + xloc);
   coords[1] = atof( buff + yloc);
   coords[2] = atof( buff + zloc);
   if( is_ecliptical)         /* rotate to equatorial */
      {
      static const double sin_obliq_2000 = 0.397777155931913701597179975942380896684;
      static const double cos_obliq_2000 = 0.917482062069181825744000384639406458043;
      const double temp    = coords[2] * cos_obliq_2000 + coords[1] * sin_obliq_2000;

      coords[1] = coords[1] * cos_obliq_2000 - coords[2] * sin_obliq_2000;
      coords[2] = temp;
      }
   return( 0);
}

static char *fgets_trimmed( char *buff, const size_t buffsize, FILE *ifile)
{
   char *rval = fgets( buff, (int)buffsize, ifile);

   if( rval)
      {
      size_t i = strlen( buff);

      while( i && buff[i - 1] <= ' ')
         i--;
      rval[i] = '\0';
      }
   return( rval);
}

static double ephemeris_line_jd( const char *buff)
{
   const double rval = atof( buff);

   if( rval > 2000000. && rval < 3000000. &&
               strlen( buff) == 54 && !memcmp( buff + 17, " = A.D.", 7)
               && buff[42] == ':' && buff[45] == '.'
               && !memcmp( buff + 50, " TDB", 4))
      return( rval);
   else                          /* not an ephemeris line */
      return( 0.);
}

int main( const int argc, const char **argv)
{
   FILE *ifile = (argc > 1 ? fopen( argv[1], "rb") : NULL);
   FILE *ofile = (argc > 2 ? fopen( argv[2], "wb") : stdout);
   char buff[200];
   unsigned n_written = 0;
   double jd0 = 0., step_size = 0., jd, frac_jd0 = 0.;
   int int_jd0 = 0, i;
   bool state_vectors = false, is_equatorial = false;
   bool is_ecliptical = false, in_km_s = false;
   bool output_in_au_days = true;
   const char *header_fmt = "%13.5f %14.10f %4u";
   char object_name[90];
   const double AU_IN_KM = 1.495978707e+8;
   const double seconds_per_day = 24. * 60. * 60.;
   time_t t0;

   if( argc > 1 && !ifile)
      fprintf( stderr, "\nCouldn't open the Horizons file '%s'\n", argv[1]);
   if( !ofile)
      fprintf( stderr, "\nCouldn't open the output file '%s'\n", argv[2]);

   if( argc < 2 || !ifile || !ofile)
      {
      fprintf( stderr, "\nJPL2MPC takes input ephemeri(de)s generated by HORIZONS  and,\n"
              "produces file(s) suitable for use in DASO or eph2tle.  The name of\n"
              "the input ephemeris must be provided as a command-line argument.\n"
              "For example:\n"
              "\njpl2mpc gaia.txt\n\n"
              "The JPL ephemeris must be in text form (can use the 'download/save'\n"
              "option for this).\n"
              "   The bottom of 'jpl2mpc.cpp' shows how to submit a job via e-mail\n"
              "to the Horizons server that will get you an ephemeris in the necessary\n"
              "format,  or how to get such ephemerides using a URL.\n");
      exit( -1);
      }
   for( i = 2; i < argc; i++)
      if( argv[i][0] == '-')
         switch( argv[i][1])
            {
            case 'k':
               output_in_au_days = false;
               break;
            }

   while( fgets_trimmed( buff, sizeof( buff), ifile))
      {
      if( (jd = ephemeris_line_jd( buff)) > 0.)
         {
         if( !n_written)
            {
            jd0 = jd;
            int_jd0 = atoi( buff);
            frac_jd0 = atof( buff + 7);
            }
         else if( n_written == 1)
            step_size = (atof( buff + 7) - frac_jd0)
                                  + (double)( atoi( buff) - int_jd0);
         n_written++;
         }
      else if( !memcmp( buff, "   VX    VY    VZ", 17))
         state_vectors = true;
      else if( strstr( buff, "Earth Mean Equator and Equinox"))
         is_equatorial = true;
      else if( strstr( buff, "Reference frame : ICRF"))
         is_equatorial = true;
      else if( strstr( buff, "Ecliptic and Mean Equinox of Reference Epoch"))
         is_ecliptical = true;
      else if( strstr( buff, "Reference frame : Ecliptic of J2000"))
         is_ecliptical = true;
      else if( !memcmp( buff, " Revised:", 9))
         {
         char *id = strchr( buff + 70, '-');

         assert( id);
         look_up_name( object_name, sizeof( object_name), atoi( id));
         }
      else if( !memcmp( buff, "Target body name:", 17))
         {
         const char *tptr = strstr( buff, "(-");

         if( tptr)
            look_up_name( object_name, sizeof( object_name), atoi( tptr + 1));
         }
      else if( !memcmp( buff,  "Output units    : KM-S", 22))
         in_km_s = true;
      }
   if( is_equatorial == is_ecliptical)       /* should be one or the other */
      {
      fprintf( stderr, "Input coordinates must be in the Earth mean equator\n"
                       "and equinox,  or in J2000 ecliptic coordinates.\n");
      return( -1);
      }
   fprintf( ofile, header_fmt, jd0, step_size, n_written);
   if( output_in_au_days)
      fprintf( ofile, " 0,1,1");  /* i.e.,  ecliptic J2000 in AU and days */
   else
      fprintf( ofile, " 0,149597870.7,86400");     /* km and km/s */
   if( *object_name)
      fprintf( ofile, " (500) Geocentric: %s\n", object_name);
   else
      fprintf( ofile, "\n");
                  /* Now go back to start of input and read through, */
                  /* looking for actual ephemeris data to output : */
   fseek( ifile, 0L, SEEK_SET);
   while( fgets_trimmed( buff, sizeof( buff), ifile))
      if( (jd = ephemeris_line_jd( buff)) > 0.)
         {
         double coords[3];
         int i;

         if( !fgets_trimmed( buff, sizeof( buff), ifile))
            {
            fprintf( stderr, "Failed to get data from input file\n");
            return( -2);
            }
         get_coords_from_buff( coords, buff, is_ecliptical);
         if( in_km_s)
            for( i = 0; i < 3; i++)
               coords[i] /= AU_IN_KM;
         if( output_in_au_days)
            fprintf( ofile, "%13.5f%21.16f%21.16f%21.16f", jd,
                  coords[0], coords[1], coords[2]);
         else
            fprintf( ofile, "%13.5f%21.7f%21.7f%21.7f", jd,
                       coords[0] * AU_IN_KM,
                       coords[1] * AU_IN_KM,
                       coords[2] * AU_IN_KM);
         if( !state_vectors)
            fprintf( ofile, "\n");
         else
            {
            const char *vel_format = (output_in_au_days ?
                         " %21.17f%21.17f%21.17f\n" :
                         " %21.13f%21.13f%21.13f\n");

            if( !fgets_trimmed( buff, sizeof( buff), ifile))
               {
               fprintf( stderr, "Failed to get data from input file\n");
               return( -2);
               }
            get_coords_from_buff( coords, buff, is_ecliptical);
            if( in_km_s)
               for( i = 0; i < 3; i++)
                  coords[i] *= seconds_per_day / AU_IN_KM;
            if( !output_in_au_days)
               for( i = 0; i < 3; i++)
                  coords[i] *= AU_IN_KM / seconds_per_day;
            fprintf( ofile, vel_format,
                  coords[0], coords[1], coords[2]);
            }
         }

   t0 = time( NULL);
   fprintf( ofile, "\n\nCreated from Horizons data by 'jpl2mpc', ver %s, at %.24s UTC\n",
                                        __DATE__, asctime( gmtime( &t0)));
                     /* Seek back to start of input file & write header data: */
   fseek( ifile, 0L, SEEK_SET);
   while( fgets_trimmed( buff, sizeof( buff), ifile) && memcmp( buff, "$$SOE", 5))
      fprintf( ofile, "%s\n", buff);

   fclose( ifile);
   return( 0);
}

/* Following is an example e-mail request to the Horizons server for a
suitable text ephemeris for Gaia (followed by a similar example
showing how to send a request on a URL,  which is probably the
method I'll be using in the future... you get the same result
either way,  but the URL modification is a little easier.)

   For other objects,  you would modify the COMMAND and possibly
CENTER lines in the following (if you didn't want geocentric vectors)
as well as the START_TIME,  STOP_TIME, and STEP_SIZE. And, of course,
the EMAIL_ADDR.

   Aside from that,  all is as it should be:  vectors are requested
with positions (or positions/velocities),  with no light-time corrections.

   After making those modifications,  you would send the result to
horizons@ssd.jpl.nasa.gov, subject line JOB.

!$$SOF (ssd)       JPL/Horizons Execution Control VARLIST
! Full directions are at
! ftp://ssd.jpl.nasa.gov/pub/ssd/horizons_batch_example.long

! EMAIL_ADDR sets e-mail address output is sent to. Enclose
! in quotes. Null assignment uses mailer return address.

 EMAIL_ADDR = 'pluto@projectpluto.com'
 COMMAND    = 'Gaia'

! MAKE_EPHEM toggles generation of ephemeris, if possible.
! Values: YES or NO

 MAKE_EPHEM = 'YES'

! TABLE_TYPE selects type of table to generate, if possible.
! Values: OBSERVER, ELEMENTS, VECTORS
! (or unique abbreviation of those values).

 TABLE_TYPE = 'VECTORS'
 CENTER     = '500@399'
 REF_PLANE  = 'FRAME'

! START_TIME specifies ephemeris start time
! (i.e. YYYY-MMM-DD {HH:MM} {UT/TT}) ... where braces "{}"
! denote optional inputs. See program user's guide for
! lists of the numerous ways to specify times. Time zone
! offsets can be set. For example, '1998-JAN-1 10:00 UT-8'
! would produce a table in Pacific Standard Time. 'UT+00:00'
! is the same as 'UT'. Offsets are not applied to TT
! (Terrestrial Time) tables. See TIME_ZONE variable also.

 START_TIME = '2014-OCT-14 00:00 TDB'

! STOP_TIME specifies ephemeris stop time
! (i.e. YYYY-MMM-DD {HH:MM}).

 STOP_TIME  = '2016-JAN-01'
 STEP_SIZE  = '1 day'
 QUANTITIES = '
 REF_SYSTEM = 'J2000'
 OUT_UNITS  = 'AU-D'

! VECT_TABLE = 1 means XYZ only,  no velocity, light-time,
! range, or range-rate.  Use VECT_TABLE = 2 to also get the
! velocity,  to produce state vector ephemerides resembling
! those from Find_Orb :
 VECT_TABLE = '1'

! VECT_CORR selects level of correction: NONE=geometric states
! (which we happen to want); 'LT' = astrometric states, 'LT+S'
! = same with stellar aberration included.
 VECT_CORR = 'NONE'

 CAL_FORMAT = 'CAL'

!$$EOF++++++++++++++++++++++++++++++++++++++++++++++++++++++

https://ssd.jpl.nasa.gov/horizons_batch.cgi?batch=1&COMMAND='-139479'&OBJ_DATA='NO'&TABLE_TYPE='V'&START_TIME='2020-01-01'&STOP_TIME='2021-01-01'&STEP_SIZE='3660'&VEC_TABLE='2'&VEC_LABELS='N'

For TESS,  2021 :

https://ssd.jpl.nasa.gov/horizons_batch.cgi?batch=1&COMMAND='-95'&OBJ_DATA='NO'&TABLE_TYPE='V'&START_TIME='2021-01-01'&STOP_TIME='2022-01-01'&STEP_SIZE='3650'&VEC_TABLE='2'&VEC_LABELS='N'

*/
