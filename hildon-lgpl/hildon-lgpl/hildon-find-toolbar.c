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
#include "hildon-defines.h"
#include <gtk/gtklabel.h>
#include <gtk/gtkentry.h>
#include <gtk/gtkbutton.h>
#include <gtk/gtktoolbutton.h>
#include <gtk/gtktoolitem.h>
#include <gtk/gtkcomboboxentry.h>
#include <gtk/gtkseparatortoolitem.h>
#include <string.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <libintl.h>
#define _(String) dgettext(PACKAGE, String)

/*same define as gtkentry.c as entry will further handle this*/
#define MAX_SIZE G_MAXUSHORT
#define FIND_LABEL_XPADDING 6
#define FIND_LABEL_YPADDING 0

enum
{
  SEARCH = 0,
  CLOSE,
  INVALID_INPUT,
  HISTORY_APPEND,

  LAST_SIGNAL
};

enum
{
  PROP_LABEL = 1,
  PROP_PREFIX,
  PROP_LIST,
  PROP_COLUMN,
  PROP_MAX,
  PROP_HISTORY_LIMIT
};

struct _HildonFindToolbarPrivate
{
  GtkWidget*		label;
  GtkWidget*		entry_combo_box;
  GtkToolItem*		find_button;
  GtkToolItem*		separator;
  GtkToolItem*		close_button;

  /*GtkListStore*		list;*/
  gint			history_limit;
};
static guint HildonFindToolbar_signal[LAST_SIGNAL] = {0};

G_DEFINE_TYPE(HildonFindToolbar, \
	      hildon_find_toolbar, GTK_TYPE_TOOLBAR)

static gboolean
hildon_find_toolbar_filter(GtkTreeModel *model,
			   GtkTreeIter *iter,
			   gpointer self)
{
  GtkTreePath *path;
  gint *indices;
  gint n;
  gint limit;

  g_object_get(G_OBJECT(self), "history_limit", &limit, NULL);
  path = gtk_tree_model_get_path(model, iter);
  indices = gtk_tree_path_get_indices (path);
  /*get the first level index, and free the path*/
  n = indices[0];
  gtk_tree_path_free(path);
  
  if(n < limit)
    return TRUE;
  else
    return FALSE;
}

static void
hildon_find_toolbar_apply_filter(HildonFindToolbar *self, 
				 GtkListStore *list,
				 gboolean set_column)
{
  GtkTreeModel *filter;
  gint c_n = 0;
  HildonFindToolbarPrivate *priv = self->priv;

  filter = gtk_tree_model_filter_new(GTK_TREE_MODEL(list), NULL);
  
  /*get the text column*/
  if(gtk_combo_box_get_model(GTK_COMBO_BOX(priv->entry_combo_box)) != NULL)
    g_object_get(G_OBJECT(self), "column", &c_n, NULL);
  
  /*add filter function*/
  gtk_tree_model_filter_set_visible_func(GTK_TREE_MODEL_FILTER(filter), 
					 hildon_find_toolbar_filter, 
					 (gpointer) self, NULL);
  
  /*set the new filtered model*/
  gtk_combo_box_set_model(GTK_COMBO_BOX(priv->entry_combo_box), filter);

  /*once set, we need to remove one reference count for the fitler*/
  g_object_unref(filter);

  /*reset the text column if needed*/
  if ( gtk_combo_box_entry_get_text_column(
	GTK_COMBO_BOX_ENTRY(priv->entry_combo_box)) == -1 && set_column)/*unset*/
    gtk_combo_box_entry_set_text_column(
	GTK_COMBO_BOX_ENTRY(priv->entry_combo_box), c_n);
}

static void
hildon_find_toolbar_get_property(GObject     *object,
				 guint        prop_id,
				 GValue      *value,
				 GParamSpec  *pspec)
{
  const gchar *string;
  gint c_n;
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
      g_value_set_object(value, 
			 (gpointer) gtk_tree_model_filter_get_model(
			 GTK_TREE_MODEL_FILTER(gtk_combo_box_get_model(
			 GTK_COMBO_BOX(priv->entry_combo_box)))));
      break;
    case PROP_COLUMN:
      c_n = gtk_combo_box_entry_get_text_column(
	GTK_COMBO_BOX_ENTRY(priv->entry_combo_box));
      g_value_set_int(value, c_n);
      break;
    case PROP_MAX:
      g_value_set_int(value, gtk_entry_get_max_length(GTK_ENTRY(gtk_bin_get_child
						(GTK_BIN(priv->entry_combo_box)))));
      break;
    case PROP_HISTORY_LIMIT:
      g_value_set_int(value, priv->history_limit);
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
      hildon_find_toolbar_apply_filter(HILDON_FIND_TOOLBAR(object), 
				GTK_LIST_STORE(g_value_get_object(value)), 
				FALSE);
      break;
    case PROP_COLUMN:
      gtk_combo_box_entry_set_text_column (
       GTK_COMBO_BOX_ENTRY(priv->entry_combo_box), g_value_get_int(value));
      break;
    case PROP_MAX:
      gtk_entry_set_max_length(GTK_ENTRY(gtk_bin_get_child
			      (GTK_BIN(priv->entry_combo_box))), 
			      g_value_get_int(value));
      break;
    case PROP_HISTORY_LIMIT:
      priv->history_limit = g_value_get_int(value);
      model = gtk_combo_box_get_model(GTK_COMBO_BOX(priv->entry_combo_box));
      if(model != NULL)
	{
	  /*FIXME refilter function doesnt update the status of the combobox
	   * pop up arrowi, which will cause crash, recall apply filter 
	   * solves the problem*/
	  
	  /*gtk_tree_model_filter_refilter(GTK_TREE_MODEL_FILTER(model));*/
	  
	  hildon_find_toolbar_apply_filter(HILDON_FIND_TOOLBAR(object),GTK_LIST_STORE(
			gtk_tree_model_filter_get_model(GTK_TREE_MODEL_FILTER(model))),
					   TRUE);
	}
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static gboolean
hildon_find_toolbar_history_append(HildonFindToolbar *self,
				   gpointer data) 
{
  gchar *string;
  gchar *old_string;
  gboolean occupy;
  gint c_n = 0;
  GtkListStore *list = NULL;
  GtkTreeIter iter;
  gboolean self_create = FALSE;
  HildonFindToolbarPrivate *priv = HILDON_FIND_TOOLBAR(self)->priv;
  
  g_object_get(G_OBJECT(self), "prefix", &string, NULL);
  
  if(strcmp(string, "") != 0)
    {
      if(gtk_combo_box_get_model(GTK_COMBO_BOX(priv->entry_combo_box)) != NULL)
	list = GTK_LIST_STORE(gtk_tree_model_filter_get_model(
			GTK_TREE_MODEL_FILTER(gtk_combo_box_get_model(							  GTK_COMBO_BOX(priv->entry_combo_box)))));

      if(list == NULL)
	{
	  list = gtk_list_store_new(1, G_TYPE_STRING);
	  self_create = TRUE;
	}
      else
	g_object_get(G_OBJECT(self), "column", &c_n, NULL);

      /*add new item into the list*/
      occupy = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(list), &iter);
      while(occupy)
	{
	  gtk_tree_model_get (GTK_TREE_MODEL(list), 
			      &iter, c_n, &old_string, -1);
	  if(old_string != NULL && 
	     strcmp(string, old_string) == 0)
	    {
	      occupy = FALSE;
	      /*remove the existing one, so we could prepend to the begining
	       *to show that it is the latest item*/
	      gtk_list_store_remove (list, &iter);
	    }
	  else
	    occupy = gtk_tree_model_iter_next(GTK_TREE_MODEL(list), &iter);
	}
      if(c_n >= 0)
	{
	  gtk_list_store_prepend(list, &iter);
	  gtk_list_store_set(list, &iter, c_n, string, -1);
	  if(self_create)
	    {
	      hildon_find_toolbar_apply_filter(self, list, TRUE);
	      /*need to unref our g object, after it has been packed 
	       *the first time*/
	      g_object_unref(list);
	    }
	  else
	    gtk_tree_model_filter_refilter(GTK_TREE_MODEL_FILTER(
		gtk_combo_box_get_model(GTK_COMBO_BOX(priv->entry_combo_box))));
	}
    }
  g_free(string);
 
  return TRUE;
}

static void
hildon_find_toolbar_emit_search(GtkButton *button, gpointer self)
{
  gboolean rb;
  
  g_signal_emit_by_name(self, "search", NULL);
  g_signal_emit_by_name(self, "history_append", &rb, NULL);
}

static void
hildon_find_toolbar_emit_close(GtkButton *button, gpointer self)
{
  g_signal_emit_by_name(self, "close", NULL);
}

static void
hildon_find_toolbar_emit_invalid_input(GtkEntry *entry, 
				       GtkInvalidInputType type, 
				       gpointer self)
{
  if(type == GTK_INVALID_INPUT_MAX_CHARS_REACHED)
    g_signal_emit_by_name(self, "invalid_input", NULL);
}

static void
hildon_find_toolbar_class_init(HildonFindToolbarClass *klass)
{
  GObjectClass *object_class;

  g_type_class_add_private(klass, sizeof(HildonFindToolbarPrivate));

  object_class = G_OBJECT_CLASS(klass);

  object_class->get_property = hildon_find_toolbar_get_property;
  object_class->set_property = hildon_find_toolbar_set_property;

  klass->history_append = hildon_find_toolbar_history_append;
  
  g_object_class_install_property(object_class, PROP_LABEL, 
				  g_param_spec_string("label", 
				  "Label", "Displayed name for"
				  " find-toolbar",
                                  _("Ecdg_ti_find_toolbar_label"),
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

  g_object_class_install_property(object_class, PROP_MAX,
				  g_param_spec_int("max_characters",
				  "Maximum amount of characters",
				  "Maximum amount of characters "
				  "in search string",
				  0, MAX_SIZE,
				  0, G_PARAM_READWRITE |
				  G_PARAM_CONSTRUCT));
  
  g_object_class_install_property(object_class, PROP_HISTORY_LIMIT,
				  g_param_spec_int("history_limit",
				  "Maximum number of history items",
				  "Maximum number of history items "
				  "in searching combobox",
				  0, G_MAXINT,
				  5, G_PARAM_READWRITE |
				  G_PARAM_CONSTRUCT));

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
  
  HildonFindToolbar_signal[INVALID_INPUT] = 
    			     g_signal_new(
			     "invalid_input", HILDON_TYPE_FIND_TOOLBAR,
			     G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET 
			     (HildonFindToolbarClass, invalid_input),
			     NULL, NULL, gtk_marshal_VOID__VOID,
			     G_TYPE_NONE, 0);
  
  HildonFindToolbar_signal[HISTORY_APPEND] = 
    			     g_signal_new(
			     "history_append", HILDON_TYPE_FIND_TOOLBAR,
			     G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET 
			     (HildonFindToolbarClass, history_append),
			     g_signal_accumulator_true_handled, NULL, 
			     gtk_marshal_BOOLEAN__VOID,
			     G_TYPE_BOOLEAN, 0);
}

static void
hildon_find_toolbar_init(HildonFindToolbar *self)
{
  GtkToolItem *label_container;
  GtkToolItem *entry_combo_box_container;
  
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, \
					   HILDON_TYPE_FIND_TOOLBAR, 
					   HildonFindToolbarPrivate);

  /* Create the label */
  self->priv->label = gtk_label_new(_("Ecdg_ti_find_toolbar_label"));
  
  gtk_misc_set_padding (GTK_MISC(self->priv->label), FIND_LABEL_XPADDING,
                        FIND_LABEL_YPADDING);

  label_container = gtk_tool_item_new();
  gtk_container_add(GTK_CONTAINER(label_container), 
		    self->priv->label);
  gtk_widget_show_all(GTK_WIDGET(label_container));
  gtk_toolbar_insert (GTK_TOOLBAR(self), label_container, -1);
  
  /* ComboBox */
  self->priv->entry_combo_box = gtk_combo_box_entry_new();
  g_signal_connect(G_OBJECT(gtk_bin_get_child(
		   GTK_BIN(self->priv->entry_combo_box))), 
		   "invalid_input", 
		   G_CALLBACK(hildon_find_toolbar_emit_invalid_input),
		   (gpointer) self);
  
  entry_combo_box_container = gtk_tool_item_new();
  gtk_tool_item_set_expand(entry_combo_box_container, TRUE);
  gtk_container_add(GTK_CONTAINER(entry_combo_box_container),
		    self->priv->entry_combo_box);
  gtk_widget_show_all(GTK_WIDGET(entry_combo_box_container));
  gtk_toolbar_insert (GTK_TOOLBAR(self), entry_combo_box_container, -1);

  /* First button */
  self->priv->find_button = gtk_tool_button_new (
                              gtk_image_new_from_icon_name ("qgn_toolb_browser_gobutton",
                                                            HILDON_ICON_SIZE_TOOLBAR),
                              "Find");
  g_signal_connect(G_OBJECT(self->priv->find_button), "clicked",
		   G_CALLBACK(hildon_find_toolbar_emit_search), 
		   (gpointer) self);
  gtk_widget_show_all(GTK_WIDGET(self->priv->find_button));
  gtk_toolbar_insert (GTK_TOOLBAR(self), self->priv->find_button, -1);
  
  /* Separator */
  self->priv->separator = gtk_separator_tool_item_new();
  gtk_widget_show(GTK_WIDGET(self->priv->separator));
  gtk_toolbar_insert (GTK_TOOLBAR(self), self->priv->separator, -1);
  
  /* Second button */
  self->priv->close_button = gtk_tool_button_new (
                               gtk_image_new_from_icon_name ("qgn_toolb_gene_close",
                                                             HILDON_ICON_SIZE_TOOLBAR),
                               "Close");
  g_signal_connect(G_OBJECT(self->priv->close_button), "clicked",
		   G_CALLBACK(hildon_find_toolbar_emit_close), 
		   (gpointer) self);
  gtk_widget_show_all(GTK_WIDGET(self->priv->close_button));
  gtk_toolbar_insert (GTK_TOOLBAR(self), self->priv->close_button, -1);
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

/**
 * hildon_find_toolbar_highlight_entry
 * @HildonFindToolbar: Find Toolbar whose entry is to be highlighted.
 * @get_focus: if user passes TRUE to this value, then the text in
 * the entry will not only get highlighted, but also get focused.
 * 
 * */
void
hildon_find_toolbar_highlight_entry(HildonFindToolbar *ftb,
                                    gboolean get_focus)
{
  GtkWidget *entry = NULL;
  
  if(!HILDON_IS_FIND_TOOLBAR(ftb))
    return;
  
  entry = gtk_bin_get_child(GTK_BIN(ftb->priv->entry_combo_box));
  
  gtk_editable_select_region(GTK_EDITABLE(entry), 0, -1);
  
  if(get_focus)
    gtk_widget_grab_focus(entry);
}
