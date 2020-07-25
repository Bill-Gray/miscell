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

/* Reads the MPC archive page at

http://www.minorplanetcenter.net/iau/ECS/MPCArchive/MPCArchive.html

and outputs a "computer friendly" list (example lines follow) :

20000523 MPO       1-   822
20100411 MPS  320571-322138
20100330 MPC   69147- 69662
20170402 MPS  781895-785056
20170326 MPS  778819-781894
20170319 MPS  777639-778818
20170312 MPC  103031-103972

   suitable for use in determining a date for a given MPC,  MPS,  or
MPO reference.  Note that,  as of 2017 April 10,  it looks as if the
MPS references will pass the million mark sometime around 2019 or 2020;
we'll probably have to make fixes for the "MPS1M" bug at that point. */

int main( const int argc, const char **argv)
{
   FILE *ifile = fopen( argc == 1 ? "archive.htm" : argv[1], "rb");
   char buff[200];


   assert( ifile);
   while( fgets( buff, sizeof( buff), ifile))
      if( !memcmp( buff, "<li><a href=\"/iau/ECS/MPCArchive/", 33))
         {
         const size_t ilen = strlen( buff);
         long a, b;
         int n_fields;

         assert( ilen >= 72);
         assert( !memcmp( buff + 50, ".pdf\"><i>", 9));
         assert( !memcmp( buff + 62, "</i>", 4));
         n_fields = sscanf( buff + 66, "%ld-%ld", &a, &b);
         assert( n_fields == 2);
         printf( "%.8s %.3s %7ld %7ld\n", buff + 42, buff + 59, a, b);
         }
   fclose( ifile);
   return( 0);
}
