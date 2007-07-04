/*
 * This file is part of hildon-fm package
 *
 * Copyright (C) 2005-2007 Nokia Corporation.  All rights reserved.
 *
 * Contact: Marius Vollmer <marius.vollmer@nokia.com>
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

#include <config.h>

#include <string.h>
#include <stdlib.h>
#include <errno.h> 
#include <sys/statfs.h>

#include <gtk/gtk.h>
#include <libgnomevfs/gnome-vfs.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib-lowlevel.h>

#include <hildon/hildon.h>

#include "hildon-file-common-private.h"
#include "hildon-file-system-storage-dialog.h"

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), HILDON_TYPE_FILE_SYSTEM_STORAGE_DIALOG, HildonFileSystemStorageDialogPriv))

/*#define ENABLE_DEBUGGING*/

#define KILOBYTE_FACTOR (1024.0)
#define MEGABYTE_FACTOR (1024.0 * 1024.0)
#define GIGABYTE_FACTOR (1024.0 * 1024.0 * 1024.0)

typedef enum {
	URI_TYPE_FILE_SYSTEM,
	URI_TYPE_INTERNAL_MMC,
	URI_TYPE_EXTERNAL_MMC,
	URI_TYPE_UNKNOWN
} URIType;

struct _HildonFileSystemStorageDialogPriv {
        DBusPendingCall  *pending_call;
        guint             get_apps_id;
        GString          *apps_string;

        /* Stats */
        gchar            *uri_str;
        URIType           uri_type;

        guint             file_count;
        guint             folder_count;

        GnomeVFSFileSize  email_size;
        GnomeVFSFileSize  image_size;
        GnomeVFSFileSize  video_size;
        GnomeVFSFileSize  audio_size;
        GnomeVFSFileSize  html_size;
        GnomeVFSFileSize  doc_size;
        GnomeVFSFileSize  contact_size;
        GnomeVFSFileSize  installed_app_size;
        GnomeVFSFileSize  other_size;

        GnomeVFSFileSize  in_use_size;

	GnomeVFSMonitorHandle *monitor_handle;

        /* Containers */
        GtkWidget        *viewport_common;
        GtkWidget        *viewport_data;

        /* Common widgets */
        GtkWidget        *label_name;
        GtkWidget        *image_type;
        GtkWidget        *label_type;
        GtkWidget        *label_total_size;
        GtkWidget        *label_in_use;
        GtkWidget        *label_available;

        GtkWidget        *label_read_only_stub;
        GtkWidget        *checkbutton_readonly;
};

static void     hildon_file_system_storage_dialog_class_init      (HildonFileSystemStorageDialogClass *klass);
static void     hildon_file_system_storage_dialog_init            (HildonFileSystemStorageDialog      *widget);
static void     file_system_storage_dialog_finalize               (GObject                            *object);
static void     file_system_storage_dialog_clear_data_container   (GtkWidget                          *widget);
static void     file_system_storage_dialog_stats_clear            (GtkWidget                          *widget);
static gboolean file_system_storage_dialog_stats_collect          (GtkWidget                          *widget,
								   GnomeVFSURI                        *uri);
static gboolean file_system_storage_dialog_stats_get_disk         (const gchar                        *path,
								   GnomeVFSFileSize                   *total,
								   GnomeVFSFileSize                   *available);
static void     file_system_storage_dialog_stats_get_contacts     (GtkWidget                          *widget);
static gboolean file_system_storage_dialog_stats_get_emails_cb    (const gchar                        *path,
								   GnomeVFSFileInfo                   *info,
								   gboolean                            recursing_loop,
								   GtkWidget                          *widget,
								   gboolean                           *recurse);
static void     file_system_storage_dialog_stats_get_emails       (GtkWidget                          *widget);
static gboolean file_system_storage_dialog_stats_get_apps_cb      (GIOChannel                         *source,
								   GIOCondition                        condition,
								   GtkWidget                          *widget);
static void     file_system_storage_dialog_stats_get_apps         (GtkWidget                          *widget);
static gchar *  file_system_storage_dialog_get_size_string        (GnomeVFSFileSize                    bytes);
static void     file_system_storage_dialog_request_device_name_cb (DBusPendingCall                    *pending_call,
								   gpointer                            user_data);
static void     file_system_storage_dialog_request_device_name    (HildonFileSystemStorageDialog      *dialog);
static URIType  file_system_storage_dialog_get_type               (const gchar                        *uri_str);
static void     file_system_storage_dialog_set_no_data            (GtkWidget                          *widget);
static void     file_system_storage_dialog_set_data               (GtkWidget                          *widget);
static void     file_system_storage_dialog_update                 (GtkWidget                          *widget);
static void     file_system_storage_dialog_monitor_cb             (GnomeVFSMonitorHandle              *handle,
								   const gchar                        *monitor_uri,
								   const gchar                        *info_uri,
								   GnomeVFSMonitorEventType            event_type,
								   HildonFileSystemStorageDialog      *widget);

G_DEFINE_TYPE (HildonFileSystemStorageDialog, hildon_file_system_storage_dialog, GTK_TYPE_DIALOG)

static void 
hildon_file_system_storage_dialog_class_init (HildonFileSystemStorageDialogClass *klass)
{
	GObjectClass *object_class;

        object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = file_system_storage_dialog_finalize;

	g_type_class_add_private (object_class, sizeof (HildonFileSystemStorageDialogPriv));
}

static void 
hildon_file_system_storage_dialog_init (HildonFileSystemStorageDialog *widget) 
{
        HildonFileSystemStorageDialogPriv *priv;
        GtkWidget   *notebook;
        GtkWidget   *page;
        GtkWidget   *scrolledwindow;
        GtkWidget   *viewport_common, *viewport_data;
        GtkWidget   *table;
        GtkWidget   *label_name_stub;
        GtkWidget   *label_name;
        GtkWidget   *label_total_size_stub;
        GtkWidget   *hbox;
        GtkWidget   *image_type;
        GtkWidget   *label_type;
        GtkWidget   *label_total_size;
        GtkWidget   *label_in_use_stub;
        GtkWidget   *label_in_use;
        GtkWidget   *label_available_stub;
        GtkWidget   *label_available;
        GtkWidget   *label_read_only_stub;
        GtkWidget   *checkbutton_readonly;
        GtkWidget   *label_type_stub;
        GtkWidget   *label_common;
        GtkWidget   *label_data;
	GdkGeometry  geometry;

        priv = GET_PRIV (widget);

	/* Window properties */
	gtk_window_set_title (GTK_WINDOW (widget), _("sfil_ti_storage_details"));
	gtk_window_set_resizable (GTK_WINDOW (widget), FALSE);
	gtk_dialog_set_has_separator (GTK_DIALOG (widget), FALSE);
	gtk_dialog_add_button (GTK_DIALOG (widget), 
			       _("sfil_bd_storage_details_dialog_ok"), 
			       GTK_RESPONSE_OK);

	/* Create a note book with 2 pages */
	notebook = gtk_notebook_new ();
	gtk_widget_show (notebook);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (widget)->vbox), notebook, 
			    TRUE, TRUE, 
			    HILDON_MARGIN_DEFAULT);

	/* Setup a good size, copied from the old storage details dialog. */
	geometry.min_width = 133;
	geometry.max_width = 602;

	/* Scrolled windows do not ask space for whole contents in size_request.
	 * So, we must force the dialog to have larger than minimum size 
	 */
	geometry.min_height = 240 + (2 * HILDON_MARGIN_DEFAULT);
	geometry.max_height = 240 + (2 * HILDON_MARGIN_DEFAULT);
	
	gtk_window_set_geometry_hints (GTK_WINDOW (widget),
				       GTK_WIDGET (notebook), &geometry,
				       GDK_HINT_MIN_SIZE | GDK_HINT_MAX_SIZE);
        
        /* Create first page for "Common" properties */
        scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
        gtk_widget_show (scrolledwindow);
        gtk_container_add (GTK_CONTAINER (notebook), scrolledwindow);
        gtk_container_set_border_width (GTK_CONTAINER (scrolledwindow), HILDON_MARGIN_DEFAULT);
        gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow),
                                        GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);        
        viewport_common = gtk_viewport_new (NULL, NULL);
        gtk_widget_show (viewport_common);
        gtk_container_add (GTK_CONTAINER (scrolledwindow), viewport_common);

        table = gtk_table_new (6, 2, FALSE);
        gtk_widget_show (table);
        gtk_container_add (GTK_CONTAINER (viewport_common), table);
        gtk_table_set_col_spacings (GTK_TABLE (table), HILDON_MARGIN_DOUBLE);

        label_name_stub = gtk_label_new (_("sfil_fi_storage_details_name"));
        gtk_widget_show (label_name_stub);
        gtk_table_attach (GTK_TABLE (table), label_name_stub, 0, 1, 0, 1,
                          (GtkAttachOptions) (GTK_FILL),
                          (GtkAttachOptions) (0), 0, 0);
        gtk_label_set_justify (GTK_LABEL (label_name_stub), GTK_JUSTIFY_RIGHT);
        gtk_misc_set_alignment (GTK_MISC (label_name_stub), 1.0, 0.5);

        label_name = gtk_label_new ("");
        gtk_widget_show (label_name);
        gtk_table_attach (GTK_TABLE (table), label_name, 1, 2, 0, 1,
                          (GtkAttachOptions) (GTK_FILL | GTK_EXPAND),
                          (GtkAttachOptions) (0), 0, 0);
        gtk_misc_set_alignment (GTK_MISC (label_name), 0.0, 0.5);

        label_total_size_stub = gtk_label_new (_("sfil_fi_storage_details_size"));
        gtk_widget_show (label_total_size_stub);
        gtk_table_attach (GTK_TABLE (table), label_total_size_stub, 0, 1, 2, 3,
                          (GtkAttachOptions) (GTK_FILL),
                          (GtkAttachOptions) (0), 0, 0);
        gtk_label_set_justify (GTK_LABEL (label_total_size_stub), GTK_JUSTIFY_RIGHT);
        gtk_misc_set_alignment (GTK_MISC (label_total_size_stub), 1.0, 0.5);

        hbox = gtk_hbox_new (FALSE, HILDON_MARGIN_DEFAULT);
        gtk_widget_show (hbox);
        gtk_table_attach (GTK_TABLE (table), hbox, 1, 2, 1, 2,
                          (GtkAttachOptions) (GTK_FILL),
                          (GtkAttachOptions) (GTK_FILL), 0, 0);

        label_type_stub = gtk_label_new (_("sfil_fi_storage_details_type"));
        gtk_widget_show (label_type_stub);
        gtk_table_attach (GTK_TABLE (table), label_type_stub, 0, 1, 1, 2,
                          (GtkAttachOptions) (GTK_FILL),
                          (GtkAttachOptions) (0), 0, 0);
        gtk_label_set_justify (GTK_LABEL (label_type_stub), GTK_JUSTIFY_RIGHT);
        gtk_misc_set_alignment (GTK_MISC (label_type_stub), 1.0, 0.5);

	image_type = gtk_image_new_from_icon_name ("qgn_list_filesys_removable_storage",
                                                   HILDON_ICON_SIZE_SMALL);
        gtk_widget_show (image_type);
        gtk_box_pack_start (GTK_BOX (hbox), image_type, FALSE, FALSE, 0);

        label_type = gtk_label_new ("Storage device");
        gtk_widget_show (label_type);
        gtk_box_pack_start (GTK_BOX (hbox), label_type, TRUE, TRUE, 0);
        gtk_misc_set_alignment (GTK_MISC (label_type), 0.0, 0.5);

        label_total_size = gtk_label_new ("");
        gtk_widget_show (label_total_size);
        gtk_table_attach (GTK_TABLE (table), label_total_size, 1, 2, 2, 3,
                          (GtkAttachOptions) (GTK_FILL | GTK_EXPAND),
                          (GtkAttachOptions) (0), 0, 0);
        gtk_misc_set_alignment (GTK_MISC (label_total_size), 0.0, 0.5);

        label_in_use_stub = gtk_label_new (_("sfil_fi_storage_details_in_use"));
        gtk_widget_show (label_in_use_stub);
        gtk_table_attach (GTK_TABLE (table), label_in_use_stub, 0, 1, 3, 4,
                          (GtkAttachOptions) (GTK_FILL),
                          (GtkAttachOptions) (0), 0, 0);
        gtk_label_set_justify (GTK_LABEL (label_in_use_stub), GTK_JUSTIFY_RIGHT);
        gtk_misc_set_alignment (GTK_MISC (label_in_use_stub), 1.0, 0.5);

        label_in_use = gtk_label_new ("");
        gtk_widget_show (label_in_use);
        gtk_table_attach (GTK_TABLE (table), label_in_use, 1, 2, 3, 4,
                          (GtkAttachOptions) (GTK_FILL | GTK_EXPAND),
                          (GtkAttachOptions) (0), 0, 0);
        gtk_misc_set_alignment (GTK_MISC (label_in_use), 0.0, 0.5);

        label_available_stub = gtk_label_new (_("sfil_fi_storage_details_available"));
        gtk_widget_show (label_available_stub);
        gtk_table_attach (GTK_TABLE (table), label_available_stub, 0, 1, 4, 5,
                          (GtkAttachOptions) (GTK_FILL),
                          (GtkAttachOptions) (0), 0, 0);
        gtk_label_set_justify (GTK_LABEL (label_available_stub), GTK_JUSTIFY_RIGHT);
        gtk_misc_set_alignment (GTK_MISC (label_available_stub), 1.0, 0.5);

        label_available = gtk_label_new ("");
        gtk_widget_show (label_available);
        gtk_table_attach (GTK_TABLE (table), label_available, 1, 2, 4, 5,
                          (GtkAttachOptions) (GTK_FILL | GTK_EXPAND),
                          (GtkAttachOptions) (0), 0, 0);
        gtk_misc_set_alignment (GTK_MISC (label_available), 0.0, 0.5);

        label_read_only_stub = gtk_label_new (_("sfil_fi_storage_details_readonly"));
        gtk_widget_show (label_read_only_stub);
        gtk_table_attach (GTK_TABLE (table), label_read_only_stub, 0, 1, 5, 6,
                          (GtkAttachOptions) (GTK_FILL),
                          (GtkAttachOptions) (0), 0, 0);
        gtk_label_set_justify (GTK_LABEL (label_read_only_stub), GTK_JUSTIFY_RIGHT);
        gtk_misc_set_alignment (GTK_MISC (label_read_only_stub), 1.0, 0.5);

        checkbutton_readonly = gtk_check_button_new ();
        gtk_widget_show (checkbutton_readonly);
        gtk_widget_set_sensitive (checkbutton_readonly, FALSE);
        gtk_table_attach (GTK_TABLE (table), checkbutton_readonly, 1, 2, 5, 6,
                          (GtkAttachOptions) (GTK_FILL | GTK_EXPAND),
                          (GtkAttachOptions) (0), 0, 0);

        page = gtk_notebook_get_nth_page (GTK_NOTEBOOK (notebook), 0);
        label_common = gtk_label_new ("Common"); /* FIXME: Need logical ID here. */
        gtk_widget_show (label_common);
        gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook), page, label_common);

        /* Create second page for "Data" properties */
        scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
        gtk_widget_show (scrolledwindow);
        gtk_container_add (GTK_CONTAINER (notebook), scrolledwindow);
        gtk_container_set_border_width (GTK_CONTAINER (scrolledwindow), HILDON_MARGIN_DEFAULT);
        gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow), 
                                        GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

        viewport_data = gtk_viewport_new (NULL, NULL);
        gtk_widget_show (viewport_data);
        gtk_container_add (GTK_CONTAINER (scrolledwindow), viewport_data);

        /* There is no table or widget set here, we use the _set_no_data()
         * or the _set_data() functions here to do that.
         */

        page = gtk_notebook_get_nth_page (GTK_NOTEBOOK (notebook), 1);
        label_data = gtk_label_new ("Data"); /* FIXME: Need logical ID here. */
        gtk_widget_show (label_data);
        gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook), page, label_data);

        /* Set private widgets */
        priv->viewport_common = viewport_common;
        priv->viewport_data = viewport_data;

        priv->label_name = label_name;
        priv->image_type = image_type;
        priv->label_type = label_type;
        priv->label_total_size = label_total_size;
        priv->label_in_use = label_in_use;
        priv->label_available = label_available;

        priv->label_read_only_stub = label_read_only_stub;
        priv->checkbutton_readonly = checkbutton_readonly;
}

static void
file_system_storage_dialog_finalize (GObject *object)
{
	HildonFileSystemStorageDialogPriv *priv;

	priv = GET_PRIV (object);

	if (priv->monitor_handle) {
		gnome_vfs_monitor_cancel (priv->monitor_handle);
	}

        if (priv->pending_call) {
                dbus_pending_call_cancel (priv->pending_call);
                dbus_pending_call_unref (priv->pending_call);
                priv->pending_call = NULL;
        }

        if (priv->get_apps_id) {
                g_source_remove (priv->get_apps_id);
                priv->get_apps_id = 0;
        }

        if (priv->apps_string) {
                g_string_free (priv->apps_string, TRUE);
        }

        g_free (priv->uri_str);

	G_OBJECT_CLASS (hildon_file_system_storage_dialog_parent_class)->finalize (object);
}

/* Some utility functions for URIs and paths. */
static char *
get_without_trailing_slash (const char *str)
{
	size_t len;

	if (!str) {
		return NULL;
	}

	len = strlen (str);
	if (len == 0) {
		return g_strdup (str);
	}

	if (str[len - 1] == '/') {
		return g_strndup (str, len - 1);
	} else {
		return g_strdup (str);
	}
}

/* Compares without trailing slashes in both paths. */
static gboolean
path_is_equal (const gchar *path1, const gchar *path2)
{
	gchar    *without1;
	gchar    *without2;
	gboolean  ret;

	without1 = get_without_trailing_slash (path1);
	without2 = get_without_trailing_slash (path2);

	if (strcmp (without1, without2) == 0) {
		ret = TRUE;
	} else {
		ret = FALSE;
	}

	g_free (without1);
	g_free (without2);

	return ret;
}

static void
file_system_storage_dialog_clear_data_container (GtkWidget *widget)
{
        HildonFileSystemStorageDialog     *storage;
        HildonFileSystemStorageDialogPriv *priv;
        GList                             *children;
        
        storage = HILDON_FILE_SYSTEM_STORAGE_DIALOG (widget);
        priv = GET_PRIV (storage);

        children = gtk_container_get_children (GTK_CONTAINER (priv->viewport_data));
        g_list_foreach (children, (GFunc) gtk_widget_destroy, NULL);
}

static void
file_system_storage_dialog_stats_clear (GtkWidget *widget)
{
        HildonFileSystemStorageDialogPriv *priv;
 
        priv = GET_PRIV (widget);

        priv->file_count = 0;
        priv->folder_count = 0;

        priv->email_size = 0;
        priv->image_size = 0;
        priv->video_size = 0;
        priv->audio_size = 0;
        priv->html_size = 0;
        priv->doc_size = 0;
        priv->contact_size = 0;
        priv->installed_app_size = 0;
        priv->other_size = 0;
        priv->in_use_size = 0;
}

static gboolean
file_system_storage_dialog_stats_collect (GtkWidget   *widget, 
					  GnomeVFSURI *uri)
{
        HildonFileSystemStorageDialogPriv *priv;
	GnomeVFSURI                       *child_uri;
	GnomeVFSFileInfo                  *info;
	GnomeVFSFileInfoOptions            flags;
	GnomeVFSResult                     result;
	GList                             *files = NULL, *l;
	gchar                             *uri_str;
	gboolean                           ok = TRUE;

        priv = GET_PRIV (widget);

        priv->folder_count++;

	flags = GNOME_VFS_FILE_INFO_FIELDS_TYPE |
                GNOME_VFS_FILE_INFO_FIELDS_SIZE |
		GNOME_VFS_FILE_INFO_GET_MIME_TYPE |
                GNOME_VFS_FILE_INFO_FORCE_FAST_MIME_TYPE;

	uri_str = gnome_vfs_uri_to_string (uri, GNOME_VFS_URI_HIDE_NONE);
	result = gnome_vfs_directory_list_load (&files, uri_str, flags);

	if (result != GNOME_VFS_OK) {
		g_printerr ("Could not open directory:'%s', error:%d->'%s'\n",
                            uri_str, result, gnome_vfs_result_to_string (result));
		g_free (uri_str);
		return FALSE;
	}

	for (l = files; l; l = l->next) {
		info = l->data;

		if (strcmp (info->name, ".") == 0 ||
		    strcmp (info->name, "..") == 0) {
			continue;
		}

		child_uri = gnome_vfs_uri_append_path (uri, info->name);

		if (info->type == GNOME_VFS_FILE_TYPE_DIRECTORY) {
                        file_system_storage_dialog_stats_collect (widget, child_uri);
		} else {
                        gchar *mime_type;

                        priv->file_count++;
                        priv->in_use_size += info->size;

                        if (!info->mime_type) {
                                priv->other_size += info->size;
                                gnome_vfs_uri_unref (child_uri);
                                continue;
                        }

                        mime_type = g_ascii_strdown (info->mime_type, -1);

                        if (g_str_has_prefix (mime_type, "image") ||
                            g_str_has_prefix (mime_type, "sketch/png") ||
                            g_str_has_prefix (mime_type, "application/x-sketch-png")) {
                                priv->image_size += info->size;
                        } else if (g_str_has_prefix (mime_type, "audio")) {
                                priv->audio_size += info->size;
                        } else if (g_str_has_prefix (mime_type, "video")) {
                                priv->video_size += info->size;
                        } else if (g_str_has_prefix (mime_type, "text/xml") ||
                                   g_str_has_prefix (mime_type, "text/html")) {
                                priv->html_size += info->size;
                        } else if (g_str_has_prefix (mime_type, "text/plain") ||
                                   g_str_has_prefix (mime_type, "text/x-notes") ||
                                   g_str_has_prefix (mime_type, "text/note") ||
                                   g_str_has_prefix (mime_type, "text/richtext") ||
                                   g_str_has_prefix (mime_type, "application/pdf") ||
                                   g_str_has_prefix (mime_type, "application/rss+xml")) {
                                priv->doc_size += info->size;
                        } else {
                                priv->other_size += info->size;
                        }
                        
                        g_free (mime_type);
		}

		gnome_vfs_uri_unref (child_uri);
	}

#if 0
        g_print ("Collecting stats for path: '%s' (%d files), "
                 "total size:%" GNOME_VFS_SIZE_FORMAT_STR ", "
                 "I:%" GNOME_VFS_SIZE_FORMAT_STR ", "
                 "A:%" GNOME_VFS_SIZE_FORMAT_STR ", "
                 "V:%" GNOME_VFS_SIZE_FORMAT_STR ", "
                 "D:%" GNOME_VFS_SIZE_FORMAT_STR ", "
                 "H:%" GNOME_VFS_SIZE_FORMAT_STR ", "
                 "O:%" GNOME_VFS_SIZE_FORMAT_STR "\n", 
                 gnome_vfs_uri_get_path (uri),
                 g_list_length (files), 
                 priv->in_use_size,
                 priv->image_size,
                 priv->audio_size,
                 priv->video_size,
                 priv->doc_size,
                 priv->html_size,
                 priv->other_size);
#endif

	gnome_vfs_file_info_list_free (files);

	g_free (uri_str);

	return ok;
} 

static gboolean
file_system_storage_dialog_stats_get_disk (const gchar      *path,
					   GnomeVFSFileSize *total,
					   GnomeVFSFileSize *available)
{
        struct statfs st;

        if (statfs (path, &st) == 0) {
                if (total) {
                        *total = st.f_blocks;
			*total *= st.f_bsize;
                }

                if (available) {
                        *available = st.f_bfree;
			*available *= st.f_bsize;
                }

                return TRUE;
        }

        if (total) {
                *total = 0;
        }
        
        if (available) {
                *available = 0;
        }

        return FALSE;
}

static void 
file_system_storage_dialog_stats_get_contacts (GtkWidget *widget)
{
        HildonFileSystemStorageDialogPriv *priv;
	GnomeVFSFileInfo                  *info;
	GnomeVFSResult                     result;
	gchar                             *uri_str;

        priv = GET_PRIV (widget);

	if (priv->uri_type != URI_TYPE_FILE_SYSTEM &&
	    priv->uri_type != URI_TYPE_UNKNOWN) {
		return;
	}

	uri_str = g_build_filename (g_get_home_dir (), 
				    ".osso-email",
				    "AddressBook.xml", 
				    NULL);
#if 0
	g_print ("Collecting stats for contacts by URI type:%d using path: '%s'\n", type, uri_str);
#endif

	info = gnome_vfs_file_info_new ();

	if (info != NULL) {
		result = gnome_vfs_get_file_info (uri_str, info,
						  GNOME_VFS_FILE_INFO_DEFAULT |
						  GNOME_VFS_FILE_INFO_FOLLOW_LINKS);
		if (result == GNOME_VFS_OK) {
			priv->contact_size = info->size;
			priv->in_use_size += info->size;
		}
	}

	gnome_vfs_file_info_unref (info);
	g_free (uri_str);
}

static gboolean
file_system_storage_dialog_stats_get_emails_cb (const gchar      *path,
						GnomeVFSFileInfo *info,
						gboolean          recursing_loop, 
						GtkWidget        *widget,
						gboolean         *recurse)
{
	HildonFileSystemStorageDialogPriv *priv;

	if (!path || !widget || !info) {
		return FALSE;
	}

	priv = GET_PRIV (widget);
	
	if (strcmp (info->name, ".") == 0 ||
	    strcmp (info->name, "..") == 0) {
		if (recursing_loop) {
			return FALSE;
		}
	      
		*recurse = TRUE;
	} else {
		*recurse = FALSE;
	}

	priv->email_size += info->size;
	priv->in_use_size += info->size;

	return TRUE;
}

static void 
file_system_storage_dialog_stats_get_emails (GtkWidget *widget)
{
        HildonFileSystemStorageDialogPriv *priv;
	gchar                             *uri_str;

        priv = GET_PRIV (widget);

	switch (priv->uri_type) {
	case URI_TYPE_INTERNAL_MMC:
		uri_str = g_build_path ("/",
					g_getenv ("INTERNAL_MMC_MOUNTPOINT"),
					".archive",
					NULL);
		break;

	case URI_TYPE_EXTERNAL_MMC:
		uri_str = g_build_path ("/",
					g_getenv ("MMC_MOUNTPOINT"),
					".archive",
					NULL);
		break;

	case URI_TYPE_FILE_SYSTEM:
	case URI_TYPE_UNKNOWN:
	default:
		uri_str = g_build_path ("/",
					g_get_home_dir (),
					"apps",
					"email",
					"Mail",
					NULL);
		break;
	}

#ifdef ENABLE_DEBUGGING
	g_print ("Collecting stats for emails by URI type:%d using path: '%s'\n", type, uri_str);
#endif

	gnome_vfs_directory_visit (uri_str,
				   GNOME_VFS_FILE_INFO_FOLLOW_LINKS |
				   GNOME_VFS_FILE_INFO_GET_MIME_TYPE |
                                   GNOME_VFS_FILE_INFO_FORCE_FAST_MIME_TYPE,
				   GNOME_VFS_DIRECTORY_VISIT_LOOPCHECK,
				   (GnomeVFSDirectoryVisitFunc) 
				   file_system_storage_dialog_stats_get_emails_cb, 
				   widget);
	g_free (uri_str);
}

static gboolean 
file_system_storage_dialog_stats_get_apps_cb (GIOChannel   *source, 
					      GIOCondition  condition, 
					      GtkWidget    *widget)
{
	HildonFileSystemStorageDialogPriv *priv;

        priv = GET_PRIV (widget);

	if (condition & G_IO_IN) {
		GIOStatus status;

		
		status = G_IO_STATUS_AGAIN;
		while (status == G_IO_STATUS_AGAIN) {
			gchar *input;
			gsize  len;

			status = g_io_channel_read_to_end (source, &input, &len, NULL);
			if (len < 1) {
				break;
			}
                        
			g_string_append (priv->apps_string, input);
			g_free (input);
		}
	}

	if (condition & G_IO_HUP) {
		HildonFileSystemStorageDialogPriv  *priv;
		gchar                             **rows, **row;

		priv = GET_PRIV (widget);

		g_io_channel_shutdown (source, FALSE, NULL);
		g_io_channel_unref (source);

                priv->get_apps_id = 0;
                
		/* Get install application stats */
		rows = g_strsplit (priv->apps_string->str, "\n", -1);
		for (row = rows; *row != NULL; row++) {
			GnomeVFSFileSize   bytes;
			gchar            **cols;
			gint               len;

			cols = g_strsplit (*row, "\t", -1);
			len = g_strv_length (cols) - 1;

			if (!cols || len < 0) {
				continue;
			}
		
			bytes = 1024 * atoi (cols[len]);
			priv->installed_app_size += bytes;
			priv->in_use_size += bytes;
			g_strfreev (cols);
		}
		
		g_strfreev (rows);

		file_system_storage_dialog_set_data (widget);

		return FALSE;
	}

	return TRUE;
}

static void     
file_system_storage_dialog_stats_get_apps (GtkWidget *widget)
{
        HildonFileSystemStorageDialogPriv *priv;
	GIOChannel                        *channel;
	GPid                               pid;
	gchar                             *argv[] = { "/usr/bin/maemo-list-user-packages", NULL };
	gint                               out;
	gboolean                           success;

        priv = GET_PRIV (widget);

        if (priv->apps_string) {
                g_string_truncate (priv->apps_string, 0);
        } else {
                priv->apps_string = g_string_new (NULL);
        }

        if (priv->get_apps_id) {
                g_source_remove (priv->get_apps_id);
                priv->get_apps_id = 0;
        }
        
	if (priv->uri_type != URI_TYPE_FILE_SYSTEM && priv->uri_type != URI_TYPE_UNKNOWN) {
		return;
	}

	success = g_spawn_async_with_pipes (NULL, argv, NULL, 
					    G_SPAWN_SEARCH_PATH, 
					    NULL, NULL, 
					    &pid, 
					    NULL, &out, NULL, 
					    NULL);
	if (!success) {
		g_warning ("Could not spawn command: '%s' to get list of applications", argv[0]);
		return;
	}

	channel = g_io_channel_unix_new (out);
	g_io_channel_set_flags (channel, G_IO_FLAG_NONBLOCK, NULL);
	priv->get_apps_id = g_io_add_watch_full (
                channel, 
                G_PRIORITY_DEFAULT_IDLE, 
                G_IO_IN | G_IO_HUP, 
                (GIOFunc) 
                file_system_storage_dialog_stats_get_apps_cb, 
                widget, 
                NULL);
}

static void
file_system_storage_dialog_request_device_name_cb (DBusPendingCall *pending_call, 
						   gpointer         user_data)
{
        HildonFileSystemStorageDialog     *dialog = user_data;
        HildonFileSystemStorageDialogPriv *priv;
        DBusMessage                       *reply;
        DBusError                          error;
        gchar                             *name;

        priv = GET_PRIV (dialog);
        
        name = NULL;

        reply = dbus_pending_call_steal_reply (pending_call);
        if (reply) {
                dbus_error_init (&error);

                if (!dbus_set_error_from_message (&error, reply)) {
                        gchar *tmp = NULL;
                
                        if (dbus_message_get_args (reply, NULL,
                                                   DBUS_TYPE_STRING, &tmp,
                                                   DBUS_TYPE_INVALID)) {
                                name = g_strdup (tmp);
                        }
                } else {
                        g_printerr ("Did not get the name: %s %s\n",
                                    error.name, error.message);
                }
                
                dbus_message_unref (reply);
        }
        
        if (name) {
                gtk_label_set_text (GTK_LABEL (priv->label_name), name);
        } else {
                /* This should never happen but just in case, use the same
                 * fallback as the file manager.
                 */
                gtk_label_set_text (GTK_LABEL (priv->label_name), "Internet Tablet");
        }

        dbus_pending_call_unref (priv->pending_call);
        priv->pending_call = NULL;
}

static void
file_system_storage_dialog_request_device_name (HildonFileSystemStorageDialog *dialog)
{
        HildonFileSystemStorageDialogPriv *priv;
	static DBusConnection             *conn;
	DBusMessage                       *message;

        priv = GET_PRIV (dialog);

        if (!conn) {
                conn = dbus_bus_get (DBUS_BUS_SYSTEM, NULL);
                if (!conn) {
                        return;
                }
                dbus_connection_setup_with_g_main (conn, NULL);
        }
        
        if (priv->pending_call) {
                dbus_pending_call_cancel (priv->pending_call);
                dbus_pending_call_unref (priv->pending_call);
                priv->pending_call = NULL;
        }

	message = dbus_message_new_method_call ("org.bluez",
						"/org/bluez/hci0",
						"org.bluez.Adapter",
						"GetName");

        if (dbus_connection_send_with_reply (conn, message, &priv->pending_call, -1)) {
                dbus_pending_call_set_notify (priv->pending_call,
                                              file_system_storage_dialog_request_device_name_cb,
                                              dialog, NULL);
        }

        dbus_message_unref (message);
}

static gchar *
file_system_storage_dialog_get_size_string (GnomeVFSFileSize bytes)
{
	const gchar *str;
	gdouble      size;

	if (bytes < KILOBYTE_FACTOR) {
		str = _("ckdg_va_properties_size_kb");
		return g_strdup_printf (str, 1);
	}

	if (bytes >= 1 * KILOBYTE_FACTOR && bytes < 100 * KILOBYTE_FACTOR) {
		size = bytes / KILOBYTE_FACTOR;
		str =  _("ckdg_va_properties_size_1kb_99kb");
		return g_strdup_printf (str, (int) size);
	}

	if (bytes >= 100 * KILOBYTE_FACTOR && bytes < 1 * MEGABYTE_FACTOR) {
		size = bytes / KILOBYTE_FACTOR;
		str = _("ckdg_va_properties_size_100kb_1mb");
		return g_strdup_printf (str, (int) size);
	}

	if (bytes >= 1 * MEGABYTE_FACTOR && bytes < 10 * MEGABYTE_FACTOR) {
		size = bytes / MEGABYTE_FACTOR;
		str = _("ckdg_va_properties_size_1mb_10mb");
		return g_strdup_printf (str, size);
	}

	if (bytes >= 10 * MEGABYTE_FACTOR && bytes < 1 * GIGABYTE_FACTOR) {
		size = bytes / MEGABYTE_FACTOR;
		str = _("ckdg_va_properties_size_10mb_1gb");
		return g_strdup_printf (str, size);
	}

	size = bytes / GIGABYTE_FACTOR;
	str = _("ckdg_va_properties_size_1gb_or_greater");
	return g_strdup_printf (str, size);
}

static URIType
file_system_storage_dialog_get_type (const gchar *uri_str)
{
	const gchar *env;
 	gchar       *env_uri_str;

	if (!uri_str) {
		return URI_TYPE_UNKNOWN;
	}
	
	env = g_getenv ("MYDOCSDIR");
	if (env) {
		gboolean found = FALSE;

		env_uri_str = gnome_vfs_get_uri_from_local_path (env);
		found = env_uri_str && strcmp (env_uri_str, uri_str) == 0;
		g_free (env_uri_str);

		if (found) {
			return URI_TYPE_FILE_SYSTEM;
		}
	}

	env = g_getenv ("INTERNAL_MMC_MOUNTPOINT");
	if (env) {
		gboolean found = FALSE;

		env_uri_str = gnome_vfs_get_uri_from_local_path (env);
		found = env_uri_str && strcmp (env_uri_str, uri_str) == 0;
		g_free (env_uri_str);

		if (found) {
			return URI_TYPE_INTERNAL_MMC;
		}
	}

	env = g_getenv ("MMC_MOUNTPOINT");
	if (env) {
		gboolean found = FALSE;

		env_uri_str = gnome_vfs_get_uri_from_local_path (env);
		found = env_uri_str && strcmp (env_uri_str, uri_str) == 0;
		g_free (env_uri_str);

		if (found) {
			return URI_TYPE_EXTERNAL_MMC;
		}
	}

	return URI_TYPE_UNKNOWN;
}

static void
file_system_storage_dialog_set_no_data (GtkWidget *widget) 
{
        HildonFileSystemStorageDialog     *storage;
        HildonFileSystemStorageDialogPriv *priv;
        GtkWidget                         *label_no_data;

        g_return_if_fail (HILDON_IS_FILE_SYSTEM_STORAGE_DIALOG (widget));

        storage = HILDON_FILE_SYSTEM_STORAGE_DIALOG (widget);
        priv = GET_PRIV (storage);

        /* Clear current child */
        file_system_storage_dialog_clear_data_container (widget);

        /* Create new widget */
        label_no_data = gtk_label_new (_("sfil_li_storage_details_no_data"));
        gtk_widget_show (label_no_data);
        gtk_misc_set_alignment (GTK_MISC (label_no_data), 0.0, 0.0);
        gtk_container_add (GTK_CONTAINER (priv->viewport_data), label_no_data);
}

static void
file_system_storage_dialog_set_data (GtkWidget *widget) 
{
        HildonFileSystemStorageDialogPriv *priv;
        GnomeVFSFileSize                   size;
        GtkWidget                         *table;
        GtkWidget                         *label_size;
        GtkWidget                         *label_category;
        const gchar                       *category_str;
        gchar                             *str;
        gint                               category;
        gint                               col;
        gboolean                           have_data;

        g_return_if_fail (HILDON_IS_FILE_SYSTEM_STORAGE_DIALOG (widget));

        priv = GET_PRIV (widget);

        /* Clear current child */
        file_system_storage_dialog_clear_data_container (widget);

        /* Create new table for categories */
        table = gtk_table_new (1, 1, FALSE);
        gtk_widget_show (table);
        gtk_container_add (GTK_CONTAINER (priv->viewport_data), table);
        gtk_table_set_col_spacings (GTK_TABLE (table), HILDON_MARGIN_DOUBLE);

        category = 0;
        col = 0;
        have_data = FALSE;

        while (category < 9) {
                switch (category) {
                case 0: 
                        category_str = _("sfil_li_emails");
                        size = priv->email_size;
                        break;
                case 1: 
                        category_str = _("sfil_li_images");
                        size = priv->image_size;
                        break;
                case 2: 
                        category_str = _("sfil_li_video_clips");
                        size = priv->video_size;
                        break;
                case 3: 
                        category_str = _("sfil_li_sound_clips");
                        size = priv->audio_size;
                        break;
                case 4: 
                        category_str = _("sfil_li_web_pages");
                        size = priv->html_size;
                        break;
                case 5: 
                        category_str = _("sfil_li_documents");
                        size = priv->doc_size;
                        break;
                case 6: 
                        category_str = _("sfil_li_contacts");
                        size = priv->contact_size;
                        break;
                case 7: 
                        category_str = _("sfil_li_installed_applications");
                        size = priv->installed_app_size;
                        break;
                case 8: 
		default:
                        category_str = _("sfil_li_other_files");
                        size = priv->other_size;
                        break;
                }

                category++;

                if (size < 1) {
                        continue;
                }

                str = file_system_storage_dialog_get_size_string (size);

                label_size = gtk_label_new (str);
                gtk_widget_show (label_size);
                gtk_table_attach (GTK_TABLE (table), label_size, 0, 1, col, col + 1,
                                  (GtkAttachOptions) (GTK_FILL),
                                  (GtkAttachOptions) (0), 0, 0);
                gtk_label_set_justify (GTK_LABEL (label_size), GTK_JUSTIFY_RIGHT);
                gtk_misc_set_alignment (GTK_MISC (label_size), 1.0, 0.5);
                
                label_category = gtk_label_new (category_str);
                gtk_widget_show (label_category);
                gtk_table_attach (GTK_TABLE (table), label_category, 1, 2, col, col + 1,
                                  (GtkAttachOptions) (GTK_FILL | GTK_EXPAND),
                                  (GtkAttachOptions) (0), 0, 0);
                gtk_label_set_justify (GTK_LABEL (label_size), GTK_JUSTIFY_LEFT);
                gtk_misc_set_alignment (GTK_MISC (label_category), 0.0, 0.5);
                
                col++;
                have_data = TRUE;
                
                g_free (str);
        }

        if (!have_data) {
                file_system_storage_dialog_set_no_data (widget);
        }
}

static void
file_system_storage_dialog_update (GtkWidget *widget)
{
        HildonFileSystemStorageDialogPriv *priv;
	GList                             *volumes, *l;
        GnomeVFSURI                       *uri;
        GnomeVFSFileSize                   total_size, available_size;
	const gchar                       *type_icon_name;
	const gchar                       *type_name;
        gchar                             *display_name;
        gchar                             *total;
        gchar                             *available;
        gchar                             *in_use;
        
        priv = GET_PRIV (widget);

        /* Clean up any old values in case the URI has changed. */
        file_system_storage_dialog_stats_clear (widget);
        gtk_label_set_text (GTK_LABEL (priv->label_total_size), "");
        gtk_label_set_text (GTK_LABEL (priv->label_available), "");
        gtk_label_set_text (GTK_LABEL (priv->label_in_use), "");

        /* Find out what storage we have. */
	if (priv->uri_type == URI_TYPE_INTERNAL_MMC || priv->uri_type == URI_TYPE_EXTERNAL_MMC) {
		GnomeVFSVolume *volume = NULL;

		/* Get URI and Volume to obtain details */
		volumes = gnome_vfs_volume_monitor_get_mounted_volumes (
                        gnome_vfs_get_volume_monitor ());
		
		for (l = volumes; l && !volume; l = l->next) {
			gchar *uri_str;
			
			uri_str = gnome_vfs_volume_get_activation_uri (l->data);
			if (path_is_equal (uri_str, priv->uri_str)) {
				volume = l->data;
			}
			g_free (uri_str);
		}
		
		g_list_foreach (volumes, (GFunc) gnome_vfs_volume_unref, NULL); 
		g_list_free (volumes);

		if (volume) {
			/* Read only */
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->checkbutton_readonly),
						      gnome_vfs_volume_is_read_only (volume));
			
			/* Display name */
			display_name = gnome_vfs_volume_get_display_name (volume);

                        if (display_name) {
                                if (g_str_has_prefix (display_name, "mmc-undefined-name")) {
                                        gchar *translated;

                                        translated = g_strdup (_(display_name));
                                        g_free (display_name);

                                        display_name = translated;
                                }
                        }

                        gtk_label_set_text (GTK_LABEL (priv->label_name),
                                            display_name ? display_name : "");

                        g_free (display_name);
		} else {
                        /* We didn't find any matching volume, apparently we
                         * were called with an invalid or unmounted volume. Just
                         * leave the dialog empty.
                         */
			return;
		}
	} else {
                gtk_label_set_text (GTK_LABEL (priv->label_name), "");
                file_system_storage_dialog_request_device_name (
                        HILDON_FILE_SYSTEM_STORAGE_DIALOG (widget));
	}

	/* Type label and icon */
	switch (priv->uri_type) {
	case URI_TYPE_FILE_SYSTEM:
		type_icon_name = "qgn_list_filesys_divc_cls";
		type_name = _("sfil_va_type_internal_memory");
		break;
	case URI_TYPE_INTERNAL_MMC:
		type_icon_name = "qgn_list_gene_internal_memory_card";
		type_name = _("sfil_va_type_internal_memorycard");
		break;
	case URI_TYPE_EXTERNAL_MMC:
		type_icon_name = "qgn_list_gene_removable_memory_card";
		type_name = _("sfil_va_type_removable_memorycard");
		break;
	case URI_TYPE_UNKNOWN:
	default:
		type_icon_name = "qgn_list_filesys_removable_storage";
		type_name = _("sfil_va_type_storage_other");
		break;
	}

	gtk_image_set_from_icon_name (GTK_IMAGE (priv->image_type), 
                                      type_icon_name,
                                      HILDON_ICON_SIZE_SMALL);
	gtk_label_set_text (GTK_LABEL (priv->label_type), type_name);

        /* Set volume stats */
	uri = gnome_vfs_uri_new (priv->uri_str);

        if (file_system_storage_dialog_stats_get_disk (gnome_vfs_uri_get_path (uri),
						       &total_size,
						       &available_size)) {
		total = file_system_storage_dialog_get_size_string (total_size);
		available = file_system_storage_dialog_get_size_string (available_size);
		in_use = file_system_storage_dialog_get_size_string (total_size - available_size);
	} else {
		total = g_strdup (_("sfil_va_total_size_removable_storage"));
		available = g_strdup (_("sfil_va_total_size_removable_storage"));
		in_use = g_strdup (_("sfil_va_total_size_removable_storage"));
	}

        gtk_label_set_text (GTK_LABEL (priv->label_total_size), total ? total : "");
        gtk_label_set_text (GTK_LABEL (priv->label_available), available ? available : "");
        gtk_label_set_text (GTK_LABEL (priv->label_in_use), in_use ? in_use : "");

        /* Sort out file categories */
        file_system_storage_dialog_stats_collect (widget, uri);
	file_system_storage_dialog_stats_get_contacts (widget);
	file_system_storage_dialog_stats_get_emails (widget);
	file_system_storage_dialog_stats_get_apps (widget);
        file_system_storage_dialog_set_data (widget);

        /* Clean up */
        g_free (total);
        g_free (available);
        g_free (in_use);
        gnome_vfs_uri_unref (uri);
}

static void
file_system_storage_dialog_monitor_cb (GnomeVFSMonitorHandle         *handle,
				       const gchar                   *monitor_uri,
				       const gchar                   *info_uri,
				       GnomeVFSMonitorEventType       event_type,
				       HildonFileSystemStorageDialog *widget)
{
	HildonFileSystemStorageDialogPriv *priv;

	if (event_type != GNOME_VFS_MONITOR_EVENT_DELETED) {
		return;
	}

        priv = GET_PRIV (widget);

	if (info_uri && path_is_equal (info_uri, priv->uri_str)) {
		gtk_dialog_response (GTK_DIALOG (widget), GTK_RESPONSE_OK);
	}
}

/**
 * hildon_file_system_storage_dialog_new:
 * @parent: A parent window or %NULL
 * @uri_str: A URI on string form pointing to a storage root
 *
 * Creates a storage details dialog. An example of @uri_str is
 * "file:///media/mmc1" or "file:///home/user/MyDocs". %NULL can be used if you
 * want to set the URI later with hildon_file_system_storage_dialog_set_uri().
 *
 * Return value: The created dialog
 **/
GtkWidget *
hildon_file_system_storage_dialog_new (GtkWindow   *parent,
				       const gchar *uri_str)
{
        GtkWidget *widget;

	widget = g_object_new (HILDON_TYPE_FILE_SYSTEM_STORAGE_DIALOG, NULL);

	if (parent) {
		gtk_window_set_transient_for (GTK_WINDOW (widget), parent);
	}

	if (uri_str) {
		hildon_file_system_storage_dialog_set_uri (widget, uri_str);
	}

        return GTK_WIDGET (widget);
}

/**
 * hildon_file_system_storage_dialog_set_uri:
 * @widget: The HildonFileSystemStorageDialog dialog
 * @uri_str: A URI on string form pointing to a storage root
 * 
 * Sets the storage URI for the dialog, and updates its contents. Note that it
 * should be the root of the storage, for example file:///home/user/MyDocs, if
 * you want the "device memory". If you pass in "file:///" for example, it will
 * traverse the whole file system to collect information about used memory,
 * which most likely isn't what you want.
 **/
void
hildon_file_system_storage_dialog_set_uri (GtkWidget   *widget,
					   const gchar *uri_str)
{
        HildonFileSystemStorageDialogPriv *priv;
	GnomeVFSResult                     result;

        g_return_if_fail (HILDON_IS_FILE_SYSTEM_STORAGE_DIALOG (widget));
        g_return_if_fail (uri_str != NULL && uri_str[0] != '\0');

        priv = GET_PRIV (widget);
        
        g_free (priv->uri_str);

	priv->uri_type = file_system_storage_dialog_get_type (uri_str);
        priv->uri_str = g_strdup (uri_str);

	if (priv->monitor_handle) {
		gnome_vfs_monitor_cancel (priv->monitor_handle);
		priv->monitor_handle = NULL;
	}

	result = gnome_vfs_monitor_add (&priv->monitor_handle,
					uri_str,
					GNOME_VFS_MONITOR_DIRECTORY,
					(GnomeVFSMonitorCallback)
					file_system_storage_dialog_monitor_cb,
					widget);
	
	if (result != GNOME_VFS_OK || priv->monitor_handle == NULL) {
		g_warning ("Could not add monitor for uri:'%s', %s",
			   uri_str,
			   gnome_vfs_result_to_string (result));
	}

        file_system_storage_dialog_update (widget);
}
