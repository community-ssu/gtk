#include <stdio.h>
#include <glib.h>
#include <libosso.h>

static GMainLoop *loop;

void cb(osso_hw_state_t *state, gpointer data)
{
        if (state->memory_low_ind) {
                printf("low memory signal received\n");
        }
}

int main(int argc, char* argv[])
{
        osso_context_t *context;

	loop = g_main_loop_new(NULL, 1);
        context = osso_initialize("lowmemtest", "0.1", 0,
                                  g_main_loop_get_context(loop));

        osso_hw_set_event_cb(context, NULL, cb, NULL);

        g_main_loop_run(loop);
        return 0;
}
