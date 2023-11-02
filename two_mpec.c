#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

/* Code to combine an MPEC with heliocentric elements with one with
geocentric elements. */

int text_search_and_replace( char *str, const char *oldstr,
                                     const char *newstr)
{
   size_t ilen = strlen( str), rval = 0;
   const size_t oldlen = strlen( oldstr);
   const size_t newlen = strlen( newstr);

   while( ilen >= oldlen)
      if( !memcmp( str, oldstr, oldlen))
         {
         memmove( str + newlen, str + oldlen, ilen - oldlen + 1);
         memcpy( str, newstr, newlen);
         str += newlen;
         ilen -= oldlen;
         rval++;
         }
      else
         {
         str++;
         ilen--;
         }
   return( (int)rval);
}

int main( const int argc, const char **argv)
{
   FILE *ifile1, *ifile2, *ofile;
   char buff[300];

   if( argc < 3)
      {
      printf( "Usage: two_mpec <filename1> <filename2> <filename3>\n"
         "This will combine an heliocentric pseudo-MPEC from filename1 with\n"
         "a geocentric pseudo-MPEC in filename2.  Output goes to filename3\n"
         "if specified,  stdout if it isn't.\n");
      return( -1);
      }
   ifile1 = fopen( argv[1], "rb");
   assert( ifile1);
   ifile2 = fopen( argv[2], "rb");
   assert( ifile2);
   ofile = (argc > 3 ? fopen( argv[3], "wb") : stdout);
   assert( ofile);
   while( !strstr( buff, "mean residual") && fgets( buff, sizeof( buff), ifile1))
      {
      const int changed = text_search_and_replace( buff,
                           "#elements\"> Orbital elements",
                           "#elements\"> Orbital elements (heliocentric)");

      text_search_and_replace( buff, "<b>Orbital elements:",
                                     "<b>Orbital elements (heliocentric):");
      fputs( buff, ofile);
      if( changed)
         fprintf( ofile, "<li> <a href='#elements2'> Orbital elements (geocentric) </a></li>\n");
      }
   while( fgets( buff, sizeof( buff), ifile2) && !strstr( buff, "<b>Orbital elements"))
      ;
   text_search_and_replace( buff, "<b>Orbital elements:",
                                  "<b>Orbital elements (geoocentric):");
   fputs( "<a name='elements2'></a>\n", ofile);
   fputs( buff, ofile);
   while( fgets( buff, sizeof( buff), ifile2))
      fputs( buff, ofile);
   fclose( ifile1);
   fclose( ifile2);
   fclose( ofile);
   return( 0);
}
