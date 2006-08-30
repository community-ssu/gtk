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

#ifndef __HILDON_HOME_TITLEBAR_H__
#define __HILDON_HOME_TITLEBAR_H__

#include <gtk/gtkeventbox.h>
#include <libosso.h>

#define HILDON_TYPE_HOME_TITLEBAR_MODE		(hildon_home_titlebar_mode_get_type ())
#define HILDON_TYPE_HOME_TITLEBAR		(hildon_home_titlebar_get_type ())
#define HILDON_HOME_TITLEBAR(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), HILDON_TYPE_HOME_TITLEBAR, HildonHomeTitlebar))
#define HILDON_IS_HOME_TITLEBAR(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), HILDON_TYPE_HOME_TITLEBAR))
#define HILDON_HOME_TITLEBAR_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), HILDON_TYPE_HOME_TITLEBAR, HildonHomeTitlebarClass))
#define HILDON_IS_HOME_TITLEBAR_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), HILDON_TYPE_HOME_TITLEBAR))
#define HILDON_HOME_TITLEBAR_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), HILDON_TYPE_HOME_TITLEBAR, HildonHomeTitlebarClass))

G_BEGIN_DECLS

typedef enum {
  HILDON_HOME_TITLEBAR_NORMAL,
  HILDON_HOME_TITLEBAR_LAYOUT
} HildonHomeTitlebarMode;

GType hildon_home_titlebar_mode_get_type (void) G_GNUC_CONST;

typedef struct _HildonHomeTitlebar		HildonHomeTitlebar;
typedef struct _HildonHomeTitlebarClass		HildonHomeTitlebarClass;
typedef struct _HildonHomeTitlebarPrivate	HildonHomeTitlebarPrivate;

struct _HildonHomeTitlebar
{
  GtkEventBox parent_instance;

  HildonHomeTitlebarPrivate *priv;
};

struct _HildonHomeTitlebarClass
{
  GtkEventBoxClass parent_class;

  void (*layout_accept)           (HildonHomeTitlebar *titlebar);
  void (*layout_cancel)           (HildonHomeTitlebar *titlebar);
  
  void (*select_applets_activate) (HildonHomeTitlebar *titlebar);
  void (*layout_mode_activate)    (HildonHomeTitlebar *titlebar);
  void (*applet_activate)         (HildonHomeTitlebar *titlebar,
		  	           const gchar        *applet_path);
  void (*help_activate)           (HildonHomeTitlebar *titlebar);
};

GType      hildon_home_titlebar_get_type        (void) G_GNUC_CONST;
GtkWidget *hildon_home_titlebar_new             (osso_context_t         *osso_context);

void       hildon_home_titlebar_set_mode        (HildonHomeTitlebar     *titlebar,
						 HildonHomeTitlebarMode  mode);

void       hildon_home_titlebar_set_layout_menu (HildonHomeTitlebar     *titlebar,
						 const gchar            *label,
						 GtkWidget              *menu);
gboolean   hildon_home_titlebar_get_menu_active (HildonHomeTitlebar     *titlebar);
void       hildon_home_titlebar_set_menu_active (HildonHomeTitlebar     *titlebar,
						 gboolean                active);

G_END_DECLS

#endif /* __HILDON_HOME_TITLEBAR_H__ */
