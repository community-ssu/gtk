/* -*- mode:C; c-file-style:"gnu"; -*- */
/*
 * This file is part of maemo-af-desktop
 *
 * Copyright (C) 2006 Nokia Corporation.
 *
 * Contact: Karoliina Salminen <karoliina.t.salminen@nokia.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
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

#define TYPE_BACKGROUND_MODE			(background_mode_get_type ())
#define TYPE_BACKGROUND_MANAGER			(background_manager_get_type ())
#define BACKGROUND_MANAGER_ERROR		(background_manager_error_quark ())

#define BACKGROUND_MANAGER(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_BACKGROUND_MANAGER, BackgroundManager))
#define IS_BACKGROUND_MANAGER(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_BACKGROUND_MANAGER))
#define BACKGROUND_MANAGER_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_BACKGROUND_MANAGER, BackgroundManagerClass))
#define IS_BACKGROUND_MANAGER_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_BACKGROUND_MANAGER))
#define BACKGROUND_MANAGER_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_BACKGROUND_MANAGER, BackgroundManagerClass))

#define HILDON_HOME_BACKGROUND_NO_IMAGE		"no_image"

G_BEGIN_DECLS

typedef struct _BackgroundManager		BackgroundManager;
typedef struct _BackgroundManagerPrivate	BackgroundManagerPrivate;
typedef struct _BackgroundManagerClass		BackgroundManagerClass;

typedef enum {
  BACKGROUND_CENTERED,
  BACKGROUND_SCALED,
  BACKGROUND_TILED,
  BACKGROUND_STRETCHED
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

  void (*changed) (BackgroundManager *manager,
		   GdkPixbuf         *pixbuf);
};

GType                 background_manager_get_type         (void) G_GNUC_CONST;

BackgroundManager *   background_manager_get_default      (void);

G_CONST_RETURN gchar *background_manager_get_image_uri    (BackgroundManager  *manager);
void                  background_manager_set_image_uri    (BackgroundManager  *manager,
						           const gchar        *uri);
gboolean              background_manager_preset_image_uri (BackgroundManager  *manager,
						           const gchar        *uri);
BackgroundMode        background_manager_get_mode         (BackgroundManager  *manager);
void                  background_manager_set_mode         (BackgroundManager  *manager,
							   BackgroundMode      mode);
gboolean              background_manager_preset_mode      (BackgroundManager  *manager,
							   BackgroundMode      mode);
void                  background_manager_get_color        (BackgroundManager  *manager,
						           GdkColor           *color);
void                  background_manager_set_color        (BackgroundManager  *manager,
							   const GdkColor     *color);
gboolean              background_manager_preset_color     (BackgroundManager  *manager,
							   const GdkColor     *color);

void                  background_manager_set_components   (BackgroundManager  *manager,
							   const gchar        *titlebar,
							   const gchar        *sidebar);
G_CONST_RETURN gchar *background_manager_get_cache        (BackgroundManager  *manager);
void                  background_manager_set_cache        (BackgroundManager  *manager,
						           const gchar        *uri);

void                  background_manager_apply_preset     (BackgroundManager  *manager);

G_END_DECLS

#endif /* __BACKGROUND_MANAGER_H__ */
