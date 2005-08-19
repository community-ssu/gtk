#include <libosso.h>
#include <stdio.h>
#include "app-killer.h"

gint handler(const gchar *iface, const char *method,
                  GArray *args, gpointer data, osso_rpc_t *retval)
{
       return OSSO_OK;
}

DBusHandlerResult ses_filter(DBusConnection* c,
                         DBusMessage* m,
                         void* user_data)
{
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

DBusHandlerResult sys_filter(DBusConnection* c,
                         DBusMessage* m,
                         void* user_data)
{
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

int main()
{
	GMainLoop *loop = NULL;
	DBusConnection *dconn = NULL;
	osso_return_t rc = OSSO_ERROR;
	osso_context_t *osso = NULL;

	loop = g_main_loop_new(NULL, TRUE);

	osso = osso_initialize("osso_app_killer", "1", TRUE, NULL);
	if (osso == NULL) return 1;

	rc = osso_rpc_set_cb_f(osso, SVC_NAME, BU_START_OP, BU_START_IF,
			handler, NULL);
	if (rc != OSSO_OK) return 1;
	dconn = (DBusConnection*) osso_get_dbus_connection(osso);
	if (dconn == NULL) return 1;
	if (!dbus_connection_add_filter(dconn, ses_filter, NULL, NULL)) {
		return 1;
	}
	dconn = (DBusConnection*) osso_get_sys_dbus_connection(osso);
	if (dconn == NULL) {
		printf("getting system bus connection failed\n");
		return 1;
	}
	if (!dbus_connection_add_filter(dconn, sys_filter, NULL, NULL)) {
		printf("setting system bus filter failed\n");
		return 1;
	}

	osso_deinitialize(osso);
	printf("sleeping 15 secs...");
	sleep(15);
	printf("done\n");
	osso = osso_initialize("A", "1", TRUE, NULL);
	if (osso == NULL) {
		printf("second init failed\n");
		return 1;
	}
	printf("success\n");
	return 0;
}
