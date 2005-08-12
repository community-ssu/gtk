/*
 * This file is part of maemo-af-desktop
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

#define USE_AF_DESKTOP_MAIN__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <log-functions.h>

/* Hildon includes */
#include "hildon-status-bar-lib.h"
#include "hildon-status-bar-item.h"
#include <hildon-widgets/gtk-infoprint.h>
#include <hildon-widgets/hildon-note.h>

/* System includes */
#include <string.h>

/* GTK includes */
#include <gtk/gtkwindow.h>
#include <gtk/gtkmain.h>
#include <gtk/gtkbutton.h>
#include <gtk/gtkfixed.h>

/* GDK includes */
#include <gdk/gdkwindow.h>
#include <gdk/gdkx.h>

/* X include */
#include <X11/X.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>

#include "hildon-status-bar-main.h"
#include "hildon-status-bar-interface.h"
#include "../kstrace.h"


static void init_dock( StatusBar *panel );
static void add_prespecified_items( StatusBar *panel );
static void show_infoprint( const gchar *msg );


static gint rpc_cb( const gchar *interface, 
                    const gchar *method,
                    GArray *arguments, 
                    gpointer data,
                    osso_rpc_t *retval );

static HildonStatusBarItem *get_item( StatusBar *panel, const gchar *plugin );
static HildonStatusBarItem *add_item( StatusBar *panel, const gchar *plugin );

static void free_slot( StatusBar *panel );
static void reorder_items( StatusBar *panel );
static void destroy_item( GtkObject *object,
                          gpointer user_data );




static void init_dock( StatusBar *panel )
{
    Atom atoms[3];
    gint i;
    Display   *dpy;
    Window    win;

    for( i = 0; i < HILDON_STATUS_BAR_MAX_NO_OF_ITEMS; ++i )
        panel->items[i] = NULL;

    panel->window = gtk_window_new( GTK_WINDOW_TOPLEVEL );
    gtk_window_set_accept_focus(GTK_WINDOW(panel->window), FALSE);
    gtk_container_set_border_width( GTK_CONTAINER( panel->window ), 0 );
    gtk_widget_set_name( panel->window, "HildonStatusBar" );

    /* Set window type as DOCK */
    gtk_window_set_type_hint( GTK_WINDOW( panel->window ), 
                              GDK_WINDOW_TYPE_HINT_DOCK );

    /* GtkWindow must be realized before using XChangeProperty */
    gtk_widget_realize( GTK_WIDGET( panel->window ) );

    /* We need to set this here, because otherwise the GDK will replace 
     * our XProperty settings. The GDK needs to a patch for this, this 
     * has nothing to do with the vanilla GDK. */
    g_object_set_data( G_OBJECT( panel->window->window ),
                       "_NET_WM_STATE", (gpointer)PropModeAppend );

    dpy = GDK_DISPLAY();    
    win = GDK_WINDOW_XID( panel->window->window );

    /* Crete the atoms we need */
    atoms[0] = XInternAtom( dpy, "_NET_WM_STATE", False);
    atoms[1] = XInternAtom( dpy, "_MB_WM_STATE_DOCK_TITLEBAR", False);
    atoms[2] = XInternAtom( dpy, "_MB_DOCK_TITLEBAR_SHOW_ON_DESKTOP", False);

    /* Set proper properties for the matchbox to show 
     * dock as the dock we want. 2 Items to show. */

    XChangeProperty( dpy, win, 
                     atoms[0], XA_ATOM, XLIB_FORMAT_32_BIT, 
                     PropModeReplace, 
                     (unsigned char *) &atoms[1], 2 );

    panel->fixed = gtk_fixed_new();
    gtk_container_set_border_width( GTK_CONTAINER( panel->fixed ), 0 );

    gtk_container_add( GTK_CONTAINER( panel->window ), panel->fixed );

    panel->plugin_pos_x[0] = HILDON_STATUS_BAR_ITEM0_X;
    panel->plugin_pos_x[1] = HILDON_STATUS_BAR_ITEM1_X;
    panel->plugin_pos_x[2] = HILDON_STATUS_BAR_ITEM2_X;
    panel->plugin_pos_x[3] = HILDON_STATUS_BAR_ITEM3_X;
    panel->plugin_pos_x[4] = HILDON_STATUS_BAR_ITEM4_X;
    panel->plugin_pos_x[5] = HILDON_STATUS_BAR_ITEM5_X;
    panel->plugin_pos_x[6] = HILDON_STATUS_BAR_ITEM6_X;

    panel->plugin_pos_y[0] = HILDON_STATUS_BAR_ITEM0_Y;
    panel->plugin_pos_y[1] = HILDON_STATUS_BAR_ITEM1_Y;
    panel->plugin_pos_y[2] = HILDON_STATUS_BAR_ITEM2_Y;
    panel->plugin_pos_y[3] = HILDON_STATUS_BAR_ITEM3_Y;
    panel->plugin_pos_y[4] = HILDON_STATUS_BAR_ITEM4_Y;
    panel->plugin_pos_y[5] = HILDON_STATUS_BAR_ITEM5_Y;
    panel->plugin_pos_y[6] = HILDON_STATUS_BAR_ITEM6_Y;
}

static void add_prespecified_items( StatusBar *panel )
{

    if (add_item( panel, "sound" )==NULL)
    {
        osso_log( LOG_ERR, "Statusbar item add failed for sound" );
    }
    if (add_item( panel, "internet" )==NULL)
    {
        osso_log( LOG_ERR, "Statusbar item add failed for internet" );
    }
    if (add_item( panel, "gateway" ) ==NULL)
    {
        osso_log( LOG_ERR, "Statusbar item add failed for gateway" );
    }
    if (add_item( panel, "battery" ) == NULL)
    {
        osso_log( LOG_ERR, "Statusbar item add failed for battery" );
    }
    if (add_item( panel, "display" ) == NULL)
    {
        osso_log( LOG_ERR, "Statusbar item add failed for display" );
    }
}

static void show_infoprint( const gchar *msg )
{
    g_return_if_fail( msg );

    gtk_infoprint( NULL, msg );
}

static gint rpc_cb( const gchar *interface, 
                    const gchar *method,
                    GArray *arguments, 
                    gpointer data, 
                    osso_rpc_t *retval )
{
    HildonStatusBarItem *item;
    StatusBar *panel;
    osso_rpc_t *val[4];
    gint i;
    
    /* These must be specified */
    if( !interface || !method || !arguments || !data )
    {
        retval->type = DBUS_TYPE_STRING;
        retval->value.s = g_strdup_printf( "NULL parameter" );
        g_warning( "%s\n", retval->value.s );
        return OSSO_ERROR;
    }

    panel = (StatusBar*)data;
 
    /* get the parameters */
    for( i = 0; i < arguments->len; ++i )
        val[i] = &g_array_index( arguments, osso_rpc_t, i );

    /* check the method */
    /* infoprint */
    if( g_str_equal( "system_note_infoprint", method ) )
    {
        /* Check that the parameter is of right type */
        if( arguments->len < 1 ||
            val[0]->type != DBUS_TYPE_STRING )
        {
            retval->type = DBUS_TYPE_STRING;
            if( arguments->len < 1 )
                retval->value.s = 
                    g_strdup_printf( "Not enough arguments to infoprint" );
            else
                retval->value.s = 
                    g_strdup_printf("Wrong type of parameter to infoprint (%d)",
                                     val[0]->type );
            osso_log( LOG_ERR, retval->value.s );

            return OSSO_ERROR;       
        }
        show_infoprint( val[0]->value.s );
    }
    /* dialog */
    else if( g_str_equal( "system_note_dialog", method ) )
    {
        /* check arguments */
        if( arguments->len < 2 || 
            val[0]->type != DBUS_TYPE_STRING || 
            val[1]->type != DBUS_TYPE_INT32 ) 
        {
            retval->type = DBUS_TYPE_STRING;
            if( arguments->len < 2 )
                retval->value.s = 
                    g_strdup_printf( "Not enough arguments to dialog" );
            else
                retval->value.s = 
                    g_strdup_printf("Wrong type of arguments to dialog (%d,%d)"
                                    ";  was expecting (%d, %d)",
                                    val[0]->type, val[1]->type,
                                    DBUS_TYPE_STRING, DBUS_TYPE_INT32);
            osso_log( LOG_ERR, retval->value.s );
            return OSSO_ERROR;       
        }

        hildon_status_bar_lib_prepare_dialog( val[1]->value.i, NULL, 
                                              val[0]->value.s, 0, NULL, NULL );
    }
    /* plugin */
    else if( g_str_equal( "event", method ) )
    {
        /* check arguments */
        if( arguments->len < 4 || 
            val[0]->type != DBUS_TYPE_STRING ||
            val[1]->type != DBUS_TYPE_INT32 ||
            val[2]->type != DBUS_TYPE_INT32 ||
            ( val[3]->type != DBUS_TYPE_STRING && 
              val[3]->type != DBUS_TYPE_NIL ) )
        {
            retval->type = DBUS_TYPE_STRING;
            if( arguments->len < 4 )
                retval->value.s = 
                    g_strdup_printf( "Not enough arguments to plugin" );
            else
                retval->value.s = 
                    g_strdup_printf( "Wrong type of arguments to a plugin "
                                     "(%d,%d,%d,%d); "
                                     "was expecting (%d, %d, %d ,%d or %d)",
                                     val[0]->type, val[1]->type, val[2]->type, 
                                     val[3]->type,
                                     DBUS_TYPE_STRING, DBUS_TYPE_INT32,
                                     DBUS_TYPE_INT32, DBUS_TYPE_STRING,
                                     DBUS_TYPE_NIL);
            osso_log( LOG_ERR, retval->value.s );
            return OSSO_ERROR;
        }

        /* Check if the method item is not already loaded */
        if( !(item = get_item( panel, val[0]->value.s ) ) )
            item = add_item( panel, val[0]->value.s );
        
        /* plugin loading failed */
        if( !item )
        {
            retval->type = DBUS_TYPE_STRING;
            retval->value.s = g_strdup_printf( "Failed to load plugin %s", 
                                               val[0]->value.s );
            osso_log( LOG_ERR, retval->value.s );
            return OSSO_ERROR;
        }

        hildon_status_bar_item_update( item, 
                                       val[1]->value.i, 
                                       val[2]->value.i, 
                                       val[3]->value.s );
    }
    else
    {
        retval->type = DBUS_TYPE_STRING;
        retval->value.s = g_strdup_printf( "Unknown method '%s'", method );
        osso_log( LOG_ERR, retval->value.s );
        return OSSO_ERROR;
    }

    return OSSO_OK;
}

static HildonStatusBarItem *get_item( StatusBar *panel, const gchar *plugin )
{
    gint i;

    g_return_val_if_fail( panel, NULL );
    g_return_val_if_fail( plugin, NULL );

    /* Go through the array and check if the plugin match */
    for( i = 0; i < HILDON_STATUS_BAR_MAX_NO_OF_ITEMS; ++i )
    {
        if( panel->items[i] != NULL &&
            g_str_equal( hildon_status_bar_item_get_name(
                         HILDON_STATUS_BAR_ITEM( panel->items[i] ) ), 
                         plugin ) )
            return HILDON_STATUS_BAR_ITEM( panel->items[i] );
    }
    
    /* No match found */
    return NULL;
}

static HildonStatusBarItem *add_item( StatusBar *panel, const gchar *plugin )
{
    HildonStatusBarItem *item;
    gint slot = -1;

    g_return_val_if_fail( panel, NULL );
    g_return_val_if_fail( plugin, NULL );

    /* check for prespecified items */
    if( g_str_equal( "sound", plugin ) )
        slot = HILDON_STATUS_BAR_SOUND_SLOT;
    else if( g_str_equal( "internet", plugin ) )
        slot = HILDON_STATUS_BAR_INTERNET_SLOT;
    else if( g_str_equal( "gateway", plugin ) )
        slot = HILDON_STATUS_BAR_GATEWAY_SLOT;
    else if( g_str_equal( "battery", plugin ) )
        slot = HILDON_STATUS_BAR_BATTERY_SLOT;
    else if( g_str_equal( "display", plugin ) )
        slot = HILDON_STATUS_BAR_DISPLAY_SLOT;

    item = hildon_status_bar_item_new( plugin );

    if( !item )
    {
        return NULL;
    }

    /* ref the item, so it is not destroyed when removed temporarily 
     * from container, or destroyed too early when the plugin is removed 
     * permanently */
    g_object_ref( item );

    /* The item is prespecified */
    if( slot != -1 )
    {
        panel->items[slot] = GTK_WIDGET( item );

        /* Add it to fixed container */
        gtk_fixed_put( GTK_FIXED( panel->fixed ), 
                       panel->items[slot], 
                       panel->plugin_pos_x[slot],
                       panel->plugin_pos_y[slot] );
        
    }
    /* unknown plugin */
    else
    {

        /* free the first slot and put the new item there */
        free_slot( panel );
       
        panel->items[HILDON_STATUS_BAR_FIRST_DYN_SLOT] = GTK_WIDGET( item );
        gtk_fixed_put( GTK_FIXED( panel->fixed ), 
                       panel->items[HILDON_STATUS_BAR_FIRST_DYN_SLOT],
                       panel->plugin_pos_x[HILDON_STATUS_BAR_FIRST_DYN_SLOT],
                       panel->plugin_pos_y[HILDON_STATUS_BAR_FIRST_DYN_SLOT] );
        gtk_widget_show( GTK_WIDGET( item ) );

    }

    /* catch the event of plugin destroying itself*/
    g_signal_connect( G_OBJECT( item ), "destroy",
                      G_CALLBACK( destroy_item ), 
                      panel );

    return item;
}

static void free_slot( StatusBar *panel )
{
    gint i, j;

    g_return_if_fail( panel );

    /* remove last item if too many items */
    if( panel->items[HILDON_STATUS_BAR_MAX_NO_OF_ITEMS - 1] != NULL )
    {
        osso_log( LOG_ERR, "Too many plugins. Defined maximum is %d!", 
                  HILDON_STATUS_BAR_MAX_NO_OF_ITEMS );
        
        gtk_widget_destroy(panel->items[HILDON_STATUS_BAR_MAX_NO_OF_ITEMS - 1]);
        g_object_unref( panel->items[HILDON_STATUS_BAR_MAX_NO_OF_ITEMS - 1]);
        panel->items[HILDON_STATUS_BAR_MAX_NO_OF_ITEMS - 1] = NULL;
    }    
        

    /* find the last item */
    for( i = HILDON_STATUS_BAR_MAX_NO_OF_ITEMS - 2; 
         i >= HILDON_STATUS_BAR_FIRST_DYN_SLOT; --i )
    {
        if( panel->items[i] == NULL )
            continue;
        
        /* move every item by one */
        for( j = i; j >= HILDON_STATUS_BAR_FIRST_DYN_SLOT; --j )
        {
            panel->items[j + 1] = panel->items[j];
            panel->items[j] = NULL;

            /* remove item from container if moved out of the 7 slots */
            if( j + 1 == HILDON_STATUS_BAR_VISIBLE_ITEMS )
                gtk_container_remove( GTK_CONTAINER( panel->fixed ), 
                                      panel->items[j + 1] );

            /* update the fixed if item moved inside the 7 slots */
            if( j + 1 < HILDON_STATUS_BAR_VISIBLE_ITEMS  )
                gtk_fixed_move( GTK_FIXED( panel->fixed ), panel->items[j + 1],
                                panel->plugin_pos_x[j + 1],
                                panel->plugin_pos_y[j + 1] );
        }
        break;
    }
}

/* FIXME: this should do some real ordering according to time and priority
 * but the spec does not yet specifiy how.
 * For now it only removes empty slots.
 */
static void reorder_items( StatusBar *panel )
{
    gint i, place_here;

    g_return_if_fail( panel );

    place_here = HILDON_STATUS_BAR_FIRST_DYN_SLOT;

    /* go through all dynamic items */
    for( i = place_here; 
         i < HILDON_STATUS_BAR_MAX_NO_OF_ITEMS; ++i )
    {

        if (panel->items[i] == NULL) 
        {
            continue;
        }
        if (place_here == i)
        {
            place_here++;
            continue;
        }
        
        /* i !=NULL and place_items NULL. */

        panel->items[place_here] = panel->items[i];
        panel->items[i] = NULL;

        
        /* update the fixed because location changes */
        if( place_here < HILDON_STATUS_BAR_VISIBLE_ITEMS - 1 )
            gtk_fixed_move( GTK_FIXED( panel->fixed ), 
                            panel->items[place_here],
                            panel->plugin_pos_x[place_here],
                            panel->plugin_pos_y[place_here] );
        
        /* add the plugin to the fixed */
        else if( place_here == HILDON_STATUS_BAR_VISIBLE_ITEMS - 1)
        {
            gtk_fixed_put( GTK_FIXED( panel->fixed ), 
                           panel->items[place_here],
                           panel->plugin_pos_x[place_here],
                           panel->plugin_pos_y[place_here] );
            gtk_widget_show( panel->items[place_here] );
        }

        place_here++;

    }
}


static void destroy_item( GtkObject *object,
                          gpointer user_data )
{
    gint i;
    StatusBar *panel;
    GtkWidget *item;

    g_return_if_fail( object );
    g_return_if_fail( user_data );

    panel = (StatusBar*)user_data;
    item = (GtkWidget *)object;

    /* find the item and destroy it */
    for( i = 0; i < HILDON_STATUS_BAR_MAX_NO_OF_ITEMS; ++i )
    {
        if( panel->items[i] == item )
        {
            g_object_unref( item );
            panel->items[i] = NULL;
            break;
        }
    }
    
    reorder_items( panel );
}

int status_bar_main(osso_context_t *osso, StatusBar **panel){


    StatusBar *sb_panel = NULL;
    TRACE(TDEBUG,"status_bar_main: 1 g_new0");
    sb_panel = g_new0 (StatusBar, 1);

    TRACE(TDEBUG,"status_bar_main: 2 check panel ptr");
    if( !sb_panel )
    {  
        osso_log( LOG_ERR, "g_new0 failed! Exiting.." );
        osso_deinitialize( osso );
        return 1;
    }
    TRACE(TDEBUG,"status_bar_main: 3: init dock");    

    /* initialize panel */
    init_dock( sb_panel );

    TRACE(TDEBUG,"status_bar_main: 4 add prespecified items");
    add_prespecified_items( sb_panel );
    TRACE(TDEBUG,"status_bar_main: 5 gtk widget show all");
    gtk_widget_show_all( sb_panel->window );
    TRACE(TDEBUG,"status_bar_main: 6 if rpc...");
    /* set RPC cb */
    if( osso_rpc_set_default_cb_f( osso, rpc_cb, sb_panel ) != OSSO_OK )
    {
        osso_log( LOG_ERR, "osso_rpc_set_default_cb_f() failed" );
    }
    TRACE(TDEBUG,"status_bar_main: 7, status bar initialized successfully");
    *panel = sb_panel;
    return 0;

}

void status_bar_deinitialize(osso_context_t *osso, StatusBar **panel){
  gint i=0;
  StatusBar *sb_panel = (StatusBar *)(*panel);

  TRACE(TDEBUG,"status_bar_deinitialize: 1");
  for( i = 0; i < HILDON_STATUS_BAR_MAX_NO_OF_ITEMS; ++i )
  {
      if( sb_panel->items[i] != NULL )
      {
          TRACE(TDEBUG,"status_bar_deinitialize: 2 gtk widget destroy");
          gtk_widget_destroy( sb_panel->items[i] );
      }
  }
  if( osso_rpc_unset_default_cb_f( osso, rpc_cb, sb_panel ) != OSSO_OK )
  {
      osso_log( LOG_ERR, "osso_rpc_unset_default_cb_f() failed" );
  }
  TRACE(TDEBUG,"status_bar_deinitialize: 3"); 
  gtk_widget_destroy( sb_panel->window );
  TRACE(TDEBUG,"status_bar_deinitialize: 4");
  g_free( *panel );

}


#ifndef USE_AF_DESKTOP_MAIN__

int main( int argc, char *argv[] )
{
  StatusBar *panel=NULL;
  osso_context_t *osso;

  
  TRACE(TDEBUG,"Status bar: main - this code is executed only when Status bar is run standalone.");

  gtk_init( &argc, &argv );
  
    /* initialize osso */
  osso = osso_initialize( HILDON_STATUS_BAR_NAME, 
                          HILDON_STATUS_BAR_VERSION, FALSE, NULL );
  if( !osso )
  {  
      osso_log( LOG_ERR, "osso_initialize failed! Exiting.." );
      return 1;
  }
  
  status_bar_main(osso, &panel); 
  gtk_main();
  status_bar_deinitialize(osso, &panel);  
  osso_deinitialize( osso );
  
  return 0;
}

#endif

