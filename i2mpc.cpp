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
#include <stdlib.h>

/* The IOD (Interactive Orbit Determination) observation format is
described at

http://www.satobs.org/position/IODformat.html
               K130213:032353605     (CYYMMDD HH:MM:SS.sss)

   The '-f' flag causes the date/time to be output in MPC 'standard'
format,  to a precision of 0.000001 day = 86.4 milliseconds.  By
default,  it's output in the form show above,  to millisecond
precision.  But only Find_Orb will understand that format.

   Note that there are a _lot_ of satellite observer sites listed at

https://github.com/cbassa/sattools/blob/master/data/sites.txt

   (...update) That file was deleted in early 2020;  last version of it is at

https://github.com/cbassa/sattools/blob/1da489eb347683f1eb70bbd6575e8fcd289ca567/data/sites.txt

   and is appended at bottom of this source.   */

int main( const int argc, const char **argv)
{
   char buff[200];
   int i;
   bool use_extended_time_format = true;
   FILE *ifile = fopen( argv[1], "rb"), *ofile = NULL;
   const char *code_xlate[] = {
         "0433 GRR",    /* Greg Roberts */
         "1086 AOO",    /* Odessa Astronomical Observatory, Kryzhanovka */
         "1244 AMa",    /* Andriy Makeyev */
         "1753 VMe",    /* Vitaly Mechinsky  */
         "1775 Fet",    /* Kevin Fetter */
         "1860 SGu",    /* Sergey Guryanov, Siberia, N56.1016, E94.5536, Alt=170m */
         "4171 CBa",    /* Cees Bassa */
         "4172 Alm",    /* ALMERE  52.3713 N 5.2580 E  -3 (meters?) ASL */
         "4353 Lei",    /* Leiden 52.15412 N 4.49081 E  +0 ASL */
         "4355 Cro",    /* Cronesteyn 52.13878 N 4.49947 E -2 meters ASL */
         "4541 Ran",    /* Alberto Rango */
         "4553 SOb",    /* Unknown sat observer: 53.3199N, 2.24377W, 86 m */
         "7777 I79",    /* Brad Young remote = AstroCamp Nerpio */
         "7778 E12",    /* Siding Spring */
         "8049 ScT",    /* Roberts Creek 1 (Scott Tilley) */
         "8335 Tu2",    /* Tulsa-2 (Oklahoma) +35.8311  -96.1411 1083ft, 330m */
         "8336 Tu1",    /* Tulsa-1 (Oklahoma) +36.139208,-95.983429 660ft, 201m */
         "9903 Ch5",    /* station reporting Chang'e 5 obs, 2020 Nov 24 */
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
               use_extended_time_format = false;
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

/* Last version of 'sites.txt';  see comments near top of file

# No ID   Latitude Longitude   Elev   Observer
1111 RL   38.9478 -104.5614   2073    Ron Lee
4171 CB   52.8344    6.3785     10    Cees Bassa
4172 LB   52.3713    5.2580     -3    Leo Barhorst
4553 CB   53.3210   -2.2330     86    Cees Bassa
0001 MM   30.3340  -97.7610    160    Mike McCants
0002 MM   30.3138  -97.8661    280    Mike McCants
0030 IA   47.4857   18.9727    400    Ivan Artner
0031 IA   47.5612   18.7366    149    Ivan Artner
0070 BC   53.2233   -0.6003     30    Bob Christy
0100 SG   59.4627   17.9138      0    Sven Grahn
0110 LK   32.5408  -96.8906    200    Lyn Kennedy
0433 GR  -33.9406   18.5129     10    Greg Roberts
0434 IR  -26.1030   27.9288   1646    Ian Roberts
0435 GR  -33.9369   18.5101     25    Greg Roberts
0710 LS   52.3261   10.6756     85    Lutz Schindler
0899 FM  -34.8961  -56.1227     30    Fernando Mederos
1056 MK   57.0122   23.9833      4    Martins Keruss
1086 NK   46.4778   30.7556     56    Nikolay Koshkin
1234 TS   60.1746   24.7882     24    Tomi Simola
1244 AM   44.3932   33.9701     69    Andriy Makeyev
1747 DD   45.7275  -72.3526    191    Daniel Deak
1775 KF   44.6062  -75.6910    200    Kevin Fetter
2018 PW   51.0945   -1.1188    124    Peter Wakelin
2115 MW   51.3286   -0.7950     75    Mike Waterman
2414 DH   50.7468   -1.8800     34    David Hopkins
2420 RE   55.9486   -3.1386     40    Russell Eberst
2563 PN   51.0524    2.4043     10    Pierre Nierinck
2675 DB   52.1358   -2.3264     70    David Brierley
2701 TM   43.6876  -79.3924    230    Ted Molczan
2751 BM   51.3440   -1.9849    125    Bruce MacDonald
2756 AK   56.0907   -3.1623     25    Andy Kirkham
3333 RS   53.4506   10.2205      0    Roland Proesch
4353 ML   52.1541    4.4908      0    Marco Langbroek
4354 ML   52.1168    4.5602     -2    Marco Langbroek
4355 ML   52.1388    4.4994     -2    Marco Langbroek
4541 AR   41.9639   12.4531     80    Alberto Rango
4542 AR   41.9683   12.4545     80    Alberto Rango
4641 AR   41.1060   16.9010     70    Alberto Rango
5555 SG   56.1019   94.5533    154    Sergey Guryanov
5918 BG   59.2985   18.1045     40    Bjorn Gimle
5919 BG   59.2615   18.6206     30    Bjorn Gimle
6226 SC   28.4861  -97.8194    110    Scott Campbell
7777 BY   38.1656   -2.3267   1608    Brad Young remote
7778 BY  -31.2733  149.0644   1122    Brad Young SSO
7779 BY   32.9204 -105.5283   2225    Brad Young NM
0797 PP   36.9708   22.1448      6    Pierros Papadeas
8048 ST   49.4175 -123.6420      1.   Scott Tilley
8049 ST   49.4348 -123.6685     40.   Scott Tilley
8305 PG   26.2431  -98.2163     30    Paul Gabriel
8335 BY   36.1397  -95.9838    205    Brad Young
8336 BY   36.1397  -95.9838    205    Brad Young
8438 SS   38.8108 -119.7750   1545    Sierra Stars
8536 TL   36.8479  -76.5010      4    Tim Luton
8539 SN   39.4707  -79.3388    839    Steve Newcomb
8597 TB  -34.9638  138.6333    100    Tony Beresford
8600 PC  -32.9770  151.6477     18    Paul Camilleri
8831 CL   47.7717 -121.9040    220    Chris Lewicki
0699 PC  -14.4733  132.2369    108    Paul Camilleri
8730 EC   30.3086  -97.7279    150    Ed Cannon
8739 DB   37.1133 -121.7028    282    Derek Breit
9461 KO   47.9175   19.89365   938    Konkoly Obs
9633 JN   33.0206  -96.9982    153    Jim Nix
9730 MM   30.3150  -97.8660    280    Mike McCants
6242 JM   42.9453   -2.8284    623    Jon Mikel
6241 JM   42.9565   -2.8203    619    Jon Mikel
4160 BD   51.2793    5.4768     35    Bram Dorreman
9900 AM   52.36667   4.900       0    Amsterdam
9901 CT  -30.1696  -70.8065   2000    CTIO
9910 SP   37.5011   -2.4083   1200    Telescope Live Spain
9911 AU  -34.8641  148.9763    350    Telescope Live Australia
9997 CL   47.7717 -121.9040    220    Chris Lewicki
9998 DT   52.8119    6.3961     10    Dwingeloo telescoop
9999 GR   47.348     5.5151    100    Graves
*/
