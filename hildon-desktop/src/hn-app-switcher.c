 /*
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/* Hildon includes */
#include "hn-app-switcher.h"
#include "hn-app-button.h"
#include "hn-app-tooltip.h"
#include "hn-app-sound.h"
#include "hn-app-pixbuf-anim-blinker.h"

#include <stdlib.h>
#include <string.h>

/* X include */
#include <X11/keysymdef.h>
#include <X11/keysym.h>
#include <X11/extensions/XTest.h>

/* GLib include */
#include <glib.h>
#include <glib/gi18n.h>

/* GTK includes */
#include <gtk/gtkwidget.h>
#include <gtk/gtkcontainer.h>
#include <gtk/gtkbutton.h>
#include <gtk/gtktogglebutton.h>
#include <gtk/gtkmenu.h>
#include <gtk/gtkmenuitem.h>
#include <gtk/gtkimagemenuitem.h>
#include <gtk/gtkseparatormenuitem.h>
#include <gtk/gtkeventbox.h>
#include <gtk/gtkalignment.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkmisc.h>

#ifdef HAVE_LIBOSSO
#include <libosso.h>
#endif

#ifdef HAVE_LIBHILDON
#define ENABLE_UNSTABLE_API
#include <hildon/hildon-helper.h>
#undef ENABLE_UNSTABLE_API
#else
#include <hildon-widgets/hildon-finger.h>
#endif 


#define DBUS_API_SUBJECT_TO_CHANGE
#include <dbus/dbus.h>

/* GDK includes */
#include <gdk/gdkkeysyms.h>
#include <gdk/gdkx.h>

/* X includes */
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

/* Menu item strings */
#define AS_HOME_ITEM 		_("tana_fi_home")
#define AS_HOME_ITEM_ICON 	"qgn_list_home"

/* Defined workarea atom */
#define WORKAREA_ATOM "_NET_WORKAREA"

#define AS_MENU_BUTTON_ICON      "qgn_list_tasknavigator_appswitcher"
#define AS_MENU_DEFAULT_APP_ICON "qgn_list_gene_default_app"

#define ANIM_DURATION 5000 	/* 5 Secs for blinking icons */
#define ANIM_FPS      2

#define AS_BUTTON_NAME "hildon-navigator-small-button%d"
#define AS_BUTTON_NAME_PRESSED "hildon-navigator-small-button%d-pressed"

#define AS_MENU_BUTTON_NAME "hildon-navigator-small-button4"

#define AS_UPPER_SEPARATOR "hildon-navigator-upper-separator"
#define AS_LOWER_SEPARATOR "hildon-navigator-lower-separator"

#define AS_BOX_NAME         "hildon-navigator-app-switcher"

/* Hardcoded pixel perfecting values */
#define AS_BUTTON_BORDER_WIDTH  0
#define AS_MENU_BORDER_WIDTH    20
#define AS_TIP_BORDER_WIDTH 	20
#define AS_BUTTON_HEIGHT        38
#define AS_MENU_BUTTON_HEIGHT   60
#define AS_ROW_HEIGHT 		30
#define AS_ICON_SIZE            26
#define AS_TOOLTIP_WIDTH        360
#define AS_MENU_ITEM_WIDTH      360
#define AS_INTERNAL_PADDING     10
#define AS_SEPARATOR_HEIGHT     10

/* sound samples */
#define HN_WINDOW_OPEN_SOUND        DATADIR"/sounds/ui-window_open.wav"
#define HN_WINDOW_CLOSE_SOUND       DATADIR"/sounds/ui-window_close.wav"

#define TOOLTIP_SHOW_TIMEOUT 500
#define TOOLTIP_HIDE_TIMEOUT 1500

#define _KE(a) dgettext("ke-recv", a)
#define HD_CAD_TITLE            _KE("memr_ti_close_applications")
#define HD_CAD_LABEL_OPENING    _KE("memr_ia_close_applications_opening")
#define HD_CAD_LABEL_SWITCHING  _KE("memr_ia_close_applications_switching")
#define HD_CAD_LABEL_APP_LIST   _KE("memr_ia_close_applications_application_list")
#define HD_CAD_OK               _KE("memr_bd_close_applications_ok")
#define HD_CAD_CANCEL           _KE("memr_bd_close_applications_cancel")

enum 
{
  AS_PROP_NITEMS=1,
};

static void 
hn_app_switcher_add_info_cb (HDWM *hdwm, HDEntryInfo *entry_info, gpointer data);

static void 
hn_app_switcher_remove_info_cb (HDWM *hdwm, gboolean removed_app, HDEntryInfo *entry_info, gpointer data);

static void 
hn_app_switcher_changed_info_cb (HDWM *hdwm, HDEntryInfo *entry_info, gpointer data);

static void 
hn_app_switcher_changed_stack_cb (HDWM *hdwm, HDEntryInfo *entry_info, gpointer data);

static void 
hn_app_switcher_orientation_changed_cb (HNAppSwitcher *app_switcher);

static void 
hn_app_switcher_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);

static void 
hn_app_switcher_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec);

static void
refresh_app_button (HNAppSwitcher *app_switcher, HDEntryInfo *entry, gint pos);

static void
queue_refresh_buttons (HNAppSwitcher *app_switcher);

static gboolean 
hn_app_switcher_close_application_dialog (HDWM *hdwm, HDWMCADAction action, GList *items);

/*
 * HNAppSwitcher
 */

enum
{
  ADD_INFO,
  REMOVE_INFO,
  CHANGED_INFO,
  CHANGED_STACK,
  LOWMEM,
  BGKILL,

  LAST_SIGNAL
};

#define HN_APP_SWITCHER_GET_PRIVATE(obj) \
(G_TYPE_INSTANCE_GET_PRIVATE ((obj), HN_TYPE_APP_SWITCHER, HNAppSwitcherPrivate))

struct _HNAppSwitcherPrivate
{
  gint nitems;
	
  GtkWidget **buttons;
  GSList *buttons_group;

  GtkWidget *tooltip;
  
  GtkWidget *current_toggle;
  GtkWidget *app_menu;

  /* flags */
  guint menu_icon_is_blinking : 1;
  guint tooltip_visible : 1;
  guint switched_to_desktop : 1;
  guint is_dimming_on : 1;
  guint system_inactivity : 1;
  
  /* pointer location */
  guint pointer_on_button : 1;
  gboolean is_fullscreen;
 
  guint orientation_changed : 1; 

  guint menu_button_timeout;

  GList *applications;
  
  GtkIconTheme *icon_theme;

#ifdef HAVE_LIBOSSO
  osso_context_t *osso;
#endif
  
  guint queue_refresh_id;

  /* sound samples data */
  gint esd_socket;
  gint start_sample;
  gint end_sample;
};

G_DEFINE_TYPE (HNAppSwitcher, hn_app_switcher, TASKNAVIGATOR_TYPE_ITEM);

static gint
get_app_button_pos (GtkWidget *button)
{
  gint pos;
  
  g_return_val_if_fail (GTK_IS_TOGGLE_BUTTON (button), 0);

  pos = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (button),
					    "app-button-pos"));
  return pos;  
}

static void
set_app_button_pos (GtkWidget *button,
		    gint       pos,
		    gint       maxpos)
{
  g_return_if_fail (GTK_IS_TOGGLE_BUTTON (button));
  g_return_if_fail (pos >= 0 && pos < maxpos);

  g_object_set_data (G_OBJECT (button),
		     "app-button-pos",
		     GINT_TO_POINTER (pos));
}

static void
app_button_toggled_cb (GtkToggleButton *toggle,
		       gpointer         user_data)
{
  GtkWidget *widget = GTK_WIDGET (toggle);
  gint pos = get_app_button_pos (widget);
  gboolean is_active = gtk_toggle_button_get_active (toggle);
  gboolean is_inconsistent = gtk_toggle_button_get_inconsistent (toggle);
  gchar *name,*name_pressed;
  static gint i=0;
  name =
   g_strdup_printf (AS_BUTTON_NAME,pos+1);

  name_pressed =
   g_strdup_printf (AS_BUTTON_NAME_PRESSED,pos+1);

  if (is_inconsistent)
    gtk_widget_set_name (widget, name);
  else
  {
    if (is_active)
      gtk_widget_set_name (widget, name_pressed);
    else
      gtk_widget_set_name (widget, name);
  }

  g_free (name);
  g_free (name_pressed);

  g_debug ("%i setting button (pos=%d) (inconsistent='<%s>', active='<%s>') name: %s",
	   i,pos,
	   is_inconsistent ? "true" : "false",
	   is_active ? "true" : "false",
	   gtk_widget_get_name (widget));
}

static void 
hn_app_switcher_orientation_changed_cb (HNAppSwitcher *app_switcher) 
{
  GList *children = NULL, *iter;

  app_switcher->priv->orientation_changed = TRUE;
  
  if (GTK_IS_CONTAINER (app_switcher->box))
    children = 
        gtk_container_get_children (GTK_CONTAINER (app_switcher->box));
  
  for (iter = children; iter; iter = g_list_next (iter))
  {	  
    g_object_ref (G_OBJECT (iter->data));
    gtk_container_remove (GTK_CONTAINER (app_switcher->box),GTK_WIDGET (iter->data));
  }

  if (GTK_IS_WIDGET (app_switcher->box))
      gtk_container_remove (GTK_CONTAINER (app_switcher),GTK_WIDGET (app_switcher->box)); 

  gtk_widget_push_composite_child ();
  if (HILDON_DESKTOP_PANEL_ITEM (app_switcher)->orientation == GTK_ORIENTATION_HORIZONTAL) 
    app_switcher->box = GTK_BOX (gtk_hbox_new (FALSE, 0));
  else
    app_switcher->box = GTK_BOX (gtk_vbox_new (FALSE, 0));

  gtk_container_add (GTK_CONTAINER (app_switcher),GTK_WIDGET (app_switcher->box));
  gtk_widget_set_composite_name (GTK_WIDGET (app_switcher->box), "application-switcher-button-box");
  
  gtk_widget_show (GTK_WIDGET (app_switcher->box));

  gtk_widget_pop_composite_child ();

  for (iter = children; iter; iter = g_list_next (iter))
  {
    gtk_container_add (GTK_CONTAINER (app_switcher->box),GTK_WIDGET (iter->data));
    g_object_unref (G_OBJECT (iter->data));
  }

  gtk_widget_show_all (GTK_WIDGET (app_switcher->box));

  queue_refresh_buttons (app_switcher);

  if (children)
    g_list_free (children);
}

static GtkWidget *
create_app_button (HNAppSwitcher *app_switcher,
	           gint           pos)
{
  HNAppSwitcherPrivate *priv = app_switcher->priv;
  GtkWidget *retval;
  gchar *name;

  g_assert (priv->buttons[pos] == NULL);

  name = 
    g_strdup_printf (AS_BUTTON_NAME,pos+1);
  
  g_debug ("Creating app button at pos %d (name %s)", pos, name);
 
  retval = hn_app_button_new (NULL);

  g_object_set (retval, "app-switcher", app_switcher, NULL);

  set_app_button_pos (retval, pos, priv->nitems);
  hn_app_button_set_is_blinking (HN_APP_BUTTON (retval), FALSE);

  gtk_widget_set_name (retval, name);

  g_signal_connect (retval, "toggled",
		    G_CALLBACK (app_button_toggled_cb),
		    NULL);

  priv->buttons[pos] = retval;

  g_free (name);
  
  return priv->buttons[pos];
}


static void
hn_app_switcher_build (HNAppSwitcher *app_switcher)
{
  GtkWidget *button;
  GtkRequisition req;
  gint i;

  g_assert (app_switcher);

  app_switcher->priv->buttons = g_new0 (GtkWidget*,app_switcher->priv->nitems);

  app_switcher->hdwm = hd_wm_get_singleton ();

  g_object_ref (app_switcher->hdwm);

  gtk_widget_push_composite_child ();

  /* most recent applications buttons */
  g_debug ("Adding buttons");

  for (i=0;i < app_switcher->priv->nitems;i++)
  {
    button = create_app_button (app_switcher, i);
    gtk_box_pack_start (GTK_BOX (app_switcher->box), button, TRUE, TRUE, 0);
    gtk_widget_show (button);
  }
   
  /* menu button */

  g_signal_connect (app_switcher->hdwm,
		    "entry_info_added",
		    G_CALLBACK (hn_app_switcher_add_info_cb),
		    (gpointer)app_switcher);

  g_signal_connect (app_switcher->hdwm,
		    "entry_info_removed",
		    G_CALLBACK (hn_app_switcher_remove_info_cb),
		    (gpointer)app_switcher);
 
  g_signal_connect (app_switcher->hdwm,
		    "entry_info_changed",
		    G_CALLBACK (hn_app_switcher_changed_info_cb),
		    (gpointer)app_switcher);
 
  g_signal_connect (app_switcher->hdwm,
		    "entry_info_stack_changed",
		    G_CALLBACK (hn_app_switcher_changed_stack_cb),
		    (gpointer)app_switcher);

  g_signal_connect (app_switcher->hdwm,
		    "close-app",
		    G_CALLBACK (hn_app_switcher_close_application_dialog),
		    NULL);

  g_signal_connect (app_switcher,
		    "notify::orientation",
		    G_CALLBACK (hn_app_switcher_orientation_changed_cb),
		    NULL);
 
  gtk_widget_pop_composite_child ();

  gtk_widget_size_request (GTK_WIDGET (app_switcher->box), &req);

  if (HILDON_DESKTOP_PANEL_ITEM (app_switcher)->orientation == GTK_ORIENTATION_HORIZONTAL)
    gtk_widget_set_size_request (GTK_WIDGET (app_switcher), req.width, -1);
  else
    gtk_widget_set_size_request (GTK_WIDGET (app_switcher), -1, req.height);
}

/*
 * initializes esd cache of sound samples
 */
static void
hn_app_switcher_init_sound_samples (HNAppSwitcher *app_switcher)
{
  /* initialize the sound samples data */
  app_switcher->priv->esd_socket = hn_as_sound_init ();
  app_switcher->priv->start_sample = hn_as_sound_register_sample (
          app_switcher->priv->esd_socket,
          HN_WINDOW_OPEN_SOUND);
  app_switcher->priv->end_sample = hn_as_sound_register_sample (
          app_switcher->priv->esd_socket,
          HN_WINDOW_CLOSE_SOUND);
}

#ifdef HAVE_LIBOSSO
/*
 * callback for system HW events
 * -- currently we are only interested in the system inactivity event
 */
static
void hn_app_osso_hw_cb (osso_hw_state_t *state, gpointer data)
{
  g_return_if_fail(state && data);
  HNAppSwitcher *app_switcher = HN_APP_SWITCHER (data);
  HNAppSwitcherPrivate *priv = app_switcher->priv;

  if (state->system_inactivity_ind != priv->system_inactivity)
  {
    priv->system_inactivity = state->system_inactivity_ind;
    queue_refresh_buttons (app_switcher);
  }
}

static void
hn_app_switcher_osso_initialize (HNAppSwitcher *app_switcher)
{
  HNAppSwitcherPrivate *priv = app_switcher->priv;
  osso_hw_state_t hs = {0};
  
  priv->osso = osso_initialize ("AS_DIMMED_infoprint", "0.1", FALSE, NULL);

  if (!priv->osso)
  {
    g_warning("Failed to initialize libOSSO");
      
    return;
  }

  /* register stystem inactivity handler */
  hs.system_inactivity_ind = TRUE;
  osso_hw_set_event_cb(priv->osso, &hs,
                       hn_app_osso_hw_cb, app_switcher);
  
  
}
#endif

/* We must override the "show-all" method, as we may have stuff we don't want
 * to show, like the main button or the application buttons
 */
static void
hn_app_switcher_show_all (GtkWidget *widget)
{
  HNAppSwitcherPrivate *priv = HN_APP_SWITCHER (widget)->priv;
  gint i;

  gtk_widget_show (widget);

  /* show only the buttons linked to an application */
  for (i = 0; i < priv->nitems; i++)
  {
    GtkWidget *button = priv->buttons[i];
    HDEntryInfo *info;
     
    info = hn_app_button_get_entry_info (HN_APP_BUTTON (button));
    if (info)
      gtk_widget_show (button);
  }
}

static void
remove_entry_from_app_button (HNAppSwitcher *app_switcher,
			      HDEntryInfo   *entry_info)
{
  HNAppSwitcherPrivate *priv = app_switcher->priv;
  gint pos;

  if (entry_info == NULL) return;

  for (pos = 0; pos < priv->nitems; pos++)
  {
    GtkWidget *button = priv->buttons[pos];
    HDEntryInfo *e;

    e = hn_app_button_get_entry_info (HN_APP_BUTTON (button));
    if (e == entry_info)
    {
      hn_app_button_set_group (HN_APP_BUTTON (button), NULL);
      hn_app_button_set_entry_info (HN_APP_BUTTON (button), NULL);
      priv->buttons_group = hn_app_button_get_group (HN_APP_BUTTON (button));
       break;
    }
  }
}

static void
refresh_app_button (HNAppSwitcher *app_switcher,
                    HDEntryInfo   *entry,
                    gint           pos)
{
  HNAppSwitcherPrivate *priv = app_switcher->priv;
  const GList          *l, *children = hd_entry_info_get_children(entry);
  gboolean              urgent = FALSE;
  HNAppButton          *app_button = HN_APP_BUTTON (priv->buttons[pos]);
  gboolean 	       update_icon_geometry;
 
  update_icon_geometry = 
   (hn_app_button_get_last_entry_info (app_button) != entry) ? TRUE : FALSE;
  
  /* deal with urgency flags */
  for (l = children; l != NULL; l = l->next)
  {
    if (update_icon_geometry || app_switcher->priv->orientation_changed)	    
      hd_entry_info_set_icon_geometry (l->data,	    
		      		       GTK_WIDGET (app_button)->allocation.x,
				       GTK_WIDGET (app_button)->allocation.y,
				       GTK_WIDGET (app_button)->allocation.width,
				       GTK_WIDGET (app_button)->allocation.height,
				       TRUE);
	  
    /*
     * If the entry is urgent and the ignore flag is not set, the button
     * should blink
     */
    if (hd_entry_info_is_urgent(l->data) &&
        !hd_entry_info_get_ignore_urgent(l->data))
    {
      g_debug("Found an urgent button");
      urgent = TRUE;
    }

   /*
    * if the info is not urgent, we need to clear any leftover
    * ignore_urgent flag
    */
    if(!hd_entry_info_is_urgent(l->data) &&
       hd_entry_info_get_ignore_urgent(l->data))
    {
      hd_entry_info_set_ignore_urgent(l->data, FALSE);
    }
  }
  
  /* bind the entry info to the widget, so that we can
   * use it later when the user toggles the button
   *
   * NB: this recreates the icon respecting system inactivity
   */
  hn_app_button_set_entry_info (app_button, entry);
  hn_app_button_set_group (app_button, priv->buttons_group);
  priv->buttons_group = hn_app_button_get_group (app_button);

  g_debug ("buttons_group.size := %d",
	  g_slist_length (priv->buttons_group));

  hn_app_button_set_is_blinking (app_button, urgent);

  if (hd_entry_info_is_active (entry))
  {
    GtkToggleButton *button;

    button = GTK_TOGGLE_BUTTON (priv->buttons[pos]);
    gtk_toggle_button_set_inconsistent (button, FALSE);
    gtk_toggle_button_set_active (button, TRUE);
    gtk_toggle_button_toggled (button);
  }
  
  gtk_widget_set_sensitive (priv->buttons[pos], TRUE);
}

static gboolean
refresh_buttons (gpointer user_data)
{
  HNAppSwitcher        *app_switcher = HN_APP_SWITCHER (user_data);
  HNAppSwitcherPrivate *priv = app_switcher->priv;
  GList                *l;
  gint                  pos;
  gint                  active_button = -1;
  gboolean              is_urgent = FALSE;

  /* first we reset all the buttons icons */
  for (pos = 0; pos < priv->nitems; pos++)
  {
    HNAppButton *button = HN_APP_BUTTON (priv->buttons[pos]);

    hn_app_button_set_group (button, NULL);
    hn_app_button_set_entry_info (button, NULL);
  }

  priv->buttons_group = NULL;

  /* then refresh the icons of the application buttons */
  for (l = hd_wm_get_applications (app_switcher->hdwm), pos = 0;
       l != NULL;
       l = l->next, pos++)
  {
    HDEntryInfo *entry = l->data;

    /* we just want the most recently used top-level applications */
    if (entry->type != HD_ENTRY_APPLICATION)
    {
      g_debug("Object is not an application");
      continue;
    }

    if (pos < priv->nitems)
    {	    
      if (active_button < 0 &&
          gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(priv->buttons[pos]))&&
          !hd_entry_info_is_active (entry))
      {
          active_button = pos;
      }
      
      refresh_app_button (app_switcher, entry, pos);
    }
    else
    {
      hd_entry_info_set_icon_geometry (l->data,	    
		      		       GTK_WIDGET (app_switcher)->allocation.x,
				       GTK_WIDGET (app_switcher)->allocation.y,
				       GTK_WIDGET (app_switcher)->allocation.width,
				       GTK_WIDGET (app_switcher)->allocation.height,
				       FALSE);
    }	    
  }

  if (active_button >= 0)
  {
    GtkToggleButton *button;
      
    g_debug ("Unsetting the previously active button %d",
             active_button);

    button = GTK_TOGGLE_BUTTON (priv->buttons[active_button]);
    gtk_toggle_button_set_inconsistent (button, TRUE);
    gtk_toggle_button_set_active (button, FALSE);
    gtk_toggle_button_toggled (button);
  }
  
  /*
   * now we need to process the menu button to ensure that it blinks
   * if needed
   */

  is_urgent = FALSE;
  for (l = hd_wm_get_applications (app_switcher->hdwm), pos = 0;
       l != NULL;
       l = l->next, ++pos)
  {
    const GList       *k;
    HDEntryInfo       *child;

    if (pos < priv->nitems)
      continue;

    /* set the ignore flag on any children that were causing the blinking
     * we skip the first four apps, which cause blinking of the app buttons,
     * not the menu button.
     */
    for (k = hd_entry_info_get_children (l->data); k != NULL; k = k->next)
    {
      child = k->data;

     /* If the entry is urgent and the ignore flag is not set, the
      * button should blink
      */
      
      if (hd_entry_info_is_urgent (child) && !hd_entry_info_get_ignore_urgent (child))
        is_urgent = TRUE;
         
      /*
       * if the info is not urgent, we need to clear any leftover
       * ignore_urgent flag
       */
      if (!hd_entry_info_is_urgent (child) && hd_entry_info_get_ignore_urgent (child))
        hd_entry_info_set_ignore_urgent (child, FALSE);
       
    }
  }

  app_switcher->priv->orientation_changed = FALSE;

  return FALSE;
}

static void
queue_refresh_done (gpointer user_data)
{
  HNAppSwitcher *app_switcher = HN_APP_SWITCHER (user_data);

  app_switcher->priv->queue_refresh_id = 0;
}

static void
queue_refresh_buttons (HNAppSwitcher *app_switcher)
{
  HNAppSwitcherPrivate *priv = app_switcher->priv;

  if (!priv->queue_refresh_id)
    priv->queue_refresh_id = g_timeout_add_full (G_PRIORITY_HIGH,
		    				 100,
		                                 refresh_buttons,
					         app_switcher,
					         queue_refresh_done);
}

/* Class closure for the "add" signal; this is called each time an
 * entry has been added to the applications list.
 */
static void
hn_app_switcher_add_info_cb (HDWM *hdwm, HDEntryInfo *entry_info, gpointer data)
{
  HNAppSwitcher *app_switcher = HN_APP_SWITCHER (data);
  HNAppSwitcherPrivate *priv = app_switcher->priv;
  
  if (!entry_info)
  {
    g_warning ("%s: No entry info provided!",__FILE__);
    return;
  }

  /* Play a sound */
  if (hn_as_sound_play_sample (priv->esd_socket, priv->start_sample) == -1)
  {
      /* Connection to esd was probably closed */
    hn_as_sound_deinit (priv->esd_socket);
    hn_app_switcher_init_sound_samples (app_switcher);
    hn_as_sound_play_sample (priv->esd_socket, priv->start_sample);
  }

  queue_refresh_buttons (app_switcher);
}

/* Class closure for the "remove" signal; this is called each time
 * an entry has been remove from the application list
 */
static void
hn_app_switcher_remove_info_cb (HDWM *hdwm, 
				gboolean removed_app, 
				HDEntryInfo *entry_info, 
				gpointer data)
{
  HNAppSwitcher *app_switcher = HN_APP_SWITCHER (data);
  HNAppSwitcherPrivate *priv = app_switcher->priv;
  HDEntryInfo *info_parent = hd_entry_info_get_parent(entry_info);
  
  /* Play a sound */
  if (hn_as_sound_play_sample (priv->esd_socket, priv->end_sample) == -1)
  {
    /* Connection to esd was probably closed */
    hn_as_sound_deinit (priv->esd_socket);
    hn_app_switcher_init_sound_samples (app_switcher);
    hn_as_sound_play_sample (priv->esd_socket, priv->start_sample);
  }
  
  if (removed_app)
    remove_entry_from_app_button (app_switcher, info_parent);

  queue_refresh_buttons (app_switcher);
}


/* Class closure for the "changed" signal; this is called each time
 * an entry inside the applications list has been changed by the WM,
 * for instance when the icon or the title changes.
 *
 * NB: entry_info can be NULL for global update
 */
static void
hn_app_switcher_changed_info_cb (HDWM *hdwm, HDEntryInfo *entry_info, gpointer data)
{
  HNAppSwitcher        *app_switcher = HN_APP_SWITCHER (data);
  HNAppSwitcherPrivate *priv = app_switcher->priv;
  g_debug ("In hn_app_switcher_real_changed_info");

  /* all changes have potential impact on the the main menu; app menus are
   * created on the fly, so we do not have to worry about menu changes there
   */

  /*
   * If we are given an entry info and it of the app type, we just need to
   * update at most one button
   */
  if (entry_info && entry_info->type == HD_ENTRY_APPLICATION)
  {
    gint                  pos;
    GList *               l;

    g_debug ("HDEntryInfo present and of type APPLICATION");
      
    for (l = hd_wm_get_applications (hdwm), pos = 0;
         l != NULL && pos < priv->nitems;
         l = l->next, pos++)
    {
      HDEntryInfo *entry = l->data;
          
      if (entry->type != HD_ENTRY_APPLICATION)
      {
        g_debug("Object is not an application");
        continue;
      }

      if (entry_info == entry)
      {
        refresh_app_button (app_switcher, entry_info, pos);
        return;
      }
    }
      
    return;
  }
  else 
  if (entry_info)
  {
    /* this is a change in a child entry; in addition to the main menu impact
     * we already dealt with, child changes affect the blinking of the
     * associated buttons
     */
    HDEntryInfo *parent;
    GtkWidget   *button = NULL;
    GList       *l;
    gint         pos;
      
    g_return_if_fail (entry_info->type != HD_ENTRY_INVALID);
    g_debug ("HDEntryInfo present, with child entry");

    parent = hd_entry_info_get_parent (entry_info);

    if (!parent)
    {
      g_critical ("Attempting to change orphan child item.");
      return;
    }

    /* if the info is urgent but has an ignore flag, we know that it was not
     * causing the button to blink, and will not cause the button to blink,
     * so we do not need to update the buttons
     */
    if (hd_entry_info_is_urgent (entry_info) &&
        hd_entry_info_get_ignore_urgent (entry_info))
    {
      return;
    }

    /* if the info is not urgent, but has the ignore flag set, we have to
     * clear that flag; we also know that it was not causing the associated
     * button to blink (it was being ingored), so we do not need to update
     * the button.
     */
    if (!hd_entry_info_is_urgent (entry_info) &&
        hd_entry_info_get_ignore_urgent (entry_info))
    {
      hd_entry_info_set_ignore_urgent (entry_info, FALSE);
      return;
    }

    /* the remaining cases are more complicated and require that we know
     * the state of the associated button
     */
    for (l = hd_wm_get_applications (hdwm), pos = 0;
         l != NULL && pos < priv->nitems;
         l = l->next, pos++)
    {
      HDEntryInfo *entry = l->data;
      
      /* we just want the most recently used top-level applications */
      if (entry->type != HD_ENTRY_APPLICATION)
      {
        g_debug("Object is not an application");
        continue;
      }

      if (entry == parent)
      {
        button = priv->buttons[pos];
        break;
      }
    }

    if (button)
    {
      g_debug ("Force the button's icon to update itself");
      hn_app_button_force_update_icon (HN_APP_BUTTON (button));
    }

      /* if the current info is urgent and not to be ignored, we know the app
       * button should blink; make sure it does
       */
    if (hd_entry_info_is_urgent (entry_info) &&
        !hd_entry_info_get_ignore_urgent (entry_info))
    {
      if (button)
      {
        /* child of one of the app buttons */
        if (!hn_app_button_get_is_blinking (HN_APP_BUTTON (button)))
          hn_app_button_set_is_blinking (HN_APP_BUTTON (button), TRUE);
      }
              
      return;
    }

    /* if the info is not urgent and it is not being ignored and the
     * associated button is not blinking, we know that it was not urgent
     * previously, so we do not need to update the button
     */
    if (!hd_entry_info_is_urgent (entry_info) &&
        !hd_entry_info_get_ignore_urgent (entry_info))
    {
      if ((button && !hn_app_button_get_is_blinking (HN_APP_BUTTON (button))))
      {
        return;
      }
    }
      
    /* we are left with the case where the info is not urgent, is not being
     * ignored, and the associated button is blinking -- this is probably a
     * change urgent -> !urgent, but we do not know if this button was the
     * sole cause of the blinking, so we have to update the whole shebang
     * -- fall through to refresh_buttons();
     */
  }
  
  g_debug ("Queuing a refresh cycle of the buttons (info: %s)",
           entry_info != NULL ? "yes" : "no");

  /* either global update (no entry_info) or a more complicated case that
   * was not handled above
   */
  queue_refresh_buttons (app_switcher);
}


/* Class closure for the "changed-stack" signal; this is called each time
 * The TN receives notification from MB about window/view being topped. 
 */
static void
hn_app_switcher_changed_stack_cb (HDWM *hdwm, HDEntryInfo *entry_info, gpointer data)
{
  HNAppSwitcher	       * app_switcher = HN_APP_SWITCHER (data);
  HNAppSwitcherPrivate * priv = app_switcher->priv;
  gint                   pos, active_pos;
  GList                * l;
  HDEntryInfo          * parent;
  gboolean               active_found = FALSE;

  g_debug ("In hn_app_switcher_real_changed_stack");

  
  if (!entry_info || !hd_entry_info_is_active (entry_info))
  {
    /* rebuild everything, since we were not told what has been topped
     * issue warning, as this is not how this function is meant to be
     * called
     */
    g_warning ("No entry_info provided");
      
    queue_refresh_buttons (app_switcher);
    return;
  }
  
  if (entry_info->type == HD_ENTRY_APPLICATION)
  {
    /* we only accept entries for windows and views */
    g_warning ("Cannot handle HD_ENTRY_APPLICATION");
    return;
  }
  
  if (entry_info->type != HD_ENTRY_DESKTOP)  
  {
     parent = hd_entry_info_get_parent (entry_info);

     if (!parent)
     {
       g_warning ("Orphan entry info");
       return;
     } 

     /* locate the associated button, and toggle it */
     active_pos = 0;
     for (l = hd_wm_get_applications (hdwm), pos = 0;
          l != NULL && pos < priv->nitems;
          l = l->next, pos++)
     {
       HDEntryInfo *entry = l->data;
          
       if (parent == entry)
       {
         hn_app_button_make_active (HN_APP_BUTTON(priv->buttons[pos]));
	 active_pos = pos;
	 active_found = TRUE;
	 break;
       }
     }
  }

  /* no active window in the buttons, make sure that
   * no button is active
   */  
  if (!active_found)
  {
    for (pos = 0; pos < priv->nitems; pos++)
    {
      GtkToggleButton *app_button;

      g_debug ("Setting inconsistent state for pos %d", pos);

      app_button = GTK_TOGGLE_BUTTON (priv->buttons[pos]);
      gtk_toggle_button_set_inconsistent (app_button, TRUE);
      gtk_toggle_button_set_active (app_button, FALSE);
      gtk_toggle_button_toggled (app_button);
    }
  }  

  /* we do not worry about the urgency hint here, as we will receive a
   * notification when it is cleared from the WM
   */
}

static void
hn_app_switcher_finalize (GObject *gobject)
{
  HNAppSwitcher *app_switch = HN_APP_SWITCHER (gobject);
#ifdef HAVE_LIBOSSO  
  HNAppSwitcherPrivate *priv = app_switch->priv;

  osso_deinitialize (priv->osso);
#endif

  g_object_unref (app_switch->hdwm);

  g_debug ("Destroying HNAppSwitcher");

  G_OBJECT_CLASS (hn_app_switcher_parent_class)->finalize (gobject);
}

static GObject *
hn_app_switcher_constructor (GType                  type,
			     guint                  n_construct_params,
			     GObjectConstructParam *construct_params)
{
  GObject *object;
  HNAppSwitcher *appswitcher;

  g_debug ("inside hn_app_switcher_constructor...");

  
  object      = G_OBJECT_CLASS (hn_app_switcher_parent_class)->constructor (type,n_construct_params, construct_params);
  appswitcher = HN_APP_SWITCHER (object);
  
  g_debug ("building HNAppSwitcher widget");
      
  hn_app_switcher_build (appswitcher);

#ifdef HAVE_LIBOSSO
  hn_app_switcher_osso_initialize (appswitcher);
#endif

  return object;
}

static void 
hn_app_switcher_set_property (GObject *object,
                              guint prop_id,
                              const GValue *value,
                              GParamSpec *pspec)
{
  HNAppSwitcher *app_switcher;

  app_switcher = HN_APP_SWITCHER (object);

  switch (prop_id)
  {
    case AS_PROP_NITEMS:	   
      app_switcher->priv->nitems = g_value_get_int (value);
      hn_app_switcher_orientation_changed_cb (app_switcher);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  } 
}

static void 
hn_app_switcher_get_property (GObject *object,
                              guint prop_id,
                              GValue *value,
                              GParamSpec *pspec)
{
  HNAppSwitcher *app_switcher;

  app_switcher = HN_APP_SWITCHER (object);

  switch (prop_id)
  {
    case AS_PROP_NITEMS:
      g_value_set_int (value,app_switcher->priv->nitems);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;						
  }
}

static void
hn_app_switcher_class_init (HNAppSwitcherClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  
  gobject_class->finalize = hn_app_switcher_finalize;
  gobject_class->constructor = hn_app_switcher_constructor;

  gobject_class->set_property = hn_app_switcher_set_property;
  gobject_class->get_property = hn_app_switcher_get_property;

  widget_class->show_all = hn_app_switcher_show_all;
 
  g_type_class_add_private (klass, sizeof (HNAppSwitcherPrivate));

  g_object_class_install_property (gobject_class,
                                   AS_PROP_NITEMS,
                                   g_param_spec_int("n_items",
                                                    "number_of_items",
                                                    "Number of items",
                                                    1,
						    20,
						    3,
                                                    G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE));
}

static void
hn_app_switcher_init (HNAppSwitcher *app_switcher)
{
  app_switcher->priv = HN_APP_SWITCHER_GET_PRIVATE (app_switcher);
  
  app_switcher->priv->buttons_group = NULL;

  gtk_widget_set_name (GTK_WIDGET (app_switcher), AS_BOX_NAME);
  
  /* set base properties of the app-switcher widget */
  
  gtk_container_set_border_width (GTK_CONTAINER (app_switcher), 0);

  hn_app_switcher_init_sound_samples (app_switcher);
}

static void 
hn_app_switcher_cad_item_toggled (GtkToggleButton *button, gpointer user_data)
{
  gboolean selected;
  GList *l;
  GList *items;
  GtkDialog *dialog;
  HDWMCADItem *item;
  
  item = NULL;
  selected = FALSE;

  items = (GList*) user_data;

  for (l = items; l; l = l->next)
  {
    item = (HDWMCADItem *) l->data;
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (item->button)))
      selected = TRUE;
  }

  dialog = GTK_DIALOG(gtk_widget_get_toplevel(GTK_WIDGET(item->button)));

  gtk_dialog_set_response_sensitive (dialog,
                                     GTK_RESPONSE_ACCEPT,
                                     selected);  
}

static void 
hn_app_switcher_cad_kill_selected_items (GList *items)
{
  GList *l;
  
  for (l = items; l; l = l->next)
    {
      HDWMCADItem *item = (HDWMCADItem *) l->data;
      if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (item->button))
          && item->win != NULL)
        {
          hd_wm_window_attempt_signal_kill (item->win, SIGTERM, TRUE);
        }
    }
}

static gboolean 
hn_app_switcher_close_application_dialog (HDWM *hdwm, HDWMCADAction action, GList *items)
{
  gint response;
  gint i;
  gboolean retval;
  GList *l;
  GtkWidget *dialog;
  GtkWidget *hbox;
  GtkWidget *vbox;
  GtkWidget *box;
  GtkWidget *label;
  GtkWidget *swin;
  GtkWidget *check;
  GtkRequisition req;
  
  check = NULL;
  req.height = 0;
  
  /* Creating the UI */
  dialog = gtk_dialog_new_with_buttons (HD_CAD_TITLE,
                                        NULL,
                                        GTK_DIALOG_MODAL
                                        | GTK_DIALOG_DESTROY_WITH_PARENT,
                                        HD_CAD_OK,
                                        GTK_RESPONSE_ACCEPT,
                                        HD_CAD_CANCEL,
                                        GTK_RESPONSE_CANCEL,
                                        NULL);                                        

  gtk_dialog_set_has_separator(GTK_DIALOG(dialog), FALSE);
  /* Ok is dimmed when there is nothing selected */
  gtk_dialog_set_response_sensitive(GTK_DIALOG(dialog),
                                    GTK_RESPONSE_ACCEPT,
                                    FALSE);

  swin = GTK_WIDGET (g_object_new(GTK_TYPE_SCROLLED_WINDOW,
                                  "vscrollbar-policy", GTK_POLICY_AUTOMATIC,
                                  "hscrollbar-policy", GTK_POLICY_NEVER,
                                  NULL));

  hbox = GTK_WIDGET (g_object_new(GTK_TYPE_HBOX,
                                  "spacing", 12,
                                  NULL));

  vbox = GTK_WIDGET(g_object_new(GTK_TYPE_VBOX, NULL));

  label = gtk_label_new (HD_CAD_LABEL_APP_LIST);

  /* Align to top-right */
  gtk_misc_set_alignment (GTK_MISC(label), 1, 0);

  gtk_container_add (GTK_CONTAINER(hbox), label);
  gtk_container_add (GTK_CONTAINER(hbox), vbox);
  g_debug ("there should be items %p", items);
  /* Collect open applications */
  for (l = items; l; l = l->next)
  {
    const gchar *icon_name;
    GtkWidget *image;
    GtkWidget *label;
    GdkPixbuf *icon = NULL;
    GtkIconTheme *icon_theme;
    HDWMApplication *app;
    HDWMCADItem *item = (HDWMCADItem *) l->data;
    g_debug ("item --> %p",l->data);
    if (item->win == NULL)
      continue;

    app = hd_wm_window_get_application (item->win);
      
    if (app == NULL)
      continue;
      
    label = gtk_label_new (_(hd_wm_application_get_name (app)));
    icon_name = hd_wm_application_get_icon_name(app);
    icon_theme = gtk_icon_theme_get_default ();
    
    if (icon_name)
      icon = gtk_icon_theme_load_icon (icon_theme,
                                       icon_name,
                                       ICON_SIZE,
                                       GTK_ICON_LOOKUP_NO_SVG,
                                       NULL);

    if (!icon)
      icon = gtk_icon_theme_load_icon (icon_theme,
                                       AS_MENU_DEFAULT_APP_ICON,
                                       ICON_SIZE,
                                       GTK_ICON_LOOKUP_NO_SVG,
                                       NULL);
      

    image = gtk_image_new_from_pixbuf (icon);
    if (icon)
      gdk_pixbuf_unref (icon);
    
    check = gtk_check_button_new();
    item->button = G_OBJECT (check);

    box = GTK_WIDGET (g_object_new (GTK_TYPE_HBOX,
                                    "spacing", 12,
                                    NULL));

    gtk_container_add (GTK_CONTAINER (box),   image);
    gtk_container_add (GTK_CONTAINER (box),   label);
    gtk_container_add (GTK_CONTAINER (check), box);
      
    gtk_box_pack_start(GTK_BOX(vbox), check, FALSE, TRUE, 0);
      
    g_signal_connect (item->button,
		      "toggled",
		      G_CALLBACK(hn_app_switcher_cad_item_toggled),
		      items);
  }

  switch (action)
  {
    case CAD_ACTION_OPENING:
      label = gtk_label_new (HD_CAD_LABEL_OPENING);
      break;
    case CAD_ACTION_SWITCHING:
      label = gtk_label_new (HD_CAD_LABEL_SWITCHING);
      break;
    default:
      label = gtk_label_new("Unlikely internal error happened, but feel free "
                            "to close applications anyway");
      break;
  }

  gtk_label_set_line_wrap (GTK_LABEL(label), TRUE);

  box = GTK_WIDGET(g_object_new(GTK_TYPE_VBOX,
                                "spacing", 12,
                                NULL));

  gtk_box_pack_start (GTK_BOX(box), label, FALSE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX(box), gtk_hseparator_new (), FALSE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX(box), hbox, TRUE, TRUE, 0);

  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW(swin), box);
  gtk_container_add (GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), swin);

  gtk_widget_show_all(GTK_DIALOG(dialog)->vbox);

  /* ScrolledWindow won't request visibly pleasing amounts of space so we need
   * to do it for it. We ensure that there's vertical space for the whole
   * content or at least for 5 items (plus one for padding) at a time. We trust
   * the dialog to not allocate so much space that it would go offscreen.
   */
  gtk_widget_size_request (box, &req);
  i = req.height;
  gtk_widget_size_request(check, &req);
  gtk_widget_set_size_request(swin, -1, MIN(i, req.height*6));

  response = gtk_dialog_run (GTK_DIALOG(dialog));
  
  gtk_widget_hide(dialog);
  
  switch (response)
  {
    case GTK_RESPONSE_ACCEPT:
      hn_app_switcher_cad_kill_selected_items (items);
      retval = TRUE;

      break;
    default:
      retval = FALSE;
      break;  
  }

  gtk_widget_destroy (dialog);

  /* Cleanup */
  for (l = items; l; l = l->next)
  {
    if (l->data != NULL)
      g_free (l->data);
  }

  return retval;
}

GtkWidget *
hn_app_switcher_new (gint nitems)
{
  return g_object_new (HN_TYPE_APP_SWITCHER,"n_items",nitems,NULL);
}

gboolean
hn_app_switcher_get_system_inactivity (HNAppSwitcher *app_switcher)
{
  HNAppSwitcherPrivate *priv;

  g_return_val_if_fail (HN_IS_APP_SWITCHER (app_switcher), FALSE);
  priv = app_switcher->priv;

  return priv->system_inactivity;
}

