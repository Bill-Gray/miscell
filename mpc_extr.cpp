#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Code to extract observations for a specific object from the large
MPC 80-column astrometry files (UnnObs.txt,  CmtObs.txt,  SatObs.txt,
NumObs.txt,  itf.txt).  These files contain only 80-column data,  are
always a multiple of 81 bytes (including the line feed at the end of
each line),  and are sorted by packed ID.  So if you want a particular
object,  you can just binary-search to find the first record,  then
start reading until you've gotten all the records.    */

int mpc_compare( const char *str1, const char *str2)
{
   int rval;

   if( strlen( str2) == 5)       /* numbered MP */
      rval = memcmp( str1, str2, 5);
   else                          /* provisional desig */
      rval = memcmp( str1 + 5, str2, 7);
   return( rval);
}

static char *err_fgets( char *buff, size_t nbytes, FILE *ifile)
{
   char *rval = fgets( buff, nbytes, ifile);

   if( !rval)
      {
      perror( "fgets failed");
      exit( -1);
      }
   return( rval);
}

static int err_exit( void)
{
   printf( "mpc_extr will extract data for a particular object from\n"
           "the 'large' MPC files UnnObs.txt, CmtObs.txt, SatObs.txt,\n"
           "NumObs.txt,  and itf.txt.  For example:\n\n"
           "./mpc_extr UnnObs.txt K14A00A K13YD3F\n\n"
           "would output all records for 2014 AA and 2013 YF133 to stdout.\n");
   return( -1);
}

int main( const int argc, const char **argv)
{
   FILE *ifile;
   unsigned long n_recs, recsize;
   char buff[100];
   int compare, i;
   FILE *ofile = stdout;

   if( argc < 3)
      return( err_exit( ));
   ifile = fopen( argv[1], "rb");
   if( !ifile)
      {
      printf( "%s not opened : ", argv[1]);
      fflush( stdout);
      perror( "");
      return( err_exit( ));
      }
   err_fgets( buff, sizeof( buff), ifile);
   recsize = strlen( buff);
   fseek( ifile, 0L, SEEK_END);
   n_recs = ftell( ifile) / recsize;
   for( i = 2; i < argc; i++)
      if( argv[i][0] == '-')
         switch( argv[i][1])
            {
            case 'o':
               ofile = fopen( argv[i] + 2, "wb");
               break;
            default:
               printf( "Unrecognized command-line option '%s'\n", argv[i]);
               break;
            }
   for( i = 2; i < argc; i++)
      if( argv[i][0] != '-')
         {
         unsigned long loc = 0, step, loc1;
         int n_found = 0;

         for( step = 0x8000000; step; step >>= 1)
            if( (loc1 = loc + step) < n_recs)
               {
               fseek( ifile, loc1 * recsize, SEEK_SET);
               err_fgets( buff, sizeof( buff), ifile);
               if( mpc_compare( buff, argv[i]) < 0)
                  loc = loc1;
               }
         compare = -1;
         fseek( ifile, loc * recsize, SEEK_SET);
         while( compare <= 0 && fgets( buff, sizeof( buff), ifile))
            {
            compare = mpc_compare( buff, argv[i]);
            if( !compare)
               {
               fprintf( ofile, "%s", buff);
               n_found++;
               }
            }
         printf( "%d records found for '%s'\n", n_found, argv[i]);
         }
   return( 0);
}
