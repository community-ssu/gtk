#include <stdio.h>
#include <glib.h>
#include <libosso.h>

static GMainLoop *loop;

void cb(osso_display_state_t state, gpointer data)
{
        printf("display signal received: %d\n", state);
}

int main(int argc, char* argv[])
{
        osso_context_t *context;

	loop = g_main_loop_new(NULL, 1);
        context = osso_initialize("displaysigtest", "0.1", 0,
                                  g_main_loop_get_context(loop));

        osso_hw_set_display_event_cb(context, cb, NULL);

        g_main_loop_run(loop);

        osso_deinitialize(context);

        return 0;
}
