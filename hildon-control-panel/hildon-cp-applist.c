/*
 * This file is part of hildon-control-panel
 *
 * Copyright (C) 2005 Nokia Corporation.
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

/* System includes */
#include <libintl.h>

/* Osso includes */
/* Osso logging functions */
#include <osso-log.h>

/* GTK includes */
#include <gtk/gtkscrolledwindow.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkhseparator.h>

/* Gconf includes */
#include <gconf/gconf-client.h>

/* Control Panel includes */
#include "hildon-cp-applist.h"
#include "hildon-cp-item.h"
#include "cp-grid.h"
#include "cp-grid-item.h"

#define _(String) gettext(String)

#define HILDON_CP_SEPARATOR_DEFAULT _("copa_ia_extras")


static void hcp_al_read_desktop_entries( const gchar *dir_path, GHashTable *entries );


static gboolean
hcp_al_free_item (gchar *plugin, HCPItem *item)
{
    hcp_item_free (item);

    return TRUE;
}

static void 
hcp_al_read_desktop_entries (const gchar *dir_path, GHashTable *entries)
{
    GDir *dir;
    GError * error = NULL;
    const char *filename;
    GKeyFile *keyfile;

    ULOG_DEBUG("hildon-cp-applist:read_desktop_entries");

    g_return_if_fail (dir_path && entries);

    dir = g_dir_open(dir_path, 0, &error);
    if (!dir)
    {
        ULOG_ERR (error->message);
        g_error_free (error);
        return;
    }

    keyfile = g_key_file_new ();

    while ((filename = g_dir_read_name (dir)))
    {
        error = NULL;
        gchar *desktop_path = NULL;
        gchar *name = NULL;
        gchar *plugin = NULL;
        gchar *icon = NULL;
        gchar *category = NULL;

        desktop_path = g_build_filename (dir_path, filename, NULL);

        g_key_file_load_from_file (keyfile,
                                   desktop_path,
                                   G_KEY_FILE_NONE,
                                   &error);

        g_free (desktop_path);

        if (error)
        {
            ULOG_ERR (error->message);
            g_error_free (error);
            continue;
        }

        name = g_key_file_get_locale_string (keyfile,
                HCP_DESKTOP_GROUP,
                HCP_DESKTOP_KEY_NAME,
                NULL /* current locale */,
                &error);

        if (error)
        {
            ULOG_ERR (error->message);
            g_error_free (error);
            continue;
        }

        plugin = g_key_file_get_string (keyfile,
                HCP_DESKTOP_GROUP,
                HCP_DESKTOP_KEY_PLUGIN,
                &error);

        if (error)
        {
            ULOG_ERR (error->message);
            g_error_free (error);
            continue;
        }

        icon = g_key_file_get_string (keyfile,
                HCP_DESKTOP_GROUP,
                HCP_DESKTOP_KEY_ICON,
                &error);

        if (error)
        {
            ULOG_WARN (error->message);
            g_error_free (error);
            error = NULL;
        }
        

        category = g_key_file_get_string (keyfile,
                HCP_DESKTOP_GROUP,
                HCP_DESKTOP_KEY_CATEGORY,
                &error);

        if (error)
        {
            ULOG_WARN (error->message);
            g_error_free (error);
            error = NULL;
        }

        {
            HCPItem *item = g_new0 (HCPItem, 1);
            
            item->name = name;
            item->plugin = plugin;
            item->icon = icon;
            item->category = category;

            g_hash_table_insert (entries, plugin, item);
        }
    }

    g_key_file_free (keyfile);
    g_dir_close (dir);
}

static gint
hcp_al_find_category (gpointer _category, gpointer _applet)
{
    HCPItem *applet = (HCPItem *)_applet;
    HCPCategory *category = (HCPCategory *)_category;

    if (applet->category && category->id &&
        !strcmp (applet->category, category->id))
    {
        return 0;
    }

    return 1;
}

static void
hcp_al_sort_by_category (gpointer key, gpointer value, gpointer _al)
{
    HCPAppList *al = (HCPAppList *)_al;
    GSList *category_item = NULL;
    HCPItem *applet = (HCPItem *)value;
    HCPCategory *category;

    /* Find a category for this applet */
    category_item = g_slist_find_custom (al->categories,
                                         applet,
                                         (GCompareFunc)hcp_al_find_category);

    if (category_item)
    {
        category = (HCPCategory *)category_item->data;

    }
    else
    {
        /* If category doesn't exist or wasn't matched,
         * add to the default one (Extra) */
        
        category = g_slist_last (al->categories)->data;
    }
        
/*    category->applets = g_slist_append(category->applets, applet);*/
    category->applets = g_slist_insert_sorted (
                                    category->applets,
                                    applet,
                                    (GCompareFunc)hcp_item_sort_func);
}

static void
hcp_al_get_configured_categories (HCPAppList *al)
{
    GConfClient * client     = NULL;
    GSList * group_names     = NULL;
    GSList * group_ids       = NULL;
    GError * error = NULL;

    client = gconf_client_get_default ();
    
    if (client)
    {
        /* Get the group names */
        group_names = gconf_client_get_list (client, 
			GCONF_CONTROLPANEL_GROUPS_KEY,
			GCONF_VALUE_STRING, &error);

        if (error)
        {
            ULOG_ERR (error->message);
            g_error_free (error);
            g_object_unref (client);
            goto cleanup;
        }
        
        /* Get the group ids */
        group_ids = gconf_client_get_list (client, 
			GCONF_CONTROLPANEL_GROUP_IDS_KEY,
			GCONF_VALUE_STRING, &error);
        
        if (error)
        {
            ULOG_ERR (error->message);
            g_error_free (error);
            g_object_unref (client);
            goto cleanup;
        }

        g_object_unref (client);
    }

    while (group_ids && group_names)
    {
        HCPCategory *category = g_new0 (HCPCategory, 1);
        category->name = (gchar *)group_names->data;
        category->id   = (gchar *)group_ids->data;

        al->categories = g_slist_append (al->categories, category);

        group_ids   = g_slist_next (group_ids);
        group_names = g_slist_next (group_names);
    }

cleanup:
    if (group_names)
        g_slist_free (group_names);

    if (group_ids)
        g_slist_free (group_ids);
    
}

static void
hcp_al_empty_category (HCPCategory* category)
{
    g_slist_free (category->applets);
    category->applets = NULL;
}

void
hcp_al_update_list (HCPAppList *al)
{
    /* Clean the previous list */
    g_hash_table_foreach_remove (al->applets, (GHRFunc)hcp_al_free_item, NULL);

    /* Read all the entries */
    hcp_al_read_desktop_entries (CONTROLPANEL_ENTRY_DIR, al->applets);

    /* Place them is the relevant category */
    g_slist_foreach (al->categories, (GFunc)hcp_al_empty_category, NULL);
    g_hash_table_foreach (al->applets,
                           (GHFunc)hcp_al_sort_by_category,
                           al);

}

HCPAppList *
hcp_al_new ()
{
    HCPAppList *al;
    HCPCategory *extras_category = g_new0 (HCPCategory, 1);

    al = g_new0 (HCPAppList, 1);

    al->applets = g_hash_table_new (g_str_hash, g_str_equal);

    hcp_al_get_configured_categories (al);
    
    /* Add the default category as the last one */
    extras_category->name = g_strdup (HILDON_CP_SEPARATOR_DEFAULT);
    extras_category->id   = g_strdup ("");

    al->categories = g_slist_append (al->categories, extras_category);

    return al;
}


gboolean
hildon_cp_applist_focus_item (HCPAppList *al, const gchar * entryname)
{
    HCPItem *applet;

    applet = g_hash_table_lookup (al->applets, entryname);

    if (applet)
    {   
        gtk_widget_grab_focus (applet->grid_item);
        return TRUE;
    }
    
    return FALSE;
}

