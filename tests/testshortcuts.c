#include <gtk/gtk.h>

static void
on_activate (GtkWidget *item)
{
  g_debug ("activate: %s", gtk_label_get_text (GTK_LABEL (GTK_BIN (item)->child)));
}

static void
on_clicked (GtkWidget *widget, gboolean enable)
{
  gtk_settings_set_long_property (gtk_widget_get_settings (widget),
				  "hildon-keyboard-shortcuts", enable, "user");
}

static gboolean
on_timeout (gpointer data)
{
  GtkWidget *widget = data;
  GtkSettings *settings;
  gboolean keyboard_shortcuts;

  settings = gtk_widget_get_settings (widget);
  g_object_get (settings, "hildon-keyboard-shortcuts", &keyboard_shortcuts, NULL);
  gtk_settings_set_long_property (settings, "hildon-keyboard-shortcuts",
				  !keyboard_shortcuts, "user");

  return TRUE;
}

static void
menu_item_add (GtkWidget *menu, GtkWidget *item, const char *shortcut)
{
  if (shortcut)
    {
      guint key;
      GdkModifierType mods;

      gtk_accelerator_parse (shortcut, &key, &mods);
      g_assert (key != 0);
      g_assert (mods != 0);
      gtk_widget_add_accelerator (item, "activate",
				  gtk_menu_get_accel_group (GTK_MENU (menu)),
				  key, mods, GTK_ACCEL_VISIBLE);
    }

  g_signal_connect (item, "activate", G_CALLBACK (on_activate), NULL);
  gtk_container_add (GTK_CONTAINER (menu), item);
}

int
main (int argc, char **argv)
{
  GtkWidget *window;
  GtkWidget *vbox;
  GtkWidget *menu;
  GtkWidget *item;
  GtkAccelGroup *accel_group;
  GtkWidget *button;

  if (1)
  gtk_rc_parse_string (
"style \"accel-label\" {\n"
"  fg[NORMAL] = \"#ff0000\"\n"
"  fg[PRELIGHT] = \"#00ffff\"\n"
"  font_name = \"Sans 7\"\n"
"}\n"
"widget_class \"*.GtkAccelLabel.GtkAccelLabel\" style : highest \"accel-label\"\n"
);

  gtk_init (&argc, &argv);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  g_signal_connect (window, "destroy", G_CALLBACK (gtk_main_quit), 0);

  accel_group = gtk_accel_group_new ();
  gtk_window_add_accel_group (GTK_WINDOW (window), accel_group);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (window), vbox);

  menu = gtk_menu_bar_new ();
  gtk_box_pack_start (GTK_BOX (vbox), menu, FALSE, FALSE, 0);
  item = gtk_menu_item_new_with_mnemonic ("_File");
  gtk_container_add (GTK_CONTAINER (menu), item);

  menu = gtk_menu_new ();
  gtk_menu_set_accel_group (GTK_MENU (menu), accel_group);
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), menu);

  menu_item_add (menu, gtk_menu_item_new_with_label ("Label"), NULL);
  menu_item_add (menu, gtk_menu_item_new_with_mnemonic ("_Mnemonic"), NULL);
  menu_item_add (menu, gtk_menu_item_new_with_label ("Label + Shortcut"), "<Ctrl>A");
  menu_item_add (menu, gtk_menu_item_new_with_mnemonic ("Mnemonic + _Shortcut"), "<Ctrl>B");
  menu_item_add (menu, gtk_menu_item_new_with_label ("Ampersand"), "<Ctrl>ampersand");
  menu_item_add (menu, gtk_menu_item_new_with_label ("Less"), "<Ctrl>less");
  menu_item_add (menu, gtk_menu_item_new_with_label ("Greater"), "<Ctrl>greater");

  button = gtk_button_new_with_mnemonic ("_Enable");
  g_signal_connect (button, "clicked", G_CALLBACK (on_clicked), GUINT_TO_POINTER (TRUE));
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);

  button = gtk_button_new_with_mnemonic ("_Disable");
  g_signal_connect (button, "clicked", G_CALLBACK (on_clicked), GUINT_TO_POINTER (FALSE));
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);

  if (1)
    g_timeout_add (2000, on_timeout, window);

  gtk_widget_show_all (window);

  gtk_main ();

  return 0;
}

