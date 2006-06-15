#include <config.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <obex-vfs-utils/ovu-xfer.h>
#define DBUS_API_SUBJECT_TO_CHANGE
#include <dbus/dbus-glib-lowlevel.h>


typedef struct {
	GtkWidget *dialog;
	GtkWidget *label;
	GtkWidget *label2;
	GtkWidget *file_progress;
	GtkWidget *total_progress;
	GnomeVFSAsyncHandle *handle;
	gboolean cancelled;
} TransferDialog;

static gint
update_callback (GnomeVFSAsyncHandle *handle,
		 GnomeVFSXferProgressInfo *info,
		 gpointer data)
{
	TransferDialog *dialog = data;
	GnomeVFSURI    *uri;
	gchar          *item;
	gdouble         this, total;
	gchar          *tmp;

	if (dialog->cancelled
	    && info->phase != GNOME_VFS_XFER_PHASE_COMPLETED) {
		return 0;
	}

	switch (info->phase) {
	case GNOME_VFS_XFER_PHASE_COMPLETED:
                g_print ("Done\n");
		gtk_main_quit ();
		break;
        case GNOME_VFS_XFER_PHASE_INITIAL:
                gtk_label_set_text (GTK_LABEL (dialog->label),
                                    "Initiating...");
                break;
        case GNOME_VFS_XFER_PHASE_COLLECTING:
                gtk_label_set_text (GTK_LABEL (dialog->label), 
                                    "Collecting file list...");
                break;
        case GNOME_VFS_XFER_CHECKING_DESTINATION:
                gtk_label_set_text (GTK_LABEL (dialog->label),
                                    "Checking destination...");
                break;
        case GNOME_VFS_XFER_PHASE_OPENSOURCE:
        case GNOME_VFS_XFER_PHASE_OPENTARGET:
	case GNOME_VFS_XFER_PHASE_MOVING:
	case GNOME_VFS_XFER_PHASE_COPYING:
		tmp = g_strdup_printf ("%ld of %ld",
				       info->file_index, info->files_total);
		gtk_label_set_text (GTK_LABEL (dialog->label2), tmp);
		g_free (tmp);

                if (info->source_name) {
                        uri = gnome_vfs_uri_new (info->source_name);
			item = gnome_vfs_uri_extract_short_name (uri);
		
                        gtk_label_set_text (GTK_LABEL (dialog->label), item);
                        g_free (item);
                        gnome_vfs_uri_unref (uri);
                }

		if (info->file_size > 0) {
			this = (gdouble)info->bytes_copied / (gdouble)info->file_size;
		} else {
			this = 0;
		}
		
		if (info->bytes_total > 0) {
			total = (gdouble)info->total_bytes_copied / (gdouble)info->bytes_total;
		} else {
			total = 0;
		}
		
		gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (dialog->file_progress), 
					       this);
		gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (dialog->total_progress),
					       total);
		break;

	default:
		break;
	}

	return 1;
}

static void
dialog_response (GtkWidget *widget, gint response_id,
		 TransferDialog *dialog)
{
	dialog->cancelled = TRUE;
}

static TransferDialog *
create_transfer_dialog (void)
{
	TransferDialog *dialog;
	GtkWidget      *vbox;

	dialog = g_new0 (TransferDialog, 1);
	
	dialog->dialog = gtk_dialog_new_with_buttons ("Copying",
						      NULL, 0,
						      GTK_STOCK_CANCEL,
						      GTK_RESPONSE_CANCEL, NULL);
	g_signal_connect (dialog->dialog, "response",
			  G_CALLBACK (dialog_response), dialog);
	vbox = gtk_vbox_new (FALSE, 6);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);

	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog->dialog)->vbox), 
			    vbox, TRUE, TRUE, 0);

	dialog->label = gtk_label_new (NULL);
	gtk_misc_set_alignment (GTK_MISC (dialog->label), 0.0, 0.5);
	gtk_box_pack_start (GTK_BOX (vbox), 
			    dialog->label, FALSE, FALSE, 0);

	dialog->label2 = gtk_label_new (NULL);
	gtk_misc_set_alignment (GTK_MISC (dialog->label2), 0.0, 0.5);
	gtk_box_pack_start (GTK_BOX (vbox), 
			    dialog->label2, FALSE, FALSE, 0);

	dialog->total_progress = gtk_progress_bar_new ();

	gtk_box_pack_start (GTK_BOX (vbox), 
			    dialog->total_progress, FALSE, FALSE, 0);

	dialog->file_progress = gtk_progress_bar_new ();
	gtk_box_pack_start (GTK_BOX (vbox), 
			    dialog->file_progress, FALSE, FALSE, 0);
	
	gtk_widget_show_all (dialog->dialog);
	
	return dialog;
}

static void
create_uri_lists (const gchar *source_dir, const gchar *target_dir,
		  GList **source_uris, GList **target_uris)
{
	GnomeVFSDirectoryHandle *handle;
	GnomeVFSFileInfo        *file_info;
	GnomeVFSURI             *source_uri, *target_uri;
	GnomeVFSURI             *source_base, *target_base;
	GnomeVFSResult           result;

	result = gnome_vfs_directory_open (&handle,
					   source_dir,
					   GNOME_VFS_FILE_INFO_DEFAULT);
	if (result != GNOME_VFS_OK) {
		g_printerr ("Couldn't open source dir\n");
		exit (1);
	}

	file_info = gnome_vfs_file_info_new ();
	
	source_base = gnome_vfs_uri_new (source_dir);
	target_base = gnome_vfs_uri_new (target_dir);

	if (!source_base || !target_base) {
		g_printerr ("Invalid URIs\n");
		exit (1);
	}

	while (gnome_vfs_directory_read_next (handle, file_info) == GNOME_VFS_OK) {
		if (strcmp (file_info->name, ".") == 0 ||
		    strcmp (file_info->name, "..") == 0) {
			continue;
	}
		
		source_uri = gnome_vfs_uri_append_file_name (source_base, file_info->name);
		target_uri = gnome_vfs_uri_append_file_name (target_base, file_info->name);

		*source_uris = g_list_prepend (*source_uris, source_uri);
		*target_uris = g_list_prepend (*target_uris, target_uri);
	}
	
	gnome_vfs_uri_unref (source_base);
	gnome_vfs_uri_unref (target_base);

	gnome_vfs_directory_close (handle);
	
	gnome_vfs_file_info_unref (file_info);
}

static gint
sync_callback (GnomeVFSXferProgressInfo *info,
	       gpointer data)
{
	return 1;
}

static TransferDialog *dialog;

static void
signal_handler (int signum)
{
	if (signum != SIGINT) {
		return;
	}

	if (dialog == NULL) {
		return;
	}
	
	/* Cancel the nice way. */
	printf ("Caught Ctrl-C, cancelling.\n");
	
	dialog->cancelled = TRUE;
}

int
main (gint argc, gchar **argv)
{
	GList          *source_uris = NULL;
	GList          *target_uris = NULL;
	GnomeVFSResult  result;
	const gchar    *from;
	const gchar    *to;

	gnome_vfs_init ();
	
	gtk_init (&argc, &argv);

	if (argc < 3) {
		g_printerr ("Usage: %s <from-uri> <to-uri>\n"
			    "The URIs should point to two directories.\n",
			    argv[0]);
		return 1;
	}

	from = argv[1];
	to = argv[2];
	
	dialog = create_transfer_dialog ();

	/* Catch Ctrl-C so that we can cancel correctly. */
	signal (SIGINT, signal_handler);
	
	create_uri_lists (
		from, to,
		&source_uris, &target_uris);
	
	result = ovu_async_xfer (&dialog->handle,
                                 source_uris, target_uris,
                                 GNOME_VFS_XFER_RECURSIVE,
                                 GNOME_VFS_XFER_ERROR_MODE_ABORT,
                                 GNOME_VFS_XFER_OVERWRITE_MODE_REPLACE,
                                 GNOME_VFS_PRIORITY_DEFAULT,
                                 update_callback, dialog,
                                 sync_callback, dialog);

	gtk_main ();

	return 0;
}
