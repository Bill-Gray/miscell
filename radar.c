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

#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <time.h>
#include <assert.h>
#include "mpc_func.h"
#include "stringex.h"

/* Getting radar astrometry in a timely manner can be problematic.  It
appears almost immediately at

http://ssd.jpl.nasa.gov/?grp=all&fmt=html&radar=

    but there is usually a significant delay before it shows up on AstDyS or
NEODyS,  and anything recent doesn't appear at MPC at all.  The JPL data is
now available converted to MPC 80-column format at

https://www.projectpluto.com/radar/radar.htm

   which lets you download all the radar data,  or just the data for a
desired object.  That on-line service (which uses this code) may suffice
for your needs;  you may not need to compile and run this code yourself.

   If you _do_ want to run the conversion yourself,  you will first need to
download the radar data in JSON form from JPL's API using the following URL :

https://ssd-api.jpl.nasa.gov/sb_radar.api?modified=y&notes=y&observer=y

   Save the result as 'radar.json' and run this program (no command-line
arguments required).  It will convert the data to the MPC's 80-column
punched-card format (two lines per observation). Example output lines :

A1955         R1999 09 21.375000               +    13595900   8560 253 JPLRS253
A1955         r1999 09 21.375000C                        5000       253 JPLRS253
A1955         R1999 09 21.430556   1541845400                  8560 253 JPLRS253
A1955         r1999 09 21.430556C        10000                      253 JPLRS253
A1955         R1999 09 23.395833   1482063100                  8560 253 JPLRS253
A1955         r1999 09 23.395833C         5000                      253 JPLRS253

   Note that I didn't try to combine simultaneous Doppler and range observations
into a single MPC observation.  The MPC format lets you do that,  but it's not
a requirement (and keeping them separate does make it easier to exclude one
observation but not the other.)

   Also note that another program in this repository,  'getradar.c' (q.v.),  can
be used to extract data for a specific object.  (The CGI-ified version is used
for the on-line service mentioned above.)     */

static void put_mpc_code_from_dss( char *mpc_code, const int dss_desig)
{
   const char *code;

   switch( dss_desig)
      {
      case -1:
         code = "251";     /* Arecibo */
         break;
      case -2:
         code = "254";
         break;            /* used only for two 1566 Icarus (1949 MA) obs */
      case -9:
         code = "256";     /* used for a few 2007 TU24 and one 2001 EC16 obs */
         break;
      case -13:            /* Goldstone,  DSS 13 */
         code = "252";
         break;
      case -14:
         code = "253";     /* Goldstone, DSS 14 */
         break;
      case -25:
         code = "257";     /* Goldstone, DSS 25 */
         break;            /* used only as receiver for 2006 RH120 */
      case -35:
         code = "263";     /* DSS-35 in Australia */
         break;
      case -36:
         code = "264";     /* DSS-36 in Australia */
         break;
      case -38:
         code = "255";     /* used only as receiver for 6489 Golevka (1991 JX) */
         break;
      case -43:
         code = "265";     /* DSS-43 in Australia */
         break;
      case -47:
         code = "271";     /* DSS-47 in Australia */
         break;
      case -73:
         code = "273";
         break;            /* used only for 367943 Duende (2012 DA14) */
      default:
         fprintf( stderr, "DSS designation %d unrecognized", dss_desig);
         code = "?!!";
         break;
      }
   memcpy( mpc_code, code, 3);
}

static bool havent_seen_this_name( const char *iname)
{
   static char **names = NULL;
   int i;
   const int max_names = 10000;

   if( !names)
      names = (char **)calloc( max_names, sizeof( char *));
   for( i = 0; names[i]; i++)
      if( !strcmp( names[i], iname))
         return( false);
   names[i] = (char *)malloc( strlen( iname) + 1);
   strcpy( names[i], iname);
   return( true);
}

typedef struct
   {
   char desig[20], time[20], time_modified[20];
   char measurement[20], sigma[20], freq_mhz[20];
   int receiver, xmitter;
   bool is_range;
   char bounce_point;      /* P = peak power, C = center of mass */
   int reference;
   char *observers, *notes;
   } radar_obs_t;

static bool show_unknown_names = false;

static void substitute_name( char *oname, const char *iname, const char *desig, const char *time_observed)
{
   static char **subs = NULL;
   static size_t n_subs = 0;
   size_t i;

   if( !subs)
      {
      char buff[150];
      FILE *ifile = fopen( "rnames.txt", "rb");

      assert( ifile);
      subs = (char **)calloc( 2000, sizeof( char *));
      while( fgets( buff, sizeof( buff), ifile))
         if( *buff != '#')
            {
            buff[strlen( buff) - 1] = '\0';    /* remove trailing LF */
            subs[n_subs] = strdup( buff);
            i = 35;
            while( i && buff[i - 1] == ' ')
               i--;
            assert( i);
            subs[n_subs][i] = '\0';
            n_subs++;
            }
      }

   for( i = 0; i < n_subs; i++)
      if( !strcasecmp( iname, subs[i]))
         {
         strcpy( oname, subs[i] + 35);
         return;
         }
   strcpy( oname, "!?");
   strcat( oname, iname);
   if( show_unknown_names && havent_seen_this_name( iname))
      fprintf( stderr, "%s %s %s\n", desig, time_observed, iname);
}

/* This code assumes names will be comma-separated.  Which means that
'Benner,L.A.M',  for example,  would be read as 'Benner' and 'L.A.M'.
Exceptions have to be replaced with the right name(s). */

static void fix_observers( char *buff, const char *desig, const char *time_observed)
{
   char obuff[300], *tptr, *start = buff;
   bool done = false;
   size_t i, j;
   static const char *subs[] = {
            "Benner,L.A.M.",     "Benner",
            "Benner, L.A.M.",    "Benner",
            "Benner,L. A. M.",   "Benner",
            "Benner, L. A. M.",  "Benner",
            "Benner, L.",        "Benner",
            "Benner,L.",         "Benner",
            "Bennner, L. A. M.", "Benner",
            "BENER",             "Benner",
            "BIUSCH",            "Busch",
            "Busch, M.W.",       "Busch",
            "Campbell,D.B.",     "Campbell",
            "Campbell,D.B",      "Campbell",
            "Chandler,J.F",      "Chandler",
            "GOLDSTEIN,R.M",     "Goldstein",
            "Greenberg,A.H.",    "Greenberg",
            "Harris,A.W",        "Harris",
            "Harmon, J.K.",      "Harmon",
            "Harmon,J.K",        "Harmon",
            "Hine,A.A",          "Hine",
            "Horiuchi,S.",       "Horiuchi",
            "Kamoun,P.G.",       "Kamoun",
            "LIESKE,J.H",        "Lieske",
            "Margot,J.L.,J.-L.", "Margot",
            "Margot, J. L.",     "Margot",
            "Margot,J.L.",       "Margot",
            "Margot,JL",         "Margot",
            "MAGRI,C.",          "Magri",
            "Marsden,B.G.",      "Marsden",
            "Marshall, S.",      "Marshall",
            "Naidu, S.P.",       "Naidu",
            "M. Nolan",          "M. Nolan",
            "M. NOLAN AND A. HINE",  "M. Nolan, A. Hine",
            "Nolan,MC",          "Nolan",
            "Nolan,M.C.",        "Nolan",
            "Nolan,M. C.",       "Nolan",
            "Nolan,M",           "Nolan",
            "Nolan.",            "Nolan",
            "Ostro,S.J.",        "Ostro",
            "Ostro,S.J",         "Ostro",
            "Ostro,S.",          "Ostro",
            "Ostro, S.",         "Ostro",
            "Ostro,S",           "Ostro",
            "PETTENGILL,G.H.",   "Pettengill",
            "PETTENGILL,G.H",    "Pettengill",
            "Rosema,K.D",        "Rosema",
            "SHAPIRO,I.I.",      "Shapiro",
            "SHAPIRO,I.I",       "Shapiro",
            "SHAPIRO,I",         "Shapiro",
            "Shepard, M.",       "Shepard",
            "TAYLOR,P.",         "Taylor",
            "Werner,C.L",        "Werner",
            "Young,J.W",         "Young",
            "Zaitsev,A.",        "Zaitsev",
            NULL };

   for( i = 0; subs[i]; i += 2)
      if( (tptr = strcasestr( buff, subs[i])) != NULL)
         {
         strlcpy_error( obuff, subs[i + 1]);
         strlcat_error( obuff, tptr + strlen( subs[i]));
         strcpy( tptr, obuff);
         }

   for( tptr = buff; *tptr; tptr++)
      if( (tptr == buff || tptr[-1] == ',') &&
                 isupper( tptr[0]) && isupper( tptr[1]) && tptr[2])
         {        /* cases such as 'AM DH .. ..' */
         i = 2;
         if( isupper( tptr[i]))     /* maybe ERV,  etc. */
            i++;
         if( isupper( tptr[i]))     /* Maybe LFZM, etc. */
            i++;
         while( tptr[i] == ' ' && isupper( tptr[i + 1]) && isupper( tptr[i + 2]))
            {
            tptr[i] = ',';
            i += 3;
            }
         }

   *obuff = '\0';
   while( !done)
      {
      while( *buff == ' ')
         buff++;
      i = 0;
      while( buff[i] && !strchr( ",;/&", buff[i]))
         i++;
      done = (buff[i] == '\0');
      buff[i] = '\0';
      j = i;
      while( j && buff[j - 1] == ' ')
         j--;
      buff[j] = '\0';
      substitute_name( obuff + strlen( obuff), buff, desig, time_observed);
      buff += i;
      if( !done)
         {
         strlcat_error( obuff, ", ");
         buff++;
         }
      }
   strcpy( start, obuff);
}

static void get_packed_desig( char *packed, const char *idesig)
{
   char tbuff[22];
   int i = 0;

   while( isdigit( idesig[i]))
      i++;
   if( !idesig[i])
      snprintf_err( tbuff, sizeof( tbuff), "(%s)", idesig);
   else
      strlcpy_error( tbuff, idesig);
   if( create_mpc_packed_desig( packed, tbuff))
      fprintf( stderr, "Couldn't pack '%s'\n", tbuff);
}

static void get_radar_obs( char *buff, radar_obs_t *obs)
{
   int field;

   memset( obs, 0, sizeof( radar_obs_t));
   for( field = 0; field < 12; field++)
      {
      int field_len;
      char *tptr;
      bool is_null = false;

      if( *buff == '"')
         {
         size_t i = 1;

         while( buff[i] && (buff[i] != '"' || buff[i - 1] == '\\'))
            i++;
         tptr = buff + 1;
         field_len = (int)i - 1;
         buff += i + 2;
         }
      else if( !memcmp( buff, "null", 4))
         {
         tptr = buff;
         field_len = 4;
         buff += 5;
         is_null = true;
         }
      else
         {
         fprintf( stderr, "Error at '%.40s'\n", buff);
         exit( -1);
         }
      tptr[field_len] = '\0';
      if( !is_null)
         switch( field)
            {
            case 0:
               assert( field_len < 15);
               get_packed_desig( obs->desig, tptr);
               break;
            case 1:
               strlcpy_error( obs->time, tptr);
               break;
            case 2:
               strlcpy_error( obs->measurement, tptr);
               break;
            case 3:
               strlcpy_error( obs->sigma, tptr);
               break;
            case 4:
               if( !strcmp( tptr, "Hz"))
                  obs->is_range = false;
               else if( !strcmp( tptr, "us"))
                  obs->is_range = true;
               else
                  fprintf( stderr, "Bad units '%s'\n", tptr);
               break;
            case 5:
               strlcpy_error( obs->freq_mhz, tptr);
               break;
            case 6:
               obs->receiver = atoi( tptr);
               break;
            case 7:
               obs->xmitter  = atoi( tptr);
               break;
#ifdef NO_REFERENCES_YET
            case 7:
               obs->reference = atoi( tptr);
               break;
#endif
            case 8:
               obs->bounce_point = *tptr;
               if( *tptr != 'C' && *tptr != 'P')
                  fprintf( stderr, "? Error : '%s'\n", tptr);
               assert( *tptr == 'C' || *tptr == 'P');
               break;
            case 9:
               {
               char tbuff[200];

               assert( field_len < (int)sizeof( tbuff) - 1);
               strlcpy_error( tbuff, tptr);
//             printf( "%s\n", tbuff);
               fix_observers( tbuff, obs->desig, obs->time);
               obs->observers = (char *)malloc( strlen( tbuff) + 1);
               strcpy( obs->observers, tbuff);
               }
               break;
            case 10:
               obs->notes = (char *)malloc( strlen( tptr) + 1);
               strcpy( obs->notes, tptr);
               break;
            case 11:
               assert( strlen( tptr) == 19);
               strlcpy_error( obs->time_modified, tptr);
               break;
            default:
               fprintf( stderr, "? field %d\n", field);
            }
      }
}

static void output_index( const char *buff)
{
   char **found = (char **)calloc( 10000, sizeof( char *));
   size_t n_found = 0, i;

   while( *buff)
      {
      buff++;
      if( buff[0] == '[' && buff[1] == '"')
         {
         char tbuff[20], packed[20];

         buff += 2;
         i = 0;
         while( buff[i] && buff[i] != '"')
            i++;
         assert( i < sizeof( tbuff));
         memcpy( tbuff, buff, i);
         buff += i;
         tbuff[i] = '\0';
         get_packed_desig( packed, tbuff);
         i = 0;
         while( i < n_found && strcmp( found[i], packed))
            i++;
         if( i == n_found)
            {
            found[n_found] = (char *)malloc( strlen( packed) + 1);
            strcpy( found[n_found++], packed);
            }
         }
      }
   for( i = 1; i < n_found; i++)
      if( strcmp( found[i], found[i - 1]) < 0)
         {
         char *tptr = found[i];

         found[i] = found[i - 1];
         found[i - 1] = tptr;
         if( i > 1)
            i -= 2;
         }
   for( i = 0; i < n_found; i++)
      {
      if( !(i % 5))
         printf( "\nCOM desigs :");
      printf( " %s", found[i]);
      }
   printf( "\n");
}

/* The round-trip travel time,  Doppler frequency,  and their
uncertainties are all stored with implicit decimal points.
See MPC's documentation of the radar astrometry format.  */

static void put_with_implicit_decimal( char *line, const char *text)
{
   const char *decimal = strchr( text, '.');
   size_t len;

   if( decimal)
      len = decimal - text;
   else
      len = strlen( text);
   memcpy( line - len, text, len);
   if( decimal)
      {
      size_t n_places = strlen( decimal + 1);

      if( n_places > 4)
         n_places = 4;
      memcpy( line, decimal + 1, n_places);
      }
   line -= len;
   while( *line == '0' && line[1] != ' ')
      *line++ = ' ';       /* remove leading zeroes */
}

static char last_modified[20];
static char last_observed[20];

static void put_radar_comment( const radar_obs_t *obs)
{
   const char *notes = obs->notes;
   char mpc_code[4];

   put_mpc_code_from_dss( mpc_code, obs->receiver);
   printf( "\nCOD %.3s\n", mpc_code);
   printf( "OBS %s\n", obs->observers);
   printf( "COM Last modified %s\n", obs->time_modified);
   if( strcmp( last_modified, obs->time_modified) < 0)
      strlcpy_error( last_modified, obs->time_modified);
   if( strcmp( last_observed, obs->time) < 0)
      strlcpy_error( last_observed, obs->time);
   while( notes && *notes >= ' ')
      {
      size_t len;
      const size_t max_len = 70;
      const char *insert = "";

      while( *notes == ' ')
         notes++;
      len = strlen( notes);
      if( notes[0] == '=' && notes[1] == ' ')
         insert = " ";
      if( len <= max_len)           /* finish up line */
         {
         printf( "COM %s%s\n", insert, notes);
         return;
         }
      else
         {
         len = max_len;
         while( notes[len] != ' ')
            len--;
         printf( "COM %s%.*s\n", insert, (int)len, notes);
         notes += len + 1;
         }
      }
}

static void put_radar_obs( char *line1, char *line2, const radar_obs_t *obs)
{
   const int seconds = atoi( obs->time + 17)
                + atoi( obs->time + 14) * 60 + atoi( obs->time + 11) * 3600;
   const int microdays = (seconds * 625 + 27) / 54;
   int dest_column = (obs->is_range ? 43 : 58);
   const char *measurement = obs->measurement;
   char *tptr;

   memset( line1, ' ', 80);
   line1[80] = '\0';
   memcpy( line1, obs->desig, 12);
   line1[14] = 'R';
   memcpy( line1 + 15, obs->time, 10);
   line1[19] = line1[22] = ' ';
   line1[25] = '.';
   snprintf_err( line1 + 26, 8, "%06d", microdays);
   line1[32] = ' ';
   put_mpc_code_from_dss( line1 + 68, obs->xmitter);
   memcpy( line1 + 72, "JPLRS", 5);
   put_mpc_code_from_dss( line1 + 77, obs->receiver);
   strcpy( line2, line1);
   line2[14] = 'r';
   snprintf_err( line1 + 62, 7, "%5d", atoi( obs->freq_mhz));
   line1[67] = ' ';
   tptr = strchr( obs->freq_mhz, '.');
   if( tptr)      /* store a fractional part of freq in mhz */
      {
      int loc2 = 62;

      tptr++;
      if( *tptr)
         line1[67] = *tptr++;
      assert( strlen( tptr) < 6);
      while( *tptr)
         line2[loc2++] = *tptr++;
      }
   if( !obs->is_range)
      {
      line1[47] = (*measurement == '-' ? '-' : '+');
      if( *measurement == '-')
         measurement++;
      }
   put_with_implicit_decimal( line1 + dest_column, measurement);
   put_with_implicit_decimal( line2 + dest_column, obs->sigma);
            /* I don't think it's technically necessary to zero-pad the
               sigma.  But I've always _seen_ it zero-padded,  and there's
               a chance someone reads it as a float and divides by 1000.
               So best to make sure it's zero-padded.  */
   dest_column += 2;
   while( line2[dest_column] == ' ')
      line2[dest_column--] = '0';
            /* If the 'bounce point' is P = peak power,  I'm
               setting this byte to 'S' for a surface return result.
               I'm not convinced that this is correct.   */
   line2[32] = (obs->bounce_point == 'C' ? 'C' : 'S');
}

int main( const int argc, const char **argv)
{
   const char *ifilename = "radar.json";
   int i;
   FILE *ifile;
   bool show_comments = true;

   for( i = 1; i < argc; i++)
      if( argv[i][0] == '-')
         switch( argv[i][1])
            {
            case 'n':
               show_unknown_names = true;
               break;
            case 'c':
               show_comments = false;
               break;
            default:
               fprintf( stderr, "'%s' unrecognized option\n", argv[i]);
               return( -1);
            }
      else
         ifilename = argv[i];

   ifile = fopen( ifilename, "rb");
   if( !ifile)
      fprintf( stderr, "'%s' not opened", ifilename);
   else
      {
      int len;
      char *buff;
      time_t t0 = time( NULL);

      fseek( ifile, 0L, SEEK_END);
      len = (int)ftell( ifile);
      fseek( ifile, 0L, SEEK_SET);
      buff = (char *)malloc( len + 1);
      buff[len] = '\0';
      i = fread( buff, 1, len, ifile);
      assert( i == len);
      fclose( ifile);
      i = 0;
      while( buff[i] && memcmp( buff + i, "\"data\":", 7))
         i++;
      printf( "COM 'radar' converter run at %.24s UTC\n",
                               asctime( gmtime( &t0)));
      printf( "COM 'radar' version 2025 Jan 02;  see\n"
              "COM https://github.com/Bill-Gray/miscell/blob/master/radar.c\n"
              "COM for relevant code\n");
      output_index( buff + i);
      for( ; i < len; i++)
         if( buff[i - 1] == '[' && buff[i] == '"')
            {
            radar_obs_t obs;
            char line1[90], line2[90];

            get_radar_obs( buff + i, &obs);
            if( show_comments)
               put_radar_comment( &obs);
            put_radar_obs( line1, line2, &obs);
            printf( "%s\n%s\n", line1, line2);
            }
      printf( "COM Final modification %s\n", last_modified);
      printf( "COM Final observation %s\n", last_observed);
      }
   return( 0);
}
