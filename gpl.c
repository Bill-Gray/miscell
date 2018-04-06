/* gpl.c -- code to insert a GPL copyright notice

Copyright (C) 2018, Project Pluto

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
02110-1301, USA.

This rather simple-minded program examines all files specified
on the command line for GPL copyright notices resembling the
above.  (You'd normally run it as ~/miscell/gpl *.c *.h,  or
maybe *.cpp... you get the idea.)  If it doesn't find the word
'Copyright' in the first ten lines,  it creates a temporary
file that has the copyright notice inserted at the top,
unlinks the original file,  and moves the temporary file to the
original file.   */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <assert.h>

/* Copyright notice is broken up into two string literals,  each below
the 509-character limit in C90.     */

const char *copyright_notice_part_1 =
    "/* Copyright (C) 2018, Project Pluto\n"
    "\n"
    "This program is free software; you can redistribute it and/or\n"
    "modify it under the terms of the GNU General Public License\n"
    "as published by the Free Software Foundation; either version 2\n"
    "of the License, or (at your option) any later version.\n"
    "\n"
    "This program is distributed in the hope that it will be useful,\n"
    "but WITHOUT ANY WARRANTY; without even the implied warranty of\n";

const char *copyright_notice_part_2 =
    "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the\n"
    "GNU General Public License for more details.\n"
    "\n"
    "You should have received a copy of the GNU General Public License\n"
    "along with this program; if not, write to the Free Software\n"
    "Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA\n"
    "02110-1301, USA. */\n"
    "\n";

int main( const int argc, const char **argv)
{
   int i, j;

   for( i = 1; i < argc; i++)
      {
      char buff[300];
      FILE *ifile = fopen( argv[i], "rb");
      bool has_copyright_notice = false;
      const char *temp_file_name = "ickytemp.ugh";

      assert( ifile);
      for( j = 10; j && !has_copyright_notice
                     && fgets( buff, sizeof( buff), ifile); j--)
         if( strstr( buff, "Copyright"))
            has_copyright_notice = true;
      if( !has_copyright_notice)
         {
         FILE *ofile = fopen( temp_file_name, "wb");

         printf( "%s\n", argv[i]);
         assert( ofile);
         fprintf( ofile, "%s", copyright_notice_part_1);
         fprintf( ofile, "%s", copyright_notice_part_2);
         fseek( ifile, 0L, SEEK_SET);
         while( fgets( buff, sizeof( buff), ifile))
            fwrite( buff, strlen( buff), 1, ofile);
         }
      fclose( ifile);
      if( !has_copyright_notice)
         {
         if( unlink( argv[i]))
            perror( "Unlink failed ");
         if( rename( temp_file_name, argv[i]))
            perror( "Rename failed ");
         }
      }
   return( 0);
}
