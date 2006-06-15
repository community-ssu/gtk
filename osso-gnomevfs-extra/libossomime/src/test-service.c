#include <config.h>
#include <string.h>
#include <stdlib.h>

#define DBUS_API_SUBJECT_TO_CHANGE
#include <dbus/dbus.h>
#include <dbus/dbus-glib-lowlevel.h>

static GMainLoop   *mainloop = NULL;

static const gchar *interface;
static const gchar *service;
static const gchar *path;
static const gchar *method;

static DBusHandlerResult
service_message_func (DBusConnection  *connection,
		      DBusMessage     *message,
		      void            *user_data)
{
	DBusError        error;
	DBusMessage     *reply;
	DBusMessageIter  iter;
	gint             i = 0;

	g_print ("Got message: %s\n", dbus_message_get_member (message));
	
	dbus_error_init (&error);

	if (!dbus_message_iter_init (message, &iter)) {
		reply = dbus_message_new_error (message,
						error.name,
						error.message);
		
		if (!reply) {
			g_printerr ("Could not create dbus message error\n");
			return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
		}
		
		if (!dbus_connection_send (connection, reply, NULL)) {
			g_printerr ("Could not send dbus message error\n");
			return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
		}
		
		dbus_message_unref (reply);
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	}

	do {
		gchar *str;

		dbus_message_iter_get_basic (&iter, &str);
		g_print ("Item:%d is '%s'\n", ++i, str);
	} while (dbus_message_iter_next (&iter));

	reply = dbus_message_new_method_return (message);
	if (!reply) {
		g_printerr ("Could not create dbus return method\n");
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	}
	
	if (!dbus_connection_send (connection, reply, NULL)) {
		g_printerr ("Could not send dbus return method\n");
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	}
	
	dbus_message_unref (reply);
	
	return DBUS_HANDLER_RESULT_HANDLED;
}

static void 
service_unregistered_func (DBusConnection  *connection,
			   void            *user_data)
{
	g_print ("Service unregistered\n");
}


static DBusObjectPathVTable service_vtable = {
	service_unregistered_func,
	service_message_func,
	NULL
};

int
main (int argc, char **argv)
{
	DBusError       error;
	int             result;
	DBusConnection *connection;
	
        if (argc < 5) {
                gchar *cmd;

                cmd = g_path_get_basename (argv[0]);

                g_printerr ("Usage:   %s <service> <interface> <path> <method>\n"
                            "Example: %s com.nokia.test com.nokia.test /com/nokia/browser http\n"
			    "\n"
			    "Runs as a DBus service, waits for connections and prints the list\n"
			    "of strings sent.\n"
			    "\n"
			    "Run this first then use test-uri with the appropriate scheme to\n"
			    "connect to this service. For example, use:\n"
			    "\t./test-uri -u http://www.google.com -u http://www.gnome.org\n"
			    "\n"
			    "For this to work you have to make sure that the \"http\" service\n"
			    "and method are correct in the corresponding desktop files in:\n"
			    "\t$prefix/share/applications\n"
			    "\n"
			    "The important keys in the desktop files are:\n"
			    "\t\"X-Osso-Service=com.nokia.browser\" [1]\n"
			    "\t\"Method=http\" [2]\n"
			    "\n"
			    "Where:\n"
			    "[1] is the <service> and <interface> (and path with slashes not dots)\n"
			    "[2] is the <method> (found in \"[X-Osso-URI-Action Handler SCHEME]\" sections)\n"
			    "\n",
                            cmd, cmd);
                g_free (cmd);
                return 1;
        }
	
	service = argv[1];
	interface = argv[2];
	path = argv[3];
	method = argv[4];
		
	dbus_error_init (&error);

	connection = dbus_bus_get (DBUS_BUS_SESSION, &error);
	if (!connection) {
		g_printerr ("Failed to open session connection to activating message bus:'%s'\n",
			    error.message);
		dbus_error_free (&error);
		return 1;
	}

	dbus_connection_setup_with_g_main (connection, NULL);

	if (!dbus_connection_register_object_path (connection,
						   path,
						   &service_vtable,
						   NULL)) {
		g_printerr ("Failed to register object path:'%s'\n", path);
		return 1;
	}
	
	result = dbus_bus_request_name (connection, 
					service,
					0, 
					&error);

	if (dbus_error_is_set (&error))	{
		g_printerr ("Failed to acquire service:'%s', error:'%s'\n",
			    service, error.message);
		dbus_error_free (&error);
		return 1;
	}
  
	g_print ("Waiting for DBus messages...\n");

	mainloop = g_main_loop_new (NULL, FALSE);
	g_main_loop_run (mainloop);

	return 0;
}

