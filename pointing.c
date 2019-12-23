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
#include "fitsfile.h"
#include "wcs.h"

/* Preliminary code to read through a batch of FITS files to generate JSON
formatted 'pointings' data for submission to the Minor Planet Center :

https://minorplanetcenter.net/pointings/

   Run this as,  say,

./pointing -mZ44 *.fit > pointing.json


   and a JSON file with pointings for all images in the current
directory will be generated,  with the MPC code set to (Z44).

   A few warnings :

   (1) A single JSON file will be created with pointings for all images.
It's unclear to me if that's the direction MPC intends to go.  The
idea of one JSON file per image seems silly to me.

   (2) The 'surveyExpName' field contains the file name.  MPC is currently
restrictive about what can be in that field.  I've e-mailed them to request
greater flexibility.  No reply yet (I probably won't get one.)  In the
meantime,  the 'surveyExpName' omits the directory name.

   (3) The program can get the date/time of observation and the image
orientation in the sky from the FITS header,  and usually can get the
exposure duration (tries the EXPOSURE keyword,  and if that fails,  the
EXP-TIME keyword).  If the image lacks WCS header info,  the program
has no idea where the telescope was actually pointed.

   (4) There is no 'standard' FITS keyword to specify the MPC code from
which an image was taken,  nor the limiting magnitude.  I've sort of
worked that out for the MPC code,  which presumably can just be set
on the command line,  but the limiting magnitude will vary from image
to image.  If you have a FITS keyword you use to specify that value,
let me know,  and I'll tweak the code to make use of it.

   (5) I've found three keywords thus far for the exposure time.
The full FITS-formatted time of observation is usually given by
DATE-OBS,  but this cannot be relied upon;  sometimes that's just
the date,  and you have to concatenate a T and then TIME-OBS.  I'm
sure other variants will crop up and need to be handled... the 'S'
part of 'FITS' is a rather poor joke.        */

const char *mpc_code = "703";

static int output_pointing_data( const char *filename)
{
   int lhead, nbhead;
   char *header = fitsrhead( (char *)filename, &lhead, &nbhead);
   char str[80];
   const char *exp_name = filename;
   struct WorldCoor *wcs;
   double ra, dec, width, height;

   if( !header)
      {
      fprintf( stderr, "Couldn't get FITS data from '%s'\n", filename);
      return( -1);
      }
   wcs = wcsninit( header, lhead);
   if( !wcs)
      {
      fprintf( stderr, "Couldn't init WCS header info\n");
      return( -1);
      }

   wcsfull( wcs, &ra, &dec, &width, &height);

   printf( "{\n");
   printf( "   \"action\": \"exposed\",\n");
                  /* skip path data;  the exposure name can't contain '/' */
   while( strchr( exp_name, '/'))
      exp_name = strchr( exp_name, '/') + 1;
   printf( "   \"surveyExpName\": \"%s\",\n", exp_name);
   printf( "   \"mode\": \"survey\",\n");
   printf( "   \"mpcCode\": \"%s\",\n", mpc_code);
   if( hgets( header, "DATE-OBS", sizeof( str), str))
      {
      if( strlen( str) < 10)
         fprintf( stderr, "Non-standard DATE-OBS '%s' fund in '%s'\n", str, filename);
      else
         {
         if( strlen( str) < 14)
            {
            str[10] = 'T';
            hgets( header, "TIME-OBS", sizeof( str) - 11, str + 11);
            }
         printf( "   \"time\": \"%s\",\n", str);
         }
      }
   else
      fprintf( stderr, "No DATE-OBS found in '%s'\n", filename);
   if( hgets( header, "EXPOSURE", sizeof( str), str)
            || hgets( header, "EXP-TIME", sizeof( str), str)
            || hgets( header, "EXPTIME", sizeof( str), str))
      printf( "   \"duration\": \"%.2f\",\n", atof( str));
   else
      fprintf( stderr, "No exposure duration found in '%s'\n", filename);
   strcpy( str, "19.8");
   printf( "   \"limit\": \"%s\",\n", str);
   printf( "   \"center\": [%.4f,%.4f],\n", ra, dec);
   if( width == height)
      printf( "   \"width\": %.4f\n", width);
   else
      printf( "   \"widths\": [%.4f,%.4f]\n", width, height);
   printf( "}");

#ifdef DEBUGGING_OR_FUTURE_USE
   printf( "Image name '%s'\n", filename);
   printf( "Image is %lf by %lf pixels\n", wcs->nxpix, wcs->nypix);
   printf( "   Center -> %.3f, %.3f;  size = %.3f x %.3f\n",
                        ra, dec, width, height);

   pix2wcs( wcs, 0., 0., &ra, &dec);
   printf( "   LL -> %.3f, %.3f\n", ra, dec);

   pix2wcs( wcs, wcs->nxpix, 0., &ra, &dec);
   printf( "   LR -> %.3f, %.3f\n", ra, dec);

   pix2wcs( wcs, wcs->nxpix, wcs->nypix, &ra, &dec);
   printf( "   UR -> %.3f, %.3f\n", ra, dec);

   pix2wcs( wcs, 0., wcs->nypix, &ra, &dec);
   printf( "   UL -> %.3f, %.3f\n", ra, dec);
#endif
   return( 0);
}

int main( const int argc, const char **argv)
{
   int i;

   for( i = 1; i < argc; i++)
      if( argv[i][0] == '-')
         {
         switch( argv[i][1])
            {
            case 'm':
               mpc_code = argv[i] + 2;
               break;
            default:
               fprintf( stderr, "Argument '%s' not parsed\n", argv[i]);
               break;
            }
         }
      else
         {
         if( output_pointing_data( argv[i]))
            fprintf( stderr, "Couldn't get pointing data for %s\n", argv[i]);
         else if( i != argc - 1)
            printf( ",\n");
         }
   return( 0);
}

