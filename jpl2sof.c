#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "watdefs.h"
#include "jpl_xref.h"
#include "lunar.h"
#include "date.h"

/* Code to read in ephemerides of JPL orbital elements and output them
in the .sof format.  For use with Find_Orb : you can generate such
elements for,  say,  STEREO-A = (C49),  run

./jpl2sof elem.txt C49

   and get a .sof file.  Find_Orb can then use the elements to determine
where STEREO-A was... which means it can generate ephems from that
location.  See https://www.projectpluto.com/orb_form.htm for an
explanation of the .sof format.

   Note that this isn't built in a default 'make'.  You need to run
'make extras' to build it.

   Getting the necessary ephems from JPL is fairly simple,  but there are
several steps needed,  and you have to get them all right or your ephems
will be bogus (and this code isn't great at error checking at present) :

   -- Go to the Horizons page (https://ssd.jpl.nasa.gov/horizons.cgi).
   -- Select 'Ephemeris Type' Osculating Orbital Elements.
   -- Select the 'target body' to be your desired object.
   -- Select the 'center' to be either the sun (@10) or earth (@399).  I
      do plan to add some other possible orbital element centers.
   -- Select a 'time span' to cover the desired time span.
   -- Click 'Generate Ephemeris',  then 'Download Results' to save the
      output to a text file.                               */

static int look_up_name( char *obuff, const size_t buffsize, const int idx)
{
   size_t i;

   for( i = 0; i < sizeof( jpl_xrefs) / sizeof( jpl_xrefs[0]); i++)
      if( idx == jpl_xrefs[i].jpl_desig)
         {
         if( obuff)
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

int main( const int argc, const char **argv)
{
   FILE *ifile = fopen( argv[1], "rb");
   const char *header =
      "Name |C |Te      .  |Tp              |q                |i  .      |Om .      |om .      |e        ^";
   char buff[200], buff2[200];
   int center = -1;
   char object_name[4];

   assert( argc > 1);
   printf( "%s\n", header);
   *object_name = '\0';
   while( fgets( buff, sizeof( buff), ifile))
      if( !memcmp( buff + 17, " = A.D. ", 8))
         {
         char obuff[120];

         assert( 3 == strlen( object_name));
         memset( obuff, ' ', sizeof( obuff));
         memcpy( obuff, object_name, strlen( object_name));
         full_ctime( obuff + 9, atof( buff), FULL_CTIME_YMD | FULL_CTIME_CENTIDAYS
                        | FULL_CTIME_NO_SPACES | FULL_CTIME_MONTHS_AS_DIGITS | FULL_CTIME_LEADING_ZEROES);
         if( fgets( buff, sizeof( buff), ifile) && fgets( buff2, sizeof( buff2), ifile))
            {
            double q = atof( buff + 31);

            if( q > 200.)        /* cvt km to AU */
                q /= AU_IN_KM;
            full_ctime( obuff + 21, atof( buff2 + 57), FULL_CTIME_YMD | FULL_CTIME_LEADING_ZEROES
                        | FULL_CTIME_FORMAT_DAY | FULL_CTIME_7_PLACES
                        | FULL_CTIME_NO_SPACES | FULL_CTIME_MONTHS_AS_DIGITS);
            obuff[20] = ' ';
            if( center)
               obuff[7] = '0' + center;
            sprintf( obuff + 37, " %17.14f %10.6f %10.6f %10.6f %10.8f",
                              q,
                              atof( buff + 57),                /* incl */
                              atof( buff2 + 5),                /* Omega */
                              atof( buff2 + 31),                /* omega */
                              atof( buff + 5));                 /* incl */
            printf( "%s\n", obuff);
            }
         }
      else if( !memcmp( buff, " Revised:", 9))
         {
         const int idx = look_up_name( NULL, 0, atoi( buff + 71));

         strcpy( object_name, jpl_xrefs[idx].mpc_code);
         }
      else if( !memcmp( buff, "Center body name: ", 18))
         {
         char *tptr = strchr( buff, '(');

         assert( tptr);
         switch( atoi( tptr + 1))
            {
            case 10:
               center = 0;
               break;
            case 399:
               center = 3;
               break;
            default:
               fprintf( stderr, "Unrecognized body center\n%s", buff);
               exit( -1);
            }
         }
   fclose( ifile);
   return( 0);
}
