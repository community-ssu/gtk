/*
 * This file is part of maemo-af-desktop
 *
 * Copyright (C) 2006 Nokia Corporation.
 *
 * Contact: Karoliina Salminen <karoliina.t.salminen@nokia.com>
 * Author: Johan Bilien <johan.bilien@nokia.com>
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

#ifndef __HILDON_HOME_SELECT_APPLETS_DIALOG_H__
#define __HILDON_HOME_SELECT_APPLETS_DIALOG_H__

#include <libosso.h>
#include <gtk/gtkwidget.h>
#include <gtk/gtktreemodel.h>
#include <glib/gi18n.h>

#define HOME_APPLETS_SELECT_TITLE      _("home_ti_select_applets")
#define HOME_APPLETS_SELECT_OK         _("home_bd_select_applets_ok")
#define HOME_APPLETS_SELECT_CANCEL     _("home_bd_select_applets_cancel")


GtkWidget *     hildon_home_select_applets_dialog_new_with_model 
                                                (GtkTreeModel *model,
                                                 osso_context_t *osso_home);


#endif
