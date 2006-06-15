#include <config.h>

#include <stdio.h>
#include <glib-object.h>
#include <libgnomevfs/gnome-vfs.h>
#include <libgnomevfs/gnome-vfs-mime-info.h>

int
main (int argc, char **argv)
{
	int i;
	GTimer *timer = g_timer_new ();

	g_type_init ();
	gnome_vfs_init (); /* start threads */

	g_timer_start (timer);

	for (i = 0; i < 10; i++) {
		gnome_vfs_mime_info_reload ();
	}

	fprintf (stderr, "mime parse took %g(ms)\n",
		 g_timer_elapsed (timer, NULL) * 100);

	return 0;
}
