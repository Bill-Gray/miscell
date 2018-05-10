/* gfc_xvt.c: converts .gfc (gravitational field coefficient) files into
arrays for use in C;  details below the licence

Copyright (C) 2018, Project Pluto

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
02110-1301, USA.

   This program will read the data file of spherical harmonics for various
gravitational models of the Earth,  Moon,  Mars,  and Venus in the .gfc
(gravitational field coefficient,  I think) format,  and writes those
coefficients out as a C array.  The files are documented and provided at

ftp://podaac.jpl.nasa.gov/pub/grace/GGM03
http://icgem.gfz-potsdam.de/home

   The latter site is to be preferred,  as it gives a _lot_ of different
models and includes the non-Earth ones.  Note also that a variety of models,
including some for Mercury and Vesta,  are available at

http://pds-geosciences.wustl.edu/dataserv/gravity_models.htm

   (though not in .gfc format... still,  conversion looks as if it would
be easy.  The PDS .sha format appears to have the same numbers,  just
formatted a little differently.)

   See 'geo_pot.cpp' in the Find_Orb project for an example of the use
of the output from this code.   */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#define MIN_TERMS 3

int main( const int argc, const char **argv)
{
   char buff[100];
   const char *input_filename = (argc == 1 ? "GGM03C.txt" : argv[1]);
   FILE *ifile = fopen( input_filename, "rb");
   const int max_l = (argc < 3 ? 360 : atoi( argv[2]));
   const time_t t0 = time( NULL);
   size_t i;

   if( !ifile)
      {
      fprintf( stderr, "%s not opened", input_filename);
      perror( "");
      fprintf( stderr,
            "This program reads an input .gfc file and outputs the coefficients\n"
            "in a convenient C array.  Run ./gfc_xvt (filename) (max coefficient).\n");
      return( -1);
      }
   printf( "/* Generated from input file %s at %.24s using gfc_xvt */\n",
               input_filename, ctime( &t0));
   printf( "#define N_TERMS %d\n\n", max_l);
   printf( "   const double ");
   for( i = 0; argv[1][i] && argv[1][i] != '.'; i++)
      printf( "%c", tolower( argv[1][i]));
   printf( "_terms[] = {");
   while( fgets( buff, sizeof( buff), ifile))
      if( !memcmp( buff, "gfc ", 4) && atoi( buff + 3) <= max_l)
         {
         const int l = atoi( buff + 5);
         const int m = atoi( buff + 9);

         buff[13] = buff[33] = buff[53] = '\0';
         buff[29] = buff[49] = 'E';
         if( m == 0)
            printf( (l < MIN_TERMS) ? "\n" : "#if( N_TERMS >= %d)\n", l);
         printf( "     %c %s, %s%c   /\x2a %d %d \x2a/\n",
                ((m == 0 && l >= MIN_TERMS) ? ',' : ' '),
                buff + 14, buff + 34,
                ((m == l && l >= MIN_TERMS - 1) ? ' ' : ','),
                l, m);
         if( m == l && l >= MIN_TERMS)
            printf( "#endif\n");
         }
   printf( "};\n");         /* closing bracket of array */
   fclose( ifile);
   return( 0);
}
