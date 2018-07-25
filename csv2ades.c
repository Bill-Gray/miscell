/* Code to process Gaia-DR2 observations from CSV files at

http://cdn.gea.esac.esa.int/Gaia/gdr2/sso_observation/csv/

   into the ADES format.  Run as,  for example,

./csv2ades input_file.csv > output.ades
./csv2ades -4179 input_file.csv > output.ades

   The second example would output only observations for object (4179)
Toutatis.  The first would output all of the observations. */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "watdefs.h"
#include "date.h"

static char *get_csv( char *obuff, const char *ibuff, unsigned nval)
{
   size_t i;

   while( nval-- && ibuff)
      {
      ibuff = strchr( ibuff, ',');
      if( ibuff)
         ibuff++;
      }
   assert( ibuff);
   for( i = 0; ibuff[i] && ibuff[i] != ','; i++)
      obuff[i] = ibuff[i];
   obuff[i] = '\0';
   return( obuff);
}

static void csv_to_ades( const char *ibuff, const char *perm_ID)
{
   char tbuff[90];

   get_csv( tbuff, ibuff, 3);
   if( !perm_ID || !strcmp( perm_ID, tbuff))
      {
      const double jd_epoch = 2455197.5;    /* 2010 Jan 1.0 */
      double jd_obs;

      printf( "   <optical>\n");
      printf( "    <permID>%s</permID>\n", tbuff);
      printf( "    <mode>TDI</mode>\n");
      printf( "    <stn>258</stn>\n");
      printf( "    <sys>ICRF_AU</sys>\n");
      printf( "    <ctr>0</ctr>\n");
      printf( "    <pos1>%s</pos1>\n", get_csv( tbuff, ibuff, 18));
      printf( "    <pos2>%s</pos2>\n", get_csv( tbuff, ibuff, 19));
      printf( "    <pos3>%s</pos3>\n", get_csv( tbuff, ibuff, 20));
      jd_obs = atof( get_csv( tbuff, ibuff, 6)) + jd_epoch;
      full_ctime( tbuff, jd_obs, FULL_CTIME_MILLISECS | FULL_CTIME_YMD
                           | FULL_CTIME_LEADING_ZEROES | FULL_CTIME_MONTHS_AS_DIGITS);
      tbuff[4] = tbuff[7] = '-';
      tbuff[10] = 'T';
      printf( "    <obsTime>%s</obsTime>\n", tbuff);
      printf( "    <ra>%s</ra>\n",           get_csv( tbuff, ibuff, 7));
      printf( "    <dec>%s</dec>\n",         get_csv( tbuff, ibuff, 8));
      printf( "    <rmsRA>%f</rmsRA>\n",     atof( get_csv( tbuff, ibuff, 12)) / 1000.);
      printf( "    <rmsDec>%f</rmsDec>\n",   atof( get_csv( tbuff, ibuff, 13)) / 1000.);
      printf( "    <rmsCorr>%s</rmsCorr>\n", get_csv( tbuff, ibuff, 14));
      printf( "    <astCat>Gaia2</astCat>\n");
      printf( "   </optical>\n");
      }
}

int main( const int argc, const char **argv)
{
   FILE *hdr_file = fopen( "gaia.hdr", "rb");
   char buff[800];
   const char *perm_ID = NULL;
   int i;

   while( fgets( buff, sizeof( buff), hdr_file))
      if( *buff != '*')
         printf( "%s", buff);
      else for( i = 1; i < argc; i++)
         if( argv[i][0] == '-')
            perm_ID = argv[i] + 1;
         else
            {
            FILE *ifile = fopen( argv[i], "rb");

            assert( ifile);
            fgets( buff, sizeof( buff), ifile);
            while( fgets( buff, sizeof( buff), ifile))
               csv_to_ades( buff, perm_ID);
            fclose( ifile);
            }
   fclose( hdr_file);
   return( 0);
}

/*
   <optical>
    <permID>3200</permID>        number_mp
    <mode>TDI</mode>
    <stn>258</stn>
    <sys>ICRF_AU</sys>
    <ctr>0</ctr>
    <pos1>-0.9241930322</pos1>    x_gaia
    <pos2>+0.3423918499</pos2>    y_gaia
    <pos3>+0.1490296836</pos3>    z_gaia
    <obsTime>2016-02-27T15:45:22.593Z</obsTime>     ! epoch_utc (days since 2016 Feb 27 = JD 2457445.5)
    <ra>29.28052137</ra>                              ra
    <dec>+21.81708923</dec>                           dec
    <rmsRA>0.30945110</rmsRA>                       ! ra_error_random (IN milliarcsec)
    <rmsDec>0.52852813</rmsDec>                     ! dec_error_random
    <rmsCorr>0.9999607648</rmsCorr>                   ra_dec_correlation_random
    <astCat>Gaia2</astCat>
    <exp>4.4</exp>
    <remarks>TCB=2016-02-27T15:46:49.938Z, transitId=47101903417545381, observationId=471019034175453811, rmsRaSyst=0.00848502 as, rmsDecSyst=0.00394375 as, rmsCorrSyst=0.88192202, positionScanAngle=300.348 deg</remarks>
   </optical>

solution_id,source_id,observation_id,number_mp,
epoch,epoch_err,epoch_utc,ra,
dec,ra_error_systematic,dec_error_systematic,ra_dec_correlation_systematic,
ra_error_random,dec_error_random,ra_dec_correlation_random,g_mag,
g_flux,g_flux_error,x_gaia,y_gaia,
z_gaia,vx_gaia,vy_gaia,vz_gaia,
position_angle_scan,level_of_confidence
*/
