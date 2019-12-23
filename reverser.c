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
   FILE *ifile = (argc == 2 ? fopen( argv[1], "rb") : NULL);
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
