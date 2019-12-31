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
#include <assert.h>
#include <string.h>

/* Code to read in 'makefile' (for GNU/Linux and/or MinGW) and
produce 'bsdmake' for my TrueOS (FreeBSD) box.  Note that in
general,  such a conversion is fraught/hopeless.  But my makefiles
are pretty simple,  and with some rather ugly kludges,  I have
something that at least works with all of my current makefiles. */

static int removetab( const char *buff)
{
   while( *buff > ' ' && *buff < '~' && *buff != '=')
      buff++;
   while( *buff == ' ' || *buff == '\t')
      buff++;
   return( *buff == '=');
}

int main( const int argc, const char **argv)
{
   char buff[300];
   FILE *ifile = fopen( (argc > 1 ? argv[1] : "makefile"), "rb");
   FILE *ofile = fopen( (argc > 2 ? argv[2] : "bsdmake"), "wb");

   assert( ifile);
   assert( ofile);
   while( fgets( buff, sizeof( buff), ifile))
      if( !memcmp( buff, "ifdef", 5) || !memcmp( buff, "else", 4)
                 || !memcmp( buff, "ifeq", 4) || !memcmp( buff, "endif", 5))
         fprintf( ofile, ".%s", buff);
      else if( !strcmp( buff, "CC=$(PREFIX)g++\n"))
         fprintf( ofile, "CC=$(PREFIX)cc\n");
      else if( !strcmp( buff, "CC=g++\n"))
         fprintf( ofile, "CC=cc\n");
      else if( !strcmp( buff, "CURL=-lcurl\n"))
         {
         fprintf( ofile, "CURL=`pkg-config --libs libcurl`\n");
         fprintf( ofile, "CURLI=`pkg-config --cflags libcurl`\n");
         }
      else if( buff[0] == '\t' && removetab( buff + 1))
         fprintf( ofile, "%s", buff + 1);
      else
         fprintf( ofile, "%s", buff);
   fclose( ifile);
   fclose( ofile);
   return( 0);
}
