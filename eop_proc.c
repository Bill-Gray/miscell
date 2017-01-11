#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

/* Code to translate the EOP (Earth Orientation Parameter) file
'finals.all',  provided at

ftp://maia.usno.navy.mil/ser7/finals.all

into a form that is considerably easier to use,  in the file
'all_eops.txt'.

First problem this fixes : 'finals.all' gives you UT1-UTC,  when one
would really like to have TDT-UT1 = Delta-T.  This code corrects for
the leap seconds,  noting them as instances where UT1-UTC makes a
sudden jump.

'finals.all' gives you IERS values for all five parameters from
Bulletin A,  right up to within a couple of weeks of the time the file
was made. Then it switches over to predictions for the nutation
parameters Psi and dEps (i.e.,  you get a 'P' in column 96).

After that,  the next couple of weeks (leading up to roughly the day
the file was made) still give IERS data for polar motion and Delta-T.
But then,  predictions are used for the remaining three values as well
(i.e.,  columns 17 and 58 also switch from 'I' to 'P').

You then get about 70 days with predictions for all five EOPs.  After
that,  dPsi and dEps get blanked.

Predictions then continue for about ten months for the remaining three
values,  after which you get nada.

So we keep track of 'n_iersl' (number of lines where all five values
came from IERS);  'n_part_iers' (number of lines where the first three
values are IERS);  'n_predicts' (number of lines for which predictions
are given for all five values);  and 'n_part_predicts' (number of lines
where you get predictions for the first three values,  but not necessarily
anything else).

There's a squirrelly thing in that the data up through late 1980 has
'predicts' for the nutation values.  I think this just means they don't
have data,  and are computing estimated values going backward.  Since
those values aren't changing anyway,  I'm treating them as IERS.  */

int main( const int argc, const char **argv)
{
   const char *output_filename = "all_eops.txt";
   const char *input_filename = "finals.all";
   FILE *ifile, *ofile;
   char buff[200];
   int starting_mjd = 0, stepsize = 1, i;
   int n_iers = 0, n_part_iers = 0, n_predicts = 0, n_part_predicts = 0;
   double tdt_minus_utc = 44.184;
   double prev_ut1_minus_utc = 0.808;
   const char *header_line = " PM-x (arcsec) PM-y  TDT-UT1     dPsi (mas) dEps\n";

   for( i = 1; i < argc; i++)
      if( argv[i][0] == '-')
         switch( argv[i][1])
            {
            case 'i':
               input_filename = argv[i] + 2;
               break;
            case 'o':
               output_filename = argv[i] + 2;
               break;
            case 's':
               stepsize = atoi( argv[i] + 2);
               break;
            default:
               printf( "Command-line option '%s' unrecognized\n", argv[i]);
               return( -1);
            }

   ifile = fopen( input_filename, "rb");
   assert( ifile);
   ofile = fopen( output_filename, "wb");
   assert( ofile);
            /* Header is 70 bytes.  Write garbage;  we'll come back and */
            /* write the 'correct' header when we're done.              */
   fwrite( buff, 70, 1, ofile);
   fprintf( ofile, "%s", header_line);
   for( i = 0; fgets( buff, sizeof( buff), ifile) && buff[16] != ' '; i++)
      if( !(i % stepsize))
         {
         const double ut1_minus_utc = atof( buff + 58);

         if( !i)
            starting_mjd = atoi( buff + 7);
         if( prev_ut1_minus_utc - ut1_minus_utc < -0.5)
            tdt_minus_utc += 1.;    /* leap second happened here */
         prev_ut1_minus_utc = ut1_minus_utc;
         fprintf( ofile, "%.9s %.9s %10.7f %.8s %.8s\n",
                  buff + 18, buff + 37,       /* polar motion x & y */
                  tdt_minus_utc - ut1_minus_utc,  /* Delta-T = TDT-UT1 */
                  buff + 98, buff + 117);
         if( buff[95] == 'I' || i < 2900)
            n_iers++;
         if( buff[16] == 'I')
            n_part_iers++;
         if( buff[95] != ' ')
            n_predicts++;
         n_part_predicts++;
         }
   fprintf( ofile, "%s", header_line);
   fprintf( ofile, "Above was derived from\n"
               "ftp://maia.usno.navy.mil/ser7/finals.all\n"
               "with only the five Bulletin A earth orientation parameters\n"
               "extracted (no sigmas,  BullB,  etc.)\n"
               "Run with 'eop_proc' version " __DATE__ " " __TIME__ "\n");

         /* Now go back and write out a correct header : */
   fseek( ofile, 0L, SEEK_SET);
   fprintf( ofile, "See bottom of file for description\n");
   fprintf( ofile, "%5d %5d %5d %5d %5d %5d\n",
         starting_mjd, stepsize,
         starting_mjd + stepsize * n_iers,
         starting_mjd + stepsize * n_part_iers,
         starting_mjd + stepsize * n_predicts,
         starting_mjd + stepsize * n_part_predicts);
   fclose( ofile);
   return( 0);
}
