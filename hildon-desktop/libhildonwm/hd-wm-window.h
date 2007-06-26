/* -*- mode:C; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

/* 
 * This file is part of libhildonwm
 *
 * Copyright (C) 2005, 2006, 2007 Nokia Corporation.
 *
 * Contact: Karoliina Salminen <karoliina.t.salminen@nokia.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

/* A HDWMWindow is a running watched / tracked instance of a window 
 * that references a valid HDWMApplication. A watched window *may* contain
 * a list of views ( see below ). 
 */

#ifndef __HD_WM_WINDOW_H__
#define __HD_WM_WINDOW_H__

#include <libhildonwm/hd-wm.h>
#include <libhildonwm/hd-wm-util.h>
#include <libhildonwm/hd-wm-types.h>
#include <sys/types.h>
#include <signal.h>

G_BEGIN_DECLS

/* For window_sync(), should go in enum */

#define HD_WM_SYNC_NAME                (1<<1)
#define HD_WM_SYNC_BIN_NAME            (1<<2)
#define HD_WM_SYNC_WMHINTS             (1<<3)
#define HD_WM_SYNC_TRANSIANT           (1<<4)
#define HD_WM_SYNC_HILDON_APP_KILLABLE (1<<5)
#define HD_WM_SYNC_WM_STATE            (1<<6)
#define HD_WM_SYNC_HILDON_VIEW_LIST    (1<<7)
#define HD_WM_SYNC_HILDON_VIEW_ACTIVE  (1<<8)
#define HD_WM_SYNC_ICON                (1<<9)
#define HD_WM_SYNC_USER_TIME           (1<<10)
#define HD_WM_SYNC_WINDOW_ROLE         (1<<11)
#define HD_WM_SYNC_ALL                 (G_MAXULONG)

/* Application relaunch indicator data*/
#define RESTORED "restored"

#define HD_WM_TYPE_WINDOW            (hd_wm_window_get_type ())
#define HD_WM_WINDOW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), HD_WM_TYPE_WINDOW, HDWMWindow))
#define HD_WM_WINDOW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  HD_WM_TYPE_WINDOW, HDWMWindowClass))
#define HD_WM_IS_WINDOW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HD_WM_TYPE_WINDOW))
#define HD_IS_WM_WINDOW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  HD_WM_TYPE_WINDOW))
#define HD_WM_WINDOW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  HD_WM_TYPE_WINDOW, HDWMWindowClass))

typedef struct _HDWMWindowClass HDWMWindowClass;
typedef struct _HDWMWindowPrivate HDWMWindowPrivate;

struct _HDWMWindow
{
  GObject parent;
 
  HDWMWindowPrivate *priv; 
};

struct _HDWMWindowClass
{
  GObjectClass parent_class;
};

GType 
hd_wm_window_get_type (void);

HDWMWindow*
hd_wm_window_new (Window            xid,
			  HDWMApplication *app);
gboolean
hd_wm_window_props_sync (HDWMWindow *win, gulong props);

HDWMApplication*
hd_wm_window_get_application (HDWMWindow *win);

Window
hd_wm_window_get_x_win (HDWMWindow *win);

void
hd_wm_window_reset_x_win (HDWMWindow * win);

gboolean
hd_wm_window_is_urgent (HDWMWindow *win);

gboolean
hd_wm_window_wants_no_initial_focus (HDWMWindow *win);

const gchar*
hd_wm_window_get_name (HDWMWindow *win);

const gchar*
hd_wm_window_get_sub_name (HDWMWindow *win);

void
hd_wm_window_set_name (HDWMWindow *win,
			       const gchar       *name);

void
hd_wm_window_set_gdk_wrapper_win (HDWMWindow *win,
                                          GdkWindow         *wrapper_win);

GdkWindow *
hd_wm_window_get_gdk_wrapper_win (HDWMWindow *win);

const gchar*
hd_wm_window_get_hibernation_key (HDWMWindow *win);


GtkWidget*
hd_wm_window_get_menu (HDWMWindow *win);

void
hd_wm_window_set_menu (HDWMWindow *win,
			       GtkWidget         *menu);

/* Note, returns a copy of pixbuf will need freeing */
GdkPixbuf*
hd_wm_window_get_custom_icon (HDWMWindow *win);

GList*
hd_wm_window_get_views (HDWMWindow *win);

gint
hd_wm_window_get_n_views (HDWMWindow *win);

gboolean
hd_wm_window_attempt_signal_kill (HDWMWindow *win,
                                          int sig,
                                          gboolean ensure);

gboolean
hd_wm_window_is_hibernating (HDWMWindow *win);

void
hd_wm_window_awake (HDWMWindow *win);

void
hd_wm_window_destroy (HDWMWindow *win);

void
hd_wm_window_close (HDWMWindow *win);

void
hd_wm_window_set_info (HDWMWindow *win,
			       HDWMEntryInfo       *info);

HDEntryInfo *
hd_wm_window_peek_info (HDWMWindow *win);

HDEntryInfo *
hd_wm_window_create_new_info (HDWMWindow *win);

void
hd_wm_window_destroy_info (HDWMWindow *win);

gboolean
hd_wm_window_is_active (HDWMWindow *win);

void         
hd_wm_window_set_icon_geometry (HDWMWindow *win,
                                gint x,
                                gint y,
                                gint width,
                                gint height,
                                gboolean override);

G_END_DECLS

#endif
