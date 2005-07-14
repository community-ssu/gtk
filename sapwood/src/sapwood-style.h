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

#include <gtk/gtkstyle.h>

typedef struct _SapwoodStyle SapwoodStyle;
typedef struct _SapwoodStyleClass SapwoodStyleClass;

extern GType sapwood_type_style;

#define SAPWOOD_TYPE_STYLE              sapwood_type_style
#define SAPWOOD_STYLE(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), SAPWOOD_TYPE_STYLE, SapwoodStyle))
#define SAPWOOD_STYLE_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), SAPWOOD_TYPE_STYLE, SapwoodStyleClass))
#define SAPWOOD_IS_STYLE(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), SAPWOOD_TYPE_STYLE))
#define SAPWOOD_IS_STYLE_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), SAPWOOD_TYPE_STYLE))
#define SAPWOOD_STYLE_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), SAPWOOD_TYPE_STYLE, SapwoodStyleClass))

struct _SapwoodStyle
{
  GtkStyle parent_instance;
};

struct _SapwoodStyleClass
{
  GtkStyleClass parent_class;
};

void sapwood_style_register_type (GTypeModule *module);


