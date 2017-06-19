#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

/* Code to read in a .csv file,  figure out the field lengths,  and write
out a fixed-field file that is more human-readable and editor-sortable.
Though,  of course,  (usually) much larger.

Only command-line argument is the name of the input .csv file.  Output
is to stdout.        */

static int find_csv_field( char *ibuff)
{
   int i = 0;

   if( *ibuff == '"')
      {
      for( i = 1; ibuff[i] && ibuff[i] != '"'; i++)
         ibuff[i - 1] = ibuff[i];
      if( ibuff[i] == '"')
         {
         ibuff[i - 1] = '\0';
         i++;
         }
      else     /* no matching " found */
         return( -1);
      }
   while( ibuff[i] && ibuff[i] != ',')
      i++;
   if( ibuff[i] == ',')
      ibuff[i++] = '\0';
   return( i);
}

#define MAX_COLS 1000
#define MAX_BUFF_SIZE 10000

int main( const int argc, const char **argv)
{
   FILE *ifile;
   size_t sizes[MAX_COLS];
   int pass, i, j, lines_to_skip = 0;
   char *buff = (char *)malloc( MAX_BUFF_SIZE);

   if( argc < 1)
      {
      printf( "Specify the input file name as a command-line argument\n");
      return( -1);
      }
   ifile = fopen( argv[1], "rb");
   if( !ifile)
      {
      printf( "%s not found\n", argv[1]);
      return( -2);
      }
   for( i = 1; argv[i]; i++)
      if( argv[i][0] == '-')
         switch( argv[i][1])
            {
            case 'n': case 'N':
               lines_to_skip = atoi( argv[i] + 2);
               break;
            default:
               printf( "Argument '%s' not recognized\n", argv[i]);
               return( -3);
            }
   for( i = 0; i < MAX_COLS; i++)
      sizes[i] = 0;
   for( pass = 0; pass < 2; pass++)
      {
      fseek( ifile, 0L, SEEK_SET);
      i = lines_to_skip;
      while( i && fgets( buff, MAX_BUFF_SIZE, ifile))
         i--;
      assert( !i);
      while( fgets( buff, MAX_BUFF_SIZE, ifile))
         {
         int loc = 0;

         for( i = 0; buff[i] != 10 && buff[i] != 13 && buff[i]; i++)
            ;           /* remove trailing CR/LF */
         buff[i] = '\0';

         j = find_csv_field( buff);
         if( j > 0 && buff[j])     /* yes,  it's got at least one field */
            {
            if( pass)
               printf( "%-*s", (int)sizes[0] + 1, buff);
            if( sizes[0] < strlen( buff))
               sizes[0] = strlen( buff);
            loc = j;
            i = 1;
            while( buff[loc])
               {
               j = find_csv_field( buff + loc);
               if( pass)
                  printf( "%-*s", (int)sizes[i] + 1, buff + loc);
               if( sizes[i] < strlen( buff + loc))
                  sizes[i] = strlen( buff + loc);
               loc += j;
               i++;
               assert( i < MAX_COLS);
               }
            if( pass)
               printf( "\n");
//          if( i > n_fields)
//             n_fields = i;
            }
         }
      }
   fclose( ifile);
   return( 0);
}
