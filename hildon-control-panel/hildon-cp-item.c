/*
 * This file is part of hildon-control-panel
 *
 * Copyright (C) 2006 Nokia Corporation.
 *
 * Contact: Karoliina Salminen <karoliina.t.salminen@nokia.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
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

#include "hildon-cp-item.h"
#include "hildon-control-panel-main.h"

#include <dlfcn.h>
#include <osso-log.h>


/* exec() callback prototype */
typedef osso_return_t (hcp_plugin_exec_f) (
                          osso_context_t * osso,
                          gpointer data,
                          gboolean user_activated);


typedef struct _HCPPlugin
{
    void * handle;
    hcp_plugin_exec_f *exec;
} HCPPlugin;



/* FIXME: get rid of this one */
static HCP *hcp;

static void hcp_item_load    (HCPItem *item, HCPPlugin *plugin);
static void hcp_item_unload  (HCPItem *item, HCPPlugin *plugin);

struct PluginLaunchData
{
    HCPItem *item;
    HCPPlugin *plugin;
    gboolean user_activated;
};

static gboolean
hcp_item_idle_launch (struct PluginLaunchData *d)
{
    HCPPlugin *p;
    
    p = g_new0 (HCPPlugin, 1);
    
    hcp_item_load (d->item, p);
    
    if (!p->handle)
        goto cleanup;
    
    hcp->execute = 1;
    d->item->running = TRUE;

    /* Always use hcp->window as parent. If CP is launched without
     * UI (run_applet RPC) the applet's dialog will be system modal */
    p->exec (hcp->osso, hcp->window, d->user_activated);

    d->item->running = FALSE;
    hcp->execute = 0;

    hcp_item_unload (d->item, p);

    if (!hcp->window)
        /* HCP was launched window less, so we can exit once we are done
         * with this applet */
        gtk_main_quit ();

cleanup:
    g_free (p);
    g_free (d);
    return FALSE;

}


void
hcp_item_init (gpointer _hcp)
{
    hcp = (HCP*)_hcp;
}

void
hcp_item_free (HCPItem *item)
{
    g_free (item->name);
    g_free (item->plugin);
    g_free (item->icon);
    g_free (item->category);

    g_free (item);
}

void
hcp_item_launch (HCPItem *item, gboolean user_activated)
{
    struct PluginLaunchData *d;

    g_return_if_fail (hcp);
    
    d = g_new0 (struct PluginLaunchData, 1);

    d->user_activated = user_activated;
    d->item = item;

    /* We launch plugins inside an idle loop so we are still able
     * to receive DBus messages */
    g_idle_add ((GSourceFunc)hcp_item_idle_launch, d);

}

void
hcp_item_focus (HCPItem *item)
{
    g_return_if_fail (hcp);
    
    hcp->focused_item = item;
}


static void
hcp_item_load (HCPItem *item, HCPPlugin *plugin)
{
    gchar *plugin_path = NULL;

    g_return_if_fail (item && item->plugin);

    if (*item->plugin == G_DIR_SEPARATOR)
    {
        /* .desktop provided fullpath, use that */
        plugin_path = g_strdup (item->plugin);
    }

    else
    {
        plugin_path = g_build_filename (HCP_PLUGIN_DIR, item->plugin, NULL);
    }

    plugin->handle = dlopen (plugin_path, RTLD_LAZY);

    g_free (plugin_path);

    if (!plugin->handle)
    {
        ULOG_ERR ("Could not load hildon-control-panel applet %s: %s",
                  item->plugin,
                  dlerror());
        return;
    }

    plugin->exec = dlsym (plugin->handle, HCP_PLUGIN_EXEC_SYMBOL);

    if (!plugin->exec)
    {
        ULOG_ERR ("Could not find "HCP_PLUGIN_SYMBOL" symbol in "
                  "hildon-control-panel applet %s: %s",
                  item->plugin,
                  dlerror());
        dlclose (plugin->handle);
        plugin->handle = NULL;
    }

}

static void
hcp_item_unload (HCPItem *item, HCPPlugin *plugin)
{
    g_return_if_fail (plugin->handle);

    if (dlclose (plugin->handle))
    {
        ULOG_ERR ("An error occurred when unloading hildon-control-panel "
                  "applet %s: %s",
                  item->plugin,
                  dlerror ());
    }
}
