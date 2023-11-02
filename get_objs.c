#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* Code to read in one file containing a list of object designations
in packed form,  and then to read in another file of punched-card
astrometry and extract just those objects.  To do that,  we read in
all the lines in the first file (well,  just their packed designations)
and sort them.  Then we read in the _second_ file and,  for each line,
do a binary search to see if the temporary designation found in that
line occurs in our sorted list from the first file.

   Usually,  the data in the second line will be sorted,  and we'll
get plenty of lines for a given object.  So we only do the searching
when the packed designation changes.  That speeds matters up a fair bit.
*/

static void error_exit( void)
{
   fprintf( stderr,
           "'get_objs' needs two command line arguments : the name of a file\n"
           "listing packed designations of objects for which astrometry is to be\n"
           "extracted,  and the name of a file containing the astrometry.\n");
   exit( -1);
}

static int compare( const void *a, const void *b)
{
   return( strcmp( (const char *)a, (const char *)b));
}

#define IS_POWER_OF_TWO( n)  (!((n) & ((n) - 1)))

int main( const int argc, const char **argv)
{
   FILE *ifile;
   char buff[200], *desigs = NULL;
   char prev_desig[13];
   int it_matches = 0;
   unsigned i, n_desigs = 0;
   const unsigned desig_len = 13;   /* 12 bytes plus a trailing nul */

   if( argc < 3)
      error_exit( );
   ifile = fopen( argv[1], "rb");
   if( !ifile)
      {
      fprintf( stderr, "Couldn't open '%s' :", argv[1]);
      perror( NULL);
      error_exit( );
      }
   while( fgets( buff, sizeof( buff), ifile))
      if( *buff != '#')             /* both temp and permanent desigs */
         {
         char tdesig[20];

         buff[12] = '\0';
         sscanf( buff, "%s", tdesig);
         if( strlen( tdesig) == 12)
            {
            n_desigs++;
            if( IS_POWER_OF_TWO( n_desigs))
               desigs = (char *)realloc( desigs, n_desigs * 2 * desig_len);
            strcpy( desigs + (n_desigs - 1) * desig_len, tdesig + 5);
            tdesig[5] = '\0';
            }
         n_desigs++;
         if( IS_POWER_OF_TWO( n_desigs))
            desigs = (char *)realloc( desigs, n_desigs * 2 * desig_len);
         strcpy( desigs + (n_desigs - 1) * desig_len, tdesig);
         }
   fclose( ifile);
   if( !n_desigs)
      {
      printf( "No designations found in '%s'\n", argv[1]);
      error_exit( );
      }
   qsort( desigs, n_desigs, desig_len, compare);
   for( i = 0; i < n_desigs; i++)
      printf( "(%u) '%s'\n", i, desigs + i * desig_len);
   ifile = fopen( argv[2], "rb");
   if( !ifile)
      {
      fprintf( stderr, "Couldn't open '%s' :", argv[2]);
      perror( NULL);
      error_exit( );
      }
   memset( prev_desig, 0, sizeof( prev_desig));
   while( fgets( buff, sizeof( buff), ifile))
      {
      if( memcmp( prev_desig, buff, 12))
         {
         char tdesig[20], tchar = buff[12];

         buff[12] = '\0';
         sscanf( buff, "%s", tdesig);
         it_matches = (NULL != bsearch( tdesig, desigs, n_desigs, desig_len, compare));
         buff[12] = tchar;
         }
      if( it_matches)
         printf( "%s", buff);
      }
   fclose( ifile);
   free( desigs);
   return( 0);
}
