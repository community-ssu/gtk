/* -*- mode:C; c-file-style:"gnu"; -*- */
/*
 * This file is part of maemo-af-desktop
 *
 * Copyright (C) 2006 Nokia Corporation.
 *
 * Contact: Karoliina Salminen <karoliina.t.salminen@nokia.com>
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
 *
 */

/**
* @file home-applet-manager.h
*/

#ifndef HOME_APPLET_MANAGER_H
#define HOME_APPLET_MANAGER_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

#include "home-applet-handler.h"

#define TYPE_APPLET_MANAGER		(applet_manager_get_type ())

#define APPLET_MANAGER(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_APPLET_MANAGER, AppletManager))
#define IS_APPLET_MANAGER(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_APPLET_MANAGER))
#define APPLET_MANAGER_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_APPLET_MANAGER, AppletManagerClass))
#define IS_APPLET_MANAGER_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_APPLET_MANAGER))
#define APPLET_MANAGER_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_APPLET_MANAGER, AppletManagerClass))

G_BEGIN_DECLS

typedef struct _AppletManager	     AppletManager;
typedef struct _AppletManagerPrivate AppletManagerPrivate;
typedef struct _AppletManagerClass   AppletManagerClass;

struct _AppletManager
{
  GObject                parent_instance;
  AppletManagerPrivate * priv;
};

struct _AppletManagerClass
{
  GObjectClass parent_class;
};

GType applet_manager_get_type (void) G_GNUC_CONST;


/**
 * applet_manager_get_instance:
 *
 *
 * @Returns a pointer to an instance AppletManager; the caller needs to
 * use g_object_unref () when no longer needed.
 */
AppletManager *
applet_manager_get_instance( void );

/**
 * applet_manager_add_applet:
 *
 *  Adds applet from desktopfile
 *   
 *  @param man Applet manager
 *  @param desktoppath File path for .desktop defining applet
 **/
void
applet_manager_add_applet (AppletManager * man, const gchar * desktoppath);

/**
 * applet_manager_set_window:
 *
 *  Sets the associated Home window
 *   
 *  @param man Applet manager 
 *  @param window Home window
 **/
void
applet_manager_set_window (AppletManager * man, GtkWidget * window);

/**
 * applet_manager_remove_applet_by_handler:
 *
 *  Removes applet from manager
 *   
 *  @param man Applet manager
 *  @param handler Applet handler representing applet
 **/
void
applet_manager_remove_applet_by_handler (AppletManager     * man,
					 HomeAppletHandler * handler);
/**
 * applet_manager_remove_applet:
 *
 *  Removes given applet from manager
 *   
 *  @param man Applet manager
 *  @param identifier Unique identier of applet 
 **/
void
applet_manager_remove_applet (AppletManager * man, const gchar * identifier);

/**
 * applet_manager_configure_save_all:
 *
 *  Saves all applets' location information to configure file
 *   
 *  @param man Applet manager
 **/
gboolean
applet_manager_configure_save_all (AppletManager *man);

/**
 * applet_manager_read_configuration:
 *
 *  Reads all applets' configured location information and relocates
 *  them accordingly
 *   
 *  @param man Applet manager 
 **/
void
applet_manager_read_configuration (AppletManager *man);

/**
 * applet_manager_foreground_all:
 *
 *  Foregrounds all applets
 *   
 *  @param man Applet manager
 **/
void
applet_manager_foreground_all (AppletManager *man);

/**
 * applet_manager_state_save_all:
 *
 *  State saves all applets
 *   
 *  @param man Applet manager 
 **/
void
applet_manager_state_save_all (AppletManager *man);

/**
 * applet_manager_background_all:
 *
 *  Backgrounds all applets
 *   
 *  @param man Applet manager 
 **/
void
applet_manager_background_all (AppletManager *man);

/**
 * applet_manager_background_state_save_all:
 *
 *  State saves and backgrounds all applets
 *   
 *  @param man Applet manager
 **/
void
applet_manager_background_state_save_all (AppletManager *man);

/**
 * applet_manager_get_eventbox:
 *
 *  Retrieves given applet's eventbox
 *   
 *  @param man Applet manager
 *  @param identifier Unique identier of applet instance
 *  @Returns eventbox of applet 
 **/
GtkEventBox *
applet_manager_get_eventbox (AppletManager * man, const gchar * identifier);

/**
 * applet_manager_get_settings:
 *
 *  Retrieves given applet's menu item widget 
 *   
 *  @param man Applet manager
 *  @param identifier Unique identier of applet instance
 *  @param parent a parent window.
 *  @Returns menu item widget connected to settings of applet 
 **/
GtkWidget *
applet_manager_get_settings(AppletManager * man, const gchar * identifier);

/**
 * applet_manager_get_identifier_all:
 *
 *  Retrieves list of used applet desktop file filepaths 
 *   
 *  @param man Applet manager 
 *  @return list of unique identiers of applet instances
 *
 *  NB: the caller must free the list when no longer needed, but must
 *  *not* free or modify the identifiers stored in the list.
 **/
GList *
applet_manager_get_identifier_all (AppletManager * man);

/**
 * applet_manager_set_coordinates:
 *
 *  Sets given applet's coordinates
 *   
 *  @param man Applet manager
 *  @param identifier Unique identier of applet instance
 *  @param x new x coordinate
 *  @param y new y coordinate
 **/
void
applet_manager_set_coordinates(AppletManager * man,
			       const gchar   * identifier,
			       gint            x,
			       gint            y);

/**
 * applet_manager_get_coordinates:
 *
 *  Gets given applet's coordinates
 *   
 *  @param man Applet manager
 *  @param identifier Unique identier of applet instance
 *  @param x storage place where x coordinate is saved
 *  @param y storage place where y coordinate is saved
 **/
void
applet_manager_get_coordinates (AppletManager * man, 
				const gchar   * identifier,
				gint          * x,
				gint          * y);

/**
 * applet_manager_set_size:
 *
 *  Sets given applet's size
 *   
 *  @param man Applet manager
 *  @param identifier Unique identier of applet instance
 *  @param width new width
 *  @param height new height
 **/
void
applet_manager_set_size (AppletManager * man, 
			 const gchar   * identifier,
			 gint            width,
			 gint            height);

/**
 * applet_manager_get_size:
 *
 *  Gets given applet's size
 *   
 *  @param man Applet manager
 *  @param identifier Unique identier of applet instance
 *  @param width storage place where width is saved
 *  @param height storage place where height is saved
 **/
void
applet_manager_get_size (AppletManager * man, 
			 const gchar   * identifier,
			 gint          * width,
			 gint          * height);
/**
 * applet_manager_set_minimum_size:
 *
 *  Sets given applet's minimum size
 *   
 *  @param man Applet manager
 *  @param identifier Unique identier of applet instance
 *  @param minwidth new width minimum
 *  @param minheight new height minimum
 **/
void
applet_manager_set_minimum_size (AppletManager * man, 
				 const gchar   * identifier,
				 gint            minwidth,
				 gint            minheight);

/**
 * applet_manager_get_minimum_size:
 *
 *  Gets given applet's minimum size
 *   
 *  @param man Applet manager
 *  @param identifier Unique identier of applet instance
 *  @param minwidth storage place where width minimum is saved
 *  @param minheight storage place where height minimum is saved
 **/
void
applet_manager_get_minimum_size (AppletManager * man, 
				 const gchar   * identifier,
				 gint          * minwidth,
				 gint          * minheight);

/**
 * applet_manager_set_resizable:
 *
 *  Sets given applet's resizable status
 *   
 *  @param man Applet manager
 *  @param identifier Unique identier of applet instance
 *  @param resizable_width new resizable status of width is saved
 *  @param resizable_height new resizable status of height is saved
 **/
void
applet_manager_set_resizable(AppletManager * man, 
			     const gchar   * identifier,
			     gboolean        resizable_width, 
			     gboolean        resizable_height);
/**
 * applet_manager_get_resizable:
 *
 *  Gets given applet's resizable status
 *   
 *  @param man Applet manager as returned by 
 *             applet_manager_singleton_get_instance
 *  @param identifier Unique identier of applet instance
 *  @param resizable_width storage place where resizable status of
 *  width is saved
 *  @param resizable_height storage place where resizable status of
 *  height is saved
 **/
void
applet_manager_get_resizable (AppletManager * man, 
			      const gchar   * identifier,
			      gboolean      * resizable_width, 
			      gboolean      * resizable_height);

/**
 * applet_manager_applet_is_resizable:
 *
 *  Checks if given applet is resizable to any direction
 *   
 *  @param man Applet manager
 *  @param identifier Unique identier of applet instance
 *  @return TRUE if either dimension is resizable
 **/
gboolean
applet_manager_applet_is_resizable (AppletManager * man,
				    const gchar   * identifier);

/**
 * applet_manager_identifier_exists:
 *
 *  Checks existence of identifier
 *   
 *  @param man Applet manager as returned by 
 *             applet_manager_singleton_get_instance
 *  @param identifier Unique identier of applet instance
 *  @return TRUE if identifier exists
 **/
gboolean
applet_manager_identifier_exists (AppletManager * man, 
				  const gchar   * identifier);

G_END_DECLS

#endif /* APPLET_MANAGER_H */

