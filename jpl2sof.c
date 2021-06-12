#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "watdefs.h"
#include "date.h"

/* Code to read in ephemerides of JPL orbital elements and output them
in the .sof format.  For use with Find_Orb : you can generate such
elements for,  say,  STEREO-A = (C49),  run

./jpl2sof elem.txt C49

   and get a .sof file.  Find_Orb can then use the elements to determine
where STEREO-A was... which means it can generate ephems from that
location.  See https://www.projectpluto.com/orb_form.htm for an
explanation of the .sof format.

   Getting the necessary ephems from JPL is fairly simple,  but there are
several steps needed,  and you have to get them all right or your ephems
will be bogus (and this code isn't great at error checking at present) :

   -- Go to the Horizons page (https://ssd.jpl.nasa.gov/horizons.cgi).
   -- Select 'Ephemeris Type' ELEMENTS.
   -- Select the 'target body' to be your desired object.
   -- Select the 'center' to be either the sun (@10) or earth (@399).  I
      do plan to add some other possible orbital element centers.
   -- Select 'Display/Output' to be plain text.
   -- Select a 'time span' to cover the desired time span.
   -- Click 'Generate Ephemeris',  and save the output to a text file.
*/

int main( const int argc, const char **argv)
{
   FILE *ifile = fopen( argv[1], "rb");
   const char *header =
      "Name |C |Te      .  |Tp              |q                |i  .      |Om .      |om .      |e        ^";
   char buff[200], buff2[200];
   int center = -1;

   assert( argc > 1);
   printf( "%s\n", header);
   while( fgets( buff, sizeof( buff), ifile))
      if( !memcmp( buff + 17, " = A.D. ", 8))
         {
         char obuff[120];

         memset( obuff, ' ', sizeof( obuff));
         full_ctime( obuff + 9, atof( buff), FULL_CTIME_YMD | FULL_CTIME_CENTIDAYS
                        | FULL_CTIME_NO_SPACES | FULL_CTIME_MONTHS_AS_DIGITS | FULL_CTIME_LEADING_ZEROES);
         if( fgets( buff, sizeof( buff), ifile) && fgets( buff2, sizeof( buff2), ifile))
            {
            full_ctime( obuff + 21, atof( buff2 + 57), FULL_CTIME_YMD | FULL_CTIME_LEADING_ZEROES
                        | FULL_CTIME_FORMAT_DAY | FULL_CTIME_7_PLACES
                        | FULL_CTIME_NO_SPACES | FULL_CTIME_MONTHS_AS_DIGITS);
            obuff[20] = ' ';
            if( center)
               obuff[7] = '0' + center;
            sprintf( obuff + 37, " %17.14f %10.6f %10.6f %10.6f %10.8f",
                              atof( buff + 31),                /* q */
                              atof( buff + 57),                /* incl */
                              atof( buff2 + 5),                /* Omega */
                              atof( buff2 + 31),                /* omega */
                              atof( buff + 5));                 /* incl */
            printf( "%s\n", obuff);
            }
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
}
