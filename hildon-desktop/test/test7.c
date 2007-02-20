#include <libhildonwm/hd-wm.h>

gboolean  
button_press (gpointer data)
{
  HDWM *hdwm = (HDWM *)data;
  GList *l = NULL;
  static gint i=0;

  for (l = hd_wm_get_applications (hdwm); l ; l = g_list_next (l))
  {
    g_debug ("Iteration: %d",i++);
    hd_wm_close_application (hdwm,(HDEntryInfo *)l->data);
  }
 
  return FALSE;
}

int 
main (int argc, char **argv)
{
  HDWM *hdwm;
  GList *l = NULL;
  gint i;
  
  gtk_init (&argc,&argv);

  hdwm = hd_wm_get_singleton (); 
  
  hd_wm_update_client_list (hdwm);

  i = g_timeout_add (2000,(GSourceFunc)button_press,(gpointer)hdwm);
	 
  gtk_main ();
  
  return 0;
}
