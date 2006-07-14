/*
 * This file is part of maemo-af-desktop
 *
 * Copyright (C) 2005,2006 Nokia Corporation.
 *
 * Contact: Karoliina Salminen <karoliina.t.salminen@nokia.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

#include <stdlib.h>
#include <string.h>
#include <gdk/gdkx.h>

#include "hn-wm-util.h"

#define BUF_SIZE 80

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

/* Function to retrieve the size of VmData for a process
 * Returns -1 on failure
 */
gint hn_wm_get_vmdata_for_pid(gint pid)
{
  gchar *fname;
  const gchar str[7] = "VmData";
  gchar buf[BUF_SIZE];
  int c;
  gint i;
  gboolean read = FALSE;
  FILE *f;
  
  fname = g_strdup_printf("/proc/%i/status", pid);
  f = fopen(fname, "r");

  if (f == NULL)
    {
      g_print("No process data available for %s\n", fname);
      g_free (fname);
      return -1;
    }

  g_free (fname);

  do
  {
    c = fgetc(f);
    if (c == EOF)
      {
        break;
      }
    /* Match incrementally until we find the string "VmData" */
    for (i = 0; i < 6; i++)
    {
      if (c != str[i])
        {
          break;
        }
      c = fgetc(f);
      if (i == 5)
        {
          read = TRUE;
          break;
        }
    }
    if (read == TRUE)
      {
        /* Skip extra chars */
        while (c != 32 && c != EOF && c != '\n')
          {
            c = fgetc(f);
          }
        /* Skip whitespace */
        while (c == 32 && c != EOF && c != '\n')
          {
            c = fgetc(f);
          }

        /* Read the number */
        i = 0;
        while (c != 32 && c != EOF && c != '\n' && i < BUF_SIZE)
          {
            buf[i] = c;
            i++;
            c = fgetc(f);
          }
        fclose (f);
        return (atoi(buf));
        break;
      }
    
  } while (c != EOF);

  /* Failed, return -1 */

  return -1;
}
