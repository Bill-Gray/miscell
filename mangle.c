/* Code to 'mangle' e-mail addresses to make it slightly more difficult
for spammers to harvest them.  (Or to mangle other text to make it less
easy for robots to parse it.)  Given an address,  it adds diacritical
marks and uses the bi-directional capabilities of Unicode to generate
a version humans will understand easily,  but spambots will not.
(Unless,  of course,  this becomes a wildly popular method and it
becomes worthwhile for spammers to modify their address harvesting code.
But that seems unlikely.)  See the bottom of

https://www.projectpluto.com

for an example of an address obfuscated in this manner.  You can run
it as,  e.g.,

./mangle youraddress@yourdomain.org 20 > z.htm

to create an HTML file with 20 variants on your address.

There are a variety of other techniques that could certainly be used,
such as inserting invisible characters... I gather scammers have
developed all sorts of tricks in this department;  we could probably
point such weapons both ways.  But this should do for the nonce. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <wchar.h>

/* For each lowercase letter,  a (not necessarily comprehensive) list
of Unicode points that look like that letter,  but with a diacritical
mark.  */

static const unsigned a_remaps[] =
     { 0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0x227, 0 };
static const unsigned c_remaps[] =
      { 0xe7, 0x107, 0x109, 0x10d, 0 };
static const unsigned d_remaps[] =
      { 0x10f, 0x111, 0 };
static const unsigned e_remaps[] =
      { 0xe8, 0xe9, 0xea, 0xeb, 0 };
static const unsigned g_remaps[] =
      { 0x11d, 0x11f, 0x121, 0x123, 0 };
static const unsigned h_remaps[] =
      { 0x125, 0x127, 0x21f, 0 };
static const unsigned i_remaps[] =
      { 0xec, 0xed, 0xee, 0xef, 0x129, 0x12b, 0x12d, 0 };
static const unsigned j_remaps[] =
      { 0x135, 0x249, 0x237, 0 };
static const unsigned k_remaps[] =
      { 0x137, 0xa7a3, 0 };
static const unsigned l_remaps[] =
      { 0x13a, 0x13c, 0x13e,  0 };
static const unsigned n_remaps[] =
      { 0xf1, 0x144, 0x146, 0 };
static const unsigned o_remaps[] =
      { 0xf0, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf8, 0 };
static const unsigned r_remaps[] =
      { 0x157, 0x159, 0x211, 0x213, 0 };
static const unsigned s_remaps[] =
      { 0x15b, 0x15d, 0x15f, 0x161, 0x219, 0 };
static const unsigned t_remaps[] =
      { 0x163, 0x167, 0x21b, 0 };
static const unsigned u_remaps[] =
      { 0xf9, 0xfa, 0xfb, 0xfc,  0 };
static const unsigned w_remaps[] =
      { 0x175, 0 };
static const unsigned y_remaps[] =
      { 0xfd, 0xff, 0x177, 0 };
static const unsigned z_remaps[] =
      { 0x17c, 0x17e, 0x1b6, 0 };

static const unsigned *remaps[] = {
      a_remaps, NULL, c_remaps, d_remaps, e_remaps, NULL, g_remaps,
      h_remaps, i_remaps, j_remaps, k_remaps, l_remaps, NULL, n_remaps,
      o_remaps, NULL, NULL, r_remaps, s_remaps, t_remaps, u_remaps,
      NULL, w_remaps, NULL, y_remaps, z_remaps };

static const char *default_header =
  "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\">\n"
  "<HTML><head>\n"
  "   <TITLE>Mangled e-mail</TITLE>\n"
  "   <META http-equiv=Content-Type content=\"text/html; charset=utf-8\">\n"
  "</HEAD>\n"
  "<BODY>\n"
  "<p>\n";

static void output_mangled_text( const char *text)
{
   int i;

   printf( "&#x202e;");       /* bi-di start marker */
   for( i = strlen( text) - 1; i >= 0; i--)
      {
      const int idx = (int)text[i] - 'a';

      if( idx < 0 || idx > 25)
         printf( "%c&#x3%02x;", text[i], rand( ) % 33);
//       printf( "%c", text[i]);
      else
         {
         if( remaps[idx])
            {
            int n = 0;

            while( remaps[idx][n])
               n++;
            assert( n);
            n = rand( ) % n;
            printf( "&#x%x;", (wchar_t)remaps[idx][n]);
            }
         else           /* add a random combining mark */
            printf( "%c&#x3%02x;", text[i], rand( ) % 33);
         }
      }
   printf( "&#x202c;");     /* bi-di end marker  */
}


int main( const int argc, const char **argv)
{
   int n_repeats = 3;
   const char *text = argv[1];

   srand( time( NULL));
   if( argc > 2)
      {
      printf( "%s", default_header);
      n_repeats = atoi( argv[2]);
      }
   while( n_repeats--)
      {
      output_mangled_text( text);
      if( argc > 2)
         printf( " <br>\n");
      else
         printf( "\n");
      }
   if( argc > 2)
      printf( "\n</p></body></html>\n");
   return( 0);
}
