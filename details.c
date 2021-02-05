#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* MPECs give observatory information (names of observers/measurers
and telescope specifications) in a non-machine friendly form,
such as

Observer details:
I47 Pierre Auger Observatory, Malargue.  Observer M. Masek.  0.3-m f/6.8
    reflector + CCD.
Q62 iTelescope Observatory, Siding Spring.  Observers H. Sato, R. Ligustri, A.
    Valvasori, E. Guido.  Measurers H. Sato, R. Ligustri, A. Valvasori.
    0.51-m f/6.8 astrograph + CCD + f/4.5 focal reducer, 0.43-m f/6.8
    reflector + CCD.

   This code will read an MPEC and convert the above to

COD I47
OBS M. Masek
TEL 0.3-m f/6.8 reflector + CCD

COD Q62
OBS H. Sato, R. Liguistri, A. Valvasori, E. Guido
MEA H. Sato, R. Liguistri, A. Valvasori
TEL 0.51-m f/6.8 astrograph + CCD + f/4.5 focal reducer
TEL 0.43-m f/6.8 reflector + CCD

   This is the form described at
https://www.minorplanetcenter.net/iau/info/ObsDetails.html

   Takes the name of an MPEC as a command-line argument.   */

/* Anything in the input between angle brackets is assumed to be an HTML
tag,  and irrelevant/unwanted.   Strip 'em all out : */

static void remove_html_tags( char *buff)
{
   while( (buff = strchr( buff, '<')) != NULL)
      {
      char *tptr = strchr( buff, '>');

      assert( tptr);
      memmove( buff, tptr + 1, strlen( tptr));
      }
}

/* Telescopes start with the aperture,  in (number)-m form. */

static char *find_telescope( char *buff)
{
   size_t i = 0;

   while( buff[i])
      if( buff[i] <= '9' && buff[i] >= '0' && !memcmp( buff + i + 1, "-m ", 3))
         {
         while( i && buff[i - 1] != ' ')
            i--;
         return( buff + i);
         }
      else
         i++;
   return( NULL);
}

/* Telescope details and observer/measurer name lists usually end
with a '.',  and sometimes a line feed and/or spaces.  */

static void strip_trailing_period_and_whitespace( char *buff)
{
   size_t i = strlen( buff);

   while( i && (buff[i - 1] == '.' || buff[i - 1] <= ' '))
      i--;
   buff[i] = '\0';
}

static void error_exit( const char *error_message)
{
   fprintf( stderr, "%s\n", error_message);
   fprintf( stderr, "'details' requires the name of an MPEC file as a command line\n"
                    "argument.  It will extract observer details and output them in a\n"
                    "more machine-friendly form.\n");
   exit( -1);
}

int main( const int argc, const char **argv)
{
   FILE *ifile = fopen( argv[1], "rb");
   char buff[1000];

   if( argc < 2)
      error_exit( "No input file specified");
   ifile = fopen( argv[1], "rb");
   if( !ifile)
      error_exit( "Input file not found");
   while( fgets( buff, sizeof( buff), ifile))
      if( !memcmp( buff, "Observer details:", 17))
         {
         while( fgets( buff, sizeof( buff), ifile) && *buff >= ' ')
            {
            char tbuff[90], *scope;

            remove_html_tags( buff);
            assert( buff[0] != ' ' && buff[1] != ' ' && buff[2] != ' ');
            assert( buff[3] == ' ');
            while( fgets( tbuff, sizeof( tbuff), ifile) && tbuff[0] == ' ')
               {
               assert( !memcmp( tbuff, "    ", 4));
               assert( tbuff[4] > ' ');
               strcpy( buff + strlen( buff) - 1, tbuff + 3);
               }
            strip_trailing_period_and_whitespace( buff);
            scope = find_telescope( buff);
            if( scope)
               {
               int pass;
               char *tptr;

               printf( "\nCOD %.3s\n", buff);
               scope[-1] = '\0';
               for( pass = 0; pass < 2 && strlen( buff); pass++)
                  {
                  const char *format = (pass ? "OBS %s\n" : "MEA %s\n");

                  tptr = strstr( buff, (pass ? "Observer" : "Measurer"));
                  if( tptr)
                     {
                     *tptr++ = '\0';
                     while( *tptr > ' ')
                        tptr++;
                     while( *tptr == ' ' || *tptr == ',')
                        tptr++;
                     strip_trailing_period_and_whitespace( tptr);
                     while( strlen( tptr) > 70)
                        {
                        size_t i = 70;

                        while( i && tptr[i] != ',')
                           i--;
                        tptr[i] = '\0';
                        printf( format, tptr);
                        tptr += i + 1;
                        while( *tptr == ' ')
                           tptr++;
                        }
                     printf( format, tptr);
                     }
                  }
               while( (tptr = strchr( scope, ',')) != NULL)
                  {
                  *tptr = '\0';
                  printf( "TEL %s\n", scope);
                  scope = tptr + 1;
                  while( *scope == ' ')
                     scope++;
                  }
               printf( "TEL %s\n", scope);
               }
            fseek( ifile, -(long)strlen( tbuff), SEEK_CUR);
            }
         return( 0);
         }
   return( -1);
}
