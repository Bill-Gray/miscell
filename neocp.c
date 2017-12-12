// #define CURL_STATICLIB
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <curl/curl.h>
#include <curl/easy.h>
#if defined( __linux__) || defined( __unix__) || defined( __APPLE__)
   #include <sys/time.h>         /* these allow resource limiting */
   #include <sys/resource.h>     /* see 'avoid_runaway_process'   */
#endif

   /* Limit the program to a certain amount of CPU time.  In this
      case,  if it's taking more than 200 seconds,  something
      has gone wrong,  and the process should die.       */

static void avoid_runaway_process( void)
{
   struct rlimit r;       /* Make sure process ends after 200 seconds,  with */
                          /* a "hard limit" of 220 seconds                   */
   r.rlim_cur = 200;
   r.rlim_max = 220;
   setrlimit( RLIMIT_CPU, &r);
}

   /* We have several files that should,  without question,  be
      openable.  If they aren't,  we should just fail.        */

static FILE *err_fopen( const char *filename, const char *permits)
{
   FILE *rval = fopen( filename, permits);

   if( !rval)
      {
      printf( "Couldn't open %s\n", filename);
      perror( NULL);
      exit( -1);
      }
   return( rval);
}

static unsigned fetch_a_file( const char *url, const char *filename, const int flags)
{
   CURL *curl = curl_easy_init();
   unsigned rval = 0;

   assert( curl);
   if( curl)
      {
      FILE *fp = fopen( filename, (flags & 1) ? "ab" : "wb");
      const long starting_loc = ftell( fp);

      if( !fp)
         {
         printf( "Couldn't append %s to %s\n", url, filename);
         perror( NULL);
         exit( -1);
         }
      else
         {
         CURLcode res;

         curl_easy_setopt( curl, CURLOPT_URL, url);
         curl_easy_setopt( curl, CURLOPT_WRITEFUNCTION, fwrite);
         curl_easy_setopt( curl, CURLOPT_WRITEDATA, fp);
         if( flags & 2)
            {
            curl_easy_setopt( curl, CURLOPT_NOBODY, 1);
            curl_easy_setopt( curl, CURLOPT_HEADER, 1);
            }
         res = curl_easy_perform( curl);
         if( res)
            {
            printf( "libcurl error %d occurred\n", res);
            printf( "File '%s'; url %s\n", filename, url);
            exit( -1);
            }
         curl_easy_cleanup( curl);
         rval = (unsigned)( ftell( fp) - starting_loc);
         fclose( fp);
         }
      }
   return( rval);
}

/* Lines in 'neocp.txt' (plaintext summary of which objects are currently
   on NEOCP) have certain fixed traits,  such as always being 95 bytes.
   (Up to 2015 June 4,  when they switched to being 102 bytes.)  */

#define NEOCPLST_LINE_LEN  102

static bool is_valid_neocplst_line( const char *buff)
{
   const bool is_valid = (strlen( buff) == NEOCPLST_LINE_LEN
            && (!memcmp( buff + 48, "Updated ", 8) || !memcmp( buff + 48, "Added ", 6))
            && !memcmp( buff + 11, " 20", 3) && buff[37] == '.');

   return( is_valid);
}

struct neocp_summary
   {
   char desig[20];
   unsigned n_obs;
   unsigned checksum;
   bool exists_in_other_list;
   };

#define MAX_OBJS 1000

static struct neocp_summary *get_neocp_summary( const char *filename,
                              unsigned *n_objs_found)
{
   struct neocp_summary *rval = (struct neocp_summary *)
                       calloc( MAX_OBJS, sizeof( struct neocp_summary));
   unsigned i, n_found = 0;
   char buff[200];
   FILE *ifile = fopen( filename, "rb");

   if( !ifile)
      {
      *n_objs_found = 0;
      printf( "'%s' not found\n", filename);
      return( rval);
      }
   printf( "Getting %s\n", filename);
   while( fgets( buff, sizeof( buff), ifile))
      if( is_valid_neocplst_line( buff))
         {
         memset( buff + 7, 0, 41);
         memset( buff + 95, 0, 7);
         for( i = 7; i && buff[i - 1] == ' '; i--)
            ;
         buff[i] = '\0';
         strcpy( rval[n_found].desig, buff);
         rval[n_found].n_obs = (unsigned)atoi( buff + 79);
         rval[n_found].exists_in_other_list = false;
         for( i = 0; i < NEOCPLST_LINE_LEN; i++)
            {
            rval[n_found].checksum += (unsigned)buff[i];
            rval[n_found].checksum *= 123456789u;
            }
         n_found++;
         assert( n_found < MAX_OBJS);
         }
    printf( "Got %d\n", n_found);
    memset( rval + n_found, 0, sizeof( struct neocp_summary));
    fclose( ifile);
    if( n_objs_found)
      *n_objs_found = n_found;
    i = 0;
    while( i + 1 < n_found)   /* "gnome-sort" objs by desig */
      if( strcmp( rval[i].desig, rval[i + 1].desig) > 0)
         {
         const struct neocp_summary temp = rval[i];

         rval[i] = rval[i + 1];
         rval[i + 1] = temp;
         if( i)
            i--;
         }
      else
         i++;
    printf( "Sorted\n");
    return( rval);
}


static unsigned xfer_obs( const char *desig, FILE *ofile, FILE *ifile)
{
   char buff[90];
   char padded_desig[40];
   unsigned rval = 0;

   assert( ifile);
   fseek( ifile, 0L, SEEK_SET);
   snprintf( padded_desig, sizeof( padded_desig), "%s        ", desig);
   while( fgets( buff, sizeof( buff), ifile))
      if( !memcmp( buff + 5, padded_desig, 7))
         {
         fputs( buff, ofile);
         rval++;
         }
   return( rval);
}

/* We want to know which objects from the 'before' list exist in the
'after' list;  if they don't,  they've been removed from NEOCP and
ought to be added to 'neocp.old'.

We also want to know which objects in the 'after' list are unchanged
from versions in the 'before' list.  The rest of the 'after' list
represents new or changed objects for which astrometry should be
extracted.        */

static void crossreference( struct neocp_summary *before, struct neocp_summary *after)
{
   unsigned i, j;

   for( i = 0; before[i].n_obs; i++)
      {
      for( j = 0; after[j].n_obs; j++)
         if( !strcmp( before[i].desig, after[j].desig))
            before[i].exists_in_other_list = true;
      }
   for( j = 0; after[j].n_obs; j++)
      {
      for( i = 0; before[i].n_obs; i++)
         if( !strcmp( before[i].desig, after[j].desig)
                     && before[i].checksum == after[j].checksum
                     && before[i].n_obs == after[j].n_obs)
            after[j].exists_in_other_list = true;
      }
}

static unsigned n_not_found_in_other_list( const struct neocp_summary *list,
               unsigned list_size)
{
   unsigned rval = 0;

   while( list_size--)
      {
      if( !list->exists_in_other_list)
         rval++;
      list++;
      }
   return( rval);
}

/* Here,  we load up the designations from 'before' (neocplst.txt
and neocp.txt) and 'after' (neocplst.tmp).  Objects appearing in
the former but not the latter are assumed to have been removed; data
for the removed objects are found in 'neocp.txt' and appended to
'neocp.old'. Objects which have changed in some way (new/revised
observations, or simply new objects) are written to 'neocp.new'.
We create an 'neocp.tmp' file which includes the unchanged objects
from before,  plus the new stuff we've written out to 'neocp.new'. */

static void show_differences( void)
{
   unsigned n_before, n_after, i, j, n_new;
   struct neocp_summary *before = get_neocp_summary( "neocplst.txt", &n_before);
   struct neocp_summary *after  = get_neocp_summary( "neocplst.tmp", &n_after);
   FILE *ifile, *ofile;
   const time_t t0 = time( NULL);

   printf( "Run at %s", ctime( &t0));
   printf( "%u objects before; %u after\n", n_before, n_after);
   crossreference( before, after);
   n_new = n_not_found_in_other_list( after, n_after);
   printf( "%u objects removed; %u new objects\n",
               n_not_found_in_other_list( before, n_before), n_new);
   printf( "Removed objects:\n");
   ofile = err_fopen( "neocp.old", "ab");
   fprintf( ofile, "# New objs added %s", ctime( &t0));
   if( n_before)           /* possibly no objects were on NEOCP,  or we're */
      ifile = err_fopen( "neocp.txt", "rb"); /* running for the first time */
   else
      ifile = NULL;
   for( i = j = 0; i < n_before; i++)
      if( !before[i].exists_in_other_list)
         {
         printf( "   (%u) %s: %d obs\n", ++j, before[i].desig, before[i].n_obs);
         xfer_obs( before[i].desig, ofile, ifile);
         }
   fclose( ofile);

            /* Now transfer old,  unchanged objects to 'neocp.tmp': */
   ofile = fopen( "neocp.tmp", "wb");
   for( i = 0; i < n_after; i++)
      if( after[i].exists_in_other_list)
         xfer_obs( after[i].desig, ofile, ifile);
   if( ifile)
      fclose( ifile);

   if( n_new)
      {
      printf( "New/changed objects :\n");
      unlink( "neocp.new");
      for( i = j = 0; i < n_after; i++)
         if( !after[i].exists_in_other_list)
            {
            char url[200];
            unsigned n_lines_read, n_obs_previously = 0, k;
            const unsigned line_len = 81;      /* MPC 80-column record + LF */

            for( k = 0; k < n_before; k++)
               if( !strcmp( before[k].desig, after[i].desig))
                  n_obs_previously = before[k].n_obs;
            printf( "   (%u) %s: %u obs (was %u)\n", ++j, after[i].desig,
                                after[i].n_obs, n_obs_previously);
            strcpy( url, "https://minorplanetcenter.net/cgi-bin/showobsorbs.cgi?Obj=");
            strcat( url, after[i].desig);
            strcat( url, "&obs=y");
            n_lines_read = fetch_a_file( url, "neocp.new", 1) / line_len;
            if( n_lines_read != after[i].n_obs)
               printf( "!!! %u obs read\n", n_lines_read);
            }
      ifile = fopen( "neocp.new", "rb");
      if( ifile)              /* append "new" objects to neocp.tmp, */
         {                    /* skipping HTML stuff */
         char buff[90];

         fprintf( ofile, "# New objs added %s", ctime( &t0));
         while( fgets( buff, sizeof( buff), ifile))
            if( strlen( buff) > 80)
               fputs( buff, ofile);
         fclose( ifile);
         }
      }
   fclose( ofile);
   free( before);
   free( after);
}

/* Turns out that the NEOCP plain-text summary file is updated every half
hour,  even if nothing has changed.  (And possibly is modified solely on
that schedule;  that is to say,  the summary file can lag actual changes.)
Until that's fixed,  I'm leaving CHECK_HEAD undefined;  we just download
the file and compare it to our previous version.      */

#ifdef CHECK_HEAD
static int header_check( void)
{
   const char *filenames[2] = { "neocplst.txt", "neocplst.tmp" };
   char buff[2][100];
   size_t i;

   for( i = 0; i < 2; i++)
      {
      FILE *ifile = fopen( filenames[i], "rb");

      if( !ifile)     /* assume this means files got removed;  a simple */
         return( 1);  /* header check won't suffice */
      while( fgets( buff[i], sizeof( buff[i]), ifile) &&
                        memcmp( buff[i], "Last-Modified: ", 15))
         ;
      fclose( ifile);
      printf( "  %s: %s", (i ? "After " : "Before"), buff[i]);
      }
   return( strcmp( buff[0], buff[1]));
}
#endif

/* Our procedure is as follows :

   -- Download the HEAD data for the NEOCP text summary.  It contains the
         date/time that the file was modified.  If it matches the date/time
         from the previous run (stored in 'neocplst.txt'),  nothing has
         changed on NEOCP and our work here is done;  we can quit.
   -- Otherwise,  download the new 'neocplst.txt',  with the name 'neocplst.tmp'.
   -- If that works,  compare it to the existing 'neocplst.txt',
         and create 'neocp.new'.
   -- If _that_ works,  merge 'neocp.txt' and 'neocp.new' to create 'neocp.tmp'.
   -- If that works,  look for removed objects;  find their data in 'neocp.txt'
         and append it to 'neocp.old'.
   -- Finally,  unlink 'neocp.txt' and move 'neocp.tmp' there,  and
         unlink 'neocplst.txt' and move 'neocplst.tmp' there.

   The advantage of this is that,  should any of the downloading fail
(due to bad connection,  NEOCP being down,  etc.),  we just give up and
keep the existing neocp.txt and neocplst.txt files.         */

int main( const int argc, const char **argv)
{
    unsigned bytes_read;
    int i;
    const char *neocp_text_summary =
                     "https://www.minorplanetcenter.net/iau/NEO/neocp.txt";

    printf( "Content-type: text/html\n\n");
    avoid_runaway_process( );
    for( i = 1; i < argc; i++)
       if( argv[i][0] == '-')
          switch( argv[i][1])
             {
             default:
                printf( "Command-line option '%s' unknown\n", argv[i]);
                return( 0);
             }

#ifdef CHECK_HEAD
                     /* just get the headers... */
    bytes_read = fetch_a_file( neocp_text_summary, "neocplst.tmp", 2);
    if( !bytes_read)
      {
      printf( "Couldn't get NEOCP summary header data\n");
      return( -2);
      }
    if( !header_check( ))
      {
      printf( "NEOCP summary file is unmodified\n");
      unlink( "neocplst.tmp");
      return( -3);
      }
#endif
    bytes_read = fetch_a_file( neocp_text_summary, "neocplst.tmp", 0);
    printf( "%u objects to load\n", bytes_read / (unsigned)NEOCPLST_LINE_LEN);
            /* file should be an even multiple of NEOCPLST_LINE_LEN bytes long : */
    if( bytes_read % (unsigned)NEOCPLST_LINE_LEN)
       {
       printf( "WARNING: bytes_read = %u\n", bytes_read);
       printf( "File is supposed to be a multiple of %d bytes long.  It isn't.\n",
                           NEOCPLST_LINE_LEN);
       return( -1);
       }

    show_differences( );

            /* If we got here,  everything worked.  So unlink the old */
            /* files and use the new ones :                           */
    unlink( "neocplst.txt");
    rename( "neocplst.tmp", "neocplst.txt");
    unlink( "neocp.txt");
    rename( "neocp.tmp", "neocp.txt");
    return 0;
}

/*
https://scully.cfa.harvard.edu/cgi-bin/showobsorbs.cgi?Obj=UKA774F&obs=y
https://www.minorplanetcenter.net/iau/NEO/ToConfirm.html
https://www.minorplanetcenter.net/iau/NEO/neocp.txt   (plain-text form)
*/
