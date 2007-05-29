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
}

gboolean
close_notification (gpointer n)
{
  notify_notification_close (NOTIFY_NOTIFICATION (n), NULL);

  return FALSE;
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
  			       "system.note.dialog");

  /*notify_notification_set_timeout (NOTIFY_NOTIFICATION (n), 4000);*/

  notify_notification_set_hint_int32 (NOTIFY_NOTIFICATION (n), "dialog-type", 4);
  
  hildon_notification_add_dbus_action (n, 
  		                       "default", 
  				       "",
  				       "com.nokia.controlpanel",
  				       "/com/nokia/controlpanel",
  				       "com.nokia.controlpanel",
  				       "top_application",
  				       G_TYPE_STRING, "Test",
  				       G_TYPE_INT, 21,
  				       G_TYPE_DOUBLE, 21.21,
  				       -1);

  g_signal_connect (G_OBJECT (n), "closed", G_CALLBACK (closed), NULL);

  if (!notify_notification_show (NOTIFY_NOTIFICATION (n), NULL)) 
  {
    g_warning ("Failed to send notification");

    return 1;
  }

  /*g_timeout_add (5000, close_notification, n);*/
  
  g_main_loop_run (loop);
  
  return 0;
}
