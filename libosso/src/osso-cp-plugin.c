/**
 * @file osso-cp-plugin.c
 *
 * This file is part of libosso
 *
 * Copyright (C) 2005-2006 Nokia Corporation. All rights reserved.
 *
 * Contact: Kimmo Hämäläinen <kimmo.hamalainen@nokia.com>
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

#include "osso-internal.h"
#include "osso-cp-plugin.h"
#include "osso-log.h"
#include <linux/limits.h>
#include <errno.h>

/* hildon-control-panel RPC */
#define HCP_APPLICATION_NAME               "controlpanel"
#define HCP_SERVICE                        "com.nokia.controlpanel"
#define HCP_RPC_METHOD_RUN_APPLET          "run_applet"
#define HCP_RPC_METHOD_TOP_APPLICATION     "top_application"
#define HCP_RPC_METHOD_SAVE_STATE_APPLET   "save_state_applet"
#define HCP_RPC_METHOD_IS_APPLET_RUNNING   "is_applet_running"

static gboolean
is_applet_running_in_cp (osso_context_t *osso,
                         const char *filename)
{
  DBusError *error = NULL;
  gboolean cp_running;
  osso_return_t ret;
  osso_rpc_t retval;

  cp_running = dbus_bus_name_has_owner (osso->conn,
                                        HCP_SERVICE,
                                        error);
  if (error)
    {
      ULOG_ERR_F("Error in dbus_bus_name_has_owner: %s", error->message);
      dbus_error_free (error);
      return FALSE;
    }

  if (!cp_running)
    return FALSE;


  ret = osso_rpc_run_with_defaults(osso,
                                   HCP_APPLICATION_NAME,
                                   HCP_RPC_METHOD_IS_APPLET_RUNNING,
                                   &retval,
                                   DBUS_TYPE_STRING,
                                   filename,
                                   DBUS_TYPE_INVALID);

  if (ret != OSSO_OK)
    {
      ULOG_ERR_F("Error in RPC call");
      return FALSE;
    }

  if (retval.type != DBUS_TYPE_BOOLEAN)
    {
      ULOG_ERR_F("Unexpected return value in RPC call");
      return FALSE;
    }

  return retval.value.b;
}


static void *
try_plugin (osso_context_t *osso, const char *dir, const char *file)
{
  char libname[PATH_MAX];
  void *handle = NULL;

  if (snprintf (libname, PATH_MAX, "%s/%s", dir, file) >= PATH_MAX)
    {
      ULOG_ERR_F("PATH_MAX exceeded");
      return NULL;
    }

  if (osso->cp_plugins)
    handle = g_hash_table_lookup (osso->cp_plugins, libname);

  if (handle)
    return handle;

  handle = dlopen (libname, RTLD_LAZY | RTLD_GLOBAL);
  if (handle == NULL)
    {
      ULOG_ERR_F("Unable to load library '%s': %s", libname, dlerror());
    }

  g_hash_table_insert (osso->cp_plugins, g_strdup (libname), handle);
  return handle;
}

osso_return_t osso_cp_plugin_execute(osso_context_t *osso,
				     const gchar *filename,
				     gpointer data, gboolean user_activated)
{
    void *handle = NULL;
    osso_return_t ret;
    osso_cp_plugin_exec_f *exec = NULL;
   
    if (osso == NULL || filename == NULL) {
	ULOG_ERR_F("invalid arguments");
	return OSSO_INVALID;
    }
    dprint("executing library '%s'",filename);

    /* First of all, if the applet is started as system modal (data == NULL),
     * and if controlpanel is already running this applet, just top
     * controlpanel */
    if (data == NULL && is_applet_running_in_cp (osso, filename))
      {
        ret = osso_rpc_run_with_defaults(osso,
                                         HCP_APPLICATION_NAME,
                                         HCP_RPC_METHOD_TOP_APPLICATION,
                                         NULL,
                                         DBUS_TYPE_INVALID);
        if (ret == OSSO_OK)
          return OSSO_OK;
      }
        

    /* First try builtin path */
    handle = try_plugin (osso, OSSO_CTRLPANELPLUGINDIR, filename);

    if (handle == NULL)
      {
	/* Then try the directories in LIBOSSO_CP_PLUGIN_DIRS
	 */
	const char *dirs = getenv ("LIBOSSO_CP_PLUGIN_DIRS");
	if (dirs)
	  {
	    char *dirs_copy = strdup (dirs), *ptr = dirs_copy, *tok;
	    while ((tok = strsep (&ptr, ":")))
	      {
		handle = try_plugin (osso, tok, filename);
		if (handle)
		  break;
	      }
	    if (dirs_copy)
	      free (dirs_copy);
	    else
	      ULOG_ERR_F("strdup failed");
	  }
      }

    if (handle == NULL) {
        ULOG_ERR_F("library '%s' could not be opened", filename);
        return OSSO_ERROR;
    }

    dprint("..");
    fflush(stderr);
    
    exec = dlsym(handle, "execute");
    dprint("..");
    /* function wasn't found or it was NULL */
    if (exec == NULL) {
	ULOG_ERR_F("function 'execute' not found in library");
	ret = OSSO_ERROR;
	goto _exec_err1;
    }

    dprint("user_activated = %s",user_activated?"TRUE":"FALSE");
    
    ret = exec(osso, data, user_activated);  
    _exec_err1:

    return ret;
}

osso_return_t osso_cp_plugin_save_state(osso_context_t *osso,
					const gchar *filename,
					gpointer data)
{
    void *handle = NULL;
    
    if (osso == NULL || filename == NULL) {
	ULOG_ERR_F("invalid arguments");
	return OSSO_INVALID;
    }

    if (is_applet_running_in_cp (osso, filename))
    {
        /* Do not save state in this case, as the applet is run
         * by controlpanel, which will take care of saving its state */
        return OSSO_OK;
    }

    if (osso->cp_plugins)
      handle = g_hash_table_lookup(osso->cp_plugins, filename);

    if (handle) {
	osso_return_t ret;
	osso_cp_plugin_save_state_f *ss = NULL;

 	ss = dlsym(handle, "save_state"); 
	
	/* function wasn't found or it was NULL */
	if (ss == NULL) {
	    ULOG_ERR_F("symbol 'save_state' not found in library, or "
		       "it has a 'NULL' value");
	    return OSSO_ERROR;
	}
		
	ret = ss(osso, data);
	if (ret != OSSO_OK) {
	    ULOG_WARN_F("'save_state' did not return OSSO_OK");
	}
	return ret;
    }
    else {
	ULOG_ERR_F("plugin '%s' was not found", filename);
	return OSSO_ERROR;
    }
}
