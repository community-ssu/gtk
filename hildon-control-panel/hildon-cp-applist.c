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

#include <langinfo.h>
#include <hildon-widgets/hildon-grid.h>
#include <hildon-widgets/hildon-grid-item.h>
#include <gtk/gtkscrolledwindow.h>
#include <gtk/gtkhbox.h>
#include <gdk/gdkkeysyms.h>
#include "hildon-cp-applist.h"

#include <libosso.h>

/* Log include */
#include <log-functions.h>

#include <locale.h>
#include <libintl.h>

#include <linux/limits.h>

#include <libmb/mbutil.h>

#define _(String) gettext(String)

/* Should these be static, perhaps? */
GtkWidget *table = NULL;
gchar entry_path[PATH_MAX];
hildon_applist_focus_cb_f * global_focus_callback;
hildon_applist_activate_cb_f * global_activate_callback;

static GtkWidget * focused_item;

/* Type for List of (HildonGridItem*,MbDotDesktop*) pairs */

typedef struct _item_desktop {
    GtkWidget * item;
    MBDotDesktop *entry;
} item_desktop_st;

/* List of (HildonGridItem*,MbDotDesktop*) pairs */

static GSList *app_list = NULL;

/* Applist handling functions */
static void applist_free(GSList *list);
static gboolean applist_append(GSList **list, GtkWidget * item,
                               MBDotDesktop * entry );
static MBDotDesktop* applist_search_entry(GSList *list, GtkWidget *item);

/* HildonGrid callbacks */
static void _activate(HildonGrid*, HildonGridItem *item, gpointer data);
static void _changefocus(GtkContainer * container, GtkWidget * widget,
                         gpointer data);

static void read_desktop_entries( const gchar *dir_path, GSList **items );
static void create_items( HildonGrid * grid, const gchar * entry_path);

/*
 * Return 0 when the names of the mbdot desktop entries  are equal
 */
static gint mb_list_compare( MBDotDesktop *self_entry,
                             MBDotDesktop *item_entry )
{
    gint iRet;
    gchar *self_name, *item_name;

    g_return_val_if_fail( self_entry, 0 );
    g_return_val_if_fail( item_entry, 0 );

    self_name = (gchar *)mb_dotdesktop_get(self_entry, "Name"); 
    item_name = (gchar *)mb_dotdesktop_get(item_entry, "Name"); 

    if ( self_name == NULL )
        return -1;

    if ( item_name == NULL )
        return 1;

    iRet = g_utf8_collate(self_name, item_name);

    if (iRet == 0)
    {
        gchar *self_file, *item_file;
        
        self_file = mb_dotdesktop_get_filename(self_entry);
        item_file = mb_dotdesktop_get_filename(item_entry);

        if ( self_file == NULL )
            return -1;
        
        if ( item_file == NULL )
            return 1;

        iRet = g_utf8_collate(self_file, item_file);
    }

    return iRet;
}

/**
 * read_desktop_entry:
 * @dir_path: path to directory where dotdesktop files are
 * @items: list where items are stored
 *
 * Reads  desktop entries to the  items list
 *
 **/

static void read_desktop_entries( const gchar *dir_path, GSList **items )
{
  GDir *dir;
  GError * error = NULL;
  MBDotDesktop * entry;

  g_return_if_fail(dir_path);
  g_return_if_fail(items);
  dir = g_dir_open(dir_path, 0, &error);
  if(!dir)
  {
    osso_log( LOG_ERR, error->message);
    g_error_free(error);

    return;
  }
  while (TRUE)
  {
    const gchar *name;
    gchar *path=NULL;
    name = g_dir_read_name(dir);
    if (name == NULL)
    {
      break;
    }

    path = g_strconcat(dir_path, G_DIR_SEPARATOR_S, name, NULL);

    if (path == NULL)
    {
        continue;
    }

    if (g_file_test(path, G_FILE_TEST_IS_DIR))
    {
      g_free(path);
      continue;
    }

    if (!g_str_has_suffix(path, ".desktop") &&
        !g_str_has_suffix(path, ".kdelnk")) /*TODO: needed? */
    {
      g_free(path);
      continue;
    }
    entry = mb_dotdesktop_new_from_file(path);
    if(!entry)
    {
      osso_log( LOG_WARNING, "Error reading entry file %s\n", path );
      g_free(path);
      continue;
    }

    *items = g_slist_append(*items, entry);
    g_free(path);
  }

  g_dir_close(dir);
}

/**
 * create_items:
 * @grid: grid where to add items
 * @entry_path: where to find the files
 *
 * Creates HildonGridItems and adds to the grid.
 **/
static void create_items( HildonGrid * grid,
                          const gchar * entry_path)
{
    gboolean success = TRUE;
    GSList * dlist;
    GSList * desktop_entries = NULL;
    const gchar * additional_applet_dir;

    g_return_if_fail(grid);

    read_desktop_entries(entry_path, &desktop_entries);

    additional_applet_dir = g_getenv(ADDITIONAL_CP_APPLETS_DIR_ENVIRONMENT);

    if (additional_applet_dir != NULL) 
    {
        read_desktop_entries(additional_applet_dir, &desktop_entries);
    }


    if(desktop_entries)
        desktop_entries = g_slist_sort(desktop_entries,
                                       (GCompareFunc) mb_list_compare);

    dlist = desktop_entries;
    while (dlist && success)
    {
        MBDotDesktop *entry = (MBDotDesktop *)dlist->data;
        GtkWidget *item = hildon_grid_item_new_with_label(
            (gchar *)mb_dotdesktop_get(entry, "Icon"),
            _((gchar *)mb_dotdesktop_get(entry, "Name")));
        success = applist_append( &app_list, item, entry );
        gtk_container_add(GTK_CONTAINER(grid), item);
        g_print("Adding item %s %d\n", 
                (gchar *)mb_dotdesktop_get(entry, "Name"),
                (int)item);
        dlist = dlist->next;
    }
    g_slist_free(desktop_entries);
}

/**
 * _set_focus_after_dnotify:
 *
 * Resets the focus to the item that was previously focused (based on 
 * the filename). If the previously focused item has been removed, the
 * next in alphabetical order is focused
 */

static void _set_focus_after_dnotify(GtkWidget * table,
                                     const gchar * filename,
                                     const gchar * name)
{
    GSList * cur = app_list;
    GSList * prev = app_list;

    g_message("_set_focus_after_dnotify filename=%s name=%s", filename,
              name);

    if( (table==NULL) || ! HILDON_IS_GRID(table) ||
        (filename==NULL) || name == NULL)
        return;
    
    if(hildon_cp_applist_focus_item(filename))
        return;
    else
         /* the filename was removed, look for the next */
    {
        g_message("previous focused %s was removed", filename);
        while(cur)
        {
            gchar * curname = mb_dotdesktop_get(
                ((item_desktop_st*) cur->data)->entry, "Name");
            gchar * curfname = mb_dotdesktop_get_filename(
                ((item_desktop_st*) cur->data)->entry);
            gint namecmp, filecmp;

            g_message("Checking %s against %s", curfname, filename);

            /* invalid names are considered to be the last */
            /* Name -- visible to the user and primary sort key */
            if(curname == NULL)
                namecmp = -1;
            else
                namecmp = g_utf8_collate ( curname, name );

            /* Filename -- .desktop file name, secondary sort key */
            if(curfname == NULL)
                filecmp = -1;
            else
                filecmp = g_utf8_collate( curfname, filename );
            
            /* continue the loop, curitem is smaller than the previous */
            if( (namecmp <= -1) ||
                ((namecmp == 0) && (filecmp <= -1)) )
            {
                prev = cur;
                cur = cur->next;
                g_message("continuing the loop");
            } 

            /* focus the current one, break the loop */
            else if( (namecmp >= 1) || 
                     ((namecmp == 0) && (filecmp >= 1)) )
            {
                hildon_cp_applist_focus_item(curfname);
                g_message("ending the loop: name=%d file=%d",
                          namecmp, filecmp);
                return;
            }
        }
        g_message("out of the loop");
    }

    /* focus the last entry */
    if(prev && prev->data)
        hildon_cp_applist_focus_item(
            mb_dotdesktop_get_filename(
                ((item_desktop_st*) prev->data)->entry));
    
    g_message("focused the last plugin");
    
}


/**
 * hildon_cp_applist_initialize:
 * @activate_callback: called when an item gets activated
 * @activate_data: passed to activate_callback
 * @focus_callback: called when focus changes
 * @focus_data: passed to focus_callback
 * @hildon_appview: A GtkContainer where the grid is packed
 * @path: where .desktop files are found
 **/
void hildon_cp_applist_initialize(
    hildon_applist_activate_cb_f * activate_callback,
    gpointer activate_data,
    hildon_applist_focus_cb_f * focus_callback,
    gpointer focus_data,
    GtkWidget *hildon_appview,
    gchar *path)
{
    GDir * ret;
    
    g_return_if_fail(activate_callback);
    g_return_if_fail(hildon_appview);
    g_return_if_fail(path);
    g_return_if_fail(focus_callback);


    global_focus_callback = focus_callback;
    global_activate_callback = activate_callback;
    
    ret = g_dir_open(path, 0, NULL);
    g_return_if_fail(ret);
    g_dir_close(ret);

    strncpy(entry_path, path, PATH_MAX);

    table = hildon_grid_new();
    g_signal_connect(table, "activate-child",
                     G_CALLBACK(_activate),
                     activate_data);
    g_signal_connect(table, "set-focus-child",
                     G_CALLBACK(_changefocus),
                     focus_data);
    
    gtk_container_add( GTK_CONTAINER(hildon_appview), GTK_WIDGET(table) );

    create_items(HILDON_GRID(table), entry_path);
}


/**
 * hildon_cp_applist_reread_dot_desktops:
 **/
void hildon_cp_applist_reread_dot_desktops( void )
{
    GSList *list;
    GtkWidget *item;
    MBDotDesktop *focus_entry;

    gchar * filename = NULL;
    gchar * name = NULL;
    gint scrollvalue;
    
    focus_entry = applist_search_entry(app_list, focused_item);
    if(focus_entry != NULL)
    {
        name = g_strdup((gchar *)mb_dotdesktop_get(focus_entry, "Name"));
        filename = g_strdup(mb_dotdesktop_get_filename(focus_entry));
    }
    /* scrollbar position */

    scrollvalue = hildon_grid_get_scrollbar_pos(HILDON_GRID(table));

    list = app_list;

    while(list)
    {
        item = ((item_desktop_st *)list->data)->item;
        gtk_container_remove( GTK_CONTAINER(table), item);
        list = list->next;
    }
    applist_free(app_list);
    app_list = NULL;
    create_items( HILDON_GRID(table), entry_path);

    /* Reset the scrollbar position */
    hildon_grid_set_scrollbar_pos(HILDON_GRID(table), scrollvalue);

    /* set focus for 
       1) The same item as before (matched against the .desktop filename)
       2) The next item that exists, if the previously focused item has
          been removed
       3) The last item if all items after the previously focused item
          have been removed.
    */
    _set_focus_after_dnotify(table, filename, name);

    gtk_widget_show_all( GTK_WIDGET(table) );
}

/* Return a handle to the HildonGrid */
GtkWidget * hildon_cp_applist_get_grid( void )
{
    return table;
}


/* Find the desktop entry struct that corresponds to filename entryname.
 * Return NULL if the entry was not found. */

MBDotDesktop*
hildon_cp_applist_get_entry( const gchar * entryname)
{
    GSList * list = app_list;
    MBDotDesktop * ret = NULL;
    if (entryname == NULL)
        return NULL;
    while(list)
    {
        if( strcmp( entryname, 
                    mb_dotdesktop_get_filename(
                        ((item_desktop_st *)list->data)->entry)) == 0)
        {
            ret = ((item_desktop_st *)list->data)->entry;
            break;
        }
        list=list->next;
    }
    return ret;
}

/* Set focus to the HildonGridItem corresponding to .desktop filenam
   entryname. This is used on state retreival by the application.
 * Return TRUE if the entry name was found, false otherwise.
 */
gboolean
hildon_cp_applist_focus_item(const gchar * entryname)
{
    GSList * list = app_list;
    if(entryname == NULL)
        return FALSE;
    while(list)
    {
        if( strcmp( entryname, 
                    mb_dotdesktop_get_filename(
                        ((item_desktop_st *)list->data)->entry)) == 0)
        {
            g_message("Setting focus for %s\n",mb_dotdesktop_get_filename(
                          ((item_desktop_st *)list->data)->entry) );
            gtk_container_set_focus_child(GTK_CONTAINER(table),
                                   ((item_desktop_st *)list->data)->item);
            return TRUE;
        }
        list=list->next;
    }
    return FALSE;
}

/* Free the list of griditem,mbdotdesktop pairs */
static void applist_free(GSList * list)
{
    while(list)
    {
        mb_dotdesktop_free( ((item_desktop_st *)list->data)->entry);
        g_free(list->data);
        list=list->next;
    }
    g_slist_free(list);
}


/* Add a griditem,mbdotdesktop pair to the list */

static gboolean applist_append(GSList **list, GtkWidget * item,
                               MBDotDesktop * entry )
{
    item_desktop_st * itempair = g_malloc0(sizeof(item_desktop_st));
    if(!itempair)
        return FALSE;

    itempair->item = item;
    itempair->entry = entry;
    *list = g_slist_append(*list, itempair);

    return TRUE;
}

/* Return the MBDotDesktop that corresponds to GridItem item */

static MBDotDesktop* applist_search_entry(GSList *list, GtkWidget *item)
{
    while(list && (item_desktop_st *) list->data
          && ((item_desktop_st *) list->data)->item != item)
    {
        list = list->next;
    }
    if(list)
    {
        return ((item_desktop_st*) list->data)->entry;
    }
    else
        return NULL;
}

/* Callback for HildonGrid activation */
static void _activate(HildonGrid* grid, HildonGridItem *item,gpointer data)
{
    global_activate_callback(applist_search_entry(app_list,
                                                  GTK_WIDGET(item)),data,1);
}

/* Callback for HildonGrid focus change */
static void _changefocus(GtkContainer * container, GtkWidget * widget,
                         gpointer data)
{
    MBDotDesktop* dd = applist_search_entry(app_list, widget);

    global_focus_callback(dd, data);
    focused_item = GTK_WIDGET(widget);
}
