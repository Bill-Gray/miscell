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

https://www.minorplanetcenter.net/iau/ECS/MPCArchive/MPCArchive.html

and outputs a "computer friendly" list (example lines follow) :

20250912 MPS 2422989 2436880
20250814 MPC  185967  187040
20250814 MPS 2418159 2422988
20250814 MPO  930001  940620
19831220 MPC    8323    8436

   suitable for use in determining a date for a given MPC,  MPS,  or
MPO reference.  Compile with

g++ -Wall -Wextra -pedantic -o archive archive.cpp

   Find_Orb already deciphers MPC references;  it'd be nice if it could
also give the date of the reference and perhaps a URL for it.  This table
would help it to do so.       */

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
