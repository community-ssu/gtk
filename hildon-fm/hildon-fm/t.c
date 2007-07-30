#include <gtk/gtk.h>

GtkWidget *scrolled (GtkWidget *);
void clicked (GtkWidget *w, gpointer d);

int cur;
GtkWidget *labels[3];
GtkWidget *hbox;

GtkWidget *
scrolled (GtkWidget *w)
{
  GtkWidget *scroller = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroller),
				  GTK_POLICY_AUTOMATIC,
				  GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scroller),
					 w);
  return scroller;
}

void
clicked (GtkWidget *w, gpointer d)
{
  gtk_widget_hide (labels[cur]);
  cur = (cur + 1) % 3;
  gtk_widget_show (labels[cur]);
}

int
main(int argc, char **argv)
{
  int i;

  GtkWidget *window, *hpaned, *button;
  
  gtk_init (NULL, NULL);

  labels[0] = scrolled (gtk_label_new ("Label 1 - Arrrrrrrrrrrrrrrrrrrr"));
  labels[1] = gtk_label_new ("Label 2 - Arrrrrrrrrrrrrrr");
  labels[2] = scrolled (gtk_label_new ("Label 3 - Arrrrrrrrrrrrrrrrrrrrrrrrrrrrr"));

  hbox = gtk_hbox_new (FALSE, 0);
  for (i = 0; i < 3; i++)
    gtk_box_pack_start (GTK_BOX (hbox), labels[i], TRUE, TRUE, 0);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  button = gtk_button_new_with_label ("Foooooooooooooooooooooooooooooooooooo");
  g_signal_connect (button, "clicked",
		    G_CALLBACK (clicked), NULL);

  hpaned = gtk_hpaned_new ();
  gtk_paned_pack1 (GTK_PANED (hpaned),
		   scrolled (button),
		   TRUE, FALSE);
  gtk_paned_pack2 (GTK_PANED (hpaned),
		   hbox,
		   TRUE, FALSE);

  gtk_container_add (GTK_CONTAINER (window), hpaned);

  gtk_widget_show_all (window);
  cur = 0;
  gtk_widget_hide (labels[1]);
  gtk_widget_hide (labels[2]);

  gtk_main ();

  return 0;
}
