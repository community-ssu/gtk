#include <glib.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>

#include <hildon/hildon-notification.h>
#include <libnotify/notify.h>

static GMainLoop *loop;

int
main(int argc, char **argv)
{
  HildonNotification *n;
  DBusConnection *conn;
  
  if (!notify_init ("D-Bus Action Test")) exit(1);
  
  conn = dbus_bus_get (DBUS_BUS_SESSION, NULL);
  
  loop = g_main_loop_new (NULL, FALSE);
  
  dbus_connection_setup_with_g_main (conn, NULL);

  hildon_notification_helper_show (0,
		  		   "Summary", 
		                   "Body of the notification",
				   "qgn_list_help",
				   "system.note.dialog",
				   0,
				   FALSE,
  				   "com.nokia.controlpanel",
  				   "/com/nokia/controlpanel",
  				   "com.nokia.controlpanel",
  				   "top_application",
  				   -1);

  g_main_loop_run (loop);
  
  return 0;
}
