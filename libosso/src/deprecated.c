/* This file is part of libosso
 *
 * Copyright (C) 2007 Nokia Corporation. All rights reserved.
 *
 * Contact: Marius Vollmer <marius.vollmer@nokia.com>
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
 */

/* Deprecated functions.  They are not declared in any header file but
   still provided by libosso to maintain binary compatibility.
*/

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <pwd.h>
#include <stdlib.h>

#include "libosso.h"
#include "osso-state.h"
#include "osso-log.h"
#include "osso-internal.h"

int osso_state_open_write(osso_context_t *osso);
int osso_state_open_read(osso_context_t *osso);
void osso_state_close(osso_context_t * osso, gint fd);

osso_return_t osso_application_set_exit_cb(osso_context_t *osso,
					   void *cb,
					   gpointer data);

/************************************************************************/
int osso_state_open_write(osso_context_t *osso)
{
    gint fd = -1;
    gchar *tmpdir_path  = NULL;
    gchar *path = NULL, *app_path = NULL;
    struct stat statbuf;

    if (!validate_osso_context(osso)) {
      ULOG_ERR_F("appname/version invalid or osso context NULL");
      return -1;
    }
    
    tmpdir_path = getenv(LOCATION_VAR);
    
    if (tmpdir_path != NULL)
      {
	path = g_strconcat(tmpdir_path, "/",
			   osso->application, "/",
			   osso->version, NULL);
      }
    else
      {
	path = g_strconcat(FALLBACK_PREFIX "/",
			   osso->application, "/",
			   osso->version, NULL);
      }

    if (path == NULL) {
      ULOG_ERR_F("Allocation of application/version string failed");
      return -1;
    }
    
    /* Check for the existence of application directory. */

    if (tmpdir_path == NULL)
      {
	app_path = g_strconcat(FALLBACK_PREFIX "/",
			       osso->application,
			       NULL);
      }
    else {
      app_path = g_strconcat(tmpdir_path, "/", osso->application, NULL);
      
    }
    if (app_path == NULL) {
      ULOG_ERR_F("Allocation of application string failed");
      g_free(path);
      return -1;
    }
    if (stat(app_path, &statbuf) != -1) {
      if (!S_ISDIR(statbuf.st_mode)) {
	ULOG_ERR_F("Other type of file instead of app directory");
	g_free(app_path);
	g_free(path);
	return -1;
      }
    } else {

      /* It's the responsibility of the startup scripts to create
	 the directory path contained in the STATESAVEDIR env var */

      if (mkdir(app_path, S_IRUSR | S_IWUSR | S_IXUSR) != 0) {
	ULOG_ERR_F("Could not create application state directory");
	g_free(app_path);
	g_free(path);
	return -1;
      }
    }
    
    g_free(app_path);
    if ((fd = open(path, O_WRONLY | O_CREAT | O_TRUNC,
		   S_IRUSR | S_IWUSR)) == -1) {
      ULOG_ERR_F("Opening of state file failed");
    }
    g_free(path);
    return fd;
}


/************************************************************************/
int osso_state_open_read(osso_context_t *osso)
{
    gint fd = -1;
    gchar *tmpdir_path  = NULL;
    gchar *path = NULL;

    if (!validate_osso_context(osso)) {
      ULOG_ERR_F("appname/version invalid or osso context NULL");
      return -1;
    }

    /* Create the filename path from application name and version. */
    tmpdir_path = getenv(LOCATION_VAR);
    if (tmpdir_path == NULL)
      {
	path = g_strconcat(FALLBACK_PREFIX "/",
			   osso->application, "/",
			   osso->version, NULL);
      }
    else {
      path = g_strconcat(tmpdir_path, "/",
			 osso->application, "/",
			 osso->version, NULL);
      }
    if (path == NULL) {
      ULOG_ERR_F("Allocation of application/version string failed");
      return -1;
    }
    if ((fd = open(path, O_RDONLY)) == -1) {
      ULOG_ERR_F("Opening of state file failed");
    }
    g_free(path);
    return fd;
}

static gboolean reliable_close(int fd)
{
    do {
        if (close(fd) == 0) {
            return TRUE;
        } else if (errno != EINTR) {
            ULOG_ERR_F("Unable to close state file: %s", strerror(errno));
            return FALSE;
        }
    } while (1);
}

/************************************************************************/
void osso_state_close(osso_context_t * osso, gint fd)
{
    reliable_close(fd);
}

osso_return_t osso_application_set_exit_cb(osso_context_t *osso,
					   void *cb,
					   gpointer data)
{
    fprintf(stderr, "WARNING: osso_application_set_exit_cb()"
                    " is obsolete and a no-op.\n");
    ULOG_WARN_F("This function is obsolete");
    return OSSO_OK;
}
