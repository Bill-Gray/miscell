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
#include <assert.h>
#include "mpc_func.h"

/* Getting radar astrometry in a timely manner can be problematic.  It appears
almost immediately at

http://ssd.jpl.nasa.gov/?grp=all&fmt=html&radar=

    but there is usually a significant delay before it shows up on AstDyS or
NEODyS,  and it doesn't necessarily make it to MPC at all.  This code can read
in the file at the above URL and spit the data back out in the MPC's 80-column
punched-card format (two lines per observation).  Example output lines :


101955 Bennu (1999 RQ36)          1999-09-21 09:00:00     135959.00   5.000 Hz COM  8560  -14  -14
101955 Bennu (1999 RQ36)          1999-09-21 10:20:00   15418454.00  10.000 us COM  8560  -14  -14
101955 Bennu (1999 RQ36)          1999-09-23 09:30:00   14820631.00   5.000 us COM  8560  -14  -14

   And example output lines :

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
*/


int create_mpc_packed_desig( char *packed_desig, const char *obj_name);

static void put_mpc_code_from_dss( char *mpc_code, const int dss_desig)
{
   const char *code;

   switch( dss_desig)
      {
      case -73:
         code = "273";
         break;            /* used only for 367943 Duende (2012 DA14) */
      case -25:
         code = "257";     /* Goldstone, DSS 25 */
         break;            /* used only as receiver for 2006 RH120 */
      case -14:
         code = "253";     /* Goldstone, DSS 14 */
         break;
      case -1:
         code = "251";     /* Arecibo */
         break;
      case -13:            /* Goldstone,  DSS 13 */
         code = "252";
         break;
      case -38:
         code = "255";     /* used only as receiver for 6489 Golevka (1991 JX) */
         break;
      case -9:
         code = "256";     /* used for a few 2007 TU24 and one 2001 EC16 obs */
         break;
      case -2:
         code = "254";
         break;            /* used only for two 1566 Icarus (1949 MA) obs */
      default:
         code = "?!!";
         break;
      }
   memcpy( mpc_code, code, 3);
}

static char *remove_html( char *buff)
{
   char *tptr;

   while( (tptr = strchr( buff, '<')) != NULL)
      {
      char *endptr = strchr( tptr, '>');

      assert( endptr);
      memmove( tptr, endptr + 1, strlen( endptr));
      }
   tptr = buff + strlen( buff);
   while( tptr > buff && tptr[-1] <= ' ')
      tptr--;
   *tptr = '\0';
   tptr = buff;
   while( *tptr == ' ')
      tptr++;
   memmove( buff, tptr, strlen( tptr) + 1);
   return( buff);
}

typedef struct
   {
   char desig[20], time[20], time_modified[20];
   char measurement[20], sigma[20];
   int freq_mhz, receiver, xmitter;
   bool is_range;
   char bounce_point;      /* P = peak power, C = center of mass */
   int reference;
   char *observers, *notes;
   } radar_obs_t;

static void substitute_name( char *oname, const char *iname)
{
   const char *subs[] = {
            "LB", "L. Benner",
            "Benner", "L. Benner",
            "BennerA.M", "L. Benner",
            "MB", "M. Brozovic",
            "Brozovic", "M. Brozovic",
            "MWB", "M. Busch",
            "Busch", "M. Busch",
            "Campbell", "D. B. Campbell",
            "Chandler", "J. F. Chandler",
            "LF", "L. Fernanda",
            "LFZ", "L. Fernanda",
            "LFZM", "L. Fernanda",
            "Zambrano", "L. Fernanda",
            "Giorgini", "J. Giorgini",
            "GIORGI", "J. Giorgini",
            "JDG", "J. Giorgini",
            "Goldstein", "R. M. Goldstein",
            "Greenberg", "A. H. Greenberg",
            "Harmon", "J. K. Harmon",
            "Harris", "A. W. Harris",
            "DH", "D. Hickson",
            "Hine", "A. A. Hine",
            "Howell", "E. Howell",
            "HOWEL", "E. Howell",
            "HOWELLNOLANSPRINGM", "E. Howell, M. C. Nolan, ?. Springmann",
            "EH", "E. Howell",
            "Kamoun", "P. G. Kamoun",
            "Lieske", "J. H. Lieske",
            "SM", "S. Marshall",
            "SN", "S. P. Naidu",
            "Naidu", "S. P. Naidu",
            "Nolan", "M. C. Nolan",
            "Magri", "C. Magri",
            "Margot", "J. L. Margot",
            "Marsden", "B. G. Marsden",
            "Marshall", "S. Marshall",
            "SM", "S. Marshall",
            "SPN", "S. P. Naidu",
            "MCN", "M. C. Nolan",
            "MN", "M. C. Nolan",
            "M.N.", "M. C. Nolan",
            "Ostro", "S. J. Ostro",
            "Ostro.s", "S. J. Ostro",
            "Pettengill", "G. H. Pettengill",
            "PT", "P. Taylor",
            "P.T.", "P. Taylor",
            "PAT", "P. Taylor",
            "Rosema", "K. D. Rosema",
            "Shapiro", "I. I. Shapiro",
            "Shepard", "M. Shepard",
            "Taylor", "P. Taylor",
            "AV", "A. Virkki",
            "FV", "F. Venditti",
            "Werner", "C. L. Werner",
            "Young", "J. W. Young",
            "Zaitsev", "A. Zaitsev",
            NULL };
   size_t i;

   for( i = 0; subs[i]; i += 2)
      if( !strcasecmp( iname, subs[i]))
         {
         strcpy( oname, subs[i + 1]);
         return;
         }
   strcpy( oname, iname);
   fprintf( stderr, "!?%s\n", iname);
}

static void fix_observers( char *buff)
{
   char obuff[300], *tptr, *start = buff;
   bool done = false;
   size_t i, j;
   static const char *subs[] = {
            "Benner,L.A.M.", "Benner",
            "Benner, L.A.M.", "Benner",
            "Benner,L. A. M.", "Benner",
            "Benner, L. A. M.", "Benner",
            "Benner, L.", "Benner",
            "Benner,L.", "Benner",
            "Campbell,D.B.", "Campbell",
            "Campbell,D.B", "Campbell",
            "Chandler,J.F", "Chandler",
            "GOLDSTEIN,R.M", "Goldstein",
            "Greenberg,A.H.", "Greenberg",
            "Harris,A.W", "Harris",
            "Harmon, J.K.", "Harmon",
            "Harmon,J.K", "Harmon",
            "Hine,A.A", "Hine",
            "Kamoun,P.G.", "Kamoun",
            "LIESKE,J.H", "Lieske",
            "Margot, J. L.", "Margot",
            "Margot,J.L.", "Margot",
            "MAGRIHOWELL", "Magri,Howell",
            "MAGRI,C.", "Magri",
            "Marsden,B.G.", "Marsden",
            "Marshall, S.", "Marshall",
            "Nolan,M.C.", "Nolan",
            "Nolan,M", "Nolan",
            "Nolan.", "Nolan",
            "NolanHowell", "Nolan, Howell",
            "Ostro,S.J.", "Ostro",
            "Ostro,S.J", "Ostro",
            "Ostro,S.", "Ostro",
            "Ostro, S.", "Ostro",
            "Ostro,S", "Ostro",
            "PETTENGILL,G.H.", "Pettengill",
            "PETTENGILL,G.H", "Pettengill",
            "Rosema,K.D", "Rosema",
            "SHAPIRO,I.I.", "Shapiro",
            "SHAPIRO,I.I", "Shapiro",
            "SHAPIRO,I", "Shapiro",
            "Shepard, M.", "Shepard",
            "TAYLOR,P.", "Taylor",
            "Werner,C.L", "Werner",
            "Young,J.W", "Young",
            "ERV ", "ERV,",
            "LFZM ", "LFZM,",
            "PT ", "PT,",
            "Zaitsev,A.", "Zaitsev",
            NULL };

   for( i = 0; subs[i]; i += 2)
      if( (tptr = strcasestr( buff, subs[i])) != NULL)
         {
         strcpy( obuff, subs[i + 1]);
         strcat( obuff, tptr + strlen( subs[i]));
         strcpy( tptr, obuff);
         }

   if( isupper( buff[0]) && isupper( buff[1]) && buff[2])
      {        /* cases such as 'AM DH .. ..' */
      i = 2;
      if( isupper( buff[i]))     /* maybe ERV,  etc. */
         i++;
      if( isupper( buff[i]))     /* Maybe LFZM, etc. */
         i++;
      while( buff[i] == ' ' && isupper( buff[i + 1]) && isupper( buff[i + 2]))
         {
         buff[i] = ',';
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
      substitute_name( obuff + strlen( obuff), buff);
      buff += i;
      if( !done)
         {
         strcat( obuff, ", ");
         buff++;
         }
      }
   strcpy( start, obuff);
}

static int get_radar_obs( FILE *ifile, radar_obs_t *obs)
{
   char buff[300];
   int rval = -1;

   memset( obs, 0, sizeof( radar_obs_t));
   while( rval && fgets( buff, sizeof( buff), ifile))
      if( strstr( buff, "a href=\"sbdb.cgi?"))
         {
         char *tptr;

         rval = 0;
         remove_html( buff);
         if( *buff == '(')    /* preliminary desig */
            {
            buff[strlen( buff) - 1] = '\0';     /* remove trailing right paren */
            memmove( buff, buff + 1, strlen( buff));
            }
         else if( buff[1] == '/')      /* comet desig */
            {
            tptr = strchr( buff, '(');
            assert( tptr);
            tptr[-1] = '\0';
            }
         else if( (tptr = strstr( buff, "P/")) != NULL) /* numbered comet */
            tptr[1] = '\0';
         else                  /* numbered minor planet */
            sprintf( buff, "(%d)", atoi( buff));
         if( create_mpc_packed_desig( obs->desig, buff))
            {
            fprintf( stderr, "Couldn't pack '%s'\n", buff);
            rval = -1;
            }
         if( !rval && fgets( buff, sizeof( buff), ifile))
            strcpy( obs->time, remove_html( buff));
         else
            rval = -1;
         if( !rval && fgets( buff, sizeof( buff), ifile))
            strcpy( obs->measurement, remove_html( buff));
         else
            rval = -1;
         if( !rval && fgets( buff, sizeof( buff), ifile))
            strcpy( obs->sigma, remove_html( buff));
         else
            rval = -1;
         if( !rval && fgets( buff, sizeof( buff), ifile))
            {
            remove_html( buff);
            if( !strcmp( buff, "Hz"))
               obs->is_range = false;
            else if( !strcmp( buff, "us"))
               obs->is_range = true;
            else
               fprintf( stderr, "Bad units '%s'\n", buff);
            }
         else
            rval = -1;
         if( !rval && fgets( buff, sizeof( buff), ifile))
            obs->freq_mhz = atoi( remove_html( buff));
         else
            rval = -1;
         if( !rval && fgets( buff, sizeof( buff), ifile))
            obs->receiver = atoi( remove_html( buff));
         else
            rval = -1;
         if( !rval && fgets( buff, sizeof( buff), ifile))
            obs->xmitter  = atoi( remove_html( buff));
         else
            rval = -1;
         if( !rval && fgets( buff, sizeof( buff), ifile))
            obs->bounce_point = *(remove_html( buff));
         else
            rval = -1;
         if( *buff != 'C' && *buff != 'P')
            printf( "? Error : %d, '%s'\n", rval, buff);
         assert( *buff == 'C' || *buff == 'P');
         if( !rval && fgets( buff, sizeof( buff), ifile))
            obs->reference = atoi( remove_html( buff));
         else
            rval = -1;
         if( !rval && fgets( buff, sizeof( buff), ifile))
            {
            remove_html( buff);
            fix_observers( buff);
            obs->observers = (char *)malloc( strlen( buff) + 1);
            strcpy( obs->observers, buff);
            }
         else
            rval = -1;
         if( !rval && fgets( buff, sizeof( buff), ifile))
            {
            remove_html( buff);
            obs->notes = (char *)malloc( strlen( buff) + 1);
            strcpy( obs->notes, buff);
            }
         else
            rval = -1;
         if( !rval && fgets( buff, sizeof( buff), ifile))
            strcpy( obs->time_modified, remove_html( buff));
         else
            rval = -1;
         }
   return( rval);
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
      memcpy( line, decimal + 1, strlen( decimal + 1));
   line -= len;
   while( *line == '0' && line[1] != ' ')
      *line++ = ' ';       /* remove leading zeroes */
}

static void put_radar_comment( const radar_obs_t *obs)
{
   const char *notes = obs->notes;
   char mpc_code[4];

   put_mpc_code_from_dss( mpc_code, obs->receiver);
   printf( "\nCOD %.3s\n", mpc_code);
   printf( "OBS %s\n", obs->observers);
   printf( "COM Last modified %s\n", obs->time_modified);
   while( *notes >= ' ')
      {
      size_t len;
      const size_t max_len = 70;

      while( *notes == ' ')
         notes++;
      len = strlen( notes);
      if( len <= max_len)           /* finish up line */
         {
         printf( "COM %s\n", notes);
         return;
         }
      else
         {
         len = max_len;
         while( notes[len] != ' ')
            len--;
         printf( "COM %.*s\n", (int)len, notes);
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

   memset( line1, ' ', 80);
   line1[80] = '\0';
   memcpy( line1, obs->desig, 12);
   line1[14] = 'R';
   memcpy( line1 + 15, obs->time, 10);
   line1[19] = line1[22] = ' ';
   line1[25] = '.';
   sprintf( line1 + 26, "%06d", microdays);
   line1[32] = ' ';
   put_mpc_code_from_dss( line1 + 68, obs->xmitter);
   memcpy( line1 + 72, "JPLRS", 5);
   put_mpc_code_from_dss( line1 + 77, obs->receiver);
   strcpy( line2, line1);
   line2[14] = 'r';
   sprintf( line1 + 62, "%5d", obs->freq_mhz);
   line1[67] = ' ';
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
   const char *ifilename = "radar.html";
   int i;
   FILE *ifile;
   bool show_comments = true;

   for( i = 1; i < argc; i++)
      if( !strcmp( argv[i], "-c"))
         show_comments = false;
      else
         ifilename = argv[i];

   ifile = fopen( ifilename, "rb");
   if( !ifile)
      fprintf( stderr, "'%s' not opened", ifilename);
   else
      {
      radar_obs_t obs;

      while( !get_radar_obs( ifile, &obs))
         {
         char line1[90], line2[90];

         if( show_comments)
            put_radar_comment( &obs);
         put_radar_obs( line1, line2, &obs);
         printf( "%s\n%s\n", line1, line2);
         }
      fclose( ifile);
      }
   return( 0);
}
