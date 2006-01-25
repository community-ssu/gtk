/**
 * @file osso-init.c
 * This file implements all initialisation and shutdown of the library.
 *
 * Copyright (C) 2005 Nokia Corporation.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *               
 */

#include "osso-internal.h"
#include "osso-cp-plugin.h"
#include "osso-log.h"
#include <linux/limits.h>
#include <errno.h>

static void *
try_plugin (const char *dir, const char *file)
{
  char libname[PATH_MAX];
  void *handle;

  if (snprintf (libname, PATH_MAX, "%s/%s", dir, file) >= PATH_MAX)
    {
      ULOG_ERR_F("PATH_MAX exceeded");
      return NULL;
    }

  handle = dlopen (libname, RTLD_NOW | RTLD_GLOBAL);
  if (handle == NULL)
    {
      ULOG_ERR_F("Unable to load library '%s': %s", libname, dlerror());
    }
  return handle;
}

osso_return_t osso_cp_plugin_execute(osso_context_t *osso,
				     const gchar *filename,
				     gpointer data, gboolean user_activated)
{
    gint r;
    osso_return_t ret;
    osso_cp_plugin_exec_f *exec = NULL;
    _osso_cp_plugin_t p;
   
    if (osso == NULL || filename == NULL) {
	ULOG_ERR_F("invalid arguments");
	return OSSO_INVALID;
    }
    dprint("executing library '%s'",filename);

    /* First try builtin path */
    p.lib = try_plugin (OSSO_CTRLPANELPLUGINDIR, filename);
    if (p.lib == NULL)
      {
	/* Then try the directories in LIBOSSO_CP_PLUGIN_DIRS
	 */
	const char *dirs = getenv ("LIBOSSO_CP_PLUGIN_DIRS");
	if (dirs)
	  {
	    char *dirs_copy = strdup (dirs), *ptr = dirs_copy, *tok;
	    while (tok = strsep (&ptr, ":"))
	      {
		p.lib = try_plugin (tok, filename);
		if (p.lib)
		  break;
	      }
	    if (dirs_copy)
	      free (dirs_copy);
	    else
	      ULOG_ERR_F("strdup failed");
	  }
      }

    if (p.lib == NULL) {
        ULOG_ERR_F("library '%s' could not be opened", filename);
        return OSSO_ERROR;
    }

    p.name = g_path_get_basename(filename);

    dprint("..");
    fflush(stderr);
    g_array_append_val(osso->cp_plugins, p);
    
    exec = dlsym(p.lib, "execute");
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

    r = _close_lib(osso->cp_plugins, p.name); /* p.name is freed here */
    if (r != 0) {
	ULOG_ERR_F("Error closing library");
	ret = OSSO_ERROR;
    }
    return ret;
}

osso_return_t osso_cp_plugin_save_state(osso_context_t *osso,
					const gchar *filename,
					gpointer data)
{
    gchar *pluginname;
    _osso_cp_plugin_t *p = NULL;
    GArray *a;
    gint i;
    
    if (osso == NULL || filename == NULL) {
	ULOG_ERR_F("invalid arguments");
	return OSSO_INVALID;
    }
    a = osso->cp_plugins;
    
    pluginname = g_path_get_basename(filename);
    for (i = 0; i < a->len; i++) {
	p = &g_array_index(a, _osso_cp_plugin_t, i);
	if (strcmp(p->name, pluginname) == 0)
	    break;
    }
    g_free(pluginname); pluginname = NULL;

    if (i < a->len) {
	osso_return_t ret;
	osso_cp_plugin_save_state_f *ss = NULL;

 	ss = dlsym(p->lib, "save_state"); 
	
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


static int _close_lib(GArray *a, const gchar *n)
{
    gint i=0, r=0;
    _osso_cp_plugin_t *p = NULL;

    while (i < a->len) {
        p = &g_array_index(a, _osso_cp_plugin_t, i);
        if (strcmp(p->name, n) == 0)
            break;
        else
            i++;
    }

    if (i < a->len) {
        r = dlclose(p->lib);
        g_free(p->name);
        g_array_remove_index_fast(a, i);
    }

    return r;
}
                        
