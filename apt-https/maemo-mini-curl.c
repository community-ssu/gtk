/*
 * This file is part of apt-https.
 *
 * Copyright (C) 2006 Nokia Corporation.
 *
 * Contact: Marius Vollmer <marius.vollmer@nokia.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 as published by the Free Software Foundation.
 *
 * As a special exception, linking this program with OpenSSL is
 * allowed.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <utime.h>
#include <unistd.h>
#include <string.h>

#include <curl/curl.h>
#include <openssl/ssl.h>

CURL *curl;

static int
ssl_app_verify_callback (X509_STORE_CTX *ctx, void *unused)
{
  // Run maemo-mini-curl-cert-gui to verify the certificate and
  // possibly ask the user what to do.

  char name[] = "/tmp/certXXXXXX";
  int fd;
  FILE *f;
  int kid;
  int status;

  fd = mkstemp (name);
  if (fd < 0)
    goto out;

  if (fchmod (fd, S_IRUSR | S_IRGRP | S_IROTH) < 0)
    goto out;

  f = fdopen (fd, "wb");
  if (f == NULL)
    goto out;
  
  if (i2d_X509_fp (f, ctx->cert) < 0)
    goto out;
  
  if (fclose (f) != 0)
    goto out;

  if ((kid = fork ()) < 0)
    goto out;

  if (kid == 0)
    {
      char *prog = "/usr/bin/maemo-mini-curl-cert-gui";
      execl (prog, prog, name, (char *)NULL);
      perror (prog);
      _exit (2);
    }

  if (waitpid (kid, &status, 0) < 0)
    goto out;

  unlink (name);

  return WIFEXITED (status) && WEXITSTATUS (status) == 0;

 out:
  perror (name);
  return 0;
}

static CURLcode
my_sslctxfun (CURL *curl, void *sslctx, void *parm)
{
  SSL_CTX * ctx = (SSL_CTX *) sslctx ;
  SSL_CTX_set_cert_verify_callback (ctx, ssl_app_verify_callback, NULL);
  return CURLE_OK;
}

int
progress_function (void *data,
		   double t, /* dltotal */
		   double d, /* dlnow */
		   double ultotal,
		   double ulnow)
{
  static int have_started = 0;

  if (!have_started)
    {
      have_started = 1;
      printf ("Size: %lu\n", (unsigned long)t);
    }
  return 0;
}

int
fetch (const char *url, const char *dst)
{
  CURLcode res;
  char errorbuffer[CURL_ERROR_SIZE];
  FILE *f;

  f = fopen (dst, "w");
  if (f == NULL)
    {
      perror (dst);
      return 0;
    }
    
  curl_easy_setopt (curl, CURLOPT_ERRORBUFFER, errorbuffer);

  curl_easy_setopt (curl, CURLOPT_URL, url);
  curl_easy_setopt (curl, CURLOPT_WRITEDATA, f);

  curl_easy_setopt (curl, CURLOPT_NOPROGRESS, 0);
  curl_easy_setopt (curl, CURLOPT_PROGRESSFUNCTION, progress_function);
  curl_easy_setopt (curl, CURLOPT_PROGRESSDATA, NULL);

  curl_easy_setopt (curl, CURLOPT_SSL_CTX_FUNCTION, my_sslctxfun);

  res = curl_easy_perform (curl);

  if (fclose (f) == EOF)
    {
      perror (dst);
      return 0;
    }
    
  if (res)
    {
      fprintf (stderr, "%s\n", errorbuffer);
      unlink (dst);
      return 0;
    }

  return 1;
}

int
main (int argc, char **argv)
{
   curl = curl_easy_init();

   if (curl == NULL)
     {
       fprintf (stderr, "Can't init libcurl\n");
       exit (1);
     }

   if (argc < 4 || strcmp (argv[1], "-o"))
     {
       fprintf (stderr, "usage: maemo-mini-curl -o <dest> <url>\n");
       exit (1);
     }

   if (!fetch (argv[3], argv[2]))
     exit (1);

   exit (0);
}
