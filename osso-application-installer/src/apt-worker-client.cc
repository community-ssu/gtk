/*
 * This file is part of osso-application-installer
 *
 * Copyright (C) 2005 Nokia Corporation.
 *
 * Contact: Marius Vollmer <marius.vollmer@nokia.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
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

#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <errno.h>

#include <gtk/gtk.h>
#include <glib/gspawn.h>

#include "log.h"
#include "util.h"
#include "apt-worker-client.h"
#include "apt-worker-proto.h"

int apt_worker_out_fd = -1;
int apt_worker_in_fd = -1;

static GString *pmstatus_line;

static void
interpret_pmstatus (char *str)
{
  float percentage;
  char *title;

  if (strncmp (str, "pmstatus:", 9) != 0)
    return;
  str += 9;
  str = strchr (str, ':');
  if (str == NULL)
    return;
  str += 1;
  percentage = atof (str);
  str = strchr (str, ':');
  if (str == NULL)
    title = "Working";
  else
    {
      str += 1;
      title = str;
    }
	
  // printf ("STATUS: %3d %s\n", int (percentage), title);
  set_progress (title, percentage/100.0);
}

static gboolean
read_pmstatus (GIOChannel *channel, GIOCondition cond, gpointer data)
{
  char buf[256], *line_end;
  int n, fd = g_io_channel_unix_get_fd (channel);

  n = read (fd, buf, 256);
  if (n > 0)
    {
      g_string_append_len (pmstatus_line, buf, n);
      while ((line_end = strchr (pmstatus_line->str, '\n')))
	{
	  *line_end = '\0';
	  interpret_pmstatus (pmstatus_line->str);
	  g_string_erase (pmstatus_line, 0, line_end - pmstatus_line->str + 1);
	}
      return TRUE;
    }
  else
    {
      g_io_channel_shutdown (channel, 0, NULL);
      return FALSE;
    }
}

static void
setup_pmstatus_from_fd (int fd)
{
  pmstatus_line = g_string_new ("");
  GIOChannel *channel = g_io_channel_unix_new (fd);
  g_io_add_watch (channel, GIOCondition (G_IO_IN | G_IO_HUP | G_IO_ERR),
		  read_pmstatus, NULL);
  g_io_channel_unref (channel);
}

static void
must_mkfifo (char *filename, int mode)
{
  if (mkfifo (filename, mode) < 0 && errno != EEXIST)
    {
      perror (filename);
      exit (1);
    }
}

static int
must_open (char *filename, int flags)
{
  int fd = open (filename, flags);
  if (fd < 0)
    {
      perror (filename);
      exit (1);
    }
}

static void
apt_worker_child_setup (gpointer data)
{
  int *fds = (int *)data;
  close (fds[0]);
  close (fds[1]);
  close (fds[2]);
}

void
start_apt_worker (gchar *prog)
{
  int stdout_fd, stderr_fd, status_fd;
  GError *error = NULL;
  gchar *sudo;

  // XXX - be more careful with the /tmp files.

  must_mkfifo ("/tmp/apt-worker.to", 0600);
  must_mkfifo ("/tmp/apt-worker.from", 0600);
  must_mkfifo ("/tmp/apt-worker.status", 0600);

  struct stat info;
  if (stat ("/targets/links/scratchbox.config", &info))
    sudo = "/usr/bin/sudo";
  else
    sudo = "/usr/bin/fakeroot";

  gchar *args[] = {
    sudo,
    prog,
    "/tmp/apt-worker.to", "/tmp/apt-worker.from", "/tmp/apt-worker.status",
    NULL
  };

  // XXX - a non-starting apt-worker should not be fatal.

  if (!g_spawn_async_with_pipes (NULL,
				 args,
				 NULL,
				 GSpawnFlags (G_SPAWN_LEAVE_DESCRIPTORS_OPEN),
				 NULL,
				 NULL,
				 NULL,
				 NULL,
				 &stdout_fd,
				 &stderr_fd,
				 &error))
    {
      fprintf (stderr, "can't spawn %s: %s\n", prog, error->message);
      g_error_free (error);
      exit (1);
    }

  // The order here is important and must be the same as in apt-worker
  // to avoid a dead lock.
  //
  // XXX - we should open the fifo with O_NONBLOCK to deal
  //       with apt-worker startup failures.

  apt_worker_out_fd = must_open ("/tmp/apt-worker.to", O_WRONLY);
  apt_worker_in_fd = must_open ("/tmp/apt-worker.from", O_RDONLY);
  status_fd = must_open ("/tmp/apt-worker.status", O_RDONLY);

  log_from_fd (stdout_fd);
  log_from_fd (stderr_fd);
  setup_pmstatus_from_fd (status_fd);
}

void
stop_apt_worker ()
{
  if (apt_worker_out_fd >= 0)
    {
      if (close (apt_worker_out_fd) < 0)
	perror ("close");
    }
  apt_worker_out_fd = -1;
  apt_worker_in_fd = -1;
}

static bool
must_read (void *buf, size_t n)
{
  int r;

  while (n > 0)
    {
      r = read (apt_worker_in_fd, buf, n);
      if (r < 0)
	{
	  perror ("read");
	  stop_apt_worker ();
	  return false;
	}
      else if (r == 0)
	{
	  printf ("apt-worker exited.\n");
	  stop_apt_worker ();
	  return false;
	}
      n -= r;
      buf = ((char *)buf) + r;
    }
  return true;
}

static void
must_write (void *buf, int n)
{
  if (write (apt_worker_out_fd, buf, n) != n)
    perror ("client write");
}

bool
apt_worker_is_running ()
{
  return apt_worker_out_fd > 0;
}

void
send_apt_worker_request (int cmd, int seq, char *data, int len)
{
  apt_request_header req = { cmd, seq, len };
  must_write (&req, sizeof (req));
  must_write (data, len);
}

static int
next_seq ()
{
  static int seq;
  return seq++;
}

struct pending_request {
  int seq;
  apt_worker_callback *done_callback;
  void *done_data;
};

static pending_request pending[APTCMD_MAX];

void
call_apt_worker (int cmd, char *data, int len,
		 apt_worker_callback *done_callback,
		 void *done_data)
{
  assert (cmd >= 0 && cmd < APTCMD_MAX);

  if (pending[cmd].done_callback)
    {
      fprintf (stderr, "apt-worker command %d already pending\n", cmd);
      return;
    }
  else
    {
      pending[cmd].seq = next_seq ();
      pending[cmd].done_callback = done_callback;
      pending[cmd].done_data = done_data;
      send_apt_worker_request (cmd, pending[cmd].seq, data, len);
    }
}

void
handle_one_apt_worker_response ()
{
  static bool running = false;

  static apt_response_header res;
  static char *response_data = NULL;
  static int response_len = 0;
  static apt_proto_decoder dec;

  int cmd;

  assert (!running);
  running = true;
    
  if (!must_read (&res, sizeof (res)))
    return;
      
  //printf ("got response %d/%d/%d\n", res.cmd, res.seq, res.len);
  cmd = res.cmd;

  if (response_len < res.len)
    {
      if (response_data)
	delete[] response_data;
      response_data = new char[res.len];
      response_len = res.len;
    }

  if (!must_read (response_data, res.len))
    {
      running = false;
      return;
    }

  if (cmd < 0 || cmd >= APTCMD_MAX)
    {
      fprintf (stderr, "unrecognized command %d\n", res.cmd);
      running = false;
      return;
    }

  dec.reset (response_data, res.len);

  if (cmd == APTCMD_STATUS)
    {
      if (pending[cmd].done_callback)
	pending[cmd].done_callback (cmd, &dec, pending[cmd].done_data);
      running = false;
      return;
    }

  if (pending[cmd].seq != res.seq)
    {
      fprintf (stderr, "ignoring out of sequence reply.\n");
      running = false;
      return;
    }
  
  apt_worker_callback *done_callback = pending[cmd].done_callback;
  pending[cmd].done_callback = NULL;

  assert (done_callback);
  done_callback (cmd, &dec, pending[cmd].done_data);

  running = false;
}

static apt_proto_encoder request;

void
apt_worker_set_status_callback (apt_worker_callback *callback, void *data)
{
  pending[APTCMD_STATUS].done_callback = callback;
  pending[APTCMD_STATUS].done_data = data;
}

void
apt_worker_get_package_list (apt_worker_callback *callback, void *data)
{
  call_apt_worker (APTCMD_GET_PACKAGE_LIST, NULL, 0,
		   callback, data);
}

void
apt_worker_update_cache (apt_worker_callback *callback, void *data)
{
  call_apt_worker (APTCMD_UPDATE_PACKAGE_CACHE, NULL, 0,
		   callback, data);
}

void
apt_worker_get_package_info (const char *package,
			     apt_worker_callback *callback, void *data)
{
  request.reset ();
  request.encode_string (package);
  call_apt_worker (APTCMD_GET_PACKAGE_INFO,
		   request.get_buf (), request.get_len (),
		   callback, data);
}

void
apt_worker_get_package_details (const char *package,
				const char *version,
				int summary_kind,
				apt_worker_callback *callback,
				void *data)
{
  request.reset ();
  request.encode_string (package);
  request.encode_string (version);
  request.encode_int (summary_kind);
  call_apt_worker (APTCMD_GET_PACKAGE_DETAILS,
		   request.get_buf (), request.get_len (),
		   callback, data);
}

void
apt_worker_install_prepare (const char *package,
			    apt_worker_callback *callback, void *data)
{
  request.reset ();
  request.encode_string (package);
  call_apt_worker (APTCMD_INSTALL_PREPARE,
		   request.get_buf (), request.get_len (),
		   callback, data);
}

void
apt_worker_install_doit (apt_worker_callback *callback, void *data)
{
  call_apt_worker (APTCMD_INSTALL_DOIT, NULL, 0,
		   callback, data);
}

void
apt_worker_remove_package (const char *package,
			   apt_worker_callback *callback, void *data)
{
  request.reset ();
  request.encode_string (package);
  call_apt_worker (APTCMD_REMOVE_PACKAGE,
		   request.get_buf (), request.get_len (),
		   callback, data);
}
