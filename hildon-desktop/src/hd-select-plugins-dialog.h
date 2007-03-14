/*
 * This file is part of hildon-desktop
 *
 * Copyright (C) 2006 Nokia Corporation.
 *
 * Author:  Lucas Rocha <lucas.rocha@nokia.com>
 *          Johan Bilien <johan.bilien@nokia.com>
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

#ifndef __HD_SELECT_PLUGINS_DIALOG_H__
#define __HD_SELECT_PLUGINS_DIALOG_H__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_LIBHILDONHELP
#include <libosso.h>
#endif

#include <gtk/gtk.h>

gint   hd_select_plugins_dialog_run  (GList           *loaded_plugins,
#ifdef HAVE_LIBHILDONHELP
                                      osso_context_t  *osso_context,
#endif
                                      const gchar     *plugin_dir,
				      GList          **selected_plugins);

#endif /* __HD_SELECT_PLUGINS_DIALOG_H__ */
