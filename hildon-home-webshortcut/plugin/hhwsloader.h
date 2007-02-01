/*
 * This file is part of hildon-home-webshortcut
 *
 * Copyright (C) 2006, 2007 Nokia Corporation.
 *
 * Contact: Karoliina Salminen <karoliina.t.salminen@nokia.com>
 * Author:  Johan Bilien <johan.bilien@nokia.com>
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

#ifndef HHWS_LOADER_H
#define HHWS_LOADER_H

#include <glib.h>
#include <glib-object.h>
#include <gdk/gdkpixbuf.h>

G_BEGIN_DECLS

#define HHWS_TYPE_LOADER (hhws_loader_get_type ())
#define HHWS_LOADER(obj) (G_TYPE_CHECK_INSTANCE_CAST (obj, HHWS_TYPE_LOADER, HhwsLoader))
#define HHWS_LOADER_CLASS(klass) \
        (G_TYPE_CHECK_CLASS_CAST((klass),\
                               HHWS_TYPE_LOADER, HhwsLoaderClass))
#define HHWS_IS_LOADER(obj) (G_TYPE_CHECK_INSTANCE_TYPE (obj, HHWS_TYPE_LOADER))
#define HHWS_IS_LOADER_CLASS(klass) \
(G_TYPE_CHECK_CLASS_TYPE ((klass), HHWS_LOADER))


typedef struct _HhwsLoader HhwsLoader;
typedef struct _HhwsLoaderClass HhwsLoaderClass;

typedef enum {
  HHWS_LOADER_ERROR_OPEN_FAILED,
  HHWS_LOADER_ERROR_IO,
  HHWS_LOADER_ERROR_MMC_OPEN,
  HHWS_LOADER_ERROR_MEMORY,
  HHWS_LOADER_ERROR_FLASH_FULL,
  HHWS_LOADER_ERROR_CORRUPTED
} HhwsLoaderError;

GQuark hhws_loader_error_quark (void);

struct _HhwsLoader {
  GObject       parent;
};

struct _HhwsLoaderClass {
  GObjectClass  parent_class;

  void (* loading_failed)   (HhwsLoader *list, GError *error);
  void (* uri_changed)      (HhwsLoader *list);
  void (* pixbuf_changed)   (HhwsLoader *list);

};


GType hhws_loader_get_type (void);

void            hhws_loader_set_uri                 (HhwsLoader *loader,
                                                     const gchar *uri);
const gchar *   hhws_loader_get_uri                 (HhwsLoader *loader);
const gchar *   hhws_loader_get_default_uri         (HhwsLoader *loader);
const gchar *   hhws_loader_get_image_name          (HhwsLoader *loader);
const gchar *   hhws_loader_get_default_image_name  (HhwsLoader *loader);

void            hhws_loader_set_default_uri         (HhwsLoader *loader,
                                                     const gchar *default_uri);

GdkPixbuf *     hhws_loader_request_pixbuf          (HhwsLoader *loader,
                                                     guint width,
                                                     guint height);

gchar *         hhws_url_to_filename                (const gchar *uri);

G_END_DECLS
#endif /* HHWS_LOADER_H */
