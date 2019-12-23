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
lifetime,  so you have to do it roughly every 2.5 months.  You run the
'certbot' program;  for each certificate to be renewed,  it gives you a
87-byte random string that must be placed in a file with the name
/.well-known/acme-challenge/(first 43 bytes of the random string).
Then you hit a key and repeat for the next certificate.

   This bit of code reduces (very slightly) the tedium of the process.
It takes the 87-byte random string as a command-line argument.  It
then makes the required file and uploads it to the correct spot on
my ISP's server via sftp.  (You'll presumably have to modify the
server name and output directory,  of course.)

   Ideally,  I'd also automate putting the certificate and key file,
and perhaps the installation,  but I'm a little unclear as to how
to do that.  At present,  I do that using cPanel. */

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
