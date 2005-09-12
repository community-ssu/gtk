#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>

#include <glib.h>
#include <gtk/gtk.h>

#include <hildon-status-bar-lib/hildon-status-bar-item.h>

#include "libhelloworld.h"

#define HILDON_STATUS_BAR_PRIORITY  1

typedef struct
{
  HildonStatusBarItem  *item;
  GtkWidget            *button;
} PluginInfo;

/* The exported function.
 */
void helloworld_sb_entry (HildonStatusBarPluginFn_st *fn);

static void *
initialize (HildonStatusBarItem *item, GtkWidget **button)
{
  PluginInfo *info = NULL;

  g_return_val_if_fail (item, NULL);

  info = g_new0 (PluginInfo, 1);

  info->item = item;
  info->button = hello_world_button_new ();
  g_signal_connect (G_OBJECT (info->button), "clicked",
		    G_CALLBACK (hello_world_dialog_show),
		    NULL);
		    
  *button = info->button;

  return info;
}

static void
destroy (void *data)
{
  PluginInfo *info =  (PluginInfo*)data;
  g_return_if_fail (info);
  g_free (info);
}

static gint
get_priority (void *data)
{
  return HILDON_STATUS_BAR_PRIORITY;
}

void
update  (void *data, gint value1, gint value2, const gchar *str)
{
  PluginInfo *info = (PluginInfo*)data;
  g_return_if_fail (info);
  
  if (value2 == 0)
    gtk_widget_destroy (GTK_WIDGET(info->item));
  else
    gtk_widget_show_all (info->button);
  return;
}

void
helloworld_sb_entry (HildonStatusBarPluginFn_st *fn)
{
  g_return_if_fail (fn);
  fn->initialize   = initialize;
  fn->destroy      = destroy;
  fn->update       = update;
  fn->get_priority = get_priority;
}
