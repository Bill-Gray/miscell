#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

/* Given the names of two files of MPC astrometry on the command
line,  this determines which objects are in each file,  then figures
out which objects in the second file have changed from the data in
the first file.  That is to say,  it figures out which objects
ought to be recomputed using the second file's data.

Optionally,  it will then run through the second file and print out
all the observations for just those objects.  This is useful when
you're thinking "I've gotten an updated version of this file and
only want to process the objects that have actually changed."
Changes in references and similar irrelevancies are neglected.

This works by reading in all observations from the first file and
creating a list of objects,  along with a hash of the observations
in them.  (Or rather,  a hash of the parts of the observations
we're concerned about;  as noted,  we don't really care if the
reference code has changed.)  Then we do the same thing for the
second file.  The two lists are compared to determine objects
that have changed;  finally,  we read through the second file
again,  outputting only the data for the selected objects.  */

#define is_power_of_two( X)   (!((X) & ((X) - 1)))

typedef struct
{
   char packed[12];
   long hash;
} obj_t;

static long hash_80_column_astrometry( const char *buff)
{
   const long prime = 314159257;
   long rval = 42;
   size_t i;
   const char *template =
               "               2022_11_24.23624418_37_12.050"
               "+76_45_16.88         19.14oV     T05";

   for( i = 0; i < 80; i++)
      if( template[i] != ' ')
         rval = (rval * prime) ^ buff[i];
   return( rval);
}

int obj_compare( const void *a, const void *b)
{
   return( memcmp( a, b, 12));
}

/* In most files,  the observations are at least approximately sorted by
packed ID.  In some,  there is a bit of disorder.  So we look back by
ten objects to see if maybe this is something we've looked at recently.

This will sometimes fail,  because the actual object was eleven or more
objects back,  and we'll have an erroneously duplicated object.  So
after sorting by packed ID,  we do a final pass to look for such
duplicates (now adjacent in the array) and eliminate them.  The hash
value is the XORed result of the hashes for each line,  so re-ordering
of lines won't affect the final result.  It also means that when we
eliminate duplicates,  XORing the hashes from them gets us the right
answer,  just as if the file had been completely sorted. */

static obj_t *find_objects_in_file( FILE *ifile, unsigned *n_found)
{
   obj_t *rval = NULL;
   unsigned n = 0, i, j;
   char buff[90];
   const unsigned lookback = 10;

   while( fgets( buff, sizeof( buff), ifile))
      if( strlen( buff) == 81)
         {
         unsigned loc = n;

         while( loc && loc + lookback > n && memcmp( buff, rval[loc - 1].packed, 12))
            loc--;
         if( loc + lookback <= n)
            loc = 0;
         if( !loc)      /* didn't find it */
            {
            loc = n;
            n++;
            if( is_power_of_two( n))
               rval = (obj_t *)realloc( rval, 2 * n * sizeof( obj_t));
            memcpy( rval[loc].packed, buff, 12);
            rval[loc].hash = 0;
            }
         else
            loc--;
         rval[loc].hash ^= hash_80_column_astrometry( buff);
         }
   assert( n);       /* we must've gotten at least _one_ object */
   qsort( rval, n, sizeof( obj_t), obj_compare);
   for( i = j = 1; i < n; i++)
      if( memcmp( rval[i - 1].packed, rval[i].packed, 12))
         rval[j++] = rval[i];
      else
         rval[j - 1].hash ^= rval[i].hash;
   n = j;
// for( i = 0; i < n; i++)
//    printf( "%.12s %lx\n", rval[i].packed, (unsigned long)rval[i].hash);
   *n_found = n;
   return( rval);
}

int main( const int argc, const char **argv)
{
   FILE *before, *after;
   obj_t *obj_bef, *obj_aft;
   unsigned n_bef, n_aft, i, j;
   bool is_nsd_obs;

   if( argc < 3)
      {
      fprintf( stderr,
         "'ast_diff' takes the name of two files containing 80-column\n"
         "astrometry as command-line arguments,  and determines which objects\n"
         "have been updated.  (Irrelevancies such as reference changes are\n"
         "ignored.)  The astrometry for objects in the second file that didn't\n"
         "exist in the first,  or were changed,  is output to stdout (or\n"
         "to the filename specified by a third command line argument.)\n");
      return( -1);
      }
   before = fopen( argv[1], "rb");
   assert( before);
   obj_bef = find_objects_in_file( before, &n_bef);
   fclose( before);

   after = fopen( argv[2], "rb");
   assert( after);
   obj_aft = find_objects_in_file( before, &n_aft);

   for( i = j = 0; i < n_aft; i++)
      {
      int compare = -1;

      while( j < n_bef && (compare = memcmp( obj_aft[i].packed, obj_bef[j].packed, 12)) > 0)
         j++;
      if( compare)
         printf( "%.12s wasn't in %s\n", obj_aft[i].packed, argv[1]);
      else if( obj_aft[i].hash != obj_bef[j].hash)
         printf( "%.12s changed\n", obj_aft[i].packed);
      else        /* no change;  mark for removal from list */
         obj_aft[i].packed[0] = '\0';
      }
   free( obj_bef);
   for( i = j = 0; i < n_aft; i++)
      if( obj_aft[i].packed[0])
         obj_aft[j++] = obj_aft[i];
   n_aft = j;
   printf( "%u objects have changed\n", n_aft);
   is_nsd_obs = strstr( argv[1], "nsd.obs");
   if( argc >= 4 && argv[3][0] != '-')
      {
      FILE *ofile = fopen( argv[3], "wb");
      char buff[90], prev_packed[12];
      int changed_or_new = 0;

      assert( ofile);
      fseek( after, 0L, SEEK_SET);
      memset( prev_packed, 0, sizeof( prev_packed));
      while( fgets( buff, sizeof( buff), after))
         {
         if( memcmp( prev_packed, buff, 12))       /* new packed desig */
            {
            memcpy( prev_packed, buff, 12);
            changed_or_new =
                   (NULL != bsearch( buff, obj_aft, n_aft, sizeof( obj_t), obj_compare));
            }
         if( changed_or_new)
            {
            if( is_nsd_obs)      /* mark obs as 'do not distribute' */
               buff[72] = '!';
            fputs( buff, ofile);
            }
         }
      fclose( ofile);
      }
   fclose( after);
   free( obj_aft);
   return( 0);
}
