#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>

/* The IOD (Interactive Orbit Determination) observation format is
described at

http://www.satobs.org/position/IODformat.html
               K130213:032353605     (CYYMMDD HH:MM:SS.sss)
*/

int main( const int argc, const char **argv)
{
   char buff[200];
   int i;
   bool use_extended_time_format = false;
   FILE *ifile = fopen( argv[1], "rb"), *ofile = NULL;
   const char *code_xlate[] = {
         "0433 GRR",    /* Greg Roberts */
         "1086 AOO",    /* Odessa Astronomical Observatory, Kryzhanovka */
         "1244 AMa",    /* Andriy Makeyev */
         "1775 Fet",    /* Kevin Fetter */
         "4171 CBa",    /* Cees Bassa */
         "4172 Alm",    /* ALMERE  52.3713 N 5.2580 E  -3 (meters?) ASL */
         "4541 Ran",    /* Alberto Rango */
         "4553 SOb",    /* Unknown sat observer: 53.3199N, 2.24377W, 86 m */
         "8049 ScT",    /* Roberts Creek 1 (Scott Tilley) */
         "8335 Tu2",    /* Tulsa-2 (Oklahoma) +35.8311  -96.1411 1083ft, 330m */
         "8336 Tu1",    /* Tulsa-1 (Oklahoma) +36.139208,-95.983429 660ft, 201m */
         NULL };


   if( !ifile)
      {
      printf( "Couldn't find '%s'\n", argv[1]);
      return( -1);
      }
   for( i = 2; i < argc; i++)
      if( argv[i][0] != '-')
         ofile = fopen( argv[i], "wb");
      else
         switch( argv[i][1])
            {
            case 'f':
               use_extended_time_format = true;
               break;
            default:
               printf( "Option '%s' ignored\n", argv[i]);
               break;
            }
   while( fgets( buff, sizeof( buff), ifile))
      if( strlen( buff) > 63 && buff[22] == ' ')
         {
         int i;

         for( i = 23; buff[i] >= '0' && buff[i] <= '9'; i++)
            ;
         if( i == 40)
            {
            char obuff[90];
            double day;
            memset( obuff, ' ', 90);
            memcpy( obuff + 2, buff + 6, 9);   /* Intl designator */
            if( obuff[2] >= '5')               /* 20th century satellite */
               memcpy( obuff, "19", 2);
            else                               /* 21th century satellite */
               memcpy( obuff, "20", 2);
            obuff[4] = '-';                    /* put dash in intl desig */
            obuff[14] = 'C';
            if( use_extended_time_format)
               {
               obuff[15] = (buff[24] == '9' ? 'J' : 'K');
               memcpy( obuff + 16, buff + 25, 6);     /* YYMMDD */
               obuff[22] = ':';
               memcpy( obuff + 23, buff + 31, 9);     /* HHMMSSsss */
               }
            else
               {
               memcpy( obuff + 15, buff + 23, 4);     /* year */
               memcpy( obuff + 20, buff + 27, 2);     /* month */
               for( i = 29; i < 40; i++)
                  buff[i] -= '0';
               day = (double)buff[29] * 10. + (double)buff[30]
                     + (double)((int)buff[31] * 10 + buff[32]) / 24.
                     + (double)((int)buff[33] * 10 + buff[34]) / 1440.
                     + (double)((int)buff[35] * 10 + buff[36]) / 86400.
                     + (double)((int)buff[37] * 10 + buff[38]) / 8640000.;
               sprintf( obuff + 23, "%09.6f", day);
               }
//          obuff[31] = ' ';
                     /* buff[44] contains an angle format code,  allowing */
                     /* angles to be in minutes or seconds or degrees...  */
            memcpy( obuff + 32, buff + 47, 2);     /* RA hours */
            memcpy( obuff + 35, buff + 49, 2);     /* RA min */
            memcpy( obuff + 38, buff + 51, 2);     /* RA sec or .01 RA min */
            if( buff[44] == '1')    /* it's RA seconds */
               {
               obuff[40] = '.';
               obuff[41] = buff[53];      /* .1 RA sec */
               }
            else           /* 2 or 3 = HHMMmmm */
               {
               obuff[37] = '.';
               obuff[40] = buff[53];
               }
            memcpy( obuff + 44, buff + 54, 3);     /* dec deg */
            if( buff[44] == '3')    /* dec is in DDdddd */
               {
               obuff[46] = '.';
               memcpy( obuff + 47, buff + 56, 4);
               }
            else
               {
               memcpy( obuff + 48, buff + 57, 2);     /* dec arcmin */
               if( buff[44] == '2')       /* dec is in DDMMmm */
                  {
                  obuff[50] = '.';
                  memcpy( obuff + 51, buff + 59, 2);
                  }
               else                       /* '1' = dec is in DDMMSS */
                  memcpy( obuff + 51, buff + 59, 2);     /* dec arcsec */
               }

            if( buff[66] == '+')       /* positive magnitude stored */
               {
               if( buff[67] != '0')    /* skip leading zero */
                  obuff[65] = buff[67];
               obuff[66] = buff[68];
               obuff[67] = '.';
               obuff[68] = buff[69];
               }
            strcpy( obuff + 77, "???");
            for( i = 0; code_xlate[i]; i++)
               if( !memcmp( buff + 16, code_xlate[i], 4))
                  strcpy( obuff + 77, code_xlate[i] + 5);
            printf( "%s\n", obuff);
            if( ofile)
               fprintf( ofile, "%s\n", obuff);
            }
         }
   return( 0);
}

