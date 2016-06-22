#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Code to read in NOFS planet/satellite astrometry,  of the sort found
at http://www.nofs.navy.mil/data/plansat.html ,  and output the data in
the MPC's 80-column punched-card format.  */

static void fix_name( char *buff)
{
   const char *xform[] = {
               "Phoebe", "S009S",
               "Iapetu", "S008S",
               "Hyperi", "S007S",
               "Titan ", "S006S",
               "Rhea  ", "S005S",
               "Dione ", "S004S",
               "Tethys", "S003S",
               "Encela", "S002S",
               "Mimas ", "S001S",
               "Ariel ", "U001S",
               "Umbrie", "U002S",
               "Titani", "U003S",
               "Oberon", "U004S",
               "Mirand", "U005S",
               "Io    ", "J001S",
               "Europa", "J002S",
               "Ganyme", "J003S",
               "Callis", "J004S",
               "Himali", "J006S",
               "Elara ", "J007S",
               "Pasiph", "J008S",
               "Lysith", "J009S",
               "Triton", "N001S",
               "Nereid", "N002S",
               NULL };
   int i;

   for( i = 0; xform[i]; i += 2)
      if( !memcmp( buff + 5, xform[i], 6))
         {
         memcpy( buff, xform[i + 1], 5);
         memset( buff + 5, ' ', 6);
         }
}

int main( const int argc, const char **argv)
{
   FILE *ifile;
   char buff[200];

   if( argc < 2)
      {
      printf( "Need the name of a file containing NOFS astrometry on the command line\n");
      return( -1);
      }
   ifile = fopen( argv[1], "rb");
   while( fgets( buff, sizeof( buff), ifile))
      if( strlen( buff) > 100)
         {
         int month = 0, i;
         char obuff[90];

         for( i = 0; i < 12; i++)
            if( !memcmp( buff + 11,
                        "JanFebMarAprMayJunJulAugSepOctNovDec" + i * 3, 3))
               month = i + 1;
         if( month)
            {
            static const int no_blanks[7] = { 23, 32, 35, 38, 45, 48, 51 };

            memset( obuff, 0, 90);
            for( i = 0; i < 6 && buff[i + 99] > ' '; i++)
               obuff[i + 5] = buff[i + 99];
            obuff[14] = 'C';
            memcpy( obuff + 15, buff + 6, 4);       /* year */
            obuff[20] = (char)( '0' + month / 10);
            obuff[21] = (char)( '0' + month % 10);
            sprintf( obuff + 24, "%.6f",
                     atof( buff + 19) / 24. + atof( buff + 22) / 1440.
                   + atof( buff + 25) / 86400.);
            obuff[23] = buff[15];   /* day (10s) */
            obuff[24] = buff[16];   /* day (units) */
            memcpy( obuff + 32, buff + 31, 12);       /* RA */
            memcpy( obuff + 44, buff + 46, 12);       /* dec */
            if( obuff[44] == ' ')
               obuff[44] = '+';           /* correct dec sign */
            if( obuff[45] == '-')
               {
               obuff[44] = '-';           /* correct dec sign */
               obuff[45] = '0';
               }
            memcpy( obuff + 77, buff + 74, 3);       /* MPC code */
            for( i = 0; i < 80; i++)
               if( !obuff[i])
                  obuff[i] = ' ';
            for( i = 0; i < 7; i++)
               if( obuff[no_blanks[i]] == ' ')
                  obuff[no_blanks[i]] = '0';
            fix_name( obuff);
            printf( "%s\n", obuff);
            }
         }
   fclose( ifile);
   return( 0);
}
