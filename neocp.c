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

/* #define CURL_STATICLIB  */
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>
#include <curl/curl.h>
#include <curl/easy.h>
#if defined( __linux__) || defined( __unix__) || defined( __APPLE__)
   #include <sys/time.h>         /* these allow resource limiting */
   #include <sys/resource.h>     /* see 'avoid_runaway_process'   */
#endif

/* Code to "scrape" astrometry from NEOCP,  in as efficient and
error-proof a manner as reasonably possible.  Note that 'neocp2.c' does
basically the same thing,  but in a manner that may be preferable (fewer
accesses to the MPC site and less likely to encounter troubles,  but at
the cost of transferring more bytes;  read on for details.)  The choice
could be argued,  but I'm currently using 'neocp2.c' exclusively.  With
that warning,  here's how 'neocp.c' works :

   We begin by downloading the list of objects currently on NEOCP,

https://www.minorplanetcenter.net/iau/NEO/neocp.txt

   and storing it as 'neocplst.tmp'.  We do some basic format
checking.  If that fails,  we emit an error and bomb out.

    If the file looks good,  we compare it to the previously
downloaded list,  'neocplst.txt',  and look for objects which
have changed (new objects,  or the number of observations or
'added' or 'updated' times have changed).  For each of these
objects,  we download astrometry with a URL of the form

https://minorplanetcenter.net/cgi-bin/showobsorbs.cgi?Obj=A106fgF&obs=y

   with designation suitably changed.  There are some checks run
on this file (see comments above n_valid_astrometry_lines()) to make
sure we got everything we should have gotten.  The downloaded data
goes into 'neocp.new'.

   When we're done,  we read in previously downloaded astrometry
from 'neocp.txt' and merge in the new/updated astrometry from
'neocp.new'. We also take the data for removed objects and add it
to 'neocp.old'.

   If all of this has worked without error,  we should have an
updated list of NEOCP astrometry in 'neocp.tmp' and an updated
object list in 'neocplst.tmp'.  Only then can we safely remove
the old files 'neocp.txt' and 'neocplst.txt',  and rename the
.tmp files to replace them.

   Note that 'neocp2.c' (q.v.) simply downloads a file from MPC
containing all NEOCP astrometry.  That's basically our new 'neocp.txt';
comparison with the previous one allows that code to generate 'neocp.new'
and add removed objects to 'neocp.old'.

   As a result,  only one file is downloaded from NEOCP,  even if
multiple objects have changed.  (With this code,  we'll always grab
the list of objects,  and then make a separate request for data for
each object that has been updated.)  'neocp.c' will be faster and
grab less data if few objects have changed;  'neocp2.c' may be faster
if many objects have changed.  'neocp2.c' will also work even if the
list file is corrupted or out of date,  both of which have happened on
admittedly rare occasions.     */

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
      fprintf( stderr, "Couldn't open %s\n", filename);
      perror( NULL);
      exit( -1);
      }
   return( rval);
}

static char mutant_hex( const int ival)
{
   const char *buff =
        "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

   assert( ival >=0 && ival <= 61);
   return( buff[ival]);
}

/* When we download 'new' NEOCP data,  we check to see if it exists in
the data we've already got.  If it doesn't,  we put a time tag in the
normally unused bytes 58-62 so that in the future,  we'll know when we
first saw it.  This is to help in situations (which happen a fair bit)
where I'm wondering when something first showed up on NEOCP.

If it didn't already exist,  and we've already got that particular
observation,  we copy the time tag into the 'new' data.  */

static void set_time_downloaded( char *iline)
{
   static char *old_lines = NULL;
   size_t i;
   time_t t0;
   struct tm tm;

   if( iline[14] == 's' || iline[14] == 'v')
      return;     /* skip second lines */
   if( !old_lines)
      {
      FILE *ifile = err_fopen( "neocp.txt", "rb");
      long size, bytes_read;

      fseek( ifile, 0L, SEEK_END);
      size = ftell( ifile);
      old_lines = (char *)calloc( size + 1, 1);
      assert( old_lines);
      fseek( ifile, 0L, SEEK_SET);
      bytes_read = fread( old_lines, 1, size, ifile);
      assert( bytes_read == size);
      fclose( ifile);
      old_lines[size] = '\0';
      }
   for( i = 0; old_lines[i]; i++)
      if( !i || old_lines[i - 1] == 10)
         if( !memcmp( old_lines + i, iline, 57) &&
                  !memcmp( old_lines + i + 65, iline + 65, 15))
            {
            memcpy( iline + 59, old_lines + i + 59, 5);
            return;
            }
   t0 = time( NULL);
   gmtime_r( &t0, &tm);
   iline[59] = '~';
   iline[60] = mutant_hex( tm.tm_mon + 1);
   iline[61] = mutant_hex( tm.tm_mday);
   iline[62] = mutant_hex( tm.tm_hour);
   iline[63] = mutant_hex( tm.tm_min);
}

typedef struct
{
   char *obuff;
   size_t loc, max_len;
} curl_buff_t;

size_t curl_buff_write( char *ptr, size_t size, size_t nmemb, void *context_ptr)
{
   curl_buff_t *context = (curl_buff_t *)context_ptr;
   size_t bytes_to_write = size * nmemb;

   if( bytes_to_write > context->max_len - context->loc)
      bytes_to_write = context->max_len - context->loc;
   memcpy( context->obuff + context->loc, ptr, bytes_to_write);
   context->loc += bytes_to_write;
   return( bytes_to_write);
}

static unsigned fetch_a_file( const char *url, char *obuff,
                              const size_t max_len)
{
   CURL *curl = curl_easy_init();
   unsigned rval = 0;

   assert( curl);
   if( curl)
      {
      CURLcode res;
      curl_buff_t context;
      char errbuf[CURL_ERROR_SIZE];

      context.loc = 0;
      context.obuff = obuff;
      context.max_len = max_len;
      curl_easy_setopt( curl, CURLOPT_URL, url);
      curl_easy_setopt( curl, CURLOPT_WRITEFUNCTION, curl_buff_write);
      curl_easy_setopt( curl, CURLOPT_WRITEDATA, &context);
      curl_easy_setopt( curl, CURLOPT_ERRORBUFFER, errbuf);
      *errbuf = '\0';
#ifdef NOT_CURRENTLY_USED
      if( flags & 2)
         {
         curl_easy_setopt( curl, CURLOPT_NOBODY, 1);
         curl_easy_setopt( curl, CURLOPT_HEADER, 1);
         }
#endif
      res = curl_easy_perform( curl);
      if( res)
         {
         fprintf( stderr, "libcurl error %d occurred\n", res);
         fprintf( stderr, "%s\n",
                       (*errbuf ? errbuf : curl_easy_strerror( res)));
         printf( "url %s\n", url);
         exit( -1);
         }
      curl_easy_cleanup( curl);
      rval = (unsigned)context.loc;
      }
   return( rval);
}

/* Lines in the MPC's plaintext summary of which objects are currently on
   NEOCP,  https://www.minorplanetcenter.net/iau/NEO/neocp.txt,  have
   certain fixed traits.  In recent years,  they have always been 102
   bytes long.  (Unless it's been over 99 days since the last observation,
   in which case you get an extra byte.)  We do some further rudimentary
   sanity checking to avoid going too far with bogus data.  */

#define NEOCPLST_LINE_LEN  102

static bool is_valid_neocplst_line( const char *buff)
{
   const size_t len = strlen( buff);
   const bool is_valid = (len >= NEOCPLST_LINE_LEN && len <= NEOCPLST_LINE_LEN + 3
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
      fprintf( stderr, "'%s' not found\n", filename);
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
      else
         {
         fprintf( stderr, "!!! Bad NEOCP list line\n%s\n", buff);
         exit( -1);
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

/* Downloaded NEOCP astrometry should look a bit like this,

<html><body><pre>
     ZBB8B17  C2018 01 26.46759 12 09 51.81 +35 20 39.3          19.6 GUNEOCPG96
     ZBB8B17  C2018 01 26.47279 12 09 51.51 +35 20 41.9          20.4 GUNEOCPG96
     ZBB8B17  C2018 01 26.48318 12 09 51.18 +35 20 47.2          21.2 GUNEOCPG96
</pre></body></html>

   with a line feed between lines.  The header and trailer line should be
exactly as shown,  with intervening lines 80 bytes long plus a LF.   If it's
not like that,  something's wrong,  and a warning message is emitted.

   Satellite observations and roving observations will consume two lines,
so we do a little checking to make sure we're not looking at a second line
when counting observations.   */

#define CR_IN_WRONG_PLACE                        1
#define CR_NOT_FOUND_WHERE_IT_SHOULD_HAVE_BEEN   2
#define WRONG_HEADER                             4
#define WRONG_TRAILER                            8

static bool is_valid_astrometry_line( const char *buff)
{
   bool is_mpc_line = true;

   if( buff[80] == 10)
      {
      size_t j;
      const char *example_line =
                  "0000 00 00.00000 00 00 00.0   00 00 00.0";

      for( j = 0; example_line[j] && is_mpc_line; j++)
         if( example_line[j] == '0' && !isdigit( buff[j + 15]))
            is_mpc_line = false;
      }
   else
      is_mpc_line = false;
   return( is_mpc_line);
}

static unsigned n_valid_astrometry_lines( const char *buff)
{
   size_t i, prev = 0;
   unsigned n_lines_found = 0;
   const char *header = "<html><body><pre>\n";
   const char *trailer = "</pre></body></html>\n";
   const size_t header_len = 18;
   const size_t trailer_len = 21;
   int errors_found = 0;

   for( i = 0; buff[i]; i++)
      if( buff[i] == 10)
         {
         if( i == prev + 81)     /* yes,  it's an 80-column line */
            if( is_valid_astrometry_line( buff + prev + 1))
               n_lines_found++;
         prev = i;
         if( (i % 81) != header_len - 1 && buff[i + 1])
            errors_found |= CR_IN_WRONG_PLACE;
         }
      else if( (i % 81) == header_len - 1)
         errors_found |= CR_NOT_FOUND_WHERE_IT_SHOULD_HAVE_BEEN;

   if( i < header_len || memcmp( buff, header, header_len))
      errors_found |= WRONG_HEADER;
   if( i < trailer_len || memcmp( buff + i - trailer_len, trailer, trailer_len))
      errors_found |= WRONG_HEADER;
   if( errors_found)
      fprintf( stderr, "!!! Errors %d (see 'neocp.c' for meaning)\n", errors_found);
   return( n_lines_found);
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

#define MAX_ILEN 81000

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
      char *tbuff = (char *)malloc( MAX_ILEN);
      FILE *new_fp;

      printf( "New/changed objects :\n");
      new_fp = NULL;
      for( i = j = 0; i < n_after; i++)
         if( !after[i].exists_in_other_list)
            {
            char url[200];
            unsigned bytes_read, n_obs_previously = 0, k;
            unsigned n_lines_actually_read;

            for( k = 0; k < n_before; k++)
               if( !strcmp( before[k].desig, after[i].desig))
                  n_obs_previously = before[k].n_obs;
            printf( "   (%u) %s: %u obs (was %u)\n", ++j, after[i].desig,
                                after[i].n_obs, n_obs_previously);
            strcpy( url, "https://minorplanetcenter.net/cgi-bin/showobsorbs.cgi?Obj=");
            strcat( url, after[i].desig);
            strcat( url, "&obs=y");
            bytes_read = fetch_a_file( url, tbuff, MAX_ILEN - 1);
            if( bytes_read < 79)
               {
               fprintf( stderr, "ERROR: only %u bytes read\n", bytes_read);
               fprintf( stderr, "See 'neocp.c' for meaning\n");
               exit( -1);
               }
            tbuff[bytes_read] = '\0';
            n_lines_actually_read = n_valid_astrometry_lines( tbuff);
            if( n_lines_actually_read != after[i].n_obs)
               printf( "!!! %u obs read\n", n_lines_actually_read);
                        /* Note that differences between the number of */
                        /* lines reported in neocplst.txt and the number */
                        /* actually read are common and not a concern.   */
            for( k = 0; k < bytes_read - 79; k++)
               if( !k || tbuff[k - 1] == 10)
                  if( is_valid_astrometry_line( tbuff + k))
                     set_time_downloaded( tbuff + k);
            if( bytes_read)
               {
               if( !new_fp)
                  new_fp = fopen( "neocp.new", "wb");
               assert( new_fp);
               fwrite( tbuff, bytes_read, 1, new_fp);
               }
            }
      if( new_fp)
         fclose( new_fp);

      free( tbuff);
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
    FILE *ofile;
    char *tbuff;
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
                     /* just get the headers... not doing this at present */
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
    tbuff = (char *)malloc( MAX_ILEN);
    bytes_read = fetch_a_file( neocp_text_summary, tbuff, MAX_ILEN);
    printf( "%u objects to load\n", bytes_read / (unsigned)NEOCPLST_LINE_LEN);
    ofile = fopen( "neocplst.tmp", "wb");
    assert( ofile);
    fwrite( tbuff, bytes_read, 1, ofile);
    fclose( ofile);
    show_differences( );

            /* If we got here,  everything worked.  So unlink the old */
            /* files and use the new ones :                           */
    unlink( "neocplst.txt");
    rename( "neocplst.tmp", "neocplst.txt");
    unlink( "neocp.txt");
    rename( "neocp.tmp", "neocp.txt");
    free( tbuff);
    return 0;
}

/*
https://scully.cfa.harvard.edu/cgi-bin/showobsorbs.cgi?Obj=UKA774F&obs=y
https://www.minorplanetcenter.net/iau/NEO/ToConfirm.html
https://www.minorplanetcenter.net/iau/NEO/neocp.txt   (plain-text form)
*/
