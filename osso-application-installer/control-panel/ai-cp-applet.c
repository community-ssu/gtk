/**
  @file ai-cp-applet.c

  Control panel applet for launching application installer.
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



#include <config.h>
#include <libosso.h>
#include <hildon-cp-plugin/hildon-cp-plugin-interface.h>
#include <osso-log.h>

/*
#include <glib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/wait.h>
*/

#define APP_NAME "osso_application_installer"
#define APP_BINARY "/usr/bin/osso_application_installer"

extern char **environ;

osso_return_t execute (osso_context_t *osso, gpointer data,
                       gboolean user_activated)
{
/*
  ULOG_INFO("Starting AI through DBUS");
  osso_rpc_run_with_defaults(osso, APP_NAME, "activate", NULL,
			     DBUS_TYPE_INVALID);
*/

  ULOG_INFO("Starting AI through async spawn");
  g_spawn_command_line_async(APP_BINARY, NULL);

  return OSSO_OK;    
}
