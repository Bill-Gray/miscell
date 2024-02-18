#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "watdefs.h"
#include "stringex.h"

/* Code to read in observational ephemerides from Horizons and
generate synthetic astrometry.  Run,  for example,

./jpl2sof horizons_results.txt

   and synthetic astrometry will be output to stdout.
 2022-Dec-02 21:00     23 52 22.49 -04 48 46.3   16.549    n.a.  0.00250553992659  -0.1623146  105.9636 /T   73.8964   37.021695   62.310504   -8.235488         n.a.     n.a.
     A10R1Zt KC2022 11 26.98769 18 41 29.53 +76 40 47.6    ~BR2L 18.1 RVNEOCPK51
               K221126:031415...
*/

int main( const int argc, const char **argv)
{
   FILE *ifile = fopen( argv[1], "rb");
   char buff[200], obuff[81];
   const char *object_name = "Object";
   const char *months[12] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                              "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

   assert( argc > 1);
   while( fgets( buff, sizeof( buff), ifile))
      if( buff[0] == ' ' && buff[5] == '-' && buff[9] == '-' && buff[12] == ' ')
         {
         const int year = atoi( buff + 1), day = atoi( buff + 10);
         int month = 0;

         buff[9] = '\0';
         while( month < 12 && strcmp( months[month], buff + 6))
            month++;
         if( month < 12 && year > 1000 && year < 3000 && day >= 1 && day <= 31)
            {
            snprintf_err( obuff, sizeof( obuff),
                  "     %-7s  C%c%02d%02d%02d:%.2s%.2s%.2s   %.23s                 Synth500",
                           object_name,
                           'A' + (year / 100) - 10,      /* century */
                           year % 100, month + 1, day,
                           buff + 13, buff + 16, buff + 19,   /* hours minutes seconds */
                           buff + 23);
            if( obuff[27] == ' ')           /* fill in seconds if needed */
               obuff[27] = obuff[28] = '0';        /* (usually will be) */
            printf( "%s\n", obuff);
            }
         }
   fclose( ifile);
   return( 0);
}
