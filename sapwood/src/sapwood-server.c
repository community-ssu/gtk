/* GTK+ Sapwood Engine
 * Copyright (C) 2005 Nokia Corporation
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Written by Tommi Komulainen <tommi.komulainen@nokia.com>
 */
#include <config.h>

#include "sapwood-proto.h"

#include <gdk/gdk.h>
#include <gdk/gdkx.h>

#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#ifdef ENABLE_DEBUG
gboolean enable_debug = FALSE;

#define LOG(...) G_STMT_START{				  \
  if (enable_debug)					  \
    g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, __VA_ARGS__); \
}G_STMT_END

#else
#define LOG(...)
#endif

static GMainLoop *main_loop;
static GCache *pixmap_cache   = NULL;
static int     pixmap_counter = 0;
static int     pixbuf_counter = 0;

static const char *sock_path;

#ifndef HAVE_ABSTRACT_SOCKETS
static void
atexit_handler (void)
{
  unlink (sock_path);
}
#endif

static void
signal_handler (int signum)
{
  switch (signum)
    {
    case SIGHUP:
      break;
    case SIGINT:
    case SIGTERM:
      g_main_loop_quit (main_loop);
      break;
    }
}

static void
extract_pixmap_single (GdkPixbuf  *pixbuf,
		       int i, int j,
		       int x, int y,
		       int width, int height,
		       PixbufOpenResponse *rep)
{
  GdkPixmap    *pixmap;
  static GdkGC *tmp_gc = NULL;
  gboolean      need_mask;

  pixmap = gdk_pixmap_new (NULL, width, height, 16);

  if (!tmp_gc)
    tmp_gc = gdk_gc_new (pixmap);

  gdk_draw_pixbuf (pixmap, tmp_gc, pixbuf,
		   x, y, 0, 0,
		   width, height,
		   GDK_RGB_DITHER_NORMAL,
		   0, 0);

  need_mask = gdk_pixbuf_get_has_alpha (pixbuf);
  /* FIXME: if the mask would still be all ones, skip creating it altogether */

  if (need_mask)
    {
      GdkBitmap *pixmask;

      pixmask = gdk_pixmap_new (NULL, width, height, 1);
      gdk_pixbuf_render_threshold_alpha (pixbuf, pixmask,
					 x, y, 0, 0,
					 width, height,
					 128);

      rep->pixmask[i][j] = GDK_PIXMAP_XID (pixmask);
      pixmap_counter++;
    }

  rep->pixmap[i][j] = GDK_PIXMAP_XID (pixmap);
  pixmap_counter++;
}

static gboolean
extract_pixmaps (GdkPixbuf *pixbuf, const PixbufOpenRequest *req, PixbufOpenResponse *rep, GError **err)
{
  int i, j;
  gint width  = gdk_pixbuf_get_width (pixbuf);
  gint height = gdk_pixbuf_get_height (pixbuf);

  if (req->border_left + req->border_right > width ||
      req->border_top + req->border_bottom > height)
    {
      g_warning ("Invalid borders specified for theme pixmap:\n"
		 "        %s,\n"
		 "borders don't fit within the image",
		 g_basename (req->filename));
#if 0
      if (req->border_left + req->border_right > width)
	{
	  req->border_left = width / 2;
	  req->border_right = (width + 1) / 2;
	}
      if (req->border_bottom + req->border_top > height)
	{
	  req->border_top = height / 2;
	  req->border_bottom = (height + 1) / 2;
	}
#endif
    }
  else if (req->border_left == width - req->border_right ||
	   req->border_top == height - req->border_bottom)
    {
      g_warning ("Invalid borders specified for theme pixmap:\n"
		 "        %s,\n"
		 "borders are set for gradients",
		 g_basename (req->filename));
    }

  for (i = 0; i < 3; i++)
    {
      gint y0, y1;

      switch (i)
	{
	case 0:
	  y0 = 0;
	  y1 = req->border_top;
	  break;
	case 1:
	  y0 = req->border_top;
	  y1 = height - req->border_bottom;
	  break;
	default:
	  y0 = height - req->border_bottom;
	  y1 = height;
	  break;
	}
      
      for (j = 0; j < 3; j++)
	{
	  gint x0, x1;
	  
	  switch (j)
	    {
	    case 0:
	      x0 = 0;
	      x1 = req->border_left;
	      break;
	    case 1:
	      x0 = req->border_left;
	      x1 = width - req->border_right;
	      break;
	    default:
	      x0 = width - req->border_right;
	      x1 = width;
	      break;
	    }

	  if (x1-x0 > 0 && y1-y0 > 0)
	    {
	      extract_pixmap_single (pixbuf,
				     i, j,
				     x0, y0,
				     x1-x0, y1-y0,
				     rep);
	    }
	}
    }

  /* make sure the server has the pixmaps before the client */
  gdk_flush ();

  rep->width  = width;
  rep->height = height;

  pixbuf_counter++;
#ifdef DEBUG
  for (i = 0; i < 3; i++)
    {
      char buf[4] = { '\0', };
      for (j = 0; j < 3; j++)
	buf[j] = rep->pixmap[i][j] ? 'X' : '.';
      LOG ("  %s", buf);
    }

  LOG ("pixmaps: %d (%d)", pixmap_counter, pixbuf_counter);
#endif

  return TRUE;
}

static PixbufOpenResponse *
pixbuf_open_response_new (PixbufOpenRequest *req)
{
  PixbufOpenResponse *rep = NULL;
  GdkPixbuf         *pixbuf;
  GError            *err = NULL;

  pixbuf = gdk_pixbuf_new_from_file (req->filename, &err);
  if (pixbuf)
    {
      rep = g_new0 (PixbufOpenResponse, 1);
      rep->id = GPOINTER_TO_UINT(rep);

      if (!extract_pixmaps (pixbuf, req, rep, &err))
	{
	  g_free (rep);
	  rep = NULL;
	}
      g_object_unref (pixbuf);
    }

  if (!rep)
    {
      g_warning ("%s: %s", req->filename, err->message);
      g_error_free (err);
      return NULL;
    }

  return rep;
}

static void
pixbuf_open_response_destroy (PixbufOpenResponse *rep)
{
  GdkPixmap *pixmap;
  int        i, j;

  if (!rep)
    return;

  for (i = 0; i < 3; i++)
    for (j = 0; j < 3; j++)
      {
	if (rep->pixmap[i][j])
	  {
	    pixmap = gdk_xid_table_lookup (rep->pixmap[i][j]);
	    g_object_unref (pixmap);
	    rep->pixmap[i][j] = None;

	    pixmap_counter--;
	  }

	if (rep->pixmask[i][j])
	  {
	    pixmap = gdk_xid_table_lookup (rep->pixmask[i][j]);
	    g_object_unref (pixmap);
	    rep->pixmask[i][j] = None;

	    pixmap_counter--;
	  }
      }

  pixbuf_counter--;

  g_free (rep);
}

static PixbufOpenRequest *
pixbuf_open_request_dup (const PixbufOpenRequest *req)
{
  return g_memdup (req, sizeof (*req) + strlen (req->filename) + 1);
}

static void
pixbuf_open_request_destroy (PixbufOpenRequest *req)
{
  g_free (req);
}

static guint
pixbuf_open_request_hash (gconstpointer key)
{
  const PixbufOpenRequest *req = key;
  return g_str_hash (req->filename);
}

static gboolean
pixbuf_open_request_equal (gconstpointer a, gconstpointer b)
{
  const PixbufOpenRequest *ra = a;
  const PixbufOpenRequest *rb = b;

  if (!g_str_equal (ra->filename, rb->filename))
    return FALSE;

  return TRUE;
}

static ssize_t
process_buffer (int fd, char *buf, ssize_t buflen, gpointer user_data)
{
  GHashTable *cleanup = user_data;
  const PixbufBaseRequest *base = (const PixbufBaseRequest *) buf;
  ssize_t n;

  if (buflen < sizeof (PixbufBaseRequest) || buflen < base->length)
    return 0;

  if (base->op == PIXBUF_OP_OPEN)
    {
      PixbufOpenRequest  *req = (PixbufOpenRequest *) base;
      PixbufOpenResponse *rep;

      if (base->length < sizeof (PixbufOpenRequest) + 1)
	{
	  char buf[1] = { '-' };

	  write (fd, buf, 1);

	  g_warning ("short request, only %d bytes, expected at least %d",
		     base->length, sizeof (PixbufOpenRequest) + 1);
	  return -1;
	}

      LOG ("filename: '%s'", req->filename);

      rep = g_cache_insert (pixmap_cache, req);
      if (rep)
	{
	  g_hash_table_insert (cleanup, GUINT_TO_POINTER(rep->id), rep);

	  /* write reply */
	  n = write (fd, rep, sizeof (*rep));
	  if (n < 0)
	    {
	      g_warning ("write: %s", strerror (errno));
	    }
	  else if (n < sizeof (*rep))
	    {
	      g_warning ("short write, wrote only %d of %d bytes", n, sizeof (*rep)); 
	    }
	}
      else
	{
	  char buf[1] = { '-' };

	  write (fd, buf, 1);

	  g_cache_remove (pixmap_cache, rep);
	}
    }
  else if (base->op == PIXBUF_OP_CLOSE)
    {
      PixbufCloseRequest *req = (PixbufCloseRequest *) base;

      if (base->length < sizeof (PixbufCloseRequest))
	{
	  g_warning ("short request, only %d bytes, expected %d",
		     base->length, sizeof (PixbufCloseRequest));
	  return -1;
	}

      if (!g_hash_table_remove (cleanup, GUINT_TO_POINTER(req->id)))
	g_warning ("close failed, no such pixmap");

      /* no reply */
    }
  else
    {
      g_warning ("unknown opcode: %d", base->op);
    }

  return base->length;
}

static gboolean
client_sock_callback (GIOChannel   *channel,
		      GIOCondition  cond,
		      gpointer      user_data)
{
  int         fd;
  static char buf[sizeof (PixbufOpenRequest) + PATH_MAX + 1];
  static int  buflen = 0;
  ssize_t     n, ofs;

  if (cond & (G_IO_HUP | G_IO_ERR))
    {
      if (cond & G_IO_ERR)
	g_warning ("client error");
      return FALSE;
    }

  /* read request */
  fd = g_io_channel_unix_get_fd (channel);

  n = read (fd, buf + buflen, sizeof (buf) - buflen);
  if (n < 0)
    {
      int err = errno;

      g_warning ("read: %s", strerror (errno));

      if (err == EAGAIN || err == EINTR)
	return TRUE;

      return FALSE;
    }
  buflen += n;

  ofs = 0;
  while (ofs < buflen)
    {
      n = process_buffer (fd, buf + ofs, buflen - ofs, user_data);
      if (n < 0)
	return FALSE;

      ofs += n;
    }

  if (ofs != buflen)
    memmove (buf, buf + ofs, buflen - ofs);
  buflen -= ofs;

  return TRUE;
}

static void
client_sock_removed (gpointer user_data)
{
  GHashTable *cleanup = user_data;

  LOG ("client removed");

  g_hash_table_destroy (cleanup);

  LOG ("pixmaps: %d (%d)", pixmap_counter, pixbuf_counter);
}

static void
cleanup_pixmap_destroy (gpointer data)
{
  g_cache_remove (pixmap_cache, data);
}

static gboolean
main_sock_callback (GIOChannel   *channel,
		    GIOCondition  cond,
		    gpointer      user_data)
{
  struct sockaddr fromaddr;
  socklen_t       fromlen = sizeof(fromlen);
  int             fd;
  GHashTable     *cleanup;

  if (cond & (G_IO_HUP | G_IO_ERR))
    {
      if (cond & G_IO_HUP)
	g_warning ("hangup");
      if (cond & G_IO_ERR)
	g_warning ("error");
      return FALSE;
    }

  fd = accept (g_io_channel_unix_get_fd (channel), &fromaddr, &fromlen);
  if (fd == -1)
    {
      g_warning ("accept: %s", strerror (errno));
      return TRUE;
    }

  LOG ("client fd = %d", fd);

  cleanup = g_hash_table_new_full (NULL, NULL, NULL, cleanup_pixmap_destroy);

  channel = g_io_channel_unix_new (fd);
  g_io_channel_set_close_on_unref (channel, TRUE);
  g_io_add_watch_full (channel,
		       G_PRIORITY_DEFAULT,
		       G_IO_IN|G_IO_ERR|G_IO_HUP,
		       client_sock_callback, cleanup,
		       client_sock_removed);
  g_io_channel_unref (channel);

  return TRUE;
}

int
main (int argc, char **argv)
{
  struct sockaddr_un  sun;
  int                 fd;
  GIOChannel         *channel;
  struct sigaction    act;
  sigset_t            empty_mask;

#ifdef ENABLE_DEBUG
  if (g_getenv ("SAPWOOD_SERVER_DEBUG"))
    enable_debug = TRUE;
#endif

  gdk_init (&argc, &argv);

  sock_path = sapwood_socket_path_get_default ();

  fd = socket (PF_LOCAL, SOCK_STREAM, 0);
  if (fd < 0)
    g_error ("socket: %s", strerror (errno));

  /* create our socket, overriding stale socket (if any) */
  memset(&sun, '\0', sizeof(sun));
  sun.sun_family = AF_LOCAL;
#ifdef HAVE_ABSTRACT_SOCKETS
  strcpy (&sun.sun_path[1], sock_path);
  if (bind (fd, (struct sockaddr *)&sun, sizeof (sun)) < 0)
    g_error ("bind(%s): %s", sock_path, strerror (errno));
#else
  strcpy (&sun.sun_path[0], sock_path);
  if (bind (fd, (struct sockaddr *)&sun, sizeof (sun)) < 0)
    {
      if (errno != EADDRINUSE)
	g_error ("bind(%s): %s", sock_path, strerror (errno));

      if (connect (fd, (struct sockaddr *)&sun, sizeof (sun)) == 0)
	g_error ("already running on socket `%s'", sock_path); 
      else if (errno != ECONNREFUSED)
	g_error ("connect(%s): unexpected error: %s", sock_path, strerror (errno));

      g_log (G_LOG_DOMAIN, G_LOG_LEVEL_INFO,
	     "removing stale socket `%s'", sock_path);

      if (unlink (sock_path) != 0)
	g_error ("unlink(%s): %s", sock_path, strerror (errno));

      if (bind (fd, (struct sockaddr *)&sun, sizeof (sun)) < 0)
	g_error ("bind(%s): %s", sock_path, strerror (errno));
    }

  /* always clean up on exit */
  g_atexit (atexit_handler);
#endif

  if (listen (fd, 5) < 0)
    g_error ("listen: %s", strerror (errno));


  channel = g_io_channel_unix_new (fd);
  g_io_add_watch (channel, G_IO_IN|G_IO_HUP|G_IO_ERR,
		  main_sock_callback, NULL);
  g_io_channel_unref (channel);

  sigemptyset (&empty_mask);
  act.sa_handler = signal_handler;
  act.sa_mask    = empty_mask;
  act.sa_flags   = 0;
  sigaction (SIGTERM, &act, 0);
  sigaction (SIGHUP,  &act, 0);
  sigaction (SIGINT,  &act, 0);

  pixmap_cache = g_cache_new ((GCacheNewFunc)pixbuf_open_response_new,
			      (GCacheDestroyFunc)pixbuf_open_response_destroy,
			      (GCacheDupFunc)pixbuf_open_request_dup,
			      (GCacheDestroyFunc)pixbuf_open_request_destroy,
			      pixbuf_open_request_hash, g_direct_hash, pixbuf_open_request_equal);

  g_log (G_LOG_DOMAIN, G_LOG_LEVEL_INFO, "server started");

  main_loop = g_main_loop_new (NULL, FALSE);
  g_main_loop_run (main_loop);
  g_main_loop_unref (main_loop);

  close (fd);
  return 0;
}
