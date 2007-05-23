#define GTK_FILE_SYSTEM_ENABLE_UNSUPPORTED

#include <gtk/gtk.h>
#include <gtk/gtkfilesystem.h>
#include <libgnomevfs/gnome-vfs.h>

#include <stdio.h>
#include <stdlib.h>

GtkFileSystem *fs;

static void
list_gnome_vfs_volumes ()
{
  GnomeVFSVolumeMonitor *monitor;
  GList *volumes, *drives;

  monitor = gnome_vfs_get_volume_monitor ();

  fprintf (stderr, "Root entries:\n");

  volumes = gnome_vfs_volume_monitor_get_mounted_volumes (monitor);
  while (volumes)
    {
      GnomeVFSVolume *volume = volumes->data;

      if (gnome_vfs_volume_is_user_visible (volume)
          && gnome_vfs_volume_is_mounted (volume))
        fprintf (stderr,
                 " %s %s\n",
                 gnome_vfs_volume_get_device_path (volume),
                 gnome_vfs_volume_get_activation_uri (volume));
      else
        fprintf (stderr,
                 " (%s %s)\n",
                 gnome_vfs_volume_get_device_path (volume),
                 gnome_vfs_volume_get_activation_uri (volume));

      volumes = volumes->next;
    }

  drives = gnome_vfs_volume_monitor_get_connected_drives (monitor);
  while (drives)
    {
      GnomeVFSDrive *drive = drives->data;

      if (gnome_vfs_drive_is_user_visible (drive)
          && gnome_vfs_drive_is_connected (drive)
          && !gnome_vfs_drive_is_mounted (drive))
        fprintf (stderr,
                 " %s\n",
                 gnome_vfs_drive_get_device_path (drive));
      else
        fprintf (stderr,
                 " (%s)\n",
                 gnome_vfs_drive_get_device_path (drive));


      drives = drives->next;
    }

  fprintf (stderr, "\n");

}

static void
volumes_changed ()
{
  printf ("changed\n");
  list_gnome_vfs_volumes ();
}

int
main (int argc, char **argv)
{
  gtk_init (&argc, &argv);

  fs = gtk_file_system_create ("gnome-vfs");
  if (fs == NULL)
    exit (1);

  volumes_changed ();

  g_signal_connect (fs, "volumes_changed",
                    G_CALLBACK (volumes_changed), NULL);

  gtk_main ();
  return 0;
}

