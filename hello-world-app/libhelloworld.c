#include "libhelloworld.h"

static void
window_destroy (GtkWidget* widget, gpointer data)
{
  gtk_main_quit ();
}

static void
button_clicked (GtkButton* button, gpointer data)
{
  gtk_widget_destroy (GTK_WIDGET (data));
}

GtkWindow *
hello_world_new (void)
{
  GtkWidget *window, *vbox, *label, *button;

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  gtk_container_set_border_width (GTK_CONTAINER (window), 20);
    
  gtk_window_set_title (GTK_WINDOW (window), "Hello World!");

  g_signal_connect (G_OBJECT (window), "destroy",
		    G_CALLBACK (window_destroy), NULL);

  vbox = gtk_vbox_new (FALSE, 10);
  label = gtk_label_new ("Hello World!");
  button = gtk_button_new_with_label("Close");

  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK (button_clicked), window);
    
  gtk_box_pack_start (GTK_BOX(vbox), label, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX(vbox), button, FALSE, FALSE, 0);
  
  gtk_container_add (GTK_CONTAINER (window), vbox);
  
  gtk_widget_show_all (window);

  return GTK_WINDOW (window);
}

GtkDialog *
hello_world_dialog_new ()
{
  GtkWidget *dialog;

  dialog = gtk_dialog_new_with_buttons ("Hello World",
					NULL,
					GTK_DIALOG_DESTROY_WITH_PARENT |
					GTK_DIALOG_NO_SEPARATOR,
					"Close",
					GTK_RESPONSE_OK,
					NULL);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox),
		     gtk_label_new ("Hello World!"));
  gtk_widget_show_all (dialog);

  return GTK_DIALOG (dialog);
}

void
hello_world_dialog_show ()
{
  GtkWidget *dialog = hello_world_dialog_new ();
  gtk_dialog_run (dialog);
  gtk_widget_destroy (dialog);
}

GtkWidget *
hello_world_button_new (int padding)
{
  GtkIconTheme *icon_theme;
  GdkPixbuf *icon;
  GtkWidget *icon_image, *button;

  icon_theme = gtk_icon_theme_get_default ();
  icon = gtk_icon_theme_load_icon (icon_theme,
				   "hello-world",
				   40,
				   0,
				   NULL);
  if (icon == NULL)
    icon = gtk_icon_theme_load_icon (icon_theme,
				     "qgn_list_gene_default_app",
				     40,
				     0,
				     NULL);
    
  icon_image = gtk_image_new_from_pixbuf (icon);
  gtk_misc_set_padding (GTK_MISC (icon_image), padding, padding);
  g_object_unref (G_OBJECT (icon));
  button = gtk_button_new ();
  gtk_container_add (GTK_CONTAINER (button), icon_image);

  gtk_widget_show_all (button);

  return button;
}
