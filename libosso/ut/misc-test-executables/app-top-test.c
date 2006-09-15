#include <stdio.h>
#include <glib.h>
#include <libosso.h>

static GMainLoop *loop;

int main(int argc, char* argv[])
{
        osso_context_t *context;

	loop = g_main_loop_new(NULL, 1);
        context = osso_initialize("toptest", "0.1", 0,
                                  g_main_loop_get_context(loop));

        osso_application_top(context, "osso_filemanager", "dummy");
        osso_application_top(context, "foo.bar.inittest", "dummy");

        g_main_loop_run(loop);
        return 0;
}
