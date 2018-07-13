/* Reads an MPEC index created by 'mpecer' and reverses it, so you get
the most recent MPECs first.  It also inserts thin horizontal lines
between days and thicker lines between half-months.       */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define MAX_LINES 50000

int main( const int argc, const char **argv)
{
   FILE *ifile = fopen( argv[1], "rb");
   FILE *ofile = stdout;
   char buff[300];

   assert( ifile);
   assert( ofile);
   while( fgets( buff, sizeof( buff), ifile))
      if( memcmp( buff, "<a name=\"A\">", 12))
         fprintf( ofile, "%s", buff);
      else
         {
         char **lines = (char **)calloc( MAX_LINES, sizeof( char *));
         int i, n = 0, half_month = 0;
         char day[80];

         assert( lines);
         while( fgets( buff, sizeof( buff), ifile) &&
                        memcmp( buff, "<a name=\"the", 12))
            if( !memcmp( buff, "<a href=", 8))
               {
               lines[n] = (char *)malloc( strlen( buff) + 1);
               strcpy( lines[n], buff);
               n++;
               }
         *day = '\0';
         for( i = n - 1; i >= 0; i--)
            {
            const int new_half = lines[i][55];
            char new_day[80], *tptr;
            int j = 0;


            tptr = strstr( lines[i], "</a> ");
            assert( tptr);
            tptr += 5;
            while( *tptr && *tptr != ',')
               {
               if( *tptr != ' ')
                  new_day[j++] = *tptr;
               tptr++;
               }
            assert( *tptr == ',');
            if( new_half != half_month)
               {
               fprintf( ofile, "<hr class=\"halfmonth\"> <p>\n");
               fprintf( ofile, "<a name=\"%c\"> </a>\n", new_half);
               }
            else if( strcmp( day, new_day))
               fprintf( ofile, "<hr> <p>\n");
            half_month = new_half;
            strcpy( day, new_day);
            fprintf( ofile, "%s", lines[i]);
            free( lines[i]);
            }
         fprintf( ofile, "%s", buff);
         free( lines);
         }
   fclose( ifile);
   fclose( ofile);
   return( 0);
}
