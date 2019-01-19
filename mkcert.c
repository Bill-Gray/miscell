#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>

/*  A small bit of code probably only useful to me.

   LetsEncrypt.org requires you to demonstrate your ownership (or at least
control) of a site by putting certain files containing random strings into
the /.well-known/acme-challenge directory of that site.  If you have
shell access,  that can be essentially automatic.  If (like me) you don't,
you have to go through some tedium.  The certificates have a three-month
lifetime,  so you have to do it just often enough to be annoying.  This
bit of code reduces (very slightly) the tedium of the process.

   Each request consists of an 87-byte random string.  You're asked to put
that into the file /.well-known/acme-challenge/(first 43 bytes of that
random string).  This program takes the random string as a command line
argument and makes the file in question,  then uses sftp to upload it to
the correct spot on my ISP's server.

   Ideally,  I'd also automate putting the certificate and key file,
and perhaps the installation.  At present,  I do that using cPanel. */

#define NAME_LEN 44

int main( const int argc, const char **argv)
{
   FILE *ofile;
   char filename[NAME_LEN];
   int rval;

   if( argc != 2 || strlen( argv[1]) != 87)
      {
      printf( "mkcert needs the 87-byte string supplied by LetsEncrypt\n"
              "as a command-line argument.\n");
      return( -1);
      }
   memcpy( filename, argv[1], NAME_LEN - 1);
   filename[NAME_LEN - 1] = '\0';
   ofile = fopen( filename, "wb");
   if( !ofile)
      printf( "Didn't open '%s'\n", filename);
   assert( ofile);
   fprintf( ofile, "%s\n", argv[1]);
   fclose( ofile);
   printf( "File '%s' created\n", filename);

   ofile = fopen( "/tmp/put", "wb");
   assert( ofile);
   fprintf( ofile, "cd /home/projectp/public_html/.well-known/acme-challenge\n");
   fprintf( ofile, "put %s\n", filename);
   fprintf( ofile, "exit\n");
   fclose( ofile);
   rval = system( "sftp -b /tmp/put -P 2900 projectp@projectpluto.com");
   printf( "rval %d\n", rval);
   unlink( filename);
   return( rval);
}
