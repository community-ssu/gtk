/* GTK+ Sapwood Engine
 * Copyright (C) 1998-2000 Red Hat, Inc.
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
 * Written by Tommi Komulainen <tommi.komulainen@nokia.com> based on 
 * code by Owen Taylor <otaylor@redhat.com> and 
 * Carsten Haitzler <raster@rasterman.com>
 */

#include <gtk/gtkrc.h>

typedef struct _SapwoodRcStyle SapwoodRcStyle;
typedef struct _SapwoodRcStyleClass SapwoodRcStyleClass;

extern GType sapwood_type_rc_style;

#define SAPWOOD_TYPE_RC_STYLE              sapwood_type_rc_style
#define SAPWOOD_RC_STYLE(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), SAPWOOD_TYPE_RC_STYLE, SapwoodRcStyle))
#define SAPWOOD_RC_STYLE_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), SAPWOOD_TYPE_RC_STYLE, SapwoodRcStyleClass))
#define SAPWOOD_IS_RC_STYLE(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), SAPWOOD_TYPE_RC_STYLE))
#define SAPWOOD_IS_RC_STYLE_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), SAPWOOD_TYPE_RC_STYLE))
#define SAPWOOD_RC_STYLE_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), SAPWOOD_TYPE_RC_STYLE, SapwoodRcStyleClass))

struct _SapwoodRcStyle
{
  GtkRcStyle parent_instance;
  
  GList *img_list;
};

struct _SapwoodRcStyleClass
{
  GtkRcStyleClass parent_class;
};

void sapwood_rc_style_register_type (GTypeModule *module);
