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

#include <glib/gspawn.h>

#include "apt-worker-client.h"
#include "apt-worker-proto.h"

int apt_worker_out_fd = -1;
int apt_worker_in_fd = -1;

static void
apt_worker_child_setup (gpointer data)
{
  int *fds = (int *)data;
  close (fds[0]);
  close (fds[1]);
}

void
start_apt_worker ()
{
  int to_fds[2], from_fds[2], child_close_fds[2];
  char to_numbuf[20], from_numbuf[20];
  GError *error = NULL;

  if (pipe (to_fds) < 0 || pipe (from_fds) < 0)
    perror ("pipe");

  apt_worker_out_fd = to_fds[1];
  apt_worker_in_fd = from_fds[0];

  snprintf (to_numbuf, 20, "%d", to_fds[0]);
  snprintf (from_numbuf, 20, "%d", from_fds[1]);

  child_close_fds[0] = to_fds[1];
  child_close_fds[1] = from_fds[0];

  gchar *args[] = {
    "apt-worker",
    to_numbuf, from_numbuf,
    NULL
  };

  if (!g_spawn_async (NULL,
		      args,
		      NULL,
		      GSpawnFlags (G_SPAWN_LEAVE_DESCRIPTORS_OPEN),
		      apt_worker_child_setup,
		      child_close_fds,
		      NULL,
		      NULL))
    {
      fprintf (stderr, "can't spawn apt-worker: %s\n", error->message);
      g_error_free (error);
    }

  close (to_fds[0]);
  close (from_fds[1]);
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

  /* XXX - maybe use streams here for buffering.
   */
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
must_write (void *buf, size_t n)
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
  char *response_data;
  int response_len;
};

/* The entry in this table is used to catch unrecognized commands.
 */
static pending_request pending[APTCMD_MAX+1];

void
call_apt_worker (int cmd, char *data, int len,
		 apt_worker_callback *done_callback,
		 void *done_data)
{
  assert (cmd >= 0 && cmd < APTCMD_MAX);

  if (pending[cmd].done_callback)
    {
      fprintf (stderr, "apt-worker command %d already pending\n", cmd);
      done_callback (cmd, NULL, 0, done_data);
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
  apt_response_header res;
  int cmd;

  if (!must_read (&res, sizeof (res)))
    return;
      
  printf ("got response %d/%d/%d\n", res.cmd, res.seq, res.len);
  cmd = res.cmd;

  if (cmd < 0 || cmd >= APTCMD_MAX)
    {
      printf ("unrecognized command %d\n", res.cmd);
      cmd = APTCMD_MAX;
    }

  if (pending[cmd].response_data == NULL
      || pending[cmd].response_len < res.len)
    {
      if (pending[cmd].response_data)
	delete[] pending[cmd].response_data;
      pending[cmd].response_data = new char[res.len];
    }

  if (!must_read (pending[cmd].response_data, res.len))
    return;

  if (cmd == APTCMD_STATUS)
    {
      if (pending[cmd].done_callback)
	pending[cmd].done_callback (cmd, 
				    pending[cmd].response_data, res.len,
				    pending[cmd].done_data);
      return;
    }

  if (pending[cmd].seq != res.seq)
    {
      printf ("ignoring out of sequence reply.\n");
      return;
    }
  
  apt_worker_callback *done_callback = pending[cmd].done_callback;
  pending[cmd].done_callback = NULL;

  assert (done_callback);
  done_callback (cmd,
		 pending[cmd].response_data, res.len,
		 pending[cmd].done_data);
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
