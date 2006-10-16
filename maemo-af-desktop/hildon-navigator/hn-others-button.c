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

#include "hn-others-button.h"
#include "hn-wm.h"
#include "hn-app-switcher.h"

#include "libhildonmenu.h"
#include "hildon-thumb-menu-item.h"
#include "hildon-navigator.h"
#include "close-application-dialog.h"

#include <libosso.h>
#include <errno.h>
#include <sys/resource.h>
#include <string.h>

/* GLib include */
#include <glib.h>
#include <glib/gstdio.h>

/* GTK includes */
#include <gtk/gtkwidget.h>
#include <gtk/gtkcontainer.h>
#include <gtk/gtkmenu.h>
#include <gtk/gtkmenuitem.h>
#include <gtk/gtkimagemenuitem.h>
#include <gtk/gtkseparatormenuitem.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkmisc.h>

/* GDK includes */
#include <gdk/gdkkeysyms.h>
#include <gdk/gdkx.h>

/* X includes */
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

/* OSSO logging functions */
#include <osso-log.h>

/* For menu conf dir monitoring */
#include <hildon-base-lib/hildon-base-dnotify.h>

#define NAVIGATOR_BUTTON_THREE "hildon-navigator-button-three"

/* Button icon */

#define OTHERS_MENU_ICON_NAME "qgn_grid_tasknavigator_others"
#define OTHERS_MENU_ICON_SIZE 64

/* Menu icons */
#define MENU_ITEM_DEFAULT_ICON ""

#define MENU_ITEM_SUBMENU_ICON        "qgn_list_gene_fldr_cls"
#define MENU_ITEM_SUBMENU_ICON_DIMMED "qgn_list_gene_nonreadable_fldr"
#define MENU_ITEM_DEFAULT_APP_ICON    "qgn_list_gene_default_app"
#define MENU_ITEM_ICON_SIZE           26
#define MENU_ITEM_THUMB_ICON_SIZE     64

#define MENU_ITEM_EMPTY_SUBMENU_STRING _( "tana_li_of_noapps" )

#define BORDER_WIDTH 7
#define BUTTON_HEIGHT 90


#define MENU_Y_POS 180
#define MENU_MAX_WIDTH 360

#define WORKAREA_ATOM "_NET_WORKAREA"

#define HN_OTHERS_BUTTON_GET_PRIVATE(obj) \
(G_TYPE_INSTANCE_GET_PRIVATE ((obj), HN_TYPE_OTHERS_BUTTON, HNOthersButtonPrivate))

#define MENU_ITEM_N_ITEMS(n)    dngettext("hildon-fm", "sfil_li_folder_contents_item", "sfil_li_folder_contents_items", n)

/* forward declarations */
static void
hn_others_button_create_menu (HNOthersButton * button);

static void
hn_others_button_button_toggled (HNOthersButton *button, gpointer d);

static gboolean
hn_others_button_key_press (HNOthersButton *button, GdkEventKey *event,
			    gpointer d);

static gboolean
hn_others_button_button_press (HNOthersButton *button, GdkEventButton *event,
			       gpointer d);

static gboolean
hn_others_button_button_release(HNOthersButton *button, GdkEventButton *event,
				gpointer d);

     
/* HNOthersButton implementation */
struct _HNOthersButtonPrivate
{
  GtkWidget * menu;
  guint       collapse_id;
  gboolean    thumb_pressed;
  guint       dnotify_update_timeout;
};


G_DEFINE_TYPE (HNOthersButton, hn_others_button, GTK_TYPE_TOGGLE_BUTTON);

static void
hn_others_button_finalize (GObject *gobject)
{
  HNOthersButton *button = HN_OTHERS_BUTTON (gobject);

  /* cancel any pending timeouts */
  if (button->priv->collapse_id)
    g_source_remove (button->priv->collapse_id);

  if (button->priv->dnotify_update_timeout)
    g_source_remove (button->priv->dnotify_update_timeout);
  
  if (button->priv->menu)
    gtk_widget_destroy (button->priv->menu);
  
  G_OBJECT_CLASS (hn_others_button_parent_class)->finalize (gobject);
}


static void
hn_others_button_class_init (HNOthersButtonClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    
  gobject_class->finalize = hn_others_button_finalize;
    
  g_type_class_add_private (gobject_class, sizeof (HNOthersButtonPrivate));
}

static void
hn_others_button_init (HNOthersButton *button)
{
  GtkWidget * icon;
  HNOthersButtonPrivate *priv = HN_OTHERS_BUTTON_GET_PRIVATE (button);

  button->priv = priv;

  gtk_widget_set_name(GTK_WIDGET(button), NAVIGATOR_BUTTON_THREE);

  icon = gtk_image_new_from_pixbuf(get_icon( OTHERS_MENU_ICON_NAME,
					     OTHERS_MENU_ICON_SIZE ));

  if ( icon )
    {
      gtk_container_add(GTK_CONTAINER(button), icon );
    }

  /* button callbacks */
    g_signal_connect(G_OBJECT(button), "button-press-event",
 		     G_CALLBACK(hn_others_button_button_press), NULL);
    g_signal_connect(G_OBJECT(button), "button-release-event",
		     G_CALLBACK(hn_others_button_button_release), NULL);
    g_signal_connect(G_OBJECT(button), "toggled",
 		     G_CALLBACK(hn_others_button_button_toggled), NULL);
    g_signal_connect(G_OBJECT(button), "key-press-event",
 		     G_CALLBACK(hn_others_button_key_press), NULL);
  
  priv->menu                   = NULL;
  priv->collapse_id            = 0;
  priv->thumb_pressed          = FALSE;
  priv->dnotify_update_timeout = 0;
  
  hn_others_button_create_menu (button);
}

GtkWidget *
hn_others_button_new ()
{
  HNOthersButton *button;

  button = g_object_new (HN_TYPE_OTHERS_BUTTON, NULL);

  return GTK_WIDGET (button);
}

/* limits withd of the menu */
static void
hn_others_menu_size_request(GtkWidget * menu,
			    GtkRequisition * req,
			    gpointer data)
{
  if (req->width > MENU_MAX_WIDTH)
    {
      gtk_widget_set_size_request(GTK_WIDGET(menu), MENU_MAX_WIDTH, -1);
    }
}

/* untoggles button when menu is closed */
static gboolean
hn_others_menu_deactivate(GtkWidget * menu, HNOthersButton * button)
{
  g_return_val_if_fail(button, FALSE);

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),
			       FALSE);

  return TRUE;
}

static gboolean
hn_others_menu_key_press(GtkWidget      * menu,
			 GdkEventKey    * event,
			 HNOthersButton * button)
{
  if (event->keyval == GDK_Left    ||
      event->keyval == GDK_KP_Left ||
      event->keyval == GDK_Escape)
    {
      gtk_menu_shell_deactivate(GTK_MENU_SHELL(menu));

      if (event->keyval == GDK_Escape)
        {
	  /* pass focus to the last active application */
	  hn_wm_focus_active_window ();
        }
      else
	{
	  hn_wm_activate(HN_TN_ACTIVATE_KEY_FOCUS);
	  gtk_widget_grab_focus (GTK_WIDGET(button));
	}
      
      return TRUE;
    }

  return FALSE;
}

/* called when the item is activated */
static void
hn_others_menu_activate_item (GtkMenuItem * item, HNOthersButton * button)
{
    gchar  * service_field;
    gchar  * exec_field;
    gchar  * program = NULL;
    GError * error = NULL;
    
    g_return_if_fail(button);

    if ((service_field = g_object_get_data(G_OBJECT(item),
					   DESKTOP_ENTRY_SERVICE_FIELD)))
      {
	/* Launch the app or if it's already running move it to the top */
	hn_wm_top_service(service_field);
      }
    else
      {
        /* No way to track hibernation (if any), so call the dialog
         * unconditionally when in lowmem state
         */
        if (hn_wm_is_lowmem_situation())
          {
            if(!tn_close_application_dialog(CAD_ACTION_OPENING))
              {
                return;
              }
          }
        
	exec_field = g_object_get_data(G_OBJECT(item),
				       DESKTOP_ENTRY_EXEC_FIELD);

	if(exec_field)
	  {
	    gchar * space = strchr(exec_field, ' ');

	    if(space)
	      {
		gchar * cmd = g_strdup(exec_field);
		cmd[space - exec_field] = 0;

		gchar * exc = g_find_program_in_path(cmd);

		program = g_strconcat(exc, space, NULL);

		g_free(exc);
		g_free(cmd);
	      }
	    else
	      {
		program = g_find_program_in_path (exec_field);
	      }
	  }
	
        if(program)
	  {
            gint     argc;
            gchar ** argv;
            GPid     child_pid;
	    
            if (g_shell_parse_argv (program, &argc, &argv, &error))
              {
		g_spawn_async(
			      /* child's current working directory,
			       * or NULL to inherit parent's */
			      NULL,
			      /* child's argument vector. [0] is the path of
			       * the program to execute */
			      argv,
			      /* child's environment, or NULL to inherit
			       * parent's */
			      NULL,
			      /* flags from GSpawnFlags */
			      0,
			      /* function to run in the child just before
			       * exec() */
			      NULL,
			      /* user data for child_setup */
			      NULL,
			      /* return location for child process ID or
			       * NULL */
			      &child_pid,
			      /* return location for error */
			      &error);
              }
            
            if (error)
	      {
                ULOG_ERR( "others_menu_activate_item: "
			  "failed to execute %s: %s.",
                          exec_field, error->message);
		
                g_clear_error(&error);
	      }
	    else
	      {
                int priority;
                errno = 0;

                /* If the child process inherited desktop's high priority,
                 * give child default priority */
                priority = getpriority(PRIO_PROCESS, child_pid);
		
                if (!errno && priority < 0)
		  {
                    setpriority(PRIO_PROCESS, child_pid, 0);
		  }
	      }

	  }
	else
	  {
            /* We don't have the service name and we don't
             * have a path to the executable so it would be
             * quite difficult to launch the app.
             */
            ULOG_ERR( "hn_others_menu_activate_item: "
		      "both service name and binary path missing. "
                      "Unable to launch.");
        }
    }
    
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    g_free(program);
}

static void
hn_others_menu_get_items (GtkMenu * menu,
			  HNOthersButton * button,
			  GtkTreeModel * model,
			  GtkTreeIter * iter)
{
  GtkMenu *submenu = NULL;

  GtkWidget *menu_item = NULL;

  gchar        * item_name       = NULL;
  gchar        * item_comment    = NULL;
  GdkPixbuf    * item_icon       = NULL;
  GdkPixbuf    * item_thumb_icon = NULL;
  gchar        * item_exec       = NULL;
  gchar        * item_service    = NULL;
  gchar        * item_desktop_id = NULL;
  gchar        * item_text_domain= NULL;
  GtkTreeIter    child_iter;
  gint           children;
  gboolean       my_iterator = FALSE;
  
  g_return_if_fail (menu);

  if ( !model )
    {
      GtkTreeIter iter0;
      
      model = get_menu_contents();
      iter  = g_malloc0 (sizeof( GtkTreeIter ) );
      my_iterator = TRUE;
      
      /* Get the top level iterator.. */
      if ( !gtk_tree_model_get_iter_first( model, &iter0 ) ||
	   !gtk_tree_model_iter_children( model, iter, &iter0 ))
	{
	  g_object_unref(G_OBJECT(model));
	  return;
	}
    }
  else
    {
      g_object_ref(G_OBJECT(model));
    }
    
   
  /* Loop! */
  do  {
    item_name       = NULL;
    item_icon       = NULL;
    item_thumb_icon = NULL;
    item_exec       = NULL;
    item_service    = NULL;
    item_desktop_id = NULL;
    item_text_domain= NULL;

    gtk_tree_model_get( model, iter,
			TREE_MODEL_NAME,
			&item_name,
			TREE_MODEL_ICON,
			&item_icon,
			TREE_MODEL_THUMB_ICON,
			&item_thumb_icon,
			TREE_MODEL_EXEC,
			&item_exec,
			TREE_MODEL_SERVICE,
			&item_service,
			TREE_MODEL_DESKTOP_ID,
			&item_desktop_id,
			TREE_MODEL_COMMENT,
			&item_comment,
			TREE_MODEL_TEXT_DOMAIN,
			&item_text_domain,
			-1);

    children = 0;

    /* If the item has children.. */
    if ( gtk_tree_model_iter_children( model, &child_iter, iter ) )
      {
	gchar *child_string = NULL;
		    
	/* ..it's a submenu */
	submenu = GTK_MENU(gtk_menu_new());
    
	gtk_widget_set_name(GTK_WIDGET(submenu),
			    HILDON_NAVIGATOR_MENU_NAME);

	/* Create a menu item and add it to the menu.
	 */
	children     = gtk_tree_model_iter_n_children (model, iter);
    child_string = g_strdup_printf(MENU_ITEM_N_ITEMS(children), children);
	
	menu_item = hildon_thumb_menu_item_new_with_labels(
				(item_text_domain && *item_text_domain)?
				dgettext(item_text_domain, item_name):
				_(item_name),
	                        NULL,
	                        child_string);
	
	g_free(child_string);

	gtk_menu_shell_append(GTK_MENU_SHELL(menu),
			      GTK_WIDGET(menu_item));

	/* Add the submenu icon */
	if ( item_icon && item_thumb_icon )
	  {
	    hildon_thumb_menu_item_set_images(
				  HILDON_THUMB_MENU_ITEM(menu_item),
				  gtk_image_new_from_pixbuf(item_icon),
				  gtk_image_new_from_pixbuf(item_thumb_icon));
	  }
		    
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item),
				  GTK_WIDGET(submenu));

	/* Recurse! */
	hn_others_menu_get_items(submenu, button, model, &child_iter);

      }
    else if ( !item_desktop_id || strlen( item_desktop_id ) == 0 )
      {
	/* Empty submenu. Skip "Extras" */
	if ( strcmp( item_name, "tana_fi_extras" ) != 0 )
	  {
	    gchar *child_string;
	    submenu = GTK_MENU(gtk_menu_new());
    
	    gtk_widget_set_name(GTK_WIDGET(submenu),
				HILDON_NAVIGATOR_MENU_NAME);

	    /* Create a menu item and add it to the menu.
	     */
        child_string = g_strdup_printf(MENU_ITEM_N_ITEMS(children), children);
	    menu_item = hildon_thumb_menu_item_new_with_labels (
				(item_text_domain && *item_text_domain)?
				dgettext(item_text_domain, item_name):
				_(item_name),
				NULL,
				child_string);
	    g_free(child_string);

	    gtk_menu_shell_append(GTK_MENU_SHELL(menu),
				  GTK_WIDGET(menu_item));

	    /* Add the submenu icon */
	    if ( item_icon )
	      {
		hildon_thumb_menu_item_set_images(
				  HILDON_THUMB_MENU_ITEM(menu_item),
				  gtk_image_new_from_pixbuf(item_icon),
				  gtk_image_new_from_pixbuf(item_thumb_icon));
	      }

	    gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item),
				      GTK_WIDGET(submenu));

	    /* Create a menu item and add it to the menu. */
	    GtkWidget *submenu_item =
	    gtk_image_menu_item_new_with_label(MENU_ITEM_EMPTY_SUBMENU_STRING);

	    gtk_widget_set_sensitive( submenu_item, FALSE );

	    gtk_menu_shell_append(GTK_MENU_SHELL(submenu),
				  GTK_WIDGET(submenu_item));

	  }
 
      }
    else if ( strcmp( item_desktop_id, SEPARATOR_STRING ) == 0 )
      {
	/* Separator */
	menu_item = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(menu),
			      GTK_WIDGET(menu_item));
		    
      }
    else
      {
	/* Application */

	menu_item = hildon_thumb_menu_item_new_with_labels(
			(item_text_domain && *item_text_domain)?
			dgettext(item_text_domain, item_name):
			_(item_name),
			NULL,
			/* work around strange behaviour of gettext for
			 * empty  strings
			 */
			(item_comment && *item_comment)?_(item_comment):"");

	if ( !item_icon )
	  {
	    item_icon = get_icon(MENU_ITEM_DEFAULT_APP_ICON,
				 MENU_ITEM_ICON_SIZE);
	  }

    if ( !item_thumb_icon )
	  {
	    item_thumb_icon = get_icon(MENU_ITEM_DEFAULT_APP_ICON,
				 MENU_ITEM_THUMB_ICON_SIZE);
	  }

	if ( item_icon && item_thumb_icon )
	  {
	    hildon_thumb_menu_item_set_images(
				   HILDON_THUMB_MENU_ITEM(menu_item),
				   gtk_image_new_from_pixbuf(item_icon),
				   gtk_image_new_from_pixbuf(item_thumb_icon));
	  }

	gtk_menu_shell_append(GTK_MENU_SHELL(menu),
			      GTK_WIDGET(menu_item));

	g_object_set_data_full(G_OBJECT(menu_item),
			       DESKTOP_ENTRY_EXEC_FIELD,
			       g_strdup(item_exec), g_free);

	g_object_set_data_full(G_OBJECT(menu_item),
			       DESKTOP_ENTRY_SERVICE_FIELD,
			       g_strdup(item_service), g_free);

	/* Connect the signal and callback */
	g_signal_connect(G_OBJECT(menu_item), "activate",
			 G_CALLBACK(hn_others_menu_activate_item),
			 button);
      }
	    
    g_free(item_name);
    
    if (item_icon)
      g_object_unref (G_OBJECT(item_icon));
    
    if (item_thumb_icon)
      g_object_unref(G_OBJECT(item_thumb_icon));
    
    g_free (item_exec);
    g_free (item_service);
    g_free (item_desktop_id);
    g_free (item_comment);
    g_free (item_text_domain);
	    
  } while (gtk_tree_model_iter_next(model, iter));


  if( my_iterator )
    {
      gtk_tree_iter_free (iter);
      HN_DBG ("ref count remaining on model %d (should be 1)",
	      G_OBJECT (model)->ref_count);
    }
  
  g_object_unref(G_OBJECT(model));
}

static void
hn_others_menu_populate (HNOthersButton * button)
{
  hn_others_menu_get_items (GTK_MENU(button->priv->menu), button, NULL, NULL);
  gtk_widget_show_all(GTK_WIDGET(button->priv->menu));
}

/**
 * Creates and populates the menu attached to the Others button.
 * 
 * If the button already has a menu associated with it, it is destroyed and
 * replaced with a new menu.
 */
static void
hn_others_button_create_menu (HNOthersButton * button)
{
  GtkWidget * menu;
    
  g_return_if_fail (button);

  /* create the menu shell, and connect callbacks */
  menu = gtk_menu_new();
    
  gtk_widget_set_name(menu, HILDON_NAVIGATOR_MENU_NAME);

  g_signal_connect (G_OBJECT(menu), "size-request",
		    G_CALLBACK(hn_others_menu_size_request),
		    button);
    
  g_signal_connect (G_OBJECT(menu), "deactivate",
		    G_CALLBACK(hn_others_menu_deactivate),
		    button);
    
  g_signal_connect (G_OBJECT(menu), "key-press-event",
		    G_CALLBACK(hn_others_menu_key_press),
		    button);
    
  g_signal_connect (G_OBJECT(menu), "button-release-event",
		    G_CALLBACK(hn_app_switcher_menu_button_release_cb),
		    NULL);

  if(button->priv->menu)
    {
      /* destroy the previous version of the menu */
      HN_DBG ("destroying previous menu ... ");
      gtk_widget_destroy (button->priv->menu);
      HN_DBG ("done.");
    }
    
  button->priv->menu = menu;

  /* now populate the menu */
  hn_others_menu_populate (button);
}

static void
hn_others_button_get_workarea (GtkAllocation *allocation)
{
  unsigned long n;
  unsigned long extra;
  int           format;
  int           status;
  Atom          property = XInternAtom( GDK_DISPLAY(), WORKAREA_ATOM, FALSE );
  Atom          realType;
    
  /*this is needed to get rid of the punned type-pointer breaks strict
    aliasing warning*/
  union
  {
    unsigned char * char_value;
    int           * int_value;
  } value;
    
  status = XGetWindowProperty( GDK_DISPLAY(), 
			       GDK_ROOT_WINDOW(), 
			       property, 0L, 4L,
			       0, XA_CARDINAL, &realType, &format,
			       &n, &extra, 
			       (unsigned char **) &value.char_value );
    
  if ( status == Success       &&
       realType == XA_CARDINAL &&
       format == 32 && n == 4  &&
       value.char_value != NULL )
    {
      allocation->x      = value.int_value[0];
      allocation->y      = value.int_value[1];
      allocation->width  = value.int_value[2];
      allocation->height = value.int_value[3];
    }
  else
    {
      allocation->x      = 0;
      allocation->y      = 0;
      allocation->width  = 0;
      allocation->height = 0;
    }
    
  if ( value.char_value ) 
    {
      XFree(value.char_value);  
    }
}

static void
hn_others_button_get_menu_position(GtkMenu * menu,
                                   gint * x,
                                   gint * y,
                                   gboolean *push_in,
                                   GtkWidget *button)
{
  GtkRequisition   req;
  GdkScreen      * screen = gtk_widget_get_screen(GTK_WIDGET(menu));
  int              menu_height = 0;
  int              main_height = 0;
  GtkAllocation    workarea = { 0, 0, 0, 0 };
    
  hn_others_button_get_workarea(&workarea);
    
  gtk_widget_size_request(GTK_WIDGET(menu), &req);

  menu_height = req.height;
  main_height = gdk_screen_get_height(screen);

  *push_in = FALSE;
  *x =  workarea.x;

  if (main_height - button->allocation.y < menu_height)
    {
      *y = MAX(0, ((main_height - menu_height) / 2));
    }
  else
    {
      *y = button->allocation.y;
    }
}

static void
hn_others_button_menu_show (HNOthersButton * button)
{
  g_return_if_fail (button);
  
  gtk_menu_popup(GTK_MENU(button->priv->menu),
		 NULL,
		 NULL,
		 (GtkMenuPositionFunc) hn_others_button_get_menu_position,
		 button,
		 1,
		 gtk_get_current_event_time());
  
    gtk_menu_shell_select_first(GTK_MENU_SHELL(button->priv->menu), TRUE);
}


static void
hn_others_button_button_toggled (HNOthersButton *button, gpointer d)
{
  if (button->priv->collapse_id)
    return;

  if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)))
    {
      HN_DBG ("button not active -- not showing menu");
      return;
    }
	
  HN_DBG ("showing menu");
  hn_others_button_menu_show(button);
}

static gboolean
hn_others_button_key_press(HNOthersButton * button, GdkEventKey * event,
			   gpointer d)
{
  g_return_val_if_fail(button, FALSE);

  if(event->keyval == GDK_Right ||
     event->keyval == GDK_KP_Enter)
    {
      hn_others_button_menu_show (button);
      return TRUE;
    }
  else if(event->keyval == GDK_Left || event->keyval == GDK_KP_Left)
    {
      hn_wm_activate(HN_TN_ACTIVATE_LAST_APP_WINDOW);
    }
	
  return FALSE;
}

static gboolean
hn_others_button_button_press (HNOthersButton * button, GdkEventButton * event,
			       gpointer d)
{
  /* Eat all press events to stabilize the behaviour */
  HN_DBG ("swallowing button-press-event");
  return TRUE;
}

/* Since we'll get hundred press events when thumbing the button, we'll need to
 * wait for a moment until reacting
 */
static gboolean
hn_others_button_press_collapser (HNOthersButton * button)
{
  g_return_val_if_fail (button, FALSE);
  
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)))
    {
      HN_DBG ("deactivating menu");
      gtk_menu_shell_deactivate(GTK_MENU_SHELL(button->priv->menu));
      button->priv->collapse_id = 0;
      button->priv->thumb_pressed = FALSE;
      return FALSE;
    }

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);

  if (button->priv->thumb_pressed)
    {
      hildon_menu_set_thumb_mode(GTK_MENU(button->priv->menu), TRUE);
    }
  else
    {
      hildon_menu_set_thumb_mode(GTK_MENU(button->priv->menu), FALSE);
    }

  hn_others_button_menu_show (button);
  
  button->priv->collapse_id = 0;
  button->priv->thumb_pressed = FALSE;
  
  return FALSE;
}

static gboolean
hn_others_button_button_release(HNOthersButton * button,
				GdkEventButton * event,
				gpointer d)
{
  /* This has to be done in the release handler, not the press handler,
   * otherwise the button gets toggled when there is no corresponding release
   * event, or worse, before the release event arrives (in which case, the
   * release event is passed to the open menu).
   */
  if (button->priv->collapse_id <= 0)
    {
      hn_wm_activate(HN_TN_DEACTIVATE_KEY_FOCUS);
      button->priv->collapse_id =
	g_timeout_add (100,
		       (GSourceFunc)hn_others_button_press_collapser,
		       button);
    }

  if (event->button == THUMB_BUTTON_ID ||
      event->button == 2)
    {
      button->priv->thumb_pressed = TRUE;
    }

  return TRUE;
}

/** Dnotify handling
 *
 *  Dnotify dispatches large number of events when a file is modified --
 *  we use a 1s timeout to wait for the flurry to pass, and then do our stuff
 */
static gboolean
hn_others_button_menu_changed (HNOthersButton * button)
{
  g_return_val_if_fail (button, FALSE);

  HN_DBG ("Creating menu");
  
  button->priv->dnotify_update_timeout = 0;

  hn_others_button_create_menu (button);
  return FALSE;
}

static void
hn_others_button_dnotify_handler ( char *path, gpointer data )
{
  HNOthersButton * button = HN_OTHERS_BUTTON (data);
  
  if( !button->priv->dnotify_update_timeout )
    {
      button->priv->dnotify_update_timeout =
	g_timeout_add(1000,
		      (GSourceFunc)hn_others_button_menu_changed,
		      button);
    }
}

void
hn_others_button_dnotify_register (HNOthersButton * button)
{
  const gchar * home_dir;
  gchar       * dir;
  gchar	      * file = NULL;
  gchar       * conf_file;

  home_dir = getenv( "HOME" );
  
  conf_file = g_build_filename(home_dir, USER_MENU_FILE, NULL );
 
  /* We copy SYSTEMWIDE_MENU_FILE to always track the changes */

  if (!g_file_test (conf_file, G_FILE_TEST_EXISTS))
    if (g_file_get_contents (SYSTEMWIDE_MENU_FILE,&file,NULL,NULL))
    {
      g_debug ("I couldn't get contents");
      g_file_set_contents (USER_MENU_FILE,file,-1,NULL);
    }
  
  g_free (file);
  
  /* Watch systemwide menu conf */
  dir = g_path_get_dirname( SYSTEMWIDE_MENU_FILE );
  if ( hildon_dnotify_set_cb(
		(hildon_dnotify_cb_f *)hn_others_button_dnotify_handler,
		dir, button) != HILDON_OK)
    {
      ULOG_ERR( "others_menu_initialize_menu: "
		"failed setting dnotify callback "
		"for systemwide menu conf." );
    }

  g_free (dir);
  
  /* Watch user specific menu conf */
  
  if( home_dir && *home_dir)
    {    
      /* have to get the directory from the path because the USER_MENU_FILE
       * define might contain directory (it does, in fact).
       */
      dir = g_path_get_dirname( conf_file );
      g_mkdir (dir, 0755);

      if (dir && *dir)
	{
	  if ( hildon_dnotify_set_cb(
		(hildon_dnotify_cb_f *)hn_others_button_dnotify_handler,
		dir, button) != HILDON_OK)
	    {
	      ULOG_ERR( "others_menu_initialize_menu: "
			"failed setting dnotify callback "
			"for user spesific menu conf." );
	    }
	}
      else
	{
	  ULOG_ERR( "others_menu_initialize_menu: "
		    "failed to create directory '%s'", dir );
	}

      g_free (dir);
    }
      
  g_free (conf_file);
  

  /* Monitor the .desktop directories, so we can regenerate the menu
   * when a new application is installed */

  if ( hildon_dnotify_set_cb(
		      (hildon_dnotify_cb_f *)hn_others_button_dnotify_handler,
		      DESKTOPENTRYDIR,
		      button) != HILDON_OK)
    {
      ULOG_ERR( "others_menu_initialize_menu: "
		"failed setting dnotify callback "
		"for .desktop directory." );
    }
}

void
hn_others_button_close_menu (HNOthersButton * button)
{
  g_return_if_fail (button && HN_IS_OTHERS_BUTTON (button));
  g_return_if_fail (button->priv->menu);

  gtk_menu_popdown (GTK_MENU (button->priv->menu));
}
