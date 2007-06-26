/* -*- mode:C; c-file-style:"gnu"; -*- */
/*
 * This file is part of hildon-desktop
 *
 * Copyright (C) 2006 Nokia Corporation.
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

#ifndef __BACKGROUND_MANAGER_H__
#define __BACKGROUND_MANAGER_H__

#include <glib-object.h>
#include <gdk/gdkcolor.h>
#include <gdk/gdkwindow.h>

#include <dbus/dbus-glib-bindings.h>

#define TYPE_BACKGROUND_MODE			(background_mode_get_type ())
#define TYPE_BACKGROUND_MANAGER			(background_manager_get_type ())
#define BACKGROUND_MANAGER_ERROR		(background_manager_error_quark ())

#define BACKGROUND_MANAGER(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_BACKGROUND_MANAGER, BackgroundManager))
#define IS_BACKGROUND_MANAGER(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_BACKGROUND_MANAGER))
#define BACKGROUND_MANAGER_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_BACKGROUND_MANAGER, BackgroundManagerClass))
#define IS_BACKGROUND_MANAGER_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_BACKGROUND_MANAGER))
#define BACKGROUND_MANAGER_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_BACKGROUND_MANAGER, BackgroundManagerClass))



G_BEGIN_DECLS

typedef struct _BackgroundManager		    BackgroundManager;
typedef struct _BackgroundManagerPrivate	BackgroundManagerPrivate;
typedef struct _BackgroundManagerClass		BackgroundManagerClass;

typedef enum {
  BACKGROUND_CENTERED,
  BACKGROUND_SCALED,
  BACKGROUND_STRETCHED,
  BACKGROUND_TILED,
  BACKGROUND_CROPPED
} BackgroundMode;

GType background_mode_get_type (void) G_GNUC_CONST;

typedef enum {
  BACKGROUND_MANAGER_ERROR_MEMORY,
  BACKGROUND_MANAGER_ERROR_CONNECTIVITY,
  BACKGROUND_MANAGER_ERROR_CORRUPT,
  BACKGROUND_MANAGER_ERROR_UNREADABLE,
  BACKGROUND_MANAGER_ERROR_MMC_OPEN,
  BACKGROUND_MANAGER_ERROR_SYSTEM_RESOURCES,
  BACKGROUND_MANAGER_ERROR_FLASH_FULL,
  BACKGROUND_MANAGER_ERROR_IO,
  BACKGROUND_MANAGER_ERROR_WINDOW,
  BACKGROUND_MANAGER_ERROR_DISPLAY,

  BACKGROUND_MANAGER_ERROR_UNKNOWN
} BackgroundManagerError;

GQuark background_manager_error_quark (void);


struct _BackgroundManager
{
  GObject parent_instance;

  BackgroundManagerPrivate *priv;
};

struct _BackgroundManagerClass
{
  GObjectClass parent_class;

  DBusGConnection  *connection;
};

GType                 background_manager_get_type          (void) G_GNUC_CONST;

gboolean              background_manager_set_background
                            (BackgroundManager   *manager,
                             gint                 window_xid,
                             const gchar         *filename,
                             guint16              red,
                             guint16              green,
                             guint16              blue,
                             BackgroundMode       mode,
                             gint32               top_offset,
                             gint32               bottom_offset,
                             gint32               left_offset,
                             gint32               right_offset,
                             gint                *pixmap_xid,
                             GError             **error);


G_END_DECLS

#endif /* __BACKGROUND_MANAGER_H__ */
