/**
        @file dbus.c

        DBUS functionality for OSSO Application Installer
        <p>
*/

/*
 * This file is part of osso-application-installer
 *
 * Copyright (C) 2005 Nokia Corporation.
 *
 * Contact: Marius Vollmer <marius.vollmer@nokia.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

#include <libosso.h>
#include "appdata.h"
#include "dbus.h"
#include "core.h"

/* XXX - we are only ever going to handle a mime-open request for the
         first package; additional requests are ignored.  The theory
         behind this is that since the Application installer is system
         modal, there is no way to send it mime-open requests while it
         is running, and thus, all requests after the first must be
         bogus and for example result from stylus tremor.
*/

static void
mime_open_handler (gpointer raw_data, int argc, char **argv)
{
  static gboolean ignore_requests = 0;

  AppData *app_data = (AppData *)raw_data;
  int i;

  for (i = 0; i < argc; i++)
    {
      if (!ignore_requests)
	{
	  ignore_requests = 1;
	  do_install (argv[i], app_data);
	}
      else
	ULOG_INFO ("Ignoring mime-open request for %s\n", argv[i]);
    }
}

void
init_osso (AppData *app_data)
{
  osso_return_t ret;
  
  app_data->app_osso_data->osso =
    osso_initialize ("osso_application_installer",
		     PACKAGE_VERSION, TRUE, NULL);
  
  g_assert (app_data->app_osso_data->osso);

  ret = osso_mime_set_cb (app_data->app_osso_data->osso,
			  mime_open_handler,
			  app_data);
  g_assert (ret == OSSO_OK);
}

void
deinit_osso (AppData *app_data)
{  
  osso_mime_unset_cb (app_data->app_osso_data->osso);
  osso_deinitialize (app_data->app_osso_data->osso);
}
