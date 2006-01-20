#include "hn-wm-util.h"

gulong 
hn_wm_util_getenv_long (gchar *env_str, gulong val_default)
{
    gchar *val_str;
    gulong val;

    if ((val_str = getenv(env_str)) != NULL)
      val = strtoul(val_str, NULL, 10);
    else
      val = val_default;

    return val;
}

void*
hn_wm_util_get_win_prop_data_and_validate (Window     xwin, 
					   Atom       prop, 
					   Atom       type, 
					   int        expected_format,
					   int        expected_n_items,
					   int       *n_items_ret)
{
  Atom           type_ret;
  int            format_ret;
  unsigned long  items_ret;
  unsigned long  after_ret;
  unsigned char *prop_data;
  int            status;

  prop_data = NULL;
  
  gdk_error_trap_push();

  status = XGetWindowProperty (GDK_DISPLAY(), 
			       xwin, 
			       prop, 
			       0, G_MAXLONG, 
			       False,
			       type, 
			       &type_ret, 
			       &format_ret, 
			       &items_ret,
			       &after_ret, 
			       &prop_data);


  if (gdk_error_trap_pop() || status != Success || prop_data == NULL)
    goto fail;

  if (expected_format && format_ret != expected_format)
    goto fail;

  if (expected_n_items && items_ret != expected_n_items)
    goto fail;

  if (n_items_ret)
    *n_items_ret = items_ret;
  
  return prop_data;

 fail:

  if (prop_data)
    XFree(prop_data);

  return NULL;
}

gboolean
hn_wm_util_send_x_message (Window        xwin_src, 
			   Window        xwin_dest, 
			   Atom          delivery_atom,
			   long          mask,
			   unsigned long data0,
			   unsigned long data1,
			   unsigned long data2,
			   unsigned long data3,
			   unsigned long data4)
{
  XEvent ev;

  memset(&ev, 0, sizeof(ev));
  ev.xclient.type = ClientMessage;

  ev.xclient.window       = xwin_src;
  ev.xclient.message_type = delivery_atom;
  ev.xclient.format       = 32;
  ev.xclient.data.l[0]    = data0;
  ev.xclient.data.l[1]    = data1;
  ev.xclient.data.l[2]    = data2;
  ev.xclient.data.l[3]    = data3;
  ev.xclient.data.l[4]    = data4;

  gdk_error_trap_push();
    
  XSendEvent(GDK_DISPLAY(), xwin_dest, mask, False, &ev);
  XSync(GDK_DISPLAY(), FALSE);
  
  if (gdk_error_trap_pop())
    {
      return FALSE;
    }

  return TRUE;
}
