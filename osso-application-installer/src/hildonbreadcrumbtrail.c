#include "hildonbreadcrumbtrail.h"

static hildon_bread_crumb_trail_get_label *get_label;
static hildon_bread_crumb_trail_clicked *clicked;
static GList *path;

GtkWidget *
hildon_bread_crumb_trail_new (hildon_bread_crumb_trail_get_label *gl,
			      hildon_bread_crumb_trail_clicked *cl)
{
  get_label = gl;
  clicked = cl;
  return gtk_hbox_new (FALSE, 0);
}

static void
btn_callback (GtkWidget *btn, gpointer data)
{
  if (clicked)
    clicked (data);
}

void
hildon_bread_crumb_trail_set_path (GtkWidget *trail, GList *path)
{
  GList *children = gtk_container_get_children (GTK_CONTAINER (trail));
  while (children)
    {
      GList *next;
      gtk_widget_destroy (GTK_WIDGET (children->data));
      next = children->next;
      g_list_free_1 (children);
      children = next;
    }

  while (path)
    {
      const gchar *label = get_label (path);
      if (path->next)
	{
	  GtkWidget *btn, *arw;

	  btn = gtk_button_new_with_label (label);
	  gtk_box_pack_start (GTK_BOX (trail), btn, FALSE, FALSE, 0);
	  g_signal_connect (G_OBJECT (btn), "clicked",
			    G_CALLBACK (btn_callback),
			    path);
	  gtk_widget_show (btn);
	  arw = gtk_arrow_new (GTK_ARROW_RIGHT, GTK_SHADOW_OUT);
	  gtk_widget_set_size_request (arw, 40, 40);
	  gtk_box_pack_start (GTK_BOX (trail), arw, FALSE, FALSE, 0);
	  gtk_widget_show (arw);
	}
      else
	{
	  GtkWidget *lbl;
	  lbl = gtk_label_new (label);
	  gtk_box_pack_start (GTK_BOX (trail), lbl, FALSE, FALSE, 0);
	  gtk_widget_show (lbl);
	}
      path = path->next;
    }
}
