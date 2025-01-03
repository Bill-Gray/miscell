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
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>

static void show_error_message( void)
{
   printf( "\n'mpcorbx',  run without command line arguments,  will read in\n");
   printf( "'MPCORB.DAT' and output 'mpcorbcr.dat' with carriage returns\n");
   printf( "inserted.  With command-line arguments added,  one can filter\n");
   printf( "the output to contain only specified objects.  The constraints\n");
   printf( "can be combined as desired.  For example:\n\n");
   printf( "a(1.3 a)1.     Select only objects with a<1.3 AU and a>1\n");
   printf( "q)5.1          Select objects with q > 5.1 AU\n");
   printf( "Q(8.1 e).7     Select objects with Q ( 8.1 and e > .7\n");
   printf( "P)1.6          Select objects with period greater than 1.6 years\n");
   printf( "i)10 i(16      Select objects with i>10 degrees and i<16\n");
   printf( "H(10 N(1700    Select objects with H<10 and among the first 1700 objects\n");
   printf( "n(.5           Select objects with mean motion less than .5 deg/day\n");
   printf( "A)181          Ascending node (Omega)>181 degrees\n");
   printf( "p)43           Argument of perihelion (omega)<43 degrees\n");
   printf( "O(19900810     Only objects last observed before 1990 August 10\n");
   printf( "d)K10K42Q      Only provisional desigs after K10K42Q = 2010 KQ42\n");
   printf( "-ofiltered.txt Direct output to 'filtered.txt' (default is stdout)\n");
   printf( "-impcz.txt     Read input from 'mpcz.txt' (default is MPCORB.DAT)\n\n");
   printf( "Note the use of ( and ) instead of < or >.  The latter are file\n");
   printf( "redirection operators,  so sadly,  we can't use them here.\n\n");
   printf( "When filtering,  the output will default to being without carriage\n");
   printf( "returns.  Add a '-c' command line option to turn them back on.\n\n");
   printf( "mpcorbx a)1.2 i(10 -c -orandom.txt\n\n");
   printf( "would select objects with semimajor axes greater than 1.2 AU and\n");
   printf( "inclinations less than ten degrees,  and output them with carriage\n");
   printf( "returns to the file 'random.txt'.\n\n");
   printf( "See http://www.projectpluto.com/mpcorbx.htm for more information.\n");
   printf( "%s version\n", __DATE__);
}

static void output_line( FILE *ofile, const char *buff, const int use_cr)
{
   char tbuff[2];
   int i;

   for( i = 0; buff[i] >= ' '; i++);
   fwrite( buff, i, 1, ofile);
   tbuff[0] = 13;
   tbuff[1] = 10;
   if( use_cr)
      fwrite( tbuff, 2, 1, ofile);
   else
      fwrite( tbuff + 1, 1, 1, ofile);
}

int main( const int argc, const char **argv)
{
   const char *input_file_name = "MPCORB.DAT";
   FILE *output_file = stdout, *ifile;
   char buff[300];
   int i, use_cr = 0, line_no = 0, n_lines_output = 0;
   int n_lines = 0;
   size_t filesize;
   time_t t0 = time( NULL);

   for( i = 1; i < argc; i++)
      if( argv[i][0] == '-')
         switch( argv[i][1])
            {
            case '?':
               show_error_message( );
               return( 0);
               break;
            case 'c': case 'C':
               use_cr = 1;
               break;
            case 'o': case 'O':
               output_file = fopen( argv[i] + 2, "wb");
               if( !output_file)
                  {
                  printf( "'%s' not opened\n", argv[i] + 2);
                  show_error_message( );
                  return( -1);
                  }
               break;
            case 'i':
               input_file_name = argv[i] + 2;
               break;
            default:
               printf( "'%s' not recognized\n", argv[i]);
               show_error_message( );
               return( -3);
               break;
            }

   ifile = fopen( input_file_name, "rb");
   if( !ifile)
      {
      printf( "'%s' not opened\n", input_file_name);
      show_error_message( );
      return( -2);
      }
   fseek( ifile, 0L, SEEK_END);
   filesize = ftell( ifile);
   fseek( ifile, 0L, SEEK_SET);

   if( argc == 1)   /* No command-line args:  just converting to CR/LF */
      {
      use_cr = 1;
      output_file = fopen( "mpcorbcr.dat", "wb");
      }

         /* Read in and output header,  unaltered: */
   *buff = '\0';
   while( memcmp( buff, "A brief header", 14) &&
                        fgets( buff, sizeof( buff), ifile))
      output_line( output_file, buff, use_cr);


         /* Mark the header as having been processed: */
   sprintf( buff, "mpcorbx version %s,  run on %s",
                                   __DATE__, asctime( gmtime( &t0)));
   output_line( output_file, buff, use_cr);
   if( argc > 1)
      {
      strcpy( buff, "Command-line arguments:");
      for( i = 1; i < argc; i++)
         {
         strcat( buff, " ");
         strcat( buff, argv[i]);
         }
      output_line( output_file, buff, use_cr);
      }

         /* Output remainder of the header:  */
   while( memcmp( buff, "------", 6) && fgets( buff, sizeof( buff), ifile))
      output_line( output_file, buff, use_cr);

               /* Now we're ready to read asteroid records: */
   while( fgets( buff, sizeof( buff), ifile))
      if( strlen( buff) > 200 && buff[29] == '.' && buff[95] == '.')
         {
         bool show_it = true;
         int i;

         line_no++;
         if( !n_lines)
            n_lines = 1 + (filesize - ftell( ifile)) / strlen( buff);
         for( i = 1; i < argc && show_it; i++)
            {
            const bool less_than    = (NULL != strchr( "<({l", argv[i][1]));
            const bool greater_than = (NULL != strchr( ">)}g", argv[i][1]));

            if( greater_than || less_than || argv[i][1] == ':')
               {
               const double DUMMY_VAL = -31.4159207;
               double val = DUMMY_VAL;
               const char *semimajor = buff + 91;
               const char *ecc = buff + 69;

               switch( argv[i][0])
                  {
                  case 'a':
                      val = atof( semimajor);
                      break;
                  case 'P':
                      {
                      const double a = atof( semimajor);

                      val = a * sqrt( a);
                      }
                      break;
                  case 'q':
                      val = atof( semimajor) * (1. - atof( ecc));
                      break;
                  case 'Q':
                      val = atof( semimajor) * (1. + atof( ecc));
                      break;
                  case 'H':     /* Consider non-blank H values only */
                      if( buff[10] != ' ')
                         val = atof( buff + 8);
                      else
                        show_it = false;
                      break;
                  case 'n':    /* mean motion */
                      val = atof( buff + 80);
                      break;
                  case 'O':    /* date last observed */
                      val = atof( buff + 194);
                      break;
                  case 'A':    /* ascending node */
                      val = atof( buff + 48);
                      break;
                  case 'p':    /* arg perih */
                      val = atof( buff + 37);
                      break;
                  case 'N':     /* line #; = asteroid # for numbered objs */
                      val = (double)line_no;
                      break;
                  case 'd':     /* filter by provisional designation */
                     {
                     const int len = strlen( argv[i] + 2);
                     const int compare = memcmp( buff, argv[i] + 2, len);

                     if( buff[len - 1] == ' ')
                        show_it = false;
                     else
                        {
                        if( less_than && compare > 0)
                           show_it = false;
                        if( greater_than && compare < 0)
                           show_it = false;
                        }
                     }
                     break;
                  case 'e':
                      val = atof( ecc);
                      break;
                  case 'i':
                      val = atof( buff + 59);
                      break;
                  default:
                      break;
                  }
               if( val != DUMMY_VAL)
                  {
                  if( argv[i][1] == ':')
                     {
                     double low_range, high_range;

                     if( sscanf( argv[i] + 2, "%lf,%lf", &low_range, &high_range) == 2)
                        if( val < low_range || val > high_range)
                           show_it = false;
                     }
                  else
                     {
                     const double comparison = atof( argv[i] + 2);

                     if( less_than && val > comparison)
                         show_it = false;
                     if( greater_than && val < comparison)
                         show_it = false;
                     }
                  }
               }
            }
         if( show_it)
             {
             n_lines_output++;
             output_line( output_file, buff, use_cr);
             }
         }
   fclose( ifile);
   if( output_file != stdout)
      {
      printf( "%d lines read in; %d lines written\n",
                  line_no, n_lines_output);
      fclose( output_file);
      }
   return( 0);
}
