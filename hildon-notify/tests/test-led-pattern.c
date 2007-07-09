#include <glib.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>

#include <hildon/hildon-notification.h>
#include <libnotify/notify.h>

static GMainLoop *loop;

void 
closed (NotifyNotification *n, gpointer user_data)
{
  g_debug ("NOTIFICATION CLOSED!");

  g_main_loop_quit (loop);
}

int
main(int argc, char **argv)
{
  HildonNotification *n;
  DBusConnection *conn;
  
  if (!notify_init ("D-Bus Action Test")) exit(1);
  
  conn = dbus_bus_get (DBUS_BUS_SESSION, NULL);
  
  loop = g_main_loop_new (NULL, FALSE);
  
  dbus_connection_setup_with_g_main (conn, NULL);
  
  n = hildon_notification_new ("John Doe", 
  		               "Hi! How are you!?", 
  			       "qgn_contact_group_chat_invitation", 
  			       "test");

  notify_notification_set_hint_string (NOTIFY_NOTIFICATION (n), 
		  		       "led-pattern", 
				       "PatternCommunicationEmail");  
  
  g_signal_connect (G_OBJECT (n), "closed", G_CALLBACK (closed), NULL);

  if (!notify_notification_show (NOTIFY_NOTIFICATION (n), NULL)) 
  {
    g_warning ("Failed to send notification");

    return 1;
  }

  g_main_loop_run (loop);
  
  return 0;
}
