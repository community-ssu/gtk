#include <unistd.h>
#include <libgnomevfs/gnome-vfs.h>
#include <libgnomevfs/gnome-vfs-mime-info-cache.h>
#include <libgnomevfs/gnome-vfs-mime-handlers.h>
#include <libgnomevfs/gnome-vfs-mime-monitor.h>

static void mime_cache_info_reload (void);

static gpointer foo (const char *mime_type);

static void mime_cache_info_reload (void)
{
        g_print ("mime cache reloaded...\n");
}

static gpointer foo (const char *mime_type) {
        GList *desktop_file_apps, *tmp; 

        while (1) {
                g_print ("Default: %s\n",
                                gnome_vfs_mime_get_default_desktop_entry (mime_type));

                desktop_file_apps = gnome_vfs_mime_get_all_applications (mime_type);

                g_print ("All:\n");
                tmp = desktop_file_apps;
                while (tmp != NULL) {
                        GnomeVFSMimeApplication *application;

                        application = (GnomeVFSMimeApplication *) tmp->data;
                        g_print ("%s, %s\n", application->id, application->name);
                        tmp = tmp->next;
                }
                g_usleep (1000000);
        }
}

int main (int argc, char **argv)
{
        GMainLoop *main_loop;
        char *mime_type;
        int i;

        gnome_vfs_init ();

        if (argc > 1) {
                mime_type = argv[1];
        } else {
                mime_type = "text/plain";
        }

        i = 1;
        while (i--) {
                (void) g_thread_create ((GThreadFunc) foo, mime_type, FALSE, NULL);
        }

        g_signal_connect (G_OBJECT (gnome_vfs_mime_monitor_get ()),
                          "data_changed", 
                           (GCallback) mime_cache_info_reload,
                           NULL);

        main_loop = g_main_loop_new (NULL, FALSE);

        g_main_loop_run (main_loop);

        return 0;
}
