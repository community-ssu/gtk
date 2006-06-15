#include <libgnomevfs/gnome-vfs.h>
#include <libgnomevfs/gnome-vfs-volume-monitor.h>
#include <libgnomevfs/gnome-vfs-volume.h>

static GMainLoop *loop;

static void
print_volume (GnomeVFSVolume *volume)
{
	char *path, *uri, *name, *icon;
	GnomeVFSDrive *drive;

	path = gnome_vfs_volume_get_device_path (volume);
	uri = gnome_vfs_volume_get_activation_uri (volume);
	icon = gnome_vfs_volume_get_icon (volume);
	name = gnome_vfs_volume_get_display_name (volume);
	drive = gnome_vfs_volume_get_drive (volume);
	g_print ("vol(%p)[dev: %s, mount: %s, device type: %d, handles_trash: %d, icon: %s, name: %s, user_visible: %d, drive: %p]\n",
		 volume,
		 path?path:"(nil)",
		 uri,
		 gnome_vfs_volume_get_device_type (volume),
		 gnome_vfs_volume_handles_trash (volume),
		 icon,
		 name,
		 gnome_vfs_volume_is_user_visible (volume),
		 drive);
	g_free (path);
	g_free (uri);
	g_free (icon);
	g_free (name);
	gnome_vfs_drive_unref (drive);
}

static void
print_drive (GnomeVFSDrive *drive)
{
	char *path, *uri, *name, *icon;
	GnomeVFSVolume *volume;

	path = gnome_vfs_drive_get_device_path (drive);
	uri = gnome_vfs_drive_get_activation_uri (drive);
	icon = gnome_vfs_drive_get_icon (drive);
	name = gnome_vfs_drive_get_display_name (drive);
	volume = gnome_vfs_drive_get_mounted_volume (drive);
	
	g_print ("drive(%p)[dev: %s, mount: %s, device type: %d, icon: %s, name: %s, user_visible: %d, volume: %p]\n",
		 drive,
		 path?path:"(nil)",
		 uri,
		 gnome_vfs_drive_get_device_type (drive),
		 icon,
		 name,
		 gnome_vfs_drive_is_user_visible (drive),
		 volume);
	g_free (path);
	g_free (uri);
	g_free (icon);
	g_free (name);
	gnome_vfs_volume_unref (volume);
}

static void
volume_mounted (GnomeVFSVolumeMonitor *volume_monitor,
		GnomeVFSVolume	       *volume)
{
	g_print ("Volume mounted: ");
	print_volume (volume);
}

static void
volume_unmounted (GnomeVFSVolumeMonitor *volume_monitor,
		  GnomeVFSVolume	       *volume)
{
	g_print ("Volume unmounted: ");
	print_volume (volume);
}

static void
drive_connected (GnomeVFSVolumeMonitor *volume_monitor,
		 GnomeVFSDrive	       *drive)
{
	g_print ("drive connected: ");
	print_drive (drive);
}

static void
drive_disconnected (GnomeVFSVolumeMonitor *volume_monitor,
		    GnomeVFSDrive	       *drive)
{
	g_print ("drive disconnected: ");
	print_drive (drive);
}

int
main (int argc, char *argv[])
{
  GnomeVFSVolumeMonitor *monitor;
  GList *l, *volumes, *drives;
  
  gnome_vfs_init ();

  monitor = gnome_vfs_get_volume_monitor ();

  g_signal_connect (monitor, "volume_mounted",
		    G_CALLBACK (volume_mounted), NULL);
  g_signal_connect (monitor, "volume_unmounted",
		    G_CALLBACK (volume_unmounted), NULL);
  g_signal_connect (monitor, "drive_connected",
		    G_CALLBACK (drive_connected), NULL);
  g_signal_connect (monitor, "drive_disconnected",
		    G_CALLBACK (drive_disconnected), NULL);
  
  volumes = gnome_vfs_volume_monitor_get_mounted_volumes (monitor);

  g_print ("Mounted volumes:\n");
  for (l = volumes; l != NULL; l = l->next) {
	  print_volume (l->data);
	  gnome_vfs_volume_unref (l->data);
  }
  g_list_free (volumes);

  drives = gnome_vfs_volume_monitor_get_connected_drives (monitor);

  g_print ("Connected drives:\n");
  for (l = drives; l != NULL; l = l->next) {
	  print_drive (l->data);
	  gnome_vfs_drive_unref (l->data);
  }
  g_list_free (drives);

  g_print ("Waiting for volume events:\n");
  loop = g_main_loop_new (NULL, FALSE);
  g_main_loop_run (loop);
  
  gnome_vfs_shutdown ();
  
  return 0;
}
