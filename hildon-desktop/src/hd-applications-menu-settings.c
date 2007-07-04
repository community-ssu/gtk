/*
 * This file is part of hildon-desktop
 *
 * Copyright (C) 2007 Nokia Corporation.
 *
 * Author:  Johan Bilien <johan.bilien@nokia.com>
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "hd-app-menu-dialog.h"
#include <libhildondesktop/libhildonmenu.h>

GtkWidget * hildon_desktop_item_get_settings (GModule *module);

GtkWidget *
hildon_desktop_item_get_settings (GModule *module)
{
  GtkTreeModel *model;
  GtkWidget    *dialog;
 
  model = get_menu_contents ();
  
  dialog= g_object_new (HD_TYPE_APP_MENU_DIALOG,
                        "model", model,
                        NULL);

  gtk_widget_show (dialog);
  return dialog;
}
