#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>

/* Getting radar astrometry in a timely manner can be problematic.  It appears
almost immediately at

http://ssd.jpl.nasa.gov/?grp=ast&fmt=text&radar=

    but there is usually a significant delay before it shows up on AstDyS or
NEODyS,  and it doesn't necessarily make it to MPC at all.  This code can read
in the file at the above URL and spit the data back out in the MPC's 80-column
punched-card format (two lines per observation).  Example input lines :


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

/* create_mpc_packed_desig( ) takes a "normal" name for a comet/asteroid,
such as P/1999 Q1a or 2005 FF351,  and turns it into the 12-byte packed
format used in MPC reports and element files.  Documentation of this format
is given on the MPC Web site.  A 'test main' at the end of this file
shows the usage of this function.
   This should handle all "normal" asteroid and comet designations.
It doesn't handle natural satellites.  */

/* "Mutant hex" uses the usual hex digits 0123456789ABCDEF for numbers
0 to 15,  followed by G...Z for 16...35 and a...z for 36...61.  MPC stores
epochs and certain other numbers using this scheme to save space.  */

int create_mpc_packed_desig( char *packed_desig, const char *obj_name);

static inline char mutant_hex( const int number)
{
   int rval;

   if( number < 10)
      rval = '0';
   else if( number < 36)
      rval = 'A' - 10;
   else
      rval = 'a' - 36;
   return( (char)( rval + number));
}

int create_mpc_packed_desig( char *packed_desig, const char *obj_name)
{
   int i, j, rval = 0;
   char buff[20], comet_desig = 0;

               /* Check for comet-style desigs such as 'P/1995 O1' */
               /* and such.  Leading character can be P, C, X, D, or A. */
               /* Or 'S' for natural satellites.  */
   if( strchr( "PCXDAS", *obj_name) && obj_name[1] == '/')
      {
      comet_desig = *obj_name;
      obj_name += 2;
      }

               /* Create a version of the name with all spaces removed: */
   for( i = j = 0; obj_name[i] && j < 19; i++)
      if( obj_name[i] != ' ')
         buff[j++] = obj_name[i];
   buff[j] = '\0';

   for( i = 0; isdigit( buff[i]); i++)
      ;
               /* If the name starts with four digits followed by an */
               /* uppercase letter,  it's a provisional designation: */
   if( buff[i] && i == 4 && isupper( buff[4]))
      {
      const int year = atoi( buff);
      int sub_designator;

      memset( packed_desig, ' ', 12);
      for( i = 0; i < 4; i++)
         {
         const char *surveys[4] = { "P-L", "T-1", "T-2", "T-3" };

         if( !strcmp( buff + 4, surveys[i]))
            {
            const char *surveys_packed[4] = {
                     "PLS", "T1S", "T2S", "T3S" };

            memcpy( packed_desig + 8, buff, 4);
            memcpy( packed_desig + 5, surveys_packed[i], 3);
            return( rval);
            }
         }

      sprintf( packed_desig + 5, "%c%02d",
                  'A' - 10 + year / 100, year % 100);
      packed_desig[6] = buff[2];    /* decade */
      packed_desig[7] = buff[3];    /* year */

      packed_desig[8] = (char)toupper( buff[4]);    /* prelim desigs are */
      i = 5;                                        /* _very_ scrambled  */
      if( isupper( buff[i]))                        /* when packed:      */
         {
         packed_desig[11] = buff[i];
         i++;
         }
      else
         packed_desig[11] = '0';

      sub_designator = atoi( buff + i);
      packed_desig[10] = (char)( '0' + sub_designator % 10);
      if( sub_designator < 100)
         packed_desig[9] = (char)( '0' + sub_designator / 10);
      else if( sub_designator < 360)
         packed_desig[9] = (char)( 'A' + sub_designator / 10 - 10);
      else
         packed_desig[9] = (char)( 'a' + sub_designator / 10 - 36);
      if( comet_desig)
         {
         packed_desig[4] = comet_desig;
         while( isdigit( buff[i]))
            i++;
         if( buff[i] >= 'a' && buff[i] <= 'z')
            packed_desig[11] = buff[i];
         }
      }
   else if( !buff[i])       /* simple numbered asteroid or comet */
      {
      const int number = atoi( buff);

      if( comet_desig)
         sprintf( packed_desig, "%04d%c       ", number, comet_desig);
      else
         sprintf( packed_desig, "%c%04d       ", mutant_hex( number / 10000),
               number % 10000);
      }
   else                 /* strange ID that isn't decipherable.  For this, */
      {                 /* we just copy the first twelve non-space bytes, */
      while( j < 12)                              /* padding with spaces. */
         buff[j++] = ' ';
      buff[12] = '\0';
      strcpy( packed_desig, buff);
      rval = -1;
      }
   return( rval);
}

#ifdef OLD_TESTING_CODE
int main( const int argc, const char **argv)
{
   if( argc >= 2)
      {
      char obuff[80];
      const int rval = create_mpc_packed_desig( obuff, argv[1]);

      obuff[12] = '\0';
      printf( "'%s'\n", obuff);
      if( rval)
         printf( "Error code %d\n", rval);
      }
   return( 0);
}
#endif


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

static int reformat_jpl_radar_data_to_mpc( char *line1, char *line2, const char *ibuff)
{
   unsigned i;
   const double hrs_per_day = 24.;
   const double mins_per_day = hrs_per_day * 60.;
   const double secs_per_day = mins_per_day * 60.;
   const double fractional_day = (double)atoi( ibuff + 45) / hrs_per_day
                                + (double)atoi( ibuff + 48) / mins_per_day
                                + (double)atoi( ibuff + 51) / secs_per_day;
   const double quantity = atof( ibuff + 53);
   const double sigma = atof( ibuff + 67);
   unsigned offset;

   if( ibuff[5] == ' ')       /* provisional desig */
      {
      memcpy( line2, ibuff + 8, 10);
      i = 0;
      while( i < 10 && line2[i] != ')')
         i++;
      line2[i] = '\0';
      }
   else                       /* numbered object */
      {
      memcpy( line2, ibuff, 6);
      line2[6] = '\0';
      }
   if( create_mpc_packed_desig( line1, line2))
      return( -1);
   memset( line1 + 12, ' ', 68);
   line1[80] = '\0';
   line1[14] = 'R';
   memcpy( line1 + 15, ibuff + 34, 10);
   line1[19] = line1[22] = ' ';
   line1[25] = '.';
   sprintf( line1 + 26, "%06ld", (long)( fractional_day * 1e+6 + 0.5));
   memcpy( line1 + 72, "JPLRS", 5);
   put_mpc_code_from_dss( line1 + 68, atoi( ibuff + 90));
   put_mpc_code_from_dss( line1 + 77, atoi( ibuff + 94));
   for( i = 0; i < 80; i++)
      if( !line1[i])
         line1[i] = ' ';
   strcpy( line2, line1);
   memcpy( line1 + 63, ibuff + 84, 4);        /* frequency in MHz */
   line2[14] = 'r';
   if( !memcmp( ibuff + 76, "Hz", 2))
      {
      line1[47] = (quantity > 0. ? '+' : '-');
      offset = 48;
      }
   else if( !memcmp( ibuff + 76, "us", 2))
      offset = 33;
   else
      return( -2);
   sprintf( line1 + offset, "%12.0f", fabs( quantity) * 100.);
   sprintf( line2 + offset, "%13.0f", sigma * 1000.);
   line1[offset + 12] = line2[offset + 13] = ' ';
   return( 0);
}

int main( const int argc, const char **argv)
{
   if( argc >= 2)
      {
      FILE *ifile = fopen( argv[1], "rb");

      if( ifile)
         {
         char ibuff[110], line1[100], line2[100];

         while( fgets( ibuff, sizeof( ibuff), ifile))
            if( !reformat_jpl_radar_data_to_mpc( line1, line2, ibuff))
               printf( "%s\n%s\n", line1, line2);
         fclose( ifile);
         }
      else
         printf( "%s not opened\n", argv[1]);

      }
   return( 0);
}
