#include <gtk/gtk.h>

#define GTK_FILE_SYSTEM_ENABLE_UNSUPPORTED
#include <gtk/gtkfilesystem.h>

static void
create_folder_callback (GtkFileSystemHandle *handle,
			const GtkFilePath *path,
			const GError *error, gpointer data)
{
  if (error)
    fprintf (stderr, "%s: %s\n",
	     gtk_file_path_get_string (path),
	     error->message);
  else
    fprintf (stderr, "%s: success\n",
	     gtk_file_path_get_string (path));

  gtk_main_quit ();
}

int
main(int argc, char **argv)
{
  GtkFileSystem *fs;
  GtkFilePath *p;
  GtkFileSystemHandle *h;

  g_thread_init (NULL);
  gtk_init (NULL, NULL);

  if (argc > 1)
    {
      fs = gtk_file_system_create ("gnome-vfs");
      p = gtk_file_system_filename_to_path (fs, argv[1]);
      h = gtk_file_system_create_folder (fs, p,
					 create_folder_callback,
					 NULL);
  
      if (h)
	gtk_main ();
    }

  return 0;
}
