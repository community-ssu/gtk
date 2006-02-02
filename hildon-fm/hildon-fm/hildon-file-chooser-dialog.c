/*
 * This file is part of hildon-fm package
 *
 * Copyright (C) 2005 Nokia Corporation.
 *
 * Contact: Kimmo Hämäläinen <kimmo.hamalainen@nokia.com>
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

 /*
  * HildonFileChooserDialog widget
  */

#include "hildon-file-selection.h"
#include "hildon-file-chooser-dialog.h"
#include "hildon-file-system-private.h"
#include <hildon-widgets/gtk-infoprint.h>
#include <osso-log.h>
#include <gtk/gtkbutton.h>
#include <gtk/gtkbox.h>
#include <gtk/gtkentry.h>
#include <gtk/gtkmain.h>
#include <gtk/gtkradiomenuitem.h>
#include <gtk/gtkseparatormenuitem.h>
#include <gtk/gtkfilechooser.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkimage.h>
#include <gtk/gtklabel.h>
#include <string.h>
#include <hildon-widgets/hildon-caption.h>
#include <hildon-widgets/hildon-defines.h>
#include <libintl.h>
#include <gdk/gdkx.h>
#include <stdlib.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define _(String) dgettext(PACKAGE, String)
#define MAX_FILENAME_LENGTH_DEFAULT 255 /* If env doesn't define, use this */
#define HILDON_RESPONSE_FOLDER_BUTTON 12345
/* Common height for filetrees. About 8 lines. Filetree sets default margins, 
    so we need to take them into account. See #9962. */
#define FILE_SELECTION_HEIGHT (8 * 30 + 2 * HILDON_MARGIN_DEFAULT)  
#define FILE_SELECTION_WIDTH_LIST 240   /* Width used in select folder
                                           mode */
#define FILE_SELECTION_WIDTH_TOTAL 590  /* Width for full filetree (both
                                           content and navigation pane) */

/* Copy paste from gtkfilechooserprivate.h to make implementation of
   file chooser interface possible */
typedef struct _GtkFileChooserIface GtkFileChooserIface;

struct _GtkFileChooserIface {
    GTypeInterface base_iface;

    /* Methods */
     gboolean(*set_current_folder) (GtkFileChooser * chooser,
                                    const GtkFilePath * path,
                                    GError ** error);
    GtkFilePath *(*get_current_folder) (GtkFileChooser * chooser);
    void (*set_current_name) (GtkFileChooser * chooser,
                              const gchar * name);
     gboolean(*select_path) (GtkFileChooser * chooser,
                             const GtkFilePath * path, GError ** error);
    void (*unselect_path) (GtkFileChooser * chooser,
                           const GtkFilePath * path);
    void (*select_all) (GtkFileChooser * chooser);
    void (*unselect_all) (GtkFileChooser * chooser);
    GSList *(*get_paths) (GtkFileChooser * chooser);
    GtkFilePath *(*get_preview_path) (GtkFileChooser * chooser);
    GtkFileSystem *(*get_file_system) (GtkFileChooser * chooser);
    void (*add_filter) (GtkFileChooser * chooser, GtkFileFilter * filter);
    void (*remove_filter) (GtkFileChooser * chooser,
                           GtkFileFilter * filter);
    GSList *(*list_filters) (GtkFileChooser * chooser);
     gboolean(*add_shortcut_folder) (GtkFileChooser * chooser,
                                     const GtkFilePath * path,
                                     GError ** error);
     gboolean(*remove_shortcut_folder) (GtkFileChooser * chooser,
                                        const GtkFilePath * path,
                                        GError ** error);
    GSList *(*list_shortcut_folders) (GtkFileChooser * chooser);

    /* Signals */
    void (*current_folder_changed) (GtkFileChooser * chooser);
    void (*selection_changed) (GtkFileChooser * chooser);
    void (*update_preview) (GtkFileChooser * chooser);
    void (*file_activated) (GtkFileChooser * chooser);
};

/* From gtkfilechooserutils.h */

typedef enum {
    GTK_FILE_CHOOSER_PROP_FIRST = 0x1000,
    GTK_FILE_CHOOSER_PROP_ACTION = GTK_FILE_CHOOSER_PROP_FIRST,
    GTK_FILE_CHOOSER_PROP_FILE_SYSTEM_BACKEND,
    GTK_FILE_CHOOSER_PROP_FILTER,
    GTK_FILE_CHOOSER_PROP_LOCAL_ONLY,
    GTK_FILE_CHOOSER_PROP_PREVIEW_WIDGET,
    GTK_FILE_CHOOSER_PROP_PREVIEW_WIDGET_ACTIVE,
    GTK_FILE_CHOOSER_PROP_USE_PREVIEW_LABEL,
    GTK_FILE_CHOOSER_PROP_EXTRA_WIDGET,
    GTK_FILE_CHOOSER_PROP_SELECT_MULTIPLE,
    GTK_FILE_CHOOSER_PROP_SHOW_HIDDEN,
    GTK_FILE_CHOOSER_PROP_LAST = GTK_FILE_CHOOSER_PROP_SHOW_HIDDEN
} GtkFileChooserProp;

enum {
    PROP_EMPTY_TEXT = 0x2000,
    PROP_FILE_SYSTEM_MODEL,
    PROP_FOLDER_BUTTON,
    PROP_LOCATION,
    PROP_AUTONAMING,
    PROP_OPEN_BUTTON_TEXT,
    PROP_MULTIPLE_TEXT,
    PROP_MAX_NAME_LENGTH,
    PROP_MAX_FULL_PATH_LENGTH
};

void hildon_gtk_file_chooser_install_properties(GObjectClass * klass);

/* CopyPaste ends */

struct _HildonFileChooserDialogPrivate {
    GtkWidget *action_button;
    GtkWidget *folder_button;
    GtkWidget *cancel_button;
    HildonFileSelection *filetree;
    HildonFileSystemModel *model;
    GtkWidget *caption_control_name;
    GtkWidget *caption_control_location;
    GtkWidget *label_location;
    GtkWidget *entry_name;
    GtkWidget *hbox_location, *image_location, *title_location;
    GtkFileChooserAction action;
    GtkWidget *popup;
    GtkWidget *multiple_label, *hbox_items;
    gulong changed_handler;
    gint max_full_path_length;
    gint max_filename_length;
    gboolean popup_protect;

    /* Popup menu contents */
    GtkWidget *sort_type, *sort_name, *sort_date, *sort_size;

    GtkWidget *mode_list, *mode_thumbnails;
    GSList *filters;

    gchar *stub_name;   /* Set by call to set_current_name */
    gchar *ext_name;
    gboolean autonaming_enabled;
    gboolean edited;
    gboolean should_show_folder_button;
    gboolean should_show_location;
};

static void hildon_file_chooser_dialog_iface_init(GtkFileChooserIface *
                                                  iface);
static GObject *
hildon_file_chooser_dialog_constructor(GType type,
                                       guint n_construct_properties,
                                       GObjectConstructParam *
                                       construct_properties);

G_DEFINE_TYPE_EXTENDED
    (HildonFileChooserDialog, hildon_file_chooser_dialog,
     GTK_TYPE_DIALOG, 0,
     G_IMPLEMENT_INTERFACE(GTK_TYPE_FILE_CHOOSER,
                           hildon_file_chooser_dialog_iface_init))

/* <glib> 

    Borrowed from glib, because it's static there.
    I do not know if there is a better way to do
    this. conversion. */
static int
unescape_character (const char *scanner)
{
  int first_digit;
  int second_digit;

  first_digit = g_ascii_xdigit_value (scanner[0]);
  if (first_digit < 0)
    return -1;

  second_digit = g_ascii_xdigit_value (scanner[1]);
  if (second_digit < 0)
    return -1;

  return (first_digit << 4) | second_digit;
}

static gchar *
g_unescape_uri_string (const char *escaped,
                       int         len,
                       const char *illegal_escaped_characters,
                       gboolean    ascii_must_not_be_escaped)
{
  const gchar *in, *in_end;
  gchar *out, *result;
  int c;

  if (escaped == NULL)
    return NULL;

  if (len < 0)
    len = strlen (escaped);

  result = g_malloc (len + 1);

  out = result;
  for (in = escaped, in_end = escaped + len; in < in_end; in++)
    {
      c = *in;

      if (c == '%')
        {
          /* catch partial escape sequences past the end of the substring */
          if (in + 3 > in_end)
            break;

          c = unescape_character (in + 1);

          /* catch bad escape sequences and NUL characters */
          if (c <= 0)
            break;

          /* catch escaped ASCII */
          if (ascii_must_not_be_escaped && c <= 0x7F)
            break;

          /* catch other illegal escaped characters */
          if (strchr (illegal_escaped_characters, c) != NULL)
            break;

          in += 2;
        }

      *out++ = c;
    }

  g_assert (out - result <= len);
  *out = '\0';

  if (in != in_end)
    {
      g_free (result);
      return NULL;
    }

  return result;
}

/* </glib> */

static gint
get_path_length_from_uri(const char *uri)
{
  const char *delim;
  char *unescaped;
  gint len;

  if (!uri) return 0;

  /* Skip protocol and hostname */
  delim = strstr(uri, "://");
  if (!delim) return 0;
  delim = strchr(delim + 3, '/');
  if (!delim) return 0;

  unescaped = g_unescape_uri_string (delim, -1, "/", FALSE);
  if (!unescaped) return 0;

  ULOG_DEBUG("Original uri = %s", uri);
  /* Should Not be utf8, but G_FILENAME_ENCODING, BUT it still is. */ 
  len = g_utf8_strlen(unescaped, -1);
  ULOG_DEBUG("Unescaped path = %s, length = %d", unescaped, len);
  g_free(unescaped);

  return len;
}

static void
hildon_file_chooser_dialog_set_limit(HildonFileChooserDialog *self)
{
  gchar *uri;
  const char *str;
  gint max_length = 0, len;
  gboolean limited;

  /* We cannot get the current folder uri without filetree, 
     which is not available during initial setup */
  if (self->priv->max_full_path_length >= 0 && self->priv->filetree)
  {
    uri = gtk_file_chooser_get_current_folder_uri(GTK_FILE_CHOOSER(self));
    max_length = self->priv->max_full_path_length - get_path_length_from_uri(uri);
    g_free(uri);

    /* Possible extension length must be subtracted from maximum 
       possible length */
    if ((str = self->priv->ext_name) != NULL)
    {
      len = g_utf8_strlen(str, -1);
      max_length -= len;
      ULOG_DEBUG("Extension = [%s], length = %d", str, len);
    }

    /* We have to alloc space for trailing slash */
    max_length--;

    limited = TRUE;
  }
  else
    limited = FALSE;

  if (self->priv->max_filename_length >= 0 && 
     (!limited || self->priv->max_filename_length < max_length))
  {
    max_length = self->priv->max_filename_length;
    limited = TRUE;
  }

  if (limited)
  {
    /* Maximum length 0 means "no limit" for entry and negatives are
       not allowed.  Setting this to one at least. */
    if (max_length < 1) max_length = 1;

    ULOG_DEBUG("Setting maximum length to %d", max_length);
    gtk_entry_set_max_length(GTK_ENTRY(self->priv->entry_name), max_length);
  }
  else
  {
    ULOG_DEBUG("Unsetting maximum length");
    gtk_entry_set_max_length(GTK_ENTRY(self->priv->entry_name), 0);
  }
}

static void 
hildon_file_chooser_dialog_check_limit(GtkEditable *editable,
  gchar *new_text, gint new_text_length,
  gint *position, gpointer data)
{
  GtkEntry *entry = GTK_ENTRY(editable);

  if (entry->text_max_length > 0 && 
      entry->text_length + new_text_length > entry->text_max_length)
      gtk_infoprint(GTK_WINDOW(data), 
          _("ckdg_ib_maximum_characters_reached"));  
}

static void insensitive_button(GtkWidget *widget, gpointer data)
{ 
    gtk_infoprint(GTK_WINDOW(data), _("sfil_ib_select_file"));
}

static void file_activated_handler(GtkWidget * widget, gpointer user_data)
{
    gtk_dialog_response(GTK_DIALOG(user_data), GTK_RESPONSE_OK);
}

static void
hildon_file_chooser_dialog_select_text(HildonFileChooserDialogPrivate *priv)
{
  if (GTK_WIDGET_DRAWABLE(priv->entry_name)) {
    gtk_widget_grab_focus(priv->entry_name);
    gtk_editable_select_region(GTK_EDITABLE(priv->entry_name), 0, -1);
  }
}

static gboolean
hildon_file_chooser_dialog_save_multiple_set(HildonFileChooserDialogPrivate *priv)
{
  const char *text = gtk_label_get_text(GTK_LABEL(priv->multiple_label));
  return text && strlen(text) > 0;
}

static void
hildon_file_chooser_dialog_do_autonaming(HildonFileChooserDialogPrivate *
                                         priv)
{
    g_assert(HILDON_IS_FILE_SELECTION(priv->filetree));

    if (GTK_WIDGET_VISIBLE(priv->caption_control_name) &&
        priv->stub_name && !priv->edited) 
    {
        gchar *name = NULL;
        gboolean selection;
        gint pos;

        g_signal_handler_block(priv->entry_name, priv->changed_handler);
        if (priv->autonaming_enabled) {
            GtkTreeIter iter;

            ULOG_INFO("Trying [%s] [%s]", priv->stub_name, priv->ext_name);
            if (hildon_file_selection_get_current_folder_iter
                (priv->filetree, &iter)) {
                name =
                    hildon_file_system_model_new_item(priv->model, &iter,
                                                      priv->stub_name,
                                                      priv->ext_name);
                ULOG_INFO("Got [%s]", name);
            }
        }

        pos = gtk_editable_get_position(GTK_EDITABLE(priv->entry_name));
        selection =
          gtk_editable_get_selection_bounds(GTK_EDITABLE(priv->entry_name),
                                            NULL, NULL);

        if (name)
        {
          gtk_entry_set_text(GTK_ENTRY(priv->entry_name), name);
          g_free(name);
          if (selection)
            gtk_editable_select_region(GTK_EDITABLE(priv->entry_name), 0, -1);
          else
            /* if the user has already started to edit the name,
               try to preserve cursor position and don't autoselect */
            gtk_editable_set_position(GTK_EDITABLE(priv->entry_name), pos);
        }
        else
          gtk_entry_set_text(GTK_ENTRY(priv->entry_name), priv->stub_name);

        g_signal_handler_unblock(priv->entry_name, priv->changed_handler);
    }
}

static void
hildon_file_chooser_dialog_finished_loading(GObject *sender, GtkTreeIter *iter, gpointer data)
{
  GtkTreeIter current_iter;
  HildonFileChooserDialogPrivate *priv = HILDON_FILE_CHOOSER_DIALOG(data)->priv;

  if (hildon_file_selection_get_current_folder_iter(priv->filetree, &current_iter) &&
      iter->user_data == current_iter.user_data)
  {
    ULOG_DEBUG(__FUNCTION__);
    hildon_file_chooser_dialog_do_autonaming(priv);
  }
}

static void
hildon_file_chooser_dialog_update_location_info
(HildonFileChooserDialogPrivate * priv)
{
    GtkTreeIter iter;

    if (hildon_file_selection_get_current_folder_iter
        (priv->filetree, &iter)) {
        GdkPixbuf *icon;
        gchar *title;

        gtk_tree_model_get(GTK_TREE_MODEL(priv->model), &iter,
                           HILDON_FILE_SYSTEM_MODEL_COLUMN_ICON, &icon,
                           HILDON_FILE_SYSTEM_MODEL_COLUMN_DISPLAY_NAME,
                           &title, -1);

        gtk_label_set_text(GTK_LABEL(priv->title_location), title);

        if (icon) /* It's possible that we don't get an icon */
        {
          gtk_image_set_from_pixbuf(GTK_IMAGE(priv->image_location), icon);
          g_object_unref(icon);
        }

        g_free(title);
    } else
        ULOG_INFO("Failed to get current folder iter");
}

static void 
hildon_file_chooser_dialog_selection_changed(HildonFileChooserDialogPrivate 
    *priv)
{
    if (priv->action == GTK_FILE_CHOOSER_ACTION_OPEN) 
    {
        GSList *files = _hildon_file_selection_get_selected_files(
                                      HILDON_FILE_SELECTION(priv->filetree));

        gtk_widget_set_sensitive(priv->action_button, files != NULL);
        gtk_file_paths_free(files);
    }
}

static void hildon_file_chooser_dialog_current_folder_changed(GtkWidget *
  widget, gpointer data)
{
  HildonFileChooserDialogPrivate *priv = data;

  hildon_file_chooser_dialog_selection_changed(priv);

  if (GTK_WIDGET_VISIBLE(priv->caption_control_location))
    hildon_file_chooser_dialog_update_location_info(priv);
  if (GTK_WIDGET_VISIBLE(priv->caption_control_name))
    hildon_file_chooser_dialog_do_autonaming(priv);  
}

static gboolean
hildon_file_chooser_dialog_set_current_folder(GtkFileChooser * chooser,
                                              const GtkFilePath * path,
                                              GError ** error)
{
    HildonFileChooserDialog *self;
    gboolean result;

    self = HILDON_FILE_CHOOSER_DIALOG(chooser);
    result = hildon_file_selection_set_current_folder(
        self->priv->filetree, path, error);
    hildon_file_chooser_dialog_set_limit(self);

    return result;
}

static GtkFilePath
    *hildon_file_chooser_dialog_get_current_folder(GtkFileChooser *
                                                   chooser)
{
    HildonFileChooserDialogPrivate *priv =
        HILDON_FILE_CHOOSER_DIALOG(chooser)->priv;
    return hildon_file_selection_get_current_folder(priv->filetree);
}

/* Sets current name as if entered by user */
static void hildon_file_chooser_dialog_set_current_name(GtkFileChooser *
                                                        chooser,
                                                        const gchar * name)
{
    gchar *dot;
    HildonFileChooserDialogPrivate *priv =
        HILDON_FILE_CHOOSER_DIALOG(chooser)->priv;

    g_free(priv->stub_name);
    priv->stub_name = g_strdup(name);
    dot = _hildon_file_system_search_extension(priv->stub_name, NULL);

    /* Is there a dot, but not as first character */
    if (dot && dot != priv->stub_name) { 
        g_free(priv->ext_name);
        priv->ext_name = g_strdup(dot);
        *dot = '\0';
    }

    /* If we have autonaming enabled, we try to remove possible
       autonumber from stub part. We do not want to do this
       always (saving existing file would be difficult otherwise) */
    if (priv->autonaming_enabled)
      _hildon_file_system_remove_autonumber(priv->stub_name);

    ULOG_INFO("Current name set: body = %s, ext = %s", priv->stub_name, priv->ext_name);
    hildon_file_chooser_dialog_set_limit(HILDON_FILE_CHOOSER_DIALOG(chooser));
    hildon_file_chooser_dialog_do_autonaming(priv);
}

static gboolean hildon_file_chooser_dialog_select_path(GtkFileChooser *
                                                       chooser,
                                                       const GtkFilePath *
                                                       path,
                                                       GError ** error)
{
    HildonFileChooserDialogPrivate *priv =
        HILDON_FILE_CHOOSER_DIALOG(chooser)->priv;
    return hildon_file_selection_select_path(priv->filetree, path, error);
}

static void hildon_file_chooser_dialog_unselect_path(GtkFileChooser *
                                                     chooser,
                                                     const GtkFilePath *
                                                     path)
{
    HildonFileChooserDialogPrivate *priv =
        HILDON_FILE_CHOOSER_DIALOG(chooser)->priv;
    hildon_file_selection_unselect_path(priv->filetree, path);
}

static void hildon_file_chooser_dialog_select_all(GtkFileChooser * chooser)
{
    HildonFileChooserDialogPrivate *priv =
        HILDON_FILE_CHOOSER_DIALOG(chooser)->priv;
    hildon_file_selection_select_all(priv->filetree);
}

static void hildon_file_chooser_dialog_unselect_all(GtkFileChooser *
                                                    chooser)
{
    HildonFileChooserDialogPrivate *priv =
        HILDON_FILE_CHOOSER_DIALOG(chooser)->priv;
    hildon_file_selection_unselect_all(priv->filetree);
}

static GSList *hildon_file_chooser_dialog_get_paths(GtkFileChooser *
                                                    chooser)
{
    GtkFilePath *file_path, *base_path;
    GtkFileSystem *backend;
    gchar *name, *name_without_dot_prefix;

    /* If we are asking a name from user, return it. Otherwise return
       selection */
    HildonFileChooserDialogPrivate *priv =
        HILDON_FILE_CHOOSER_DIALOG(chooser)->priv;

    if (priv->action == GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER ||
        hildon_file_chooser_dialog_save_multiple_set(priv))
        return g_slist_append(NULL,
                              hildon_file_selection_get_current_folder
                              (priv->filetree));
    
    if (priv->action == GTK_FILE_CHOOSER_ACTION_OPEN)
        return _hildon_file_selection_get_selected_files(priv->filetree);

    g_object_get(priv->entry_name, "text", &name, NULL);
    g_strstrip(name);

    if (priv->ext_name) {
        /* Dot is part of the extension */
       gchar *ext_name = g_strconcat(name, priv->ext_name, NULL);
       g_free(name);
       name = ext_name;
    }

    name_without_dot_prefix = name;
    while (*name_without_dot_prefix == '.')
      name_without_dot_prefix++;

    ULOG_INFO("Inputted name: [%s]", name_without_dot_prefix);

    backend = _hildon_file_system_model_get_file_system(priv->model);
    base_path =
            hildon_file_selection_get_current_folder(priv->filetree);
    file_path =
            gtk_file_system_make_path(backend, base_path, 
            name_without_dot_prefix, NULL);

    gtk_file_path_free(base_path);
    g_free(name);

    return g_slist_append(NULL, file_path);
}

static GtkFilePath
    *hildon_file_chooser_dialog_get_preview_path(GtkFileChooser * chooser)
{
    ULOG_ERR("HildonFileChooserDialog doesn\'t implement preview");
    return NULL;
}

static GtkFileSystem
    *hildon_file_chooser_dialog_get_file_system(GtkFileChooser * chooser)
{
    HildonFileChooserDialogPrivate *priv =
        HILDON_FILE_CHOOSER_DIALOG(chooser)->priv;
    return _hildon_file_system_model_get_file_system(priv->model);
}

static void hildon_file_chooser_dialog_add_filter(GtkFileChooser * chooser,
                                                  GtkFileFilter * filter)
{
    HildonFileChooserDialogPrivate *priv =
        HILDON_FILE_CHOOSER_DIALOG(chooser)->priv;

    if (g_slist_find(priv->filters, filter)) {
        ULOG_WARN_L("gtk_file_chooser_add_filter() called on filter "
                  "already in list");
        return;
    }

    g_object_ref(filter);
    gtk_object_sink(GTK_OBJECT(filter));
    priv->filters = g_slist_append(priv->filters, filter);
}

static void hildon_file_chooser_dialog_remove_filter(GtkFileChooser *
                                                     chooser,
                                                     GtkFileFilter *
                                                     filter)
{
    gint filter_index;
    HildonFileChooserDialogPrivate *priv =
        HILDON_FILE_CHOOSER_DIALOG(chooser)->priv;

    filter_index = g_slist_index(priv->filters, filter);

    if (filter_index < 0) {
        ULOG_WARN_L("gtk_file_chooser_remove_filter() called on filter "
                  "not in list");
        return;
    }

    priv->filters = g_slist_remove(priv->filters, filter);

    if (filter == hildon_file_selection_get_filter(priv->filetree))
        hildon_file_selection_set_filter(priv->filetree, NULL);

    g_object_unref(filter);
}

static GSList *hildon_file_chooser_dialog_list_filters(GtkFileChooser *
                                                       chooser)
{
    HildonFileChooserDialogPrivate *priv =
        HILDON_FILE_CHOOSER_DIALOG(chooser)->priv;
    return g_slist_copy(priv->filters);
}

static gboolean
hildon_file_chooser_dialog_add_remove_shortcut_folder(GtkFileChooser *
                                                      chooser,
                                                      const GtkFilePath *
                                                      path,
                                                      GError ** error)
{
    ULOG_ERR("HildonFileChooserDialog doesn\'t implement shortcuts");
    return FALSE;
}

static GSList
    *hildon_file_chooser_dialog_list_shortcut_folders(GtkFileChooser *
                                                      chooser)
{
    ULOG_ERR("HildonFileChooserDialog doesn\'t implement shortcuts");
    return NULL;
}

static void update_folder_button_visibility(HildonFileChooserDialogPrivate
                                            * priv)
{
    if (priv->should_show_folder_button
        && (priv->action == GTK_FILE_CHOOSER_ACTION_SAVE
            || priv->action == GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER))
        gtk_widget_show(priv->folder_button);
    else
        gtk_widget_hide(priv->folder_button);
}

static void update_location_visibility(HildonFileChooserDialogPrivate *
                                       priv)
{
    if (priv->should_show_location
        && (priv->action == GTK_FILE_CHOOSER_ACTION_SAVE
            || priv->action == GTK_FILE_CHOOSER_ACTION_CREATE_FOLDER)) {
        hildon_file_chooser_dialog_update_location_info(priv);
        gtk_widget_show_all(priv->caption_control_location);
    } else
        gtk_widget_hide(priv->caption_control_location);
}

/* We build ui for current action */
static void build_ui(HildonFileChooserDialog * self)
{
    HildonFileChooserDialogPrivate *priv = self->priv;

    switch (priv->action) {
    case GTK_FILE_CHOOSER_ACTION_OPEN:
        gtk_widget_hide(priv->caption_control_name);
        gtk_widget_hide(priv->hbox_items);
        gtk_widget_set_size_request(GTK_WIDGET(priv->filetree),
                                    FILE_SELECTION_WIDTH_TOTAL,
                                    FILE_SELECTION_HEIGHT);
        gtk_widget_show(GTK_WIDGET(priv->filetree));
        hildon_file_selection_show_content_pane(priv->filetree);
        gtk_window_set_title(GTK_WINDOW(self), _("ckdg_ti_open_file"));
        gtk_button_set_label(GTK_BUTTON(priv->action_button),
                             _("ckdg_bd_select_object_ok_open"));
        gtk_button_set_label(GTK_BUTTON(priv->cancel_button),
                             _("ckdg_bd_select_object_cancel"));
        break;
    case GTK_FILE_CHOOSER_ACTION_SAVE:
        if (hildon_file_chooser_dialog_save_multiple_set(priv))
        {
          gtk_widget_hide(priv->caption_control_name);
          gtk_widget_show_all(priv->hbox_items);
	  hildon_caption_set_label(HILDON_CAPTION(priv->caption_control_name),
				   _("sfil_fi_save_objects_items"));
        }
        else
        {
	  hildon_caption_set_label(HILDON_CAPTION(priv->caption_control_name),
				   _("ckdg_fi_save_object_name"));
          gtk_widget_show_all(priv->caption_control_name);
          gtk_widget_hide(priv->hbox_items);
	  hildon_caption_set_label(HILDON_CAPTION(priv->caption_control_name),
				   _("ckdg_fi_save_object_name"));
        }
       
	gtk_label_set_text(GTK_LABEL(priv->label_location), 
				     _("sfil_fi_save_objects_location"));
        gtk_widget_hide(GTK_WIDGET(priv->filetree));

        /* Content pane of the filetree widget needs to be realized.
           Otherwise automatic location change etc don't work correctly.
           This is because "rows-changed" handler in GtkTreeView exits
           immediately if treeview is not realized. */
	_hildon_file_selection_realize_help(priv->filetree);
        gtk_window_set_title(GTK_WINDOW(self), _("sfil_ti_save_file"));
        gtk_button_set_label(GTK_BUTTON(priv->action_button),
                             _("ckdg_bd_save_object_dialog_ok"));
        gtk_button_set_label(GTK_BUTTON(priv->folder_button),
                             _("sfil_bd_save_object_dialog_change_folder"));
        gtk_button_set_label(GTK_BUTTON(priv->cancel_button),
                             _("ckdg_bd_save_object_dialog_cancel"));
        gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(self),
                                          _("ckdg_va_save_object_name_stub_default"));
        break;
    case GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER:
        gtk_widget_hide(priv->caption_control_name);
        gtk_widget_hide(priv->hbox_items);
        gtk_widget_set_size_request(GTK_WIDGET(priv->filetree),
                                    FILE_SELECTION_WIDTH_LIST,
                                    FILE_SELECTION_HEIGHT);
        gtk_widget_show(GTK_WIDGET(priv->filetree));
        hildon_file_selection_hide_content_pane(priv->filetree);
        gtk_window_set_title(GTK_WINDOW(self), _("ckdg_ti_change_folder"));
        gtk_button_set_label(GTK_BUTTON(priv->action_button),
                             _("ckdg_bd_change_folder_ok"));
        gtk_button_set_label(GTK_BUTTON(priv->folder_button),
                             _("ckdg_bd_change_folder_new_folder"));
        gtk_button_set_label(GTK_BUTTON(priv->cancel_button),
                             _("ckdg_bd_change_folder_cancel"));
        break;
    case GTK_FILE_CHOOSER_ACTION_CREATE_FOLDER:
        hildon_caption_set_label(HILDON_CAPTION(priv->caption_control_name),
				 _("ckdg_fi_new_folder_name"));
	gtk_label_set_text(GTK_LABEL(priv->label_location), 
				     _("ckdg_fi_new_folder_location"));
        gtk_widget_show_all(priv->caption_control_name);
        gtk_widget_hide(GTK_WIDGET(priv->filetree));
	_hildon_file_selection_realize_help(priv->filetree);        
        gtk_widget_hide(priv->hbox_items);
        gtk_window_set_title(GTK_WINDOW(self), _("ckdg_ti_new_folder"));
        gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(self),
                                          _("ckdg_va_new_folder_name_stub")); 
        gtk_button_set_label(GTK_BUTTON(priv->action_button),
                             _("ckdg_bd_new_folder_dialog_ok"));
        gtk_button_set_label(GTK_BUTTON(priv->cancel_button),
                             _("ckdg_bd_new_folder_dialog_cancel"));
        break;
    default:
        g_assert_not_reached();
    };

    update_folder_button_visibility(priv);
    update_location_visibility(priv);
}

/*
  Small help utilities for response_handler
*/
static GString *check_illegal_characters(const gchar * name)
{
    GString *illegals = NULL;

    /* Handle reserved names as special cases */
    if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
      return g_string_new(name);

    while (*name) {
        gunichar unichar;

        unichar = g_utf8_get_char(name);

        if (unichar == '\\' || unichar == '/' || unichar == ':' || unichar == '*' || unichar == '?' ||
            unichar == '\"' || unichar == '<' || unichar == '>' || unichar == '|' ) {
            if (!illegals)
                illegals = g_string_new(NULL);
            if (g_utf8_strchr(illegals->str, -1, unichar) == NULL)
                g_string_append_unichar(illegals, unichar);
        }

        name = g_utf8_next_char(name);
    }

    return illegals;
}

/* Sets current directory of the target to be the same as current
   directory of source */
static void sync_current_folders(HildonFileChooserDialog * source,
                                 HildonFileChooserDialog * target)
{
    HildonFileSelection *fs;
    gchar *uri;

    fs = HILDON_FILE_SELECTION(source->priv->filetree);

    if (hildon_file_selection_get_active_pane(fs) == 
        HILDON_FILE_SELECTION_PANE_CONTENT)
      uri = gtk_file_chooser_get_uri(GTK_FILE_CHOOSER(source));
    else
      uri = NULL;

    if (!uri)
        uri =
            gtk_file_chooser_get_current_folder_uri(GTK_FILE_CHOOSER
                                                    (source));

    gtk_file_chooser_set_current_folder_uri(GTK_FILE_CHOOSER(target), uri);
    g_free(uri);
}

/* Used to pop dialogs when folder button is pressed */
static GtkWidget
    *hildon_file_chooser_dialog_create_sub_dialog(HildonFileChooserDialog *
                                                  self,
                                                  GtkFileChooserAction
                                                  action)
{
    GtkWidget *dialog;

    dialog =
        hildon_file_chooser_dialog_new_with_properties(GTK_WINDOW(self),
                                                       "action", action,
                                                       "file-system-model",
                                                       self->priv->model,
                                                       NULL);
    sync_current_folders(self, HILDON_FILE_CHOOSER_DIALOG(dialog));

    return dialog;
}

static void handle_folder_popup(HildonFileChooserDialog *self)
{
  GtkFileSystem *backend;
  GtkWidget *dialog;

  g_return_if_fail(HILDON_IS_FILE_CHOOSER_DIALOG(self));

  /* Prevent race condition that can cause multiple
      subdialogs to be popped up (in a case mainloop
      is run before subdialog blocks additional clicks. */
  if (self->priv->popup_protect)
  {
    ULOG_INFO("Blocked multiple subdialogs");
    return;
  }

  self->priv->popup_protect = TRUE;

  if (self->priv->edited)
  {
    self->priv->edited = FALSE;
    hildon_file_chooser_dialog_set_current_name(
      GTK_FILE_CHOOSER(self),
      gtk_entry_get_text(GTK_ENTRY(self->priv->entry_name)));
  }

  backend = _hildon_file_system_model_get_file_system(self->priv->model);

  if (self->priv->action == GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER) {
    dialog = hildon_file_chooser_dialog_create_sub_dialog
      (self, GTK_FILE_CHOOSER_ACTION_CREATE_FOLDER);

    while (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
      GtkFilePath *file_path;
      gboolean success;
      GError *error = NULL;
      const gchar *message;
      gchar *uri;

      uri = gtk_file_chooser_get_uri(GTK_FILE_CHOOSER(dialog));
      ULOG_INFO("About to create folder %s", uri);

      file_path = gtk_file_system_uri_to_path(backend, uri);
      success =
        gtk_file_system_create_folder(backend, file_path, &error);
      gtk_file_path_free(file_path);

      if (success) 
      {
        (void) gtk_file_chooser_set_current_folder_uri(GTK_FILE_CHOOSER(self), uri);
        g_free(uri);
        break;
      }

      g_free(uri);
      g_assert(error != NULL);
      
      /* GtkFileSystemModelGnomeVFS returns ERROR_FAILED and GtkFileSystemUnix returns
          ERROR_NONEXISTENT (!!!) if we try to create a folder that already exists. We report
          other errors normally.
          GnomeVFS has been changed to return ALREADY EXISTS accordingly. 
          We now support this only. */
      if (g_error_matches(error, GTK_FILE_SYSTEM_ERROR,  
              GTK_FILE_SYSTEM_ERROR_ALREADY_EXISTS))
        message = _("ckdg_ib_folder_already_exists");
      else
        message = _("sfil_ni_operation_failed");

      ULOG_ERR(error->message);
      gtk_infoprint(GTK_WINDOW(dialog), message);
      g_error_free(error);

      hildon_file_chooser_dialog_select_text(self->priv);
    }
  } else {
    dialog =
        hildon_file_chooser_dialog_create_sub_dialog
            (self, GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK)
        sync_current_folders(HILDON_FILE_CHOOSER_DIALOG(dialog),
                             self);
  }

  gtk_banner_close(GTK_WINDOW(dialog));
  gtk_widget_destroy(dialog);
  gtk_window_present(GTK_WINDOW(self));
  self->priv->popup_protect = FALSE;
}

static gboolean hildon_file_chooser_dialog_location_pressed(GtkWidget 
  *widget, GdkEventButton *event, gpointer data)
{
  HildonFileChooserDialog *self = HILDON_FILE_CHOOSER_DIALOG(data);

  if (self->priv->action == GTK_FILE_CHOOSER_ACTION_SAVE)
    handle_folder_popup(self);

  return FALSE;
}

static void response_handler(GtkWidget * widget, gint arg1, gpointer data)
{
    HildonFileChooserDialog *self;
    HildonFileChooserDialogPrivate *priv;
    GtkWindow *window;

    self = HILDON_FILE_CHOOSER_DIALOG(widget);
    priv = self->priv;
    window = GTK_WINDOW(widget);

    switch (arg1) {
    case GTK_RESPONSE_OK:
        if (GTK_WIDGET_VISIBLE(priv->caption_control_name)) {
            gchar *entry_text = g_strdup(
                gtk_entry_get_text(GTK_ENTRY(priv->entry_name)));

            g_strstrip(entry_text);

            if (entry_text[0] == '\0') {
                /* We don't accept empty field */
                g_signal_stop_emission_by_name(widget, "response");
                priv->edited = FALSE;
                hildon_file_chooser_dialog_do_autonaming(priv);
                hildon_file_chooser_dialog_select_text(priv);
                gtk_infoprint(window, _("ckdg_ib_enter_name"));
            } else {
                GString *illegals = check_illegal_characters(entry_text);

                if (illegals) {
                    gchar *msg;

                    hildon_file_chooser_dialog_select_text(priv);
                    g_signal_stop_emission_by_name(widget, "response");

                    msg = g_strdup_printf(_("ckdg_ib_illegal_characters_entered"),
                                          illegals->str);
                    gtk_infoprint(window, msg);
                    g_free(msg);
                    g_string_free(illegals, TRUE);
                }
                else if (self->priv->max_full_path_length >= 0)
                     {  /* Let's check that filename is not too long. 
                           This normally should not happen, because we 
                           have limit in entry. However, very long paths 
                           could cause max input length to be less than 1. */
                    gchar *uri;
                    gint path_length;

                    uri = gtk_file_chooser_get_uri(GTK_FILE_CHOOSER(widget));
                    path_length = get_path_length_from_uri(uri);

                    if (path_length > self->priv->max_full_path_length)
                    {
                        g_signal_stop_emission_by_name(widget, "response");
                        hildon_file_chooser_dialog_select_text(priv);
                        gtk_infoprintf(window, _("file_ib_name_too_long"), 
                            path_length - self->priv->max_filename_length);
                    }
                    g_free(uri);
                }
            }
            g_free(entry_text);
        }
        break;
    case HILDON_RESPONSE_FOLDER_BUTTON:
      g_signal_stop_emission_by_name(widget, "response");
      handle_folder_popup(self);
      break;
    default:
        break;
    }
}

static void hildon_file_chooser_dialog_set_property(GObject * object,
                                                    guint prop_id,
                                                    const GValue * value,
                                                    GParamSpec * pspec)
{
    HildonFileChooserDialog *self;
    HildonFileChooserDialogPrivate *priv;

    self = HILDON_FILE_CHOOSER_DIALOG(object);
    priv = self->priv;

    switch (prop_id) {
    case GTK_FILE_CHOOSER_PROP_ACTION:
        priv->action = g_value_get_enum(value);
        build_ui(self);
        break;
    case GTK_FILE_CHOOSER_PROP_SELECT_MULTIPLE:
        g_assert(HILDON_IS_FILE_SELECTION(priv->filetree));
        hildon_file_selection_set_select_multiple(priv->filetree,
                                                  g_value_get_boolean
                                                  (value));
        break;
    case GTK_FILE_CHOOSER_PROP_SHOW_HIDDEN: /* Same handler */
    case GTK_FILE_CHOOSER_PROP_LOCAL_ONLY:
        g_assert(HILDON_IS_FILE_SELECTION(priv->filetree));
        g_object_set_property(G_OBJECT(priv->filetree), pspec->name,
                              value);
        break;
    case GTK_FILE_CHOOSER_PROP_FILE_SYSTEM_BACKEND:
    {
        const gchar *s = g_value_get_string(value);

        /* We should come here once if at all */
        g_assert(priv->model == NULL || s == NULL);

        if (!priv->model)
            priv->model =
                g_object_new(HILDON_TYPE_FILE_SYSTEM_MODEL, "backend", s,
                             "ref-widget", object, NULL);
        break;
    }
    case GTK_FILE_CHOOSER_PROP_FILTER:
        g_assert(HILDON_IS_FILE_SELECTION(priv->filetree));
        hildon_file_selection_set_filter(priv->filetree,
                                         g_value_get_object(value));
        break;
    case PROP_EMPTY_TEXT:
        g_assert(HILDON_IS_FILE_SELECTION(priv->filetree));
        g_object_set_property(G_OBJECT(priv->filetree), pspec->name,
                              value);
        break;
    case PROP_FILE_SYSTEM_MODEL:
        g_assert(priv->model == NULL || g_value_get_object(value) == NULL);
        if (!priv->model) {
            if ((priv->model = g_value_get_object(value)) != NULL)
                g_object_ref(priv->model);
        }
        break;
    case PROP_FOLDER_BUTTON:
        priv->should_show_folder_button = g_value_get_boolean(value);
        update_folder_button_visibility(priv);
        break;
    case PROP_LOCATION:
        priv->should_show_location = g_value_get_boolean(value);
        update_location_visibility(priv);
        break;
    case PROP_AUTONAMING:
        priv->autonaming_enabled = g_value_get_boolean(value);
        break;
    case PROP_OPEN_BUTTON_TEXT:
        g_object_set_property(G_OBJECT(priv->action_button),
                            "label", value);
        break;
    case PROP_MULTIPLE_TEXT:
        g_object_set_property(G_OBJECT(priv->multiple_label),
                            "label", value);
        build_ui(HILDON_FILE_CHOOSER_DIALOG(object));
        break;
    case PROP_MAX_NAME_LENGTH:
    {
        gint new_value = g_value_get_int(value);
        if (new_value != priv->max_filename_length)
        {
          ULOG_INFO("Maximum name length is %d characters", 
            new_value);
          priv->max_filename_length = new_value;
          hildon_file_chooser_dialog_set_limit(self);
        }    
        break;
    }
    case PROP_MAX_FULL_PATH_LENGTH:
    {
      gint new_value;
      const char *filename_len;

      new_value = g_value_get_int(value);

      if (new_value == 0)
      {      
        /* Figuring out maximum allowed path length */
        filename_len = g_getenv("MAX_FILENAME_LENGTH");
        if (filename_len)
          new_value = atoi(filename_len);
        if (new_value <= 0)
          new_value = MAX_FILENAME_LENGTH_DEFAULT;
      }

      if (new_value != priv->max_full_path_length)
      {
        ULOG_INFO("Maximum full path length is %d characters", 
            new_value);
        priv->max_full_path_length = new_value;
        hildon_file_chooser_dialog_set_limit(self);
      }

      break;
    }
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static void hildon_file_chooser_dialog_get_property(GObject * object,
                                                    guint prop_id,
                                                    GValue * value,
                                                    GParamSpec * pspec)
{
    HildonFileChooserDialogPrivate *priv =
        HILDON_FILE_CHOOSER_DIALOG(object)->priv;

    switch (prop_id) {
    case GTK_FILE_CHOOSER_PROP_ACTION:
        g_value_set_enum(value, priv->action);
        break;
    case GTK_FILE_CHOOSER_PROP_SELECT_MULTIPLE:
        g_value_set_boolean(value,
                            hildon_file_selection_get_select_multiple
                            (priv->filetree));
        break;
    case GTK_FILE_CHOOSER_PROP_SHOW_HIDDEN: /* Same handler */
    case GTK_FILE_CHOOSER_PROP_LOCAL_ONLY:
        g_object_get_property(G_OBJECT(priv->filetree), pspec->name,
                              value);
        break;
    case GTK_FILE_CHOOSER_PROP_FILTER:
        g_value_set_object(value,
                           hildon_file_selection_get_filter(priv->
                                                            filetree));
        break;
    case PROP_EMPTY_TEXT:
        g_object_get_property(G_OBJECT(priv->filetree), pspec->name,
                              value);
        break;
    case PROP_FILE_SYSTEM_MODEL:
        g_value_set_object(value, priv->model);
        break;
    case PROP_FOLDER_BUTTON:
        g_value_set_boolean(value, priv->should_show_folder_button);
        break;
    case PROP_LOCATION:
        g_value_set_boolean(value, priv->should_show_location);
        break;
    case PROP_AUTONAMING:
        g_value_set_boolean(value, priv->autonaming_enabled);
        break;
    case PROP_OPEN_BUTTON_TEXT:
        g_object_get_property(G_OBJECT(priv->action_button), 
                            "label", value);
        break;
    case PROP_MULTIPLE_TEXT:
        g_object_get_property(G_OBJECT(priv->multiple_label),
                            "label", value);
        break;
    case PROP_MAX_NAME_LENGTH:
        g_value_set_int(value, priv->max_filename_length);
        break;
    case PROP_MAX_FULL_PATH_LENGTH:
        g_value_set_int(value, priv->max_full_path_length);
        break;
    default:   /* Backend is not readable */
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    };
}

static void hildon_file_chooser_dialog_iface_init(GtkFileChooserIface *
                                                  iface)
{
    iface->set_current_folder =
        hildon_file_chooser_dialog_set_current_folder;
    iface->get_current_folder =
        hildon_file_chooser_dialog_get_current_folder;
    iface->set_current_name = hildon_file_chooser_dialog_set_current_name;
    iface->select_path = hildon_file_chooser_dialog_select_path;
    iface->unselect_path = hildon_file_chooser_dialog_unselect_path;
    iface->select_all = hildon_file_chooser_dialog_select_all;
    iface->unselect_all = hildon_file_chooser_dialog_unselect_all;
    iface->get_paths = hildon_file_chooser_dialog_get_paths;
    iface->get_preview_path = hildon_file_chooser_dialog_get_preview_path;
    iface->get_file_system = hildon_file_chooser_dialog_get_file_system;
    iface->add_filter = hildon_file_chooser_dialog_add_filter;
    iface->remove_filter = hildon_file_chooser_dialog_remove_filter;
    iface->list_filters = hildon_file_chooser_dialog_list_filters;
    iface->add_shortcut_folder =
        hildon_file_chooser_dialog_add_remove_shortcut_folder;
    iface->remove_shortcut_folder =
        hildon_file_chooser_dialog_add_remove_shortcut_folder;
    iface->list_shortcut_folders =
        hildon_file_chooser_dialog_list_shortcut_folders;
}

static void hildon_file_chooser_dialog_show(GtkWidget * widget)
{
    HildonFileChooserDialogPrivate *priv;

    g_return_if_fail(HILDON_IS_FILE_CHOOSER_DIALOG(widget));

    GTK_WIDGET_CLASS(hildon_file_chooser_dialog_parent_class)->
        show(widget);

    priv = HILDON_FILE_CHOOSER_DIALOG(widget)->priv;
    hildon_file_chooser_dialog_select_text(priv);
    hildon_file_chooser_dialog_selection_changed(priv); 
}

static void hildon_file_chooser_dialog_destroy(GtkObject * obj)
{
    HildonFileChooserDialog *dialog;
 
    ULOG_DEBUG(__FUNCTION__);

    dialog = HILDON_FILE_CHOOSER_DIALOG(obj);

    /* We need sometimes to break cyclic references */

    if (dialog->priv->model) {
        g_signal_handlers_disconnect_by_func
        (dialog->priv->model,
         (gpointer) hildon_file_chooser_dialog_finished_loading,
         dialog);

        g_object_unref(dialog->priv->model);
        dialog->priv->model = NULL;
    }

    GTK_OBJECT_CLASS(hildon_file_chooser_dialog_parent_class)->
        destroy(obj);
}

static void hildon_file_chooser_dialog_finalize(GObject * obj)
{
    HildonFileChooserDialogPrivate *priv =
        HILDON_FILE_CHOOSER_DIALOG(obj)->priv;

    g_free(priv->stub_name);
    g_free(priv->ext_name);

    g_slist_foreach(priv->filters, (GFunc) g_object_unref, NULL);
    g_slist_free(priv->filters);
    g_object_unref(priv->popup);

    ULOG_DEBUG(__FUNCTION__);

    G_OBJECT_CLASS(hildon_file_chooser_dialog_parent_class)->finalize(obj);
}

static void
hildon_file_chooser_dialog_class_init(HildonFileChooserDialogClass * klass)
{
    GParamSpec *pspec;
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

    g_type_class_add_private(klass,
                             sizeof(HildonFileChooserDialogPrivate));

    gobject_class->set_property = hildon_file_chooser_dialog_set_property;
    gobject_class->get_property = hildon_file_chooser_dialog_get_property;
    gobject_class->constructor = hildon_file_chooser_dialog_constructor;
    gobject_class->finalize = hildon_file_chooser_dialog_finalize;
    GTK_WIDGET_CLASS(klass)->show = hildon_file_chooser_dialog_show;
    GTK_WIDGET_CLASS(klass)->show_all = hildon_file_chooser_dialog_show;
    GTK_OBJECT_CLASS(klass)->destroy = hildon_file_chooser_dialog_destroy;

    pspec = g_param_spec_string("empty-text", "Empty text",
                                "String to use when selected "
                                "folder is empty",
                                NULL, G_PARAM_READWRITE);
    g_object_class_install_property(gobject_class, PROP_EMPTY_TEXT, pspec);

    g_object_class_install_property(gobject_class, PROP_FILE_SYSTEM_MODEL,
                                    g_param_spec_object
                                    ("file-system-model",
                                     "File system model",
                                     "Tell the file chooser to use "
                                     "existing model instead of creating "
                                     "a new one",
                                     HILDON_TYPE_FILE_SYSTEM_MODEL,
                                     G_PARAM_READWRITE |
                                     G_PARAM_CONSTRUCT_ONLY));

    pspec =
        g_param_spec_boolean("show-folder-button", "Show folder button",
                             "Whether the folder button should be visible "
                             "(if it's possible)",
                             TRUE, G_PARAM_READWRITE);
    g_object_class_install_property(gobject_class, PROP_FOLDER_BUTTON,
                                    pspec);

    pspec = g_param_spec_boolean("show-location", "Show location",
                                 "Whether the location information should "
                                 "be visible (if it's possible)",
                                 TRUE, G_PARAM_READWRITE);
    g_object_class_install_property(gobject_class, PROP_LOCATION, pspec);

    pspec = g_param_spec_boolean("autonaming", "Autonaming",
                                 "Whether the text set to name entry "
                                 "should be automatically appended by a "
                                 "counter when the given name already "
                                 "exists",
                                 TRUE, G_PARAM_READWRITE);
    g_object_class_install_property(gobject_class, PROP_AUTONAMING, pspec);

    g_object_class_install_property(gobject_class, PROP_OPEN_BUTTON_TEXT, 
            g_param_spec_string("open-button-text", "Open button text",
                                "String to use in leftmost (=open) button",
                                NULL, G_PARAM_READWRITE));

    g_object_class_install_property(gobject_class, PROP_MULTIPLE_TEXT, 
            g_param_spec_string("save-multiple", "Save multiple files",
                                "Text to be displayed in items field when saving multiple files",
                                NULL, G_PARAM_READWRITE));

    g_object_class_install_property(gobject_class, PROP_MAX_NAME_LENGTH,
            g_param_spec_int("max-name-length", "Maximum name length",
                             "Maximum length of an individual file/folder "
                             "name when entered by user. Note that the actual "
                             "limit can be smaller, if the maximum full "
                             "path length kicks in. Use -1 for no limit.", 
                             -1, G_MAXINT, -1, G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

    g_object_class_install_property(gobject_class, PROP_MAX_FULL_PATH_LENGTH,
            g_param_spec_int("max-full-path-length", "Maximum full path length",
                             "Maximum length of the whole path of an individual "
                             "file/folder name when entered by user. "
                             "Use -1 for no limit or 0 to look the value "
                             "from MAX_FILENAME_LENGTH environment variable", 
                             -1, G_MAXINT, 0, G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

    hildon_gtk_file_chooser_install_properties(gobject_class);
}

static void hildon_file_chooser_dialog_sort_changed(GtkWidget * item,
                                                    gpointer data)
{
    if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(item))) {
        HildonFileSelectionSortKey key;
        HildonFileChooserDialogPrivate *priv = data;

        if (item == priv->sort_type)
            key = HILDON_FILE_SELECTION_SORT_TYPE;
        else if (item == priv->sort_name)
            key = HILDON_FILE_SELECTION_SORT_NAME;
        else if (item == priv->sort_date)
            key = HILDON_FILE_SELECTION_SORT_MODIFIED;
        else
            key = HILDON_FILE_SELECTION_SORT_SIZE;

        hildon_file_selection_set_sort_key(priv->filetree, key,
                                           GTK_SORT_ASCENDING);
    }
}

static void hildon_file_chooser_dialog_mode_changed(GtkWidget * item,
                                                    gpointer data)
{
    if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(item))) {
        HildonFileSelectionMode mode;
        HildonFileChooserDialogPrivate *priv = data;

        mode =
            (item ==
             priv->
             mode_list ? HILDON_FILE_SELECTION_MODE_LIST :
             HILDON_FILE_SELECTION_MODE_THUMBNAILS);
        hildon_file_selection_set_mode(priv->filetree, mode);
    }
}

static void hildon_file_chooser_dialog_context(GtkWidget * widget,
                                               gpointer data)
{
    HildonFileChooserDialogPrivate *priv = data;
    guint32 time;

    /* We are not handling an event currently, so gtk_get_current_event_*
       don't do any good */
    time = gdk_x11_get_server_time(widget->window);
    gtk_menu_shell_select_first(GTK_MENU_SHELL(priv->popup), TRUE);
    gtk_menu_popup(GTK_MENU(priv->popup), NULL, NULL, NULL, NULL, 0, time);
}

static void
hildon_file_chooser_entry_changed( GtkWidget * widget,
				    gpointer data)
{
    HildonFileChooserDialogPrivate *priv;

    g_return_if_fail( GTK_IS_WIDGET(widget));
    g_return_if_fail(HILDON_IS_FILE_CHOOSER_DIALOG(data));

    priv = HILDON_FILE_CHOOSER_DIALOG( data )->priv;
    priv->edited = TRUE;
}

static void hildon_file_chooser_dialog_init(HildonFileChooserDialog * self)
{
    GtkMenuShell *shell;
    GtkRadioMenuItem *item;
    HildonFileChooserDialogPrivate *priv;
    GtkBox *box;
    GtkWidget *eventbox, *label_items;
    GtkSizeGroup *size_group;

    ULOG_INFO("Initializing");

    self->priv = priv =
        G_TYPE_INSTANCE_GET_PRIVATE(self, HILDON_TYPE_FILE_CHOOSER_DIALOG,
                                    HildonFileChooserDialogPrivate);
    size_group = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);
    priv->filters = NULL;
    priv->autonaming_enabled = TRUE;
    priv->should_show_folder_button = TRUE;
    priv->should_show_location = TRUE;
    priv->stub_name = priv->ext_name = NULL;
    priv->action = GTK_FILE_CHOOSER_ACTION_OPEN;
    priv->action_button =
        gtk_dialog_add_button(GTK_DIALOG(self),
                              _("ckdg_bd_select_object_ok_open"),
                              GTK_RESPONSE_OK);
    priv->folder_button =
        gtk_dialog_add_button(GTK_DIALOG(self),
                              _("ckdg_bd_change_folder_new_folder"),
                              HILDON_RESPONSE_FOLDER_BUTTON);
    priv->cancel_button =
        gtk_dialog_add_button(GTK_DIALOG(self),
                              _("ckdg_bd_select_object_cancel"),
                              GTK_RESPONSE_CANCEL);
    priv->entry_name = gtk_entry_new();
    
    priv->caption_control_name =
        hildon_caption_new(size_group, _("ckdg_ti_select_file"),
                           priv->entry_name, NULL,
                           HILDON_CAPTION_OPTIONAL);
    hildon_caption_set_separator(HILDON_CAPTION(priv->caption_control_name),
				 "");

    priv->changed_handler = 
          g_signal_connect( priv->entry_name, "changed",
		          G_CALLBACK( hildon_file_chooser_entry_changed ),
		          self );
    g_signal_connect(priv->entry_name, "insert-text",
		          G_CALLBACK( hildon_file_chooser_dialog_check_limit ),
              self );

    priv->hbox_location = gtk_hbox_new(FALSE, HILDON_MARGIN_DEFAULT);
    priv->hbox_items = gtk_hbox_new(FALSE, HILDON_MARGIN_DEFAULT);
    priv->image_location = gtk_image_new();
    priv->title_location = gtk_label_new(NULL);
    gtk_misc_set_alignment(GTK_MISC(priv->title_location), 0.0f, 0.5f);
    gtk_box_pack_start(GTK_BOX(priv->hbox_location), priv->image_location,
                       FALSE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(priv->hbox_location), priv->title_location,
                       TRUE, TRUE, 0);
    eventbox = gtk_event_box_new();
    gtk_event_box_set_visible_window(GTK_EVENT_BOX(eventbox), FALSE);
    gtk_container_add(GTK_CONTAINER(eventbox), priv->hbox_location);
    gtk_widget_add_events(eventbox, GDK_BUTTON_PRESS_MASK);

    priv->caption_control_location = gtk_hbox_new(FALSE, HILDON_MARGIN_DOUBLE);
    priv->label_location = g_object_new(GTK_TYPE_LABEL, "label", _("sfil_fi_save_objects_location"), "xalign", 1.0f, NULL);
    label_items = g_object_new(GTK_TYPE_LABEL, "label",  _("sfil_fi_save_objects_items"), "xalign", 1.0f, NULL);
    priv->multiple_label = gtk_label_new(NULL);
    gtk_box_pack_start(GTK_BOX(priv->hbox_items), label_items, FALSE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(priv->hbox_items), priv->multiple_label, FALSE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(priv->caption_control_location), 
		       priv->label_location, FALSE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(priv->caption_control_location), eventbox, TRUE, TRUE, 0);
    gtk_size_group_add_widget(size_group, priv->label_location);
    gtk_size_group_add_widget(size_group, label_items);
    g_object_unref(size_group);

    priv->popup = gtk_menu_new();
    shell = GTK_MENU_SHELL(priv->popup);
    g_object_ref(priv->popup);
    gtk_object_sink(GTK_OBJECT(priv->popup));

    priv->sort_type =
        gtk_radio_menu_item_new_with_label(NULL, _("sfil_me_sort_type"));
    item = GTK_RADIO_MENU_ITEM(priv->sort_type);
    priv->sort_name =
        gtk_radio_menu_item_new_with_label_from_widget(item,
                                                       _("sfil_me_sort_name"));
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(priv->sort_name),
                                   TRUE);
    priv->sort_date =
        gtk_radio_menu_item_new_with_label_from_widget(item,
                                                       _("sfil_me_sort_date"));
    priv->sort_size =
        gtk_radio_menu_item_new_with_label_from_widget(item,
                                                       _("sfil_me_sort_size"));

    priv->mode_list = gtk_radio_menu_item_new_with_label(NULL,
                                                         _("sfil_me_view_list"));
    priv->mode_thumbnails =
        gtk_radio_menu_item_new_with_label_from_widget(GTK_RADIO_MENU_ITEM
                                                       (priv->mode_list),
                                                       _("sfil_me_view_thumbnails"));
    gtk_check_menu_item_set_active(
        GTK_CHECK_MENU_ITEM(priv->mode_thumbnails), TRUE);

    gtk_menu_shell_append(shell, priv->sort_type);
    gtk_menu_shell_append(shell, priv->sort_name);
    gtk_menu_shell_append(shell, priv->sort_date);
    gtk_menu_shell_append(shell, priv->sort_size);
    gtk_menu_shell_append(shell, gtk_separator_menu_item_new());
    gtk_menu_shell_append(shell, priv->mode_list);
    gtk_menu_shell_append(shell, priv->mode_thumbnails);
    gtk_widget_show_all(priv->popup);

    box = GTK_BOX(GTK_DIALOG(self)->vbox);
    gtk_box_pack_start(box, priv->caption_control_name, FALSE, TRUE, 0);
    gtk_box_pack_start(box, priv->hbox_items, FALSE, TRUE, 0);
    gtk_box_pack_start(box, priv->caption_control_location, FALSE, TRUE,
                       0);

    g_signal_connect(self, "response", G_CALLBACK(response_handler), NULL);
    g_signal_connect(priv->mode_list, "toggled",
                     G_CALLBACK(hildon_file_chooser_dialog_mode_changed),
                     priv);
    g_signal_connect(priv->mode_thumbnails, "toggled",
                     G_CALLBACK(hildon_file_chooser_dialog_mode_changed),
                     priv);
    g_signal_connect(priv->sort_type, "toggled",
                     G_CALLBACK(hildon_file_chooser_dialog_sort_changed),
                     priv);
    g_signal_connect(priv->sort_name, "toggled",
                     G_CALLBACK(hildon_file_chooser_dialog_sort_changed),
                     priv);
    g_signal_connect(priv->sort_date, "toggled",
                     G_CALLBACK(hildon_file_chooser_dialog_sort_changed),
                     priv);
    g_signal_connect(priv->sort_size, "toggled",
                     G_CALLBACK(hildon_file_chooser_dialog_sort_changed),
                     priv);
    g_signal_connect(eventbox, "button-release-event",
                     G_CALLBACK(hildon_file_chooser_dialog_location_pressed), 
                     self); 
    g_signal_connect(priv->action_button, "insensitive-press", 
                     G_CALLBACK(insensitive_button), self);

    gtk_dialog_set_default_response(GTK_DIALOG(self), GTK_RESPONSE_OK);
}

static GObject *
hildon_file_chooser_dialog_constructor(GType type,
                                       guint n_construct_properties,
                                       GObjectConstructParam *
                                       construct_properties)
{
    GObject *obj;
    HildonFileChooserDialogPrivate *priv;

    obj =
        G_OBJECT_CLASS(hildon_file_chooser_dialog_parent_class)->
        constructor(type, n_construct_properties, construct_properties);
    /* Now we know if specific backend is requested */
    priv = HILDON_FILE_CHOOSER_DIALOG(obj)->priv;

    g_assert(priv->model);
    priv->filetree = g_object_new(HILDON_TYPE_FILE_SELECTION, 
      "model", priv->model, "visible-columns", 
      HILDON_FILE_SELECTION_SHOW_NAME | HILDON_FILE_SELECTION_SHOW_MODIFIED,
      NULL);

    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(obj)->vbox),
                       GTK_WIDGET(priv->filetree), TRUE, TRUE, 0);
    g_signal_connect(priv->filetree, "content-pane-context-menu",
                     G_CALLBACK(hildon_file_chooser_dialog_context), priv);
    g_signal_connect_swapped(priv->filetree, "selection-changed",
                     G_CALLBACK
                     (hildon_file_chooser_dialog_selection_changed), priv);
    g_signal_connect_swapped(priv->filetree, "notify::active-pane",
                     G_CALLBACK
                     (hildon_file_chooser_dialog_selection_changed), priv);
    g_signal_connect(priv->filetree, "current-folder-changed",
                     G_CALLBACK
                     (hildon_file_chooser_dialog_current_folder_changed), priv);
    g_signal_connect(priv->filetree, "file-activated",
                     G_CALLBACK(file_activated_handler), obj);
    g_signal_connect_object(priv->model, "finished-loading",
                     G_CALLBACK(hildon_file_chooser_dialog_finished_loading),
                     obj, 0);

    gtk_dialog_set_has_separator(GTK_DIALOG(obj), FALSE);
    hildon_file_chooser_dialog_set_limit(HILDON_FILE_CHOOSER_DIALOG(obj));

    return obj;
}

/** Public API ****************************************************/

/**
 * hildon_file_chooser_dialog_new:
 * @parent: Transient parent window for dialog.
 * @action: Action to perform
 *          (open file/save file/select folder/new folder).
 *
 * Creates a new #HildonFileChooserDialog using the given action.
 *
 * Return value: a new #HildonFileChooserDialog.
 */
GtkWidget *hildon_file_chooser_dialog_new(GtkWindow * parent,
                                          GtkFileChooserAction action)
{
    GtkWidget *dialog;

    dialog =
        g_object_new(HILDON_TYPE_FILE_CHOOSER_DIALOG, "action", action,
                     NULL);

    if (parent)
        gtk_window_set_transient_for(GTK_WINDOW(dialog), parent);

    return dialog;
}

/**
 * hildon_file_chooser_dialog_new_with_properties:
 * @parent: Transient parent window for dialog.
 * @first_property: First option to pass to dialog.
 * @Varargs: arguments
 *
 * Creates new HildonFileChooserDialog. This constructor is handy if you
 * need to pass several options.
 *
 * Return value: New HildonFileChooserDialog object.
 */
GtkWidget *hildon_file_chooser_dialog_new_with_properties(GtkWindow *
                                                          parent,
                                                          const gchar *
                                                          first_property,
                                                          ...)
{
    GtkWidget *dialog;
    va_list args;

    va_start(args, first_property);
    dialog =
        GTK_WIDGET(g_object_new_valist
                   (HILDON_TYPE_FILE_CHOOSER_DIALOG, first_property,
                    args));
    va_end(args);

    if (parent)
        gtk_window_set_transient_for(GTK_WINDOW(dialog), parent);

    return dialog;
}

/**
 * hildon_file_chooser_dialog_set_safe_folder:
 * @self: a #HildonFileChooserDialog widget.
 * @local_path: a path to safe folder.
 *
 * Sets a safe folder that is used as a fallback in a case
 * that automatic location change fails.
 */
void hildon_file_chooser_dialog_set_safe_folder(
  HildonFileChooserDialog *self, const gchar *local_path)
{
  GtkFileSystem *fs;
  GtkFilePath *path;

  g_return_if_fail (HILDON_IS_FILE_CHOOSER_DIALOG(self));
  fs = _hildon_file_system_model_get_file_system(self->priv->model);

  path = gtk_file_system_filename_to_path (fs, local_path);
  g_object_set(self->priv->model, "safe-folder", path, NULL);
  gtk_file_path_free(path);
}

/**
 * hildon_file_chooser_dialog_set_safe_folder_uri:
 * @self: a #HildonFileChooserDialog widget.
 * @uri: an uri to safe folder.
 *
 * See #hildon_file_chooser_dialog_set_safe_folder.
 */
void hildon_file_chooser_dialog_set_safe_folder_uri(
  HildonFileChooserDialog *self, const gchar *uri)
{
  GtkFileSystem *fs;
  GtkFilePath *path;

  g_return_if_fail (HILDON_IS_FILE_CHOOSER_DIALOG(self));

  fs = _hildon_file_system_model_get_file_system(self->priv->model);
  path = gtk_file_system_uri_to_path (fs, uri);
  g_object_set(self->priv->model, "safe-folder", path, NULL);
  gtk_file_path_free(path);
}

/**
 * hildon_file_chooser_dialog_get_safe_folder:
 * @self: a #HildonFileChooserDialog widget.
 * 
 * Gets safe folder location as local path.
 *
 * Returns: a local path. Free this with #g_free.
 */
gchar *hildon_file_chooser_dialog_get_safe_folder(
  HildonFileChooserDialog *self)
{
  GtkFileSystem *fs;
  GtkFilePath *path;
  gchar *result;

  g_return_val_if_fail (HILDON_IS_FILE_CHOOSER_DIALOG(self), NULL);

  fs = _hildon_file_system_model_get_file_system(self->priv->model);
  g_object_get(self->priv->model, "safe-folder", &path, NULL);
  if (path == NULL)
    path = _hildon_file_system_path_for_location(fs, HILDON_FILE_SYSTEM_MODEL_LOCAL_DEVICE);

  result = gtk_file_system_path_to_filename(fs, path);
  gtk_file_path_free(path);
  
  return result;
}

/**
 * hildon_file_chooser_dialog_get_safe_folder_uri:
 * @self: a #HildonFileChooserDialog widget.
 * 
 * Gets safe folder location as uri.
 *
 * Returns: an uri. Free this with #g_free.
 */
gchar *hildon_file_chooser_dialog_get_safe_folder_uri(
  HildonFileChooserDialog *self)
{
  GtkFileSystem *fs;
  GtkFilePath *path;
  gchar *result;

  g_return_val_if_fail (HILDON_IS_FILE_CHOOSER_DIALOG(self), NULL);

  fs = _hildon_file_system_model_get_file_system(self->priv->model);
  g_object_get(self->priv->model, "safe-folder", &path, NULL);
  if (path == NULL)
    path = _hildon_file_system_path_for_location(fs, HILDON_FILE_SYSTEM_MODEL_LOCAL_DEVICE);

  result = gtk_file_system_path_to_uri(fs, path);
  gtk_file_path_free(path);
  
  return result;
}
