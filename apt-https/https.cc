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

#include <apt-pkg/fileutl.h>
#include <apt-pkg/acquire-method.h>
#include <apt-pkg/error.h>

#include <sys/stat.h>
#include <utime.h>
#include <unistd.h>
#include <iostream>

#include <glib.h>

class CurlMethod : public pkgAcqMethod
{
  virtual bool Fetch(FetchItem *Itm);

  FetchResult result;
  bool success;
  GMainLoop *loop;
  GString *size_string;
  
  void reap_process (int status);
  void read_stdout (gchar *buf, gsize n);

  bool run_cmd (const char **argv);

  static void reap_process_trampoline (GPid pid, int status, gpointer data);
  static gboolean read_stdout_trampoline (GIOChannel *channel,
					  GIOCondition cond, gpointer data);

public:
  
  CurlMethod() : pkgAcqMethod("1.0",SingleInstance) {};
};

void
CurlMethod::reap_process_trampoline (GPid pid, int status, gpointer data)
{
  CurlMethod *meth = (CurlMethod *)data;
  meth->reap_process (status);
}

void
CurlMethod::reap_process (int status)
{
  // fprintf (stderr, "[ exit %d ]\n", status);
  if (WIFEXITED (status) && WEXITSTATUS (status) == 0)
    success = true;
  g_main_loop_quit (loop);
}

gboolean
CurlMethod::read_stdout_trampoline (GIOChannel *channel,
				    GIOCondition cond, gpointer data)
{
  CurlMethod *meth = (CurlMethod *)data;
  gchar buf[256];
  gsize count;
  GIOStatus status;
  
#if 0
  /* XXX - this blocks sometime.  Maybe setting the encoding to NULL
           will work, but for now we just do it the old school way...
  */
  status = g_io_channel_read_chars (channel, buf, 256, &count, NULL);
#else
  {
    int fd = g_io_channel_unix_get_fd (channel);
    int n = read (fd, buf, 256);
    if (n > 0)
      {
	status = G_IO_STATUS_NORMAL;
	count = n;
      }
    else
      {
	status = G_IO_STATUS_EOF;
	count = 0;
      }
  }
#endif

  if (status == G_IO_STATUS_NORMAL)
    {
      meth->read_stdout (buf, count);
      return TRUE;
    }
  else
    {
      g_io_channel_shutdown (channel, 0, NULL);
      return FALSE;
    }
}

void
CurlMethod::read_stdout (gchar *buf, gsize n)
{
  char *line_end;
  
  g_string_append_len (size_string, buf, n);
  while ((line_end = strchr (size_string->str, '\n')) != NULL)
    {
      *line_end = '\0';
      if (g_str_has_prefix (size_string->str, "Size:"))
	{
	  result.Size = strtoul (size_string->str + 5, NULL, 10);
	  URIStart (result);
	}
      g_string_erase (size_string, 0, line_end - size_string->str + 1);
    }
}

bool
CurlMethod::run_cmd (const char **argv)
{
  int stdout_fd, stderr_fd;
  GError *error = NULL;
  GPid child_pid;
  
  // fprintf (stderr, "[ %s ]\n", argv[0]);

  if (!g_spawn_async_with_pipes (NULL,
				 (gchar **)argv,
				 NULL,
				 GSpawnFlags (G_SPAWN_DO_NOT_REAP_CHILD),
				 NULL,
				 NULL,
				 &child_pid,
				 NULL,
				 &stdout_fd,
				 NULL,
				 &error))
    {
      _error->Error ("Can't run %s: %s\n", argv[0], error->message);
      g_error_free (error);
      return false;
    }

  success = false;
  size_string = g_string_new ("");
  loop = g_main_loop_new (NULL, 0);

  GIOChannel *channel = g_io_channel_unix_new (stdout_fd);
  g_io_add_watch (channel, GIOCondition (G_IO_IN | G_IO_HUP | G_IO_ERR),
		  read_stdout_trampoline, this);
  g_io_channel_unref (channel);
  g_child_watch_add (child_pid, reap_process_trampoline, this);
  g_main_loop_run (loop);
  return success;
}

bool
CurlMethod::Fetch (FetchItem *Itm)
{
  // fprintf (stderr, "curling %s\n", Itm->Uri.c_str ());

  result.Size = 0;
  result.Filename = Itm->DestFile;
  result.LastModified = 0;
  result.IMSHit = false;
  URIStart (result);

  const char *prog = "/usr/bin/maemo-mini-curl";
  if (access (prog, X_OK) != 0
      && access ("/usr/bin/curl", X_OK) == 0)
    prog = "/usr/bin/curl";

  const char *argv[] = {
    prog,
    "-o", Itm->DestFile.c_str (),
    Itm->Uri.c_str (),
    NULL
  };

  if (!run_cmd (argv))
    return false;

  struct stat Buf;
  if (stat(Itm->DestFile.c_str(),&Buf) != 0)
    return _error->Errno("stat","Failed to stat");

  result.Size = Buf.st_size;
  URIDone (result);

  return true;
}

int main()
{
   setlocale(LC_ALL, "");
     
   CurlMethod Mth;
   return Mth.Run();
}
