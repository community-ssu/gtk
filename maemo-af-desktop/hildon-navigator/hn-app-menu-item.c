/* -*- mode:C; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

/* hn-app-switcher.c
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

/* Hildon includes */
#include "hn-app-menu-item.h"
#include "hn-wm.h"
#include "hn-entry-info.h"
#include "hn-wm-watchable-app.h"
#include "hildon-pixbuf-anim-blinker.h"

#include <stdlib.h>
#include <string.h>

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

#include <libosso.h>

#include <hildon-widgets/gtk-infoprint.h>
#include "hildon-pixbuf-anim-blinker.h"


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

/* log include */
#include <log-functions.h>

/* Menu item strings */
#define AS_HOME_ITEM 		_("tana_fi_home")
#define AS_HOME_ITEM_ICON 	"qgn_list_home"

#define AS_MENU_DEFAULT_APP_ICON "qgn_list_gene_default_app"

#define ANIM_DURATION 5000 	/* 5 Secs for blinking icons */
#define ANIM_FPS      2

/* Hardcoded pixel perfecting values */
#define AS_BUTTON_BORDER_WIDTH  0
#define AS_MENU_BORDER_WIDTH    20
#define AS_TIP_BORDER_WIDTH 	20
#define AS_BUTTON_HEIGHT        38
#define AS_ROW_HEIGHT 		30
#define AS_ICON_SIZE            26
#define AS_ICON_THUMB_SIZE      64
#define AS_CLOSE_BUTTON_SIZE    16
#define AS_CLOSE_BUTTON_THUMB_SIZE 40
#define AS_TOOLTIP_WIDTH        360
#define AS_MENU_ITEM_WIDTH      360
#define AS_INTERNAL_PADDING     10
#define AS_SEPARATOR_HEIGHT     10
#define AS_BUTTON_BOX_PADDING   10

/*
 * HNAppMenuItem
 */

enum
{
  MENU_PROP_0,
  MENU_PROP_ENTRY_INFO,
  MENU_PROP_SHOW_CLOSE,
  MENU_PROP_IS_BLINKING,
  MENU_PROP_THUMBABLE
};

#define HN_APP_MENU_ITEM_GET_PRIVATE(obj) \
(G_TYPE_INSTANCE_GET_PRIVATE ((obj), HN_TYPE_APP_MENU_ITEM, HNAppMenuItemPrivate))

struct _HNAppMenuItemPrivate
{
  GtkWidget *icon;
  GtkWidget *label;
  GtkWidget *label2;
  GtkWidget *close;

  GtkIconTheme *icon_theme;
  
  guint show_close  : 1;
  guint thumbable    : 1;
  guint is_blinking : 1;

  GdkPixbufAnimation *pixbuf_anim;
  
  HNEntryInfo *info;

  HildonPixbufAnimBlinker *blinker;
};

G_DEFINE_TYPE (HNAppMenuItem, hn_app_menu_item, GTK_TYPE_IMAGE_MENU_ITEM);

static void
hn_app_menu_item_finalize (GObject *gobject)
{
  HNAppMenuItemPrivate *priv = HN_APP_MENU_ITEM (gobject)->priv;
  
  if (priv->pixbuf_anim)
    g_object_unref (priv->pixbuf_anim); 

  G_OBJECT_CLASS (hn_app_menu_item_parent_class)->finalize (gobject);
}

static void
hn_app_menu_item_set_property (GObject      *gobject,
			       guint         prop_id,
			       const GValue *value,
			       GParamSpec   *pspec)
{
  HNAppMenuItemPrivate *priv = HN_APP_MENU_ITEM (gobject)->priv;

  switch (prop_id)
    {
    case MENU_PROP_ENTRY_INFO:
      priv->info = g_value_get_pointer (value);
      break;
    case MENU_PROP_SHOW_CLOSE:
      priv->show_close = g_value_get_boolean (value);
      break;
    case MENU_PROP_THUMBABLE:
      priv->thumbable = g_value_get_boolean (value);
      break;
    case MENU_PROP_IS_BLINKING:
      priv->is_blinking = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
    }
}

static void
hn_app_menu_item_get_property (GObject    *gobject,
			       guint       prop_id,
			       GValue     *value,
			       GParamSpec *pspec)
{
  HNAppMenuItemPrivate *priv = HN_APP_MENU_ITEM (gobject)->priv;

  switch (prop_id)
    {
    case MENU_PROP_ENTRY_INFO:
      g_value_set_pointer (value, priv->info);
      break;
    case MENU_PROP_SHOW_CLOSE:
      g_value_set_boolean (value, priv->show_close);
      break;
    case MENU_PROP_THUMBABLE:
      g_value_set_boolean (value, priv->thumbable);
      break;
    case MENU_PROP_IS_BLINKING:
      g_value_set_boolean (value, priv->is_blinking);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
    }
}

static void
hn_app_menu_item_icon_animation (GtkWidget *icon,
	       			 gboolean   is_on)
{
  GdkPixbuf *pixbuf;
  GdkPixbufAnimation *pixbuf_anim;
  
  g_return_if_fail (GTK_IS_IMAGE (icon));

  if (is_on)
    {
      pixbuf = gtk_image_get_pixbuf (GTK_IMAGE (icon));
              
      pixbuf_anim = hildon_pixbuf_anim_blinker_new (pixbuf,
                                                    1000 / ANIM_FPS,
						    -1,
						    10);

      gtk_image_set_from_animation (GTK_IMAGE(icon), pixbuf_anim);

      g_object_unref (pixbuf_anim);
    }
  else
    {
      pixbuf_anim = gtk_image_get_animation (GTK_IMAGE (icon));
      
      /* grab static image from menu item and reset */
      pixbuf = gdk_pixbuf_animation_get_static_image (pixbuf_anim);

      gtk_image_set_from_pixbuf (GTK_IMAGE (icon), pixbuf);
      g_object_unref (pixbuf);
    }
}

static GObject *
hn_app_menu_item_constructor (GType                  type,
			      guint                  n_construct_params,
			      GObjectConstructParam *construct_params)
{
  GObject *gobject;
  HNAppMenuItem *menuitem;
  HNAppMenuItemPrivate *priv;
  GtkWidget *hbox, *vbox = NULL;
  
  gobject = G_OBJECT_CLASS (hn_app_menu_item_parent_class)->constructor (type,
		  							 n_construct_params,
									 construct_params);

  menuitem = HN_APP_MENU_ITEM (gobject);
  priv = menuitem->priv;
  g_assert (priv->info != NULL);

  gtk_widget_push_composite_child ();

  priv->icon = gtk_image_new ();
  gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menuitem), priv->icon);
  gtk_widget_show (priv->icon);
  
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_widget_set_composite_name (GTK_WIDGET (hbox), "as-app-menu-hbox");
  gtk_container_add (GTK_CONTAINER (menuitem), hbox);
  gtk_widget_show (hbox);

  if(priv->thumbable)
    {
      vbox = gtk_vbox_new (FALSE, 0);
      gtk_widget_set_composite_name (GTK_WIDGET (vbox), "as-app-menu-vbox");
      gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
      gtk_widget_show (vbox);
      
      priv->label2 = gtk_label_new (NULL);
      gtk_widget_set_composite_name (GTK_WIDGET (priv->label2),
                                     "as-app-menu-label-desc");
      gtk_misc_set_alignment (GTK_MISC (priv->label2), 0.0, 0.5);
      gtk_box_pack_end (GTK_BOX (vbox), priv->label2, TRUE, TRUE, 0);
      gtk_widget_show (priv->label2);
    }

  priv->label = gtk_label_new (NULL);
  gtk_widget_set_composite_name (GTK_WIDGET (priv->label), "as-app-menu-label");
  gtk_misc_set_alignment (GTK_MISC (priv->label), 0.0, 0.5);

  if(priv->thumbable)
    gtk_box_pack_end (GTK_BOX (vbox), priv->label, TRUE, TRUE, 0);
  else
    gtk_box_pack_start (GTK_BOX (hbox), priv->label, TRUE, TRUE, 0);
  
  gtk_widget_show (priv->label);

  if (priv->info)
    {
      GdkPixbuf *app_pixbuf;
      
      priv->is_blinking = hn_entry_info_is_urgent (priv->info);
      app_pixbuf = hn_entry_info_get_icon (priv->info);
      if (!app_pixbuf)
        {
          const gchar *icon_name;
          GError *error = NULL;

          icon_name = hn_entry_info_get_app_icon_name (priv->info);
          if (!icon_name)
            icon_name = AS_MENU_DEFAULT_APP_ICON;

          app_pixbuf =
            gtk_icon_theme_load_icon (priv->icon_theme,
                                      icon_name,
                                      priv->thumbable ?
                                      AS_ICON_THUMB_SIZE : AS_ICON_SIZE,
                                      GTK_ICON_LOOKUP_NO_SVG,
                                      &error);

          if (error)
            {
              g_warning ("Could not load icon %s from theme: %s.",
                         icon_name,
                         error->message);
              g_error_free (error);
              error = NULL;
              icon_name = AS_MENU_DEFAULT_APP_ICON;
              app_pixbuf = gtk_icon_theme_load_icon (priv->icon_theme,
                                                     icon_name,
                                                     priv->thumbable ?
                                                     AS_ICON_THUMB_SIZE : 
                                                     AS_ICON_SIZE,
                                                     GTK_ICON_LOOKUP_NO_SVG,
                                                     &error);

              if (error)
                {
                  g_warning ("Could not load icon %s from theme: %s.",
                             icon_name,
                             error->message);
                  g_error_free (error);
                }
            }
        }
      
      if (app_pixbuf)
        {
          GdkPixbuf *compose;

	  compose = NULL;
	  if (hn_entry_info_is_hibernating (priv->info))
            {
              compose = gtk_icon_theme_load_icon (priv->icon_theme,
			      			  "qgn_indi_bkilled",
						  16,
						  0,
						  NULL);
	    }

	  if (compose)
            {
              gint dest_width = gdk_pixbuf_get_width (app_pixbuf);
	      gint dest_height = gdk_pixbuf_get_height (app_pixbuf);
	      gint off_x, off_y;

	      off_x = dest_width - gdk_pixbuf_get_width (compose);
	      off_y = dest_height - gdk_pixbuf_get_height (compose);
	      
              gdk_pixbuf_composite (compose, app_pixbuf,
			            0, 0,
				    dest_width, dest_height,
				    off_x, off_y,
				    1.0, 1.0,
				    GDK_INTERP_BILINEAR,
				    0xff);
	      g_object_unref (compose);
	    }
	  
          gtk_image_set_from_pixbuf (GTK_IMAGE (priv->icon), app_pixbuf);
          g_object_unref (app_pixbuf);

          if (priv->is_blinking)
            hn_app_menu_item_icon_animation (priv->icon, TRUE);
        }

      if (!priv->thumbable)
	{
          gchar *app_name = hn_entry_info_get_title (priv->info);

	  gtk_label_set_text (GTK_LABEL (priv->label), app_name);
	  g_free (app_name);
	}
      else
	{
          gchar *app_name = hn_entry_info_get_app_name (priv->info);
	  gchar *win_name = hn_entry_info_get_window_name (priv->info);

	  if (win_name)
            gtk_label_set_text (GTK_LABEL (priv->label2), win_name);
	  
	  gtk_label_set_text (GTK_LABEL (priv->label), app_name);

	  g_free (app_name);
	  g_free (win_name);
	}
    }

  if (priv->show_close)
    {
      HNAppMenuItemClass *klass = HN_APP_MENU_ITEM_CLASS (G_OBJECT_GET_CLASS (gobject));
      priv->close = gtk_image_new ();

      if (priv->thumbable && klass->thumb_close_button)
          gtk_image_set_from_pixbuf (GTK_IMAGE (priv->close),
                                     klass->thumb_close_button);
      else if (!priv->thumbable && klass->close_button)
          gtk_image_set_from_pixbuf (GTK_IMAGE (priv->close),
                                     klass->close_button);
      else
        g_warning ("Icon missing for close button");

      gtk_box_pack_end (GTK_BOX (hbox), priv->close, FALSE, FALSE, 0);
      gtk_widget_show (priv->close);
    }
  
  return gobject;
}

static void
hn_app_menu_item_size_request (GtkWidget      *widget,
			       GtkRequisition *requisition)
{
  HNAppMenuItemPrivate *priv = HN_APP_MENU_ITEM (widget)->priv;
  GtkRequisition child_req, child_req2 = {0};
  gint child_width, child_height;

  /* if the width of icon + label (+ close) is too big,
   * we clamp it to AS_MENU_ITEM_WIDTH and ellipsize the
   * label text; the height is set by the icons
   */
  gtk_widget_size_request (priv->icon, &child_req);
  child_width = child_req.width;
  child_height = child_req.height;  
      
  gtk_widget_size_request (priv->label,  &child_req);    
  if (priv->thumbable)
    gtk_widget_size_request (priv->label2, &child_req2);    
  child_width += MAX (child_req.width, child_req2.width);

  if (priv->show_close)
    {
      gtk_widget_size_request (priv->close, &child_req);
      child_width += child_req.width;
      child_height = child_req.height;
    }
  
  GTK_WIDGET_CLASS (hn_app_menu_item_parent_class)->size_request (widget,
		 						  requisition);

  if (child_width > AS_MENU_ITEM_WIDTH)
    {
      requisition->width = AS_MENU_ITEM_WIDTH;

      gtk_label_set_ellipsize (GTK_LABEL (priv->label), PANGO_ELLIPSIZE_END);
      if (priv->thumbable)
        gtk_label_set_ellipsize (GTK_LABEL (priv->label2), PANGO_ELLIPSIZE_END);
    }
  else
    requisition->width = MAX (requisition->width, child_width);

  requisition->height = MAX (requisition->height, child_height);

  HN_DBG ("menu-item requisition: (%d, %d)",
	  requisition->width, requisition->height);
}

static void
hn_app_menu_item_activate (GtkMenuItem *menu_item)
{
  HNEntryInfo *info;

  info = hn_app_menu_item_get_entry_info (HN_APP_MENU_ITEM (menu_item));
  g_assert (info != NULL);
  
  HN_DBG ("Raising application '%s'", hn_entry_info_peek_title (info));
  
  hn_wm_top_item (info);
}

static gboolean
hn_app_menu_item_button_release_event (GtkWidget      *widget,
                                     GdkEventButton *event)
{
  HNAppMenuItem *menuitem = HN_APP_MENU_ITEM(widget);
  gint x, y;
  
  HN_DBG ("menu item clicked ended");
  
  g_return_val_if_fail (menuitem && menuitem->priv && menuitem->priv->info,
                        FALSE);

  if(!menuitem->priv->show_close ||
     !menuitem->priv->close)
    return FALSE;

  /*
   * the pointer value is relative to the menuitem, but for some reason,
   * the close button allocation is relative to the menu as whole
   */
  
  gtk_widget_get_pointer(widget, &x, &y);

  HN_DBG ("pointer [%d,%d],\n"
          "close allocation [%d, %d, %d, %d]",
          x, y,
          menuitem->priv->close->allocation.x,
          menuitem->priv->close->allocation.y,
          menuitem->priv->close->allocation.width,
          menuitem->priv->close->allocation.height);

  /* only test x here; y is always withing the button range */
  if(x >  menuitem->priv->close->allocation.x &&
     x <= menuitem->priv->close->allocation.x +
          menuitem->priv->close->allocation.width)
    {
      hn_entry_info_close (menuitem->priv->info);
      return TRUE;
    }
  
  return FALSE;
}

static gboolean
hn_app_menu_item_button_press_event (GtkWidget      *widget,
                                     GdkEventButton *event)
{
  HNAppMenuItem *menuitem = HN_APP_MENU_ITEM(widget);
  gint x, y;
  
  HN_DBG ("menu item clicked");
  
  g_return_val_if_fail (menuitem && menuitem->priv && menuitem->priv->info,
                        FALSE);

  if(!menuitem->priv->show_close ||
     !menuitem->priv->close)
    return FALSE;

  /*
   * the pointer value is relative to the menuitem, but for some reason,
   * the close button allocation is relative to the menu as whole
   */
  
  gtk_widget_get_pointer(widget, &x, &y);

  HN_DBG ("pointer [%d,%d],\n"
          "close allocation [%d, %d, %d, %d]",
          x, y,
          menuitem->priv->close->allocation.x,
          menuitem->priv->close->allocation.y,
          menuitem->priv->close->allocation.width,
          menuitem->priv->close->allocation.height);

  /* only test x here; y is always withing the button range */
  if(x >  menuitem->priv->close->allocation.x &&
     x <= menuitem->priv->close->allocation.x +
          menuitem->priv->close->allocation.width)
    {
      return TRUE;
    }
  
  return FALSE;
}


static void
hn_app_menu_item_class_init (HNAppMenuItemClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GtkMenuItemClass *menuitem_class = GTK_MENU_ITEM_CLASS (klass);
  GtkIconTheme *icon_theme = gtk_icon_theme_get_default ();
  
  gobject_class->finalize = hn_app_menu_item_finalize;
  gobject_class->set_property = hn_app_menu_item_set_property;
  gobject_class->get_property = hn_app_menu_item_get_property;
  gobject_class->constructor = hn_app_menu_item_constructor;

  widget_class->size_request = hn_app_menu_item_size_request;
  widget_class->button_press_event = hn_app_menu_item_button_press_event;
  widget_class->button_release_event = hn_app_menu_item_button_release_event;
  
  menuitem_class->activate = hn_app_menu_item_activate;

  klass->close_button = gtk_icon_theme_load_icon (icon_theme,
                                                  "qgn_list_app_close",
                                                  AS_CLOSE_BUTTON_SIZE,
                                                  0,
                                                  NULL);

  klass->thumb_close_button = gtk_icon_theme_load_icon (icon_theme,
                                                        "qgn_list_app_close",
                                                        AS_CLOSE_BUTTON_THUMB_SIZE,
                                                        0,
                                                        NULL);
  
  g_object_class_install_property (gobject_class,
		  		   MENU_PROP_SHOW_CLOSE,
				   g_param_spec_boolean ("show-close",
					   		 "Show Close",
							 "Whether to show a close button",
							 FALSE,
							 (G_PARAM_CONSTRUCT | G_PARAM_READWRITE)));
  g_object_class_install_property (gobject_class,
		  		   MENU_PROP_THUMBABLE,
				   g_param_spec_boolean ("thumbable",
					   		 "Thumbable",
							 "Whether the menu item should be thumbable",
							 FALSE,
							 (G_PARAM_CONSTRUCT | G_PARAM_READWRITE)));
  g_object_class_install_property (gobject_class,
		  		   MENU_PROP_IS_BLINKING,
				   g_param_spec_boolean ("is-blinking",
					   		 "Is blinking",
							 "Whether the menu item should blink",
							 FALSE,
							 (G_PARAM_CONSTRUCT | G_PARAM_READWRITE)));
  g_object_class_install_property (gobject_class,
		  		   MENU_PROP_ENTRY_INFO,
				   g_param_spec_pointer ("entry-info",
					   		 "Entry Info",
							 "The entry info object used to build the menu",
							 (G_PARAM_CONSTRUCT | G_PARAM_READWRITE)));

  g_type_class_add_private (klass, sizeof (HNAppMenuItemPrivate));
}

static void
hn_app_menu_item_init (HNAppMenuItem *menuitem)
{
  HNAppMenuItemPrivate *priv = HN_APP_MENU_ITEM_GET_PRIVATE (menuitem);

  menuitem->priv = priv;

  priv->icon_theme = gtk_icon_theme_get_default ();
  
  priv->show_close = FALSE;
  priv->is_blinking = FALSE;
  priv->thumbable = FALSE;
  priv->pixbuf_anim = NULL;
  priv->info = NULL;
}

GtkWidget *
hn_app_menu_item_new (HNEntryInfo *info,
                      gboolean     show_close,
                      gboolean     thumbable)
{
  g_return_val_if_fail (info != NULL, NULL);
  g_return_val_if_fail (HN_ENTRY_INFO_IS_VALID_TYPE (info->type), NULL);
  
  return g_object_new (HN_TYPE_APP_MENU_ITEM,
                       "entry-info", info,
                       "show-close", show_close,
                       "thumbable", thumbable,
                       NULL);
}

void
hn_app_menu_item_set_entry_info (HNAppMenuItem *menuitem,
		                 HNEntryInfo   *info)
{
  HNAppMenuItemPrivate *priv;
  GdkPixbuf *pixbuf;
  
  g_return_if_fail (HN_IS_APP_MENU_ITEM (menuitem));
  g_return_if_fail (info != NULL);

  priv = menuitem->priv;

  priv->info = info;
  
  pixbuf = hn_entry_info_get_icon (priv->info);
  if (!pixbuf)
    {
      const gchar *icon_name;
      GError *error = NULL;

      icon_name = hn_entry_info_get_app_icon_name (priv->info);
      if (!icon_name)
        icon_name = AS_MENU_DEFAULT_APP_ICON;

      pixbuf = gtk_icon_theme_load_icon (priv->icon_theme,
                                         icon_name,
                                         priv->thumbable ?
                                         AS_ICON_THUMB_SIZE : AS_ICON_SIZE,
                                         GTK_ICON_LOOKUP_NO_SVG,
                                         &error);
      if (error)
        {
          g_warning ("Could not load icon %s from theme: %s.",
                     icon_name,
                     error->message);
          g_error_free (error);
          error = NULL;
          icon_name = AS_MENU_DEFAULT_APP_ICON;
          pixbuf = gtk_icon_theme_load_icon (priv->icon_theme,
                                             icon_name,
                                             priv->thumbable ?
                                             AS_ICON_THUMB_SIZE : AS_ICON_SIZE,
                                             GTK_ICON_LOOKUP_NO_SVG,
                                             &error);
          if (error)
            {
              g_warning ("Could not load icon %s from theme: %s.",
                         icon_name,
                         error->message);
              g_error_free (error);
            }
        }

    }
  
  if (pixbuf)
    {
      gtk_image_set_from_pixbuf (GTK_IMAGE (priv->icon), pixbuf);
      g_object_unref (pixbuf);
    }
  
  gtk_label_set_text (GTK_LABEL (priv->label),
		      hn_entry_info_peek_title (priv->info));

  g_object_notify (G_OBJECT (menuitem), "entry-info");
}

HNEntryInfo *
hn_app_menu_item_get_entry_info (HNAppMenuItem *menuitem)
{
  g_return_val_if_fail (HN_IS_APP_MENU_ITEM (menuitem), NULL);

  return menuitem->priv->info;
}

void
hn_app_menu_item_set_is_blinking (HNAppMenuItem *menuitem,
				  gboolean       is_blinking)
{
  g_return_if_fail (HN_IS_APP_MENU_ITEM (menuitem));

  hn_app_menu_item_icon_animation (menuitem->priv->icon, is_blinking);
}

gboolean
hn_app_menu_item_get_is_blinking (HNAppMenuItem *menuitem)
{
  g_return_val_if_fail (HN_IS_APP_MENU_ITEM (menuitem), FALSE);

  return menuitem->priv->is_blinking;
}
