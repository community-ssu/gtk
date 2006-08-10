#include <stdio.h>
#include <glib.h>
#include <libosso.h>

static GMainLoop *loop;

void cb(gpointer data)
{
        printf("time changed signal received\n");
}

gboolean timeout_func(gpointer data)
{
        osso_context_t *context = data;
        printf("timeout function\n");
        osso_time_set(context, 6666);
        return FALSE;
}

int main(int argc, char* argv[])
{
        osso_context_t *context;

	loop = g_main_loop_new(NULL, 1);
        context = osso_initialize("timesignaltest", "0.1", 0,
                                  g_main_loop_get_context(loop));

        osso_time_set_notification_cb(context, cb, NULL);
        g_timeout_add(2000, timeout_func, context);

        g_main_loop_run(loop);
        return 0;
}
