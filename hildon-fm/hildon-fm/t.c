#include <gtk/gtk.h>
#include <libgnomevfs/gnome-vfs-init.h>
#include "hildon-file-system-storage-dialog.h"

int
main(int argc, char **argv)
{
  GtkWidget *dlg = NULL;

  if (argc < 2) return 1;

  gtk_init(&argc, &argv);

  gnome_vfs_init ();

  gtk_dialog_run(GTK_DIALOG(dlg = hildon_file_system_storage_dialog_new(NULL,
argv[1])));

  gtk_widget_destroy(dlg);

  return 0;
}
