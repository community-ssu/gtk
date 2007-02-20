#include <gtk/gtk.h>
#include <X11/Xlib.h>
#include <gdk/gdkx.h>
#include <gdk/gdkevents.h>

static void 
_send_message (GtkWidget *plug, GdkEvent *event, gpointer data)
{
  XEvent ev;
  Atom tray;
  Window tray_window;
  Display *dpy = GDK_DISPLAY_XDISPLAY (gtk_widget_get_display (plug));
  gchar *tray_string = g_strdup_printf ("_NET_SYSTEM_TRAY_S%d",
 	                         	GDK_SCREEN_XNUMBER (gdk_screen_get_default ()));

  tray = XInternAtom (dpy, tray_string, False);
  
  tray_window = XGetSelectionOwner (dpy, tray);

  if (tray_window == None)
  { 
    g_debug ("I couldnt get systray");
    g_free (tray_string);
    return;
  }
  
  g_debug ("SENDING %d",(GdkNativeWindow)tray_window);      

  memset(&ev, 0, sizeof(ev));
  ev.xclient.type = ClientMessage;
  ev.xclient.window = tray_window;
  ev.xclient.message_type = XInternAtom (dpy, "_NET_SYSTEM_TRAY_OPCODE", False);
  ev.xclient.format = 32;
  ev.xclient.data.l[0] = CurrentTime;
  ev.xclient.data.l[1] = 0;
  ev.xclient.data.l[2] = gtk_plug_get_id (GTK_PLUG (plug));
  ev.xclient.data.l[3] = 0;
  ev.xclient.data.l[4] = 0;

  gdk_error_trap_push ();
  
  XSendEvent (dpy, tray_window, False, NoEventMask, &ev);
  
  XSync (dpy, False);

  if (gdk_error_trap_pop ())
    g_debug ("Error");

  g_free (tray_string);
}

static void 
_show_dialog (GtkWidget *button, gpointer data)
{
  GtkWidget *dialog = gtk_dialog_new ();

  gtk_widget_grab_default (gtk_dialog_add_button (GTK_DIALOG (dialog),
			  			  GTK_STOCK_OK,
						  GTK_RESPONSE_OK));

  gtk_dialog_run (GTK_DIALOG (dialog));

  gtk_widget_destroy (dialog);
}

int 
main (int argc, char **argv)
{
  GtkWidget *button,*plug;
	
  gtk_init (&argc, &argv);

  plug    = gtk_plug_new (0);
  button = gtk_button_new ();

  gtk_button_set_image (GTK_BUTTON (button), 
		        gtk_image_new_from_stock (GTK_STOCK_CONVERT, GTK_ICON_SIZE_LARGE_TOOLBAR));

  gtk_container_add (GTK_CONTAINER (plug), button);

  g_signal_connect (plug,
		    "realize",
		    G_CALLBACK (_send_message),
		    NULL);

  g_signal_connect (button,
		    "clicked",
		    G_CALLBACK (_show_dialog),
		    NULL);

  gtk_widget_show_all (plug);

  gtk_main ();

  return 0;
}
