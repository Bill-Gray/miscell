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
#include <string.h>
#include <assert.h>

/* Code to extract CSS artsat astrometry from my inbox.  Of
zero interest to anyone who isn't me.  Ignore this.  Move
along.  Nothing to see here...

Normally, this code reads lines and ignores them (initial state is
NO_OUTPUT),  because most of my e-mail is not artsat data from
CSS.  But if the line reads something like

Subject: G96 ARTSAT C0HLNV2

   and that's the satellite we mentioned on the command line (or
we're just outputting all artsats),  the state switches to GOT_OBJECT.
If we then see any of

COD G96
COD I52
COD V06
COD 703

   the state becomes OUTPUT_TEXT.  When we see a line starting

From

   we assume we're on to the next e-mail and switch the state back
to NO_OUTPUT. */

#define NO_OUTPUT      0
#define GOT_OBJECT     1
#define OUTPUT_TEXT    2

int main( const int argc, const char **argv)
{
   const char *inbox_filename =
            "/home/phred/.thunderbird/ye4urkt7.default/ImapMail/shared5.mainehost-1.net/INBOX";
   FILE *ifile = fopen( inbox_filename, "rb");
   char buff[200];
   const char *search_obj = (argc > 1 ? argv[1] : "");
   int state = NO_OUTPUT;

   assert( ifile);
   while( fgets( buff, sizeof( buff), ifile))
      {
      if( !memcmp( buff, "Subject: 703 ARTSAT ", 20)
               || !memcmp( buff, "Subject: G96 ARTSAT ", 20)
               || !memcmp( buff, "Subject: I52 ARTSAT ", 20)
               || !memcmp( buff, "Subject: V06 ARTSAT ", 20))
         if( !memcmp( buff + 20, search_obj, strlen( search_obj)))
            {
            printf( "\n");
            state = GOT_OBJECT;
            }
      if( state == GOT_OBJECT)
         if( !memcmp( buff, "COD G96", 7) || !memcmp( buff, "COD 703", 7)
                                          || !memcmp( buff, "COD I52", 7)
                                          || !memcmp( buff, "COD V06", 7))
            state = OUTPUT_TEXT;
      if( state == OUTPUT_TEXT && !memcmp( buff, "From", 4))
         state = NO_OUTPUT;
      if( state == OUTPUT_TEXT)
         printf( "%s", buff);
      }
   fclose( ifile);
   return( 0);
}
