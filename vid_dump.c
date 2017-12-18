/* Code to reads "raw" video data from /dev/video0 and show it on-screen
as ASCII pseudo-art.  Example output showing the author's handsome visage :

....,.,::::,:,:,::::::;:;:;:;;;;;;;:;:!;!!!!##$$#!#!#!#!!!#!#!##$@@@@@
 , , :.,;:;,:,.:.;.;,,:,;:;,.:.:.:.,,.,,:,,!:$:$:;#;$;#;:$:$:$:;#!@@@@
 ..,.,..:.;.:..:,:,:,,;,;.:..,.,... ,.:,:..,,::#;:$:$:#:;#;#;#;:$$@@@@
 , , , .,.,.,..:.:.;.,:,, .  . , ,.,;:!;!;:!,:.:..;:!:#::#:#:#:;#:;@@@
 ....,. :.:.:..,.:,:,.; , . .,.,,;!!@#@!@!!$!@!$..,.!:#:;#:#;#;#$;$@@@
 , , ,  ,.,.,. :.:.:.,, . ...;,#!@#@@@@$@!;$;@;!..:.:,;::#,#,#,:;:#@@@
 ..,... , , :..,.:,:.., . ,,;$!@#@@@@$@#@!#@!#:!, ,.:.,.,!:!:!:,! ;;@@
 , , , .,...,. :.:.:...   ;:;$!@#@$$@@@$@#!@;$:!,.. . .  ;,#.;..  ,:@@
.,,.,,,,,,,,:,:,::::::,.,:#!@@@@@@@@@@@@@@@@@@##;,,......,#:;.::..!;#@
.,,,,,.,.,,,,,,,,:,:,:,..;!#$@@@@@@@$$@@@$#@$$!#:,..     ..,.. : . ::@
.,,.,,,,,,,,:,,,,,:::,:.,:#!#!#$$@#;:,,.,,$#!!;#;.. . .   ... .:.. ,,@
 ..,.,.,.,,,,,,,.:,:,,,..:.,.,.:;@!;,::::::;:;:;:,,          . :.  ..#
..,.,.,,,,,,,,,,,,,,:,:...:,:;@;:##:;#!;;,;;;!!;#!!,. .       .,,   ,:
.....,.,.,.,,,,,,,,:,,.:.::;.:!;$@@#:!#!!!!#!#!$$#!;.. .       ..   .,
..,.,.,,,.,,,,,,,,,,:,:@!;!!;;!!$@$#@!##$;##$$@@$;!:,.. ,,.     ,   ..
.,,,,,,,.,,:,,,,,,,,,,,:,,:;;!;@#@$##!$@@@@@@@$@!!!;:,:..!:,,,       ,
.,.,,:,.:,:,:,,:,:,:,.:.:.:,;#;##@!;#:!!@#$@$@!#;:;.:.:,,;.. .  .
 ,.,.:..,.,,:,.:.:.:.,,,:,::;#.:,;.  .,:$##@!$;#:::.,.;:,; . .    . .
 ,.,.,,.:.:.:,,:,:,:,.;,:,;,:!;:,,,:!:!;#;;#;#!!:,;,:.;.      ,., : ,
 ,., ,....,.,..:.:.:.,:,,,:,,;:!:;:;!;!;!;:!,;:!,,:,,..
 ... .. , , , .,.,.,. :.: ,.,::!;!;;!:#:!,::::,::,:.:.:
 . . .  . . .  , , .  . . . ., :,:,::::,:,.,.;.:.,:.,.:  , :.
 . . .  . . .  . ...  , . , .,,,,,,.:,:,;,,,,,.,,.:.,.!. ,.:#.
 . . . . . . ........... ......:,,.,,,::,,,,.,,,,,.,,:!; .::@,
. . . ..........,...,.........,.:,:,:::,:,:,:,,,,,,,;!!!,.,.!.
     . . . . . ... . . . . . . ,,;:;;!:;::,,,,...,.:;#!!:   ..:    ,..
  . . ... ....... . . . . . . ..,,:,:,:,,,,,......:!##!!;..        .,.
       . . . . . . .       .     .., , . .     ..:;#!#;!;;;        ...
  . . . .   . . . . . . . . .      ..     ..,,:,;;!###!!!;#;#:      ;!
         . . .       .             ,.. ..,.,::;:;#;#!#;#!;!;!:


   I wrote this mostly as a way to test use of cheap Webcams as hardware
random number generators (HRNGs).  Point one at a lava lamp,  cage with
dice tumbling in it, bird feeder,  or other unpredictable source,  scramble
the resulting stream with a primitive polynomial,  and you have a source of
entropy,  a la LavaRand.  But you need to be sure that the camera is
running correctly and not,  say, returning the same frame over and over, or
just a bunch of zeroes. Thus,  one might display every (say) seventh frame,
scrambling the other six to create "true" random data.   (And scrambling
the seventh as well;  maybe somebody's capturing that data,  but if they
aren't,  it's contributing as many random bits as the other six frames.)

   That seventh frame should be looked at every now and then to verify
the camera is really making random bits.  (One would use the '-f7'
option to accomplish this.)

   I expect to feed the data through a Fortuna-like scheme to get random
numbers.  Fortuna rather cleverly sidesteps the problem that we don't
really know just how "random" the images are.   (If pointed at a cage
with dice or a birdfeeder,  they are "random" to whatever degree an
attacker knows what the dice or birds did.  Plus,  of course, camera
noise.  But quantifying that is nearly impossible.)

   NOTE that this works with a Logitech(R) QuickCam Express and with
some other older cameras I found or scrounged.  But more modern cameras
often don't let you just read raw data from /dev/video0;  for those,
you get a 'couldn't open video stream' error.  This is best suited to
older,  often rather crappy cameras,  which can often be picked up free
or cheap.

   The program defaults to the 320x296 resolution of the Logitech(R)
camera I used for most of my testing.  For a Veo Stingray camera that
I scrounged,  one that produced mostly noise,  I wound up using

./vid_dump -s320,240 -c0,1

   to match its 320x240 resolution and rather weird contrast settings
(I don't think you'll usually need to adjust the contrast,  except for
really crappy or broken cameras).

   Finally,  for reasons I don't quite fathom,  the Logitech(R) camera
leaves out 64 bytes per frame;  this can be adjusted with the '-b'
option,  and I added '-b-64' to the command line for that camera.  */


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <errno.h>

static bool display_as_negative = false;

/* The image is shown using eight ASCII characters (see 'ascii'
below).  Dithering is used to get at least a little more shading. */

static const uint8_t dither_matrix[64] = {
                        0, 32,  8, 40,  2, 34, 10, 42,
                       48, 16, 56, 24, 50, 18, 58, 26,
                       12, 44,  4, 36, 14, 46,  6, 38,
                       60, 28, 52, 20, 62, 30, 54, 22,
                        3, 35, 11, 43,  1, 33,  9, 41,
                       51, 19, 59, 27, 49, 17, 57, 25,
                       15, 47,  7, 39, 13, 45,  5, 37,
                       63, 31, 55, 23, 61, 29, 53, 21 };

/* The faintest 10% of pixels are shown as spaces;  the brightest 5%
are shown as '*'s (adjustable with the -c option).  In between,  we do
a linear stretch.  The following histogramming code figures out those
10%/95% endpoints. */

static int find_histogram_point( const uint8_t *pixels, const int n_pixels,
            const double fraction)
{
   int i, hist[256], sum = 0;
   const int goal = (int)( (double)n_pixels * fraction);

   for( i = 0; i < 256; i++)
      hist[i] = 0;
   for( i = n_pixels; i; i--)
      hist[*pixels++]++;
   for( i = 0; i < 256 && sum < goal; i++)
      sum += hist[i];
   return( i);
}

#define N_POOLS      13
#define POOLSIZE     128
#define POOLMASK    (POOLSIZE - 1)

static uint64_t pool[N_POOLS][POOLSIZE];
static int pool_loc = 0;

/* Stirring is done with the primitive polynomial of the _mix_pool_bytes( )
function from the Linux random.c.  The fact that the pool consists of 64
bit quantities rather than 8 bit quantities means that the logic for the
"twist" is different. */

static void stir_pool( uint64_t *pool)
{
   size_t i;

   for( i = 0; i < POOLSIZE; i++, pool++)
      {
      *pool ^= pool[(i - 104) & POOLMASK]
             ^ pool[(i - 76) & POOLMASK] ^ pool[(i - 51) & POOLMASK]
             ^ pool[(i - 25) & POOLMASK] ^ pool[(i - 1) & POOLMASK];
      *pool ^= (*pool >> 7) ^ (*pool << 13);
      }
}

static void add_entropy( const void *idata, const size_t nbytes)
{
   const uint64_t *iptr = (const uint64_t *)idata;
   size_t n = nbytes / sizeof( uint64_t);

   while( n--)
      {
      pool[pool_loc % N_POOLS][pool_loc / N_POOLS] ^= *iptr++;
      pool_loc++;
      if( pool_loc == N_POOLS * POOLSIZE)
         {
         size_t i;

         for( i = 0; i < N_POOLS; i++)
            stir_pool( pool[i]);
         pool_loc = 0;
         }
      }
}

int main( const int argc, const char **argv)
{
   FILE *ifile = fopen( "/dev/video0", "rb");
   size_t read_size;
   unsigned frame = 0, frame_skip = 1;
   uint8_t *buff;
   int i, c = ' ';
   int byte_shift = -64;
   unsigned xsize = 360, ysize = 296;     /* size of camera image */
   unsigned xscr = 70, yscr = 30;         /* size displayed on screen */
   double contrast_low  = 0.1;      /* 10% of pixels should be black */
   double contrast_high = 0.95;     /* 5% should be full intensity */

   if( !ifile)
      {
      perror( "Couldn't open video stream /dev/video0\n");
      return( -1);
      }
   for( i = 1; i < argc; i++)
      if( argv[i][0] == '-')
         switch( argv[i][1])
            {
            case 'n':
               display_as_negative = true;
               break;
            case 'b':
               byte_shift = (unsigned)atoi( argv[i] + 2);
               break;
            case 'c':
               sscanf( argv[i] + 2, "%lf,%lf", &contrast_low, &contrast_high);
               break;
            case 'f':
               frame_skip = (unsigned)atoi( argv[i] + 2);
               break;
            case 's':
               sscanf( argv[i] + 2, "%u,%u", &xsize, &ysize);
               break;
            case 'x':
               sscanf( argv[i] + 2, "%u,%u", &xscr, &yscr);
               break;
            default:
               printf( "Unrecognized command-line option '%s'\n", argv[i]);
               return( -1);
               break;
            }
   read_size = (size_t)( 3 * xsize * ysize + (unsigned)byte_shift);
   buff = (uint8_t *)calloc( read_size, 1);
   assert( buff);

   while( ++frame && c != 27)
      {
      const size_t bytes_read =
                    fread( buff, sizeof( uint8_t), read_size, ifile);

      if( bytes_read != read_size)
         {
         perror( "Read failure");
         printf( "%u of %u bytes read\n", (unsigned)bytes_read, (unsigned)read_size);
         return( -2);
         }
      if( !(frame % frame_skip))
         {
         unsigned x, y;
         const int min_pix = find_histogram_point(
                         buff, xsize * ysize, contrast_low);
         const int range   = find_histogram_point(
                         buff, xsize * ysize, contrast_high) - min_pix + 1;
         char obuff[300];

         for( y = 0; y < yscr; y++)
            {
            unsigned y_in = y * ysize / yscr;

            for( x = 0; x < xscr; x++)
               {
               unsigned x_in = x * xsize / xscr;
               int ipixel = buff[x_in + y_in * xsize] - min_pix;

               if( ipixel <= 0)
                  obuff[x] = ' ';
               else if( ipixel >= range)
                  obuff[x] = '@';
               else
                  {
                  int oval = ipixel * 8 * 64 / range;
                  const char *ascii = " .,:;!#$@";
                  int idx = oval >> 6;

                  if( (oval & 0x3f) > dither_matrix[(x & 7) | ((y & 7) << 3)])
                     idx++;
                  obuff[x] = ascii[idx];
                  }
               }
            obuff[xscr] = '\0';
            printf( "%s\n", obuff);
            }
         printf( "\nFrame %d; min=%d range=%d\n", frame, min_pix, range);
         }
      }
   return( 0);
}
