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

/**
* @file hildon-status-bar-item.h
* @brief
* 
*/

#ifndef HILDON_STATUS_BAR_ITEM_H
#define HILDON_STATUS_BAR_ITEM_H

#include <gtk/gtkcontainer.h>

G_BEGIN_DECLS

#define HILDON_STATUS_BAR_ITEM_TYPE ( hildon_status_bar_item_get_type() )
#define HILDON_STATUS_BAR_ITEM(obj) (GTK_CHECK_CAST (obj, \
            HILDON_STATUS_BAR_ITEM_TYPE, \
            HildonStatusBarItem))
#define HILDON_STATUS_BAR_ITEM_CLASS(klass) (GTK_CHECK_CLASS_CAST ((klass), \
            HILDON_STATUS_BAR_ITEM_TYPE, HildonStatusBarItemClass))
#define HILDON_IS_STATUS_BAR_ITEM(obj) (GTK_CHECK_TYPE (obj, \
            HILDON_STATUS_BAR_ITEM_TYPE))
#define HILDON_IS_STATUS_BAR_ITEM_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), \
            HILDON_STATUS_BAR_ITEM_TYPE))
#define HILDON_STATUS_BAR_ITEM_GET_PRIVATE(obj) \
        (G_TYPE_INSTANCE_GET_PRIVATE ((obj), \
        HILDON_STATUS_BAR_ITEM_TYPE, HildonStatusBarItemPrivate));

typedef struct _HildonStatusBarItem HildonStatusBarItem; 
typedef struct _HildonStatusBarItemClass HildonStatusBarItemClass;
typedef struct _HildonStatusBarPluginFn_st HildonStatusBarPluginFn_st; 

struct _HildonStatusBarItem
{
    GtkContainer parent;
};

struct _HildonStatusBarItemClass
{
    GtkContainerClass parent_class;
    void (*hildon_status_bar_update_conditional) 
	    (HildonStatusBarItem * self, gboolean conditional_status);

    void (*hildon_status_bar_log_start) (HildonStatusBarItem * self);
    void (*hildon_status_bar_log_end)   (HildonStatusBarItem * self);
};

/* Type definitions for the plugin API */ 
typedef void (*HildonStatusBarItemEntryFn)( HildonStatusBarPluginFn_st *fn );
typedef void *(*HildonStatusBarItemInitializeFn)( HildonStatusBarItem *item,
                                                  GtkWidget **button);
typedef void (*HildonStatusBarItemDestroyFn)( void *data );
typedef void (*HildonStatusBarItemUpdateFn)( void *data, 
		                             gint value1, 
                                             gint value2, 
					     const gchar *str);
typedef int (*HildonStatusBarItemGetPriorityFn)( void *data );
typedef void (*HildonStatusBarItemSetConditionalFn)( void *data, 
		                                     gboolean cond );


/* Struct for the plugin API function pointers */
struct _HildonStatusBarPluginFn_st 
{
    HildonStatusBarItemInitializeFn     initialize;
    HildonStatusBarItemDestroyFn        destroy;
    HildonStatusBarItemUpdateFn         update;
    HildonStatusBarItemGetPriorityFn    get_priority;
    HildonStatusBarItemSetConditionalFn set_conditional;
};

/**
 * @hildon_status_bar_item_get_type
 * 
 * This function registers the status bar item type.
 */
GType hildon_status_bar_item_get_type( void );

/**
 * @hildon_status_bar_item_new
 * 
 * @param *name string naming the plugin
 * 
 * @return HildonStatusBarItem the newly created plugin reference
 *
 * Creates a new statusbar item.
 */
HildonStatusBarItem *hildon_status_bar_item_new( const char* name, gboolean mandatory );


/**
 * @hildon_status_bar_item_new
 *
 * @param *name item to initialize
 *
 * Initialize a new statusbar item.
 */
void hildon_status_bar_item_initialize (HildonStatusBarItem *item);

/**
 * @hildon_status_bar_item_update
 * 
 * @param *item the item to update
 * @param value1 parameter to be given to the plugin's own update function
 * @param value2 parameter to be given to the plugin's own update function
 * @param str parameter to be given to the plugin's own update function
 *
 * Call's the item's update function and passes on the parameters.
 */
void hildon_status_bar_item_update( HildonStatusBarItem *item, 
                                    gint value1, 
                                    gint value2, 
                                    const gchar *str );

/**
 * @hildon_status_bar_item_update_conditional
 * 
 * @param *item the item to update
 * @param gboolean user_data parameter to be given 
 * to the plugin's own update function
 *
 * Call's the item's set conditional function and passes on the parameters.
 */
void hildon_status_bar_item_update_conditional_cb( HildonStatusBarItem *item,
		                                   gboolean user_data );

/**
 * @hildon_status_bar_item_set_position
 *
 * @param *item the item to update
 * @param gint position of item
 *
 * accessor function for setting item position
 */
void hildon_status_bar_item_set_position( HildonStatusBarItem *item, 
					  gint position );

/**
 * @hildon_status_bar_item_get_position
 *
 * @param *item the item to update
 * 
 * @return gint position of the item
 * 
 * accessor function to get item position
 **/
gint hildon_status_bar_item_get_position( HildonStatusBarItem *item );

/**
 * @hildon_status_bar_item_get_priority
 * 
 * @param *item item to get priority from
 * 
 * @return gint priority of the item
 *
 * accessor function to get item priority
 */
gint hildon_status_bar_item_get_priority( HildonStatusBarItem *item );

/**
 * @hildon_status_bar_item_get_name
 * 
 * @param *item item to get the name
 * 
 * @return *gchar name
 *
 * accessor function to get item name
 */
const gchar *hildon_status_bar_item_get_name( HildonStatusBarItem *item );

/**
 * @hildon_status_bar_item_get_conditional
 * 
 * @param *item item to get conditional from
 * 
 * @return gboolean conditional status of the item
 *
 * accessor function to get item conditional status
 */
gboolean hildon_status_bar_item_get_conditional( HildonStatusBarItem *item );

/**
* @hildon_status_bar_item_get_mandatory
*
* @param *item item to get mandatory from
*
* @return gboolean mandatory status of the item
*
* accessor function to get item mandatory status
*/
gboolean hildon_status_bar_item_get_mandatory( HildonStatusBarItem *item );

/**
 * @hildon_status_bar_item_set_button
 *
 * @param *item item to set button for
 *
 * accessor function to set item button
 */
void hildon_status_bar_item_set_button( HildonStatusBarItem *item,
		                        GtkWidget *button );

/**
 * @hildon_status_bar_item_get_button
 *
 * @param *item item to get button from
 *
 * @return GtkWidget* button of the item
 *
 * accessor function to get item button
 */
GtkWidget* hildon_status_bar_item_get_button( HildonStatusBarItem *item );


G_END_DECLS

#endif /* HILDON_STATUS_BAR_ITEM_H */
