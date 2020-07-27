/* Code to search through CSS pointing data for fields between a given
pair of dates.  Examples :

./getpoint /data/seaman/nights/css_nightly.csv 2020-07-01 2020-07-07
./getpoint css7.csv 2018-12-25
./getpoint css7.csv 2019-05

   would search through the named files for fields from the first week
in July 2020,  or for all fields from Christmas 2018,  or all fields
from May 2019.  Lines that match will be dumped to stdout.

   This is used to aid Find_Orb's precovery search abilities.  When
updating the plate logs,  I can just run this program to download only
the 'new' data,  usually from the aforementioned css_nightly.csv.
If run on CSS's server,  I can create a smaller file requiring less
data transfer (bandwidth is a problem for me).

   Since this is just doing a string comparison,  the date string can
be cut short or extended as desired;  for example,

./getpoint css7.csv 2020-03-14T15:26:53.589 3

   would get you all fields after the specified time on 2020 Mar 14.
(Well,  at least until 3000 January 1... making the last parameter
'a' instead of '3' would solve even that problem.)    */

#include <stdio.h>
#include <string.h>
#include <assert.h>

int main( const int argc, const char **argv)
{
   FILE *ifile = fopen( argv[1], "rb");
   char buff[200];

   assert( ifile);
   assert( argc > 2);
   if( argc == 3)             /* only one date given */
      argv[3] = argv[2];
   while( fgets( buff, sizeof( buff), ifile))
      {
      char *tptr = strchr( buff, ',');

      if( tptr)
         tptr = strchr( tptr + 1, ',');
      assert( tptr);
      tptr++;
      if( strncmp( tptr, argv[2], strlen( argv[2])) >= 0 &&
          strncmp( tptr, argv[3], strlen( argv[3])) <= 0)
         printf( "%s", buff);
      }
   fclose( ifile);
   return( 0);
}
