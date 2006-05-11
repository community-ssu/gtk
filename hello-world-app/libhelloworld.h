#ifndef LIBHELLOWORLD_H
#define LIBHELLOWORLD_H

#include <gtk/gtk.h>

GtkWindow *hello_world_new (void);
GtkDialog *hello_world_dialog_new (void);
GtkWidget *hello_world_button_new (int padding);

void hello_world_dialog_show (void);

#endif /* !LIBHELLOWORLD_H */
