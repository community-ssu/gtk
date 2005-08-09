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

osso_return_t osso_cp_plugin_execute(osso_context_t *osso,
				     const gchar *filename,
				     gpointer data, gboolean user_activated)
{
    char libname[PATH_MAX];
    gint r;
    osso_return_t ret;
    osso_cp_plugin_exec_f *exec=NULL;
/*     osso_cp_plugin_get_svc_name_f *gsvcn=NULL; */
    _osso_cp_plugin_t p;
/*     osso_state_t state={0,NULL}; */
/*     gchar *statefile; */
   
    /* check parameters and initialize variables*/
    
    if(osso == NULL) return OSSO_INVALID;
    if(filename == NULL) return OSSO_INVALID;
    dprint("executing library '%s'",filename);

    if(sprintf(libname,"%s/%s",OSSO_CTRLPANELPLUGINDIR,filename)<0) {
        ULOG_ERR_F("Error while copying plugin name: %s", strerror(errno));
        return OSSO_ERROR;
    }
    /* load library, or return if this fails */
    p.lib = dlopen(libname,RTLD_NOW|RTLD_GLOBAL);
    dprint("libname='%s'",libname);

    /* library wasn't found, or couldn't be loaded */
    if(p.lib==NULL) {
	ULOG_ERR_F("Unable to load library '%s':",libname);
	ULOG_ERR_F("%s",dlerror());
	dprint("could not load libraru!\n");
	return OSSO_ERROR;
    }
    p.name = g_path_get_basename(filename);

    dprint("..");
    fflush(stderr);
    g_array_append_val(osso->cp_plugins, p);
    
    exec = dlsym(p.lib, "execute");
    dprint("..");
    /* function wasn't found or it was NULL */
    if(exec==NULL) {
	ULOG_ERR_F("function 'execute' not found in library");
	ret = OSSO_ERROR;
	goto _exec_err1;
    }

    dprint("user_activated = %s",user_activated?"TRUE":"FALSE");
    
    ret = exec(osso, data, user_activated);  
    _exec_err1:

    r = _close_lib(osso->cp_plugins, p.name);
    if(r!=0) {
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
    _osso_cp_plugin_t *p;
    GArray *a = osso->cp_plugins;
    gint i;
    
    if(osso == NULL) return OSSO_INVALID;
    if(filename == NULL) return OSSO_INVALID;
    
    pluginname = g_path_get_basename(filename);
    for(i=0;i<a->len;i++) {
	p = &g_array_index(a, _osso_cp_plugin_t, i);
	if(strcmp(p->name,pluginname) == 0)
	    break;
    }

    if(i<a->len) {
	osso_return_t ret;
	osso_cp_plugin_save_state_f *ss=NULL;

 	ss = dlsym(p->lib, "save_state"); 
	
	/* function wasn't found or it was NULL */
	if(ss==NULL) {
	    ULOG_ERR_F("symbol 'save_state' not found in library, or "
		       "it has a 'NULL' value");
	    return OSSO_ERROR;
	}
		
	ret = ss(osso, data);
	if(ret != OSSO_OK)
	    return ret;

	return OSSO_OK;
    }
    else {
	return OSSO_ERROR;
    }
}


static int _close_lib(GArray *a, const gchar *n)
{
    gint i=0, r=0;
    _osso_cp_plugin_t *p;

    while(i<a->len) {
        p = &g_array_index(a, _osso_cp_plugin_t, i);
        if(strcmp(p->name,n) == 0)
            break;
        else
            i++;
    }

    if(i<a->len) {
        r = dlclose(p->lib);
        free(p->name);
        /*    free(p->svcname); */
        g_array_remove_index_fast(a, i);
    }

    return r;
}
                        
