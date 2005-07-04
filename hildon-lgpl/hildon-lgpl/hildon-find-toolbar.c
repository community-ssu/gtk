/*
 * This file is part of hildon-lgpl
 *
 * Copyright (C) 2005 Nokia Corporation.
 *
 * Contact: Luc Pionchon <luc.pionchon@nokia.com>
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

#include "hildon-find-toolbar.h"
#include <gtk/gtklabel.h>
#include <gtk/gtkentry.h>
#include <gtk/gtkbutton.h>
#include <gtk/gtktoolitem.h>
#include <gtk/gtkcomboboxentry.h>
#include <gtk/gtkseparatortoolitem.h>
#include <string.h>

enum
{
  SEARCH = 0,
  CLOSE,

  LAST_SIGNAL
};

enum
{
  PROP_LABEL = 1,
  PROP_PREFIX,
  PROP_LIST,
  PROP_COLUMN
};

struct _HildonFindToolbarPrivate
{
  GtkWidget*		label;
  GtkWidget*		entry_combo_box;
  GtkWidget*		find_button;
  GtkToolItem*		separator;
  GtkWidget*		close_button;
};
static guint HildonFindToolbar_signal[LAST_SIGNAL] = {0};

G_DEFINE_TYPE(HildonFindToolbar, \
	      hildon_find_toolbar, GTK_TYPE_TOOLBAR)

static void
hildon_find_toolbar_get_property(GObject     *object,
				 guint        prop_id,
				 GValue      *value,
				 GParamSpec  *pspec)
{
  const gchar *string;
  gint c_n;
  GtkTreeModel *model;
  HildonFindToolbarPrivate *priv = HILDON_FIND_TOOLBAR(object)->priv;
  
  switch (prop_id)
    {
    case PROP_LABEL:
      string = gtk_label_get_text(GTK_LABEL(priv->label));
      g_value_set_string(value, string);
      break;
    case PROP_PREFIX:
      string = gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child
				  (GTK_BIN(priv->entry_combo_box))));
      g_value_set_string(value, string);
      break;
    case PROP_LIST:
      model = gtk_combo_box_get_model(GTK_COMBO_BOX(priv->entry_combo_box));
      g_value_set_object(value, (gpointer) model);
      break;
    case PROP_COLUMN:
      c_n = gtk_combo_box_entry_get_text_column(
	GTK_COMBO_BOX_ENTRY(priv->entry_combo_box));
      g_value_set_int(value, c_n);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
hildon_find_toolbar_set_property(GObject      *object,
				 guint         prop_id,
				 const GValue *value,
				 GParamSpec   *pspec)
{
  const gchar *string;
  GtkTreeModel *model;
  HildonFindToolbarPrivate *priv = HILDON_FIND_TOOLBAR(object)->priv;
  
  switch (prop_id)
    {
    case PROP_LABEL:
      string = g_value_get_string(value);	
      gtk_label_set_text(GTK_LABEL(priv->label), string);
      break;
    case PROP_PREFIX:
      string = g_value_get_string(value);
      gtk_entry_set_text(GTK_ENTRY(gtk_bin_get_child
			 (GTK_BIN(priv->entry_combo_box))), 
			 string);
      break;
    case PROP_LIST:
      model = GTK_TREE_MODEL(g_value_get_object(value));
      gtk_combo_box_set_model(GTK_COMBO_BOX(priv->entry_combo_box), model);
      break;
    case PROP_COLUMN:
      gtk_combo_box_entry_set_text_column (
       GTK_COMBO_BOX_ENTRY(priv->entry_combo_box), g_value_get_int(value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
hildon_find_toolbar_emit_search(GtkButton *button, gpointer self)
{
  gchar *string;
  gchar *old_string;
  gboolean occupy;
  gint c_n = 0;
  GtkListStore *list;
  GtkTreeIter iter;
  HildonFindToolbarPrivate *priv = HILDON_FIND_TOOLBAR(self)->priv;
  
  g_object_get(G_OBJECT(self), "prefix", &string, NULL);
  if(strcmp(string, "") != 0)
    {
      list = GTK_LIST_STORE(
			gtk_combo_box_get_model(GTK_COMBO_BOX(
			priv->entry_combo_box)));

      if(list == NULL)
	list = gtk_list_store_new(1, G_TYPE_STRING);
      else
	g_object_get(G_OBJECT(self), "column", &c_n, NULL);

      occupy = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(list), &iter);
      while(occupy)
	{
	  gtk_tree_model_get (GTK_TREE_MODEL(list), 
			      &iter, c_n, &old_string, -1);
	  if(old_string != NULL && 
	     strcmp(string, old_string) == 0)
	    {
	      occupy = FALSE;
	      c_n = -1;
	    }
	  else
	    occupy = gtk_tree_model_iter_next(GTK_TREE_MODEL(list), &iter);
	}
      if(c_n >= 0)
	{
	  gtk_list_store_append(list, &iter);
	  gtk_list_store_set(list, &iter, c_n, string, -1);
	  gtk_combo_box_set_model(GTK_COMBO_BOX(priv->entry_combo_box), 
				  GTK_TREE_MODEL(list));
	  if ( gtk_combo_box_entry_get_text_column(
	       GTK_COMBO_BOX_ENTRY(priv->entry_combo_box)) == -1)/*unset*/
	    gtk_combo_box_entry_set_text_column(
		GTK_COMBO_BOX_ENTRY(priv->entry_combo_box), c_n);
	}
    }
  g_free(string);
  g_signal_emit_by_name(self, "search", NULL);
}

static void
hildon_find_toolbar_emit_close(GtkButton *button, gpointer self)
{
  g_signal_emit_by_name(self, "close", NULL);
}

static void
hildon_find_toolbar_class_init(HildonFindToolbarClass *klass)
{
  GObjectClass *object_class;

  g_type_class_add_private(klass, sizeof(HildonFindToolbarPrivate));

  object_class = G_OBJECT_CLASS(klass);

  object_class->get_property = hildon_find_toolbar_get_property;
  object_class->set_property = hildon_find_toolbar_set_property;
  
  g_object_class_install_property(object_class, PROP_LABEL, 
				  g_param_spec_string("label", 
				  "Label", "Displayed name for"
				  " find-toolbar", "Find", 
				  G_PARAM_READWRITE |
				  G_PARAM_CONSTRUCT));
  
  g_object_class_install_property(object_class, PROP_PREFIX, 
				  g_param_spec_string("prefix", 
				  "Prefix", "Search that "
				  "start with this prefix are"
				  " displayed", NULL, 
				  G_PARAM_READWRITE));
  
  g_object_class_install_property(object_class, PROP_LIST,
				  g_param_spec_object("list",
                                  "List"," Storelist for the "
				  "content to be displayed in"
				  " in combobox", 
				  GTK_TYPE_LIST_STORE,
				  G_PARAM_READWRITE));

  g_object_class_install_property(object_class, PROP_COLUMN,
				  g_param_spec_int("column",
				  "Column", "Text column for"
				  " history list to retrieve"
				  " strings", 0, G_MAXINT,
				  0, G_PARAM_READWRITE));

  HildonFindToolbar_signal[SEARCH] = 
    			      g_signal_new(
			      "search", HILDON_TYPE_FIND_TOOLBAR,
			      G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET 
			      (HildonFindToolbarClass, search),
			      NULL, NULL, gtk_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);
  
  HildonFindToolbar_signal[CLOSE] = 
    			     g_signal_new(
			     "close", HILDON_TYPE_FIND_TOOLBAR,
			     G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET 
			     (HildonFindToolbarClass, close),
			     NULL, NULL, gtk_marshal_VOID__VOID,
			     G_TYPE_NONE, 0);
}

static void
hildon_find_toolbar_init(HildonFindToolbar *self)
{
  GtkToolItem *label_container;
  GtkToolItem *entry_combo_box_container;
  GtkToolItem *find_button_container;
  GtkToolItem *close_button_container;
  
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, \
					   HILDON_TYPE_FIND_TOOLBAR, 
					   HildonFindToolbarPrivate);

  self->priv->label = gtk_label_new("Find");
  label_container = gtk_tool_item_new();
  gtk_container_add(GTK_CONTAINER(label_container), 
		    self->priv->label);
  gtk_toolbar_insert (GTK_TOOLBAR(self), label_container, -1);
  
  self->priv->entry_combo_box = gtk_combo_box_entry_new();
  entry_combo_box_container = gtk_tool_item_new();
  gtk_container_add(GTK_CONTAINER(entry_combo_box_container),
		    self->priv->entry_combo_box);
  gtk_toolbar_insert (GTK_TOOLBAR(self), entry_combo_box_container, -1);

  self->priv->find_button = gtk_button_new_with_label("Search");
  g_signal_connect(G_OBJECT(self->priv->find_button), "clicked",
		   G_CALLBACK(hildon_find_toolbar_emit_search), 
		   (gpointer) self);
  find_button_container = gtk_tool_item_new();
  gtk_container_add(GTK_CONTAINER(find_button_container), 
		    self->priv->find_button);
  gtk_toolbar_insert (GTK_TOOLBAR(self), find_button_container, -1);
  
  self->priv->separator = gtk_separator_tool_item_new();
  gtk_toolbar_insert (GTK_TOOLBAR(self), self->priv->separator, -1);
  
  self->priv->close_button = gtk_button_new_with_label("Close");
  g_signal_connect(G_OBJECT(self->priv->close_button), "clicked",
		   G_CALLBACK(hildon_find_toolbar_emit_close), 
		   (gpointer) self);
  close_button_container = gtk_tool_item_new();
  gtk_container_add(GTK_CONTAINER(close_button_container), 
		    self->priv->close_button);
  gtk_toolbar_insert (GTK_TOOLBAR(self), close_button_container, -1);
}

/*Public functions*/

/**
 * hildon_find_toolbar_new:
 * @label: label for the find_toolbar, NULL to set the label to 
 *         default "Find".
 * 
 * Returns a new HildonFindToolbar.
 *
 **/

GtkWidget *
hildon_find_toolbar_new(gchar *label)
{
  GtkWidget *findtoolbar;
  
  findtoolbar =  GTK_WIDGET(g_object_new(HILDON_TYPE_FIND_TOOLBAR, NULL));
  if(label != NULL)
    g_object_set(G_OBJECT(findtoolbar), "label", label, NULL);

  return findtoolbar;
}

/**
 * hildon_find_toolbar_new_with_model
 * @label: label for the find_toolbar, NULL to set the label to 
 *         default "Find".
 * @model: A @GtkListStore.
 * @column: Indicating which column the search histry list will 
 *          retreive string from.
 * 
 * Returns a new HildonFindToolbar, with a model.
 *
 **/
GtkWidget *
hildon_find_toolbar_new_with_model(gchar *label, 
				   GtkListStore *model,
				   gint column)
{
  GtkWidget *findtoolbar;

  findtoolbar = hildon_find_toolbar_new(label);
  g_object_set(G_OBJECT(findtoolbar), "list", model, 
	       "column", column, NULL);

  return findtoolbar;
}
