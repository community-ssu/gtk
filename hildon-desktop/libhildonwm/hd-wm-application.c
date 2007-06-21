/* -*- mode:C; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/* 
 * This file is part of hildon-desktop
 *
 * Copyright (C) 2005, 2006, 2007 Nokia Corporation.
 *
 * Contact: Karoliina Salminen <karoliina.t.salminen@nokia.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>

#ifdef HAVE_LIBHILDON
#include <hildon/hildon-defines.h>
#include <hildon/hildon-banner.h>
#include <hildon/hildon-note.h>
#else
#include <hildon-widgets/hildon-defines.h>
#include <hildon-widgets/hildon-banner.h>
#include <hildon-widgets/hildon-note.h>
#endif


#define gettext(o) o
#define dgettext(o,e) o
#define _(o) o

#include "hd-wm.h"
#include "hd-wm-application.h"
#include "hd-wm-window.h"
#include "hd-entry-info.h"

#define SERVICE_PREFIX          "com.nokia."

G_DEFINE_TYPE (HDWMApplication, hd_wm_application, G_TYPE_OBJECT);

#define HD_WM_APPLICATION_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), HD_WM_TYPE_APPLICATION, HDWMApplicationPrivate))

typedef char HDWMApplicationFlags;

typedef enum 
{
  HDWM_APPLICATION_CAN_HIBERNATE  = 1 << 0,
  HDWM_APPLICATION_HIBERNATING    = 1 << 1,
  HDWM_APPLICATION_MINIMISED      = 1 << 2,
  HDWM_APPLICATION_STARTUP_NOTIFY = 1 << 3,
  HDWM_APPLICATION_DUMMY          = 1 << 4,
  HDWM_APPLICATION_LAUNCHING      = 1 << 5,
  /*
     Last dummy value for compile time check.
     If you need values > HDWM_APPLICATION_LAST, you have to change the
     definition of HDWMApplicationFlags to get more space.
  */
  HDWM_APPLICATION_LAST           = 1 << 7
}HDWMApplicationFlagsEnum;


/*
   compile time assert making sure that the storage space for our flags is
   big enough
*/
typedef char __app_compile_time_check_for_flags_size[
     (guint)(1<<(sizeof(HDWMApplicationFlags)*8))>(guint)HDWM_APPLICATION_LAST ? 1:-1
                                                    ];

#define HDWM_APPLICATION_SET_FLAG(a,f)    ((a)->priv->flags |= (f))
#define HDWM_APPLICATION_UNSET_FLAG(a,f)  ((a)->priv->flags &= ~(f))
#define HDWM_APPLICATION_IS_SET_FLAG(a,f) ((a)->priv->flags & (f))

/* watchable apps */

struct _HDWMApplicationPrivate
{
  gchar     *icon_name;
  gchar     *extra_icon;
  gchar     *service;
  gchar     *app_name; 		/* window title */
  gchar     *exec_name; 		/* class || exec field ? */
  gchar     *class_name;        
  gchar     *text_domain;        
  GtkWidget *ping_timeout_note; /* The note that is shown when the app quits responding */
  HDWMWindow    *active_window;
  HDWMApplicationFlags flags;
  HDEntryInfo          *info;
};

static void 
hd_wm_application_finalize (GObject *object)
{
  HDWMApplication *app = HD_WM_APPLICATION (object);
  
  if (app->priv->icon_name)
    g_free(app->priv->icon_name);
  
  if (app->priv->service)
    g_free(app->priv->service);

  if (app->priv->app_name)
    g_free(app->priv->app_name);
  
  if (app->priv->exec_name)
    g_free(app->priv->exec_name);

  if (app->priv->class_name)
    g_free(app->priv->class_name);

  if (app->priv->extra_icon)
    g_free(app->priv->extra_icon);
  
  if (app->priv->text_domain)
    g_free(app->priv->text_domain);
  
  if (app->priv->info)
    hd_entry_info_free (app->priv->info);
}	

static void 
hd_wm_application_init (HDWMApplication *app)
{
  app->priv = HD_WM_APPLICATION_GET_PRIVATE (app);

  app->priv->icon_name =
  app->priv->extra_icon =
  app->priv->service = 
  app->priv->app_name =
  app->priv->exec_name =         
  app->priv->class_name =
  app->priv->text_domain = NULL;
  
  app->priv->ping_timeout_note = NULL;

  app->priv->active_window = NULL;
  app->priv->info = NULL;
}	

static void 
hd_wm_application_class_init (HDWMApplicationClass *app_class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (app_class);

  object_class->finalize = hd_wm_application_finalize;

  g_type_class_add_private (app_class, sizeof (HDWMApplicationPrivate));
}	

static gboolean
hd_wm_application_has_windows_find_func (gpointer key,
					   gpointer value,
					   gpointer user_data);

static void
hd_wm_launch_banner_info_free (HDWMLaunchBannerInfo* info)
{
  g_return_if_fail(info);
  
  g_free(info->msg);
  g_free(info);
}

HDWMApplication*
hd_wm_application_new_dummy (void)
{
  HDWMApplication *app;

  app = HD_WM_APPLICATION (g_object_new (HD_WM_TYPE_APPLICATION, NULL));

  if (!app)
    return NULL;

  app->priv->icon_name  = g_strdup("qgn_list_gene_default_app"); 
  app->priv->app_name   = g_strdup("?");
  app->priv->class_name = g_strdup("?");

  HDWM_APPLICATION_SET_FLAG (app, HDWM_APPLICATION_DUMMY);
  
  g_debug ("Registered new non-watchable app\n\tapp_name: "
         "%s\n\tclass name: %s\n\texec name (service): %s", 
         app->priv->app_name, app->priv->class_name, app->priv->service);

  return app;
}

HDWMApplication*
hd_wm_application_new (const gchar *file)
{
  HDWMApplication *app = NULL;
  gchar            *service, *icon_name, *startup_notify;
  gchar            *startup_wmclass, *exec_name, *app_name, *text_domain;
  GKeyFile         *key_file;

  g_return_val_if_fail (file,NULL);
 
  key_file = g_key_file_new ();

  if (!key_file) 
    return NULL;

  if (!g_key_file_load_from_file (key_file, file, G_KEY_FILE_NONE, NULL))
  {
    g_warning ("Could not load keyfile [%s]", file);
    g_key_file_free (key_file);
    return NULL;
  }
  
  app_name = g_key_file_get_value(key_file,
                                  "Desktop Entry",
                                  DESKTOP_VISIBLE_FIELD,
                                  NULL);

  startup_wmclass = g_key_file_get_value(key_file,
                                         "Desktop Entry",
                                         DESKTOP_SUP_WMCLASS,
                                         NULL);

  exec_name = g_key_file_get_value(key_file,
                                   "Desktop Entry",
                                   DESKTOP_EXEC_FIELD,
                                   NULL);
  
  if (app_name == NULL || (exec_name == NULL && startup_wmclass == NULL))
  {
    g_warning ("HN: Desktop file has invalid fields");
    g_free(app_name);
    g_free(exec_name);
    g_free(startup_wmclass);
    return NULL;
  }
  
  /* DESKTOP_LAUNCH_:FIELD maps to X-Osso-Service */
  service = g_key_file_get_value(key_file,
                                 "Desktop Entry",
                                 DESKTOP_LAUNCH_FIELD,
                                 NULL);

  if (service && !strchr (service, '.'))
  {
    /* unqualified service name; prefix com.nokia. */
    gchar *s = g_strconcat (SERVICE_PREFIX, service, NULL);
    g_free (service);
    service = s;
  }
  
  icon_name = g_key_file_get_value(key_file,
                                   "Desktop Entry",
                                   DESKTOP_ICON_FIELD,
                                   NULL);

  startup_notify = g_key_file_get_value(key_file,
                                        "Desktop Entry",
                                        DESKTOP_STARTUPNOTIFY,
                                        NULL);
  
  text_domain = g_key_file_get_value(key_file,
                                   "Desktop Entry",
                                   DESKTOP_TEXT_DOMAIN_FIELD,
                                   NULL);
  
  app = HD_WM_APPLICATION (g_object_new (HD_WM_TYPE_APPLICATION, NULL));

  if (!app)
  {
    g_free(app_name);
    g_free(startup_wmclass);
    g_free(exec_name);
    g_free(service);
    g_free(icon_name);
    g_free(startup_notify);
    g_free(text_domain);
    goto out;
  }

  /* translate the application name as appropriate */
  if (app_name)
    {
      app->priv->app_name = g_strdup ((text_domain && *text_domain) ?
                                dgettext(text_domain, app_name): _(app_name));

      g_free (app_name);
    }
  
  app->priv->icon_name      = icon_name; 
  app->priv->service        = service;    
  app->priv->exec_name      = exec_name;
  app->priv->text_domain    = text_domain;

  HDWM_APPLICATION_SET_FLAG (app, HDWM_APPLICATION_STARTUP_NOTIFY); /* Default */

  if (startup_notify)
    {
      if(!g_ascii_strcasecmp(startup_notify, "false"))
        HDWM_APPLICATION_UNSET_FLAG (app, HDWM_APPLICATION_STARTUP_NOTIFY);

      g_free(startup_notify);
    }

  if (startup_wmclass != NULL)
    app->priv->class_name = startup_wmclass;
  else
    app->priv->class_name = g_path_get_basename(exec_name);

  app->priv->active_window = NULL;
  g_debug ("Registered new watchable app\n"
	 "\texec name: %s\n"
	 "\tapp name: %s\n"
	 "\tclass name: %s\n"
	 "\tservice: %s",
	 app->priv->exec_name, app->priv->app_name, app->priv->class_name, app->priv->service);

 out:
  g_key_file_free (key_file);
  
  return app;
}

gboolean
hd_wm_application_update (HDWMApplication *app, HDWMApplication *update)
{
  gboolean retval = FALSE;
  
  /* can only update if the two represent the same application */
  g_return_val_if_fail(app && update &&
                       app->priv->class_name && update->priv->class_name &&
                       g_str_equal (app->priv->class_name, update->priv->class_name),
                       FALSE);

  /*
   * we only update fields that make sense to update -- update should not be
   * running application, so we do not update flags, etc
   */
  if ((!update->priv->icon_name && app->priv->icon_name) ||
      (update->priv->icon_name && !app->priv->icon_name) ||
      (update->priv->icon_name && app->priv->icon_name && g_str_equal (update->priv->icon_name,app->priv->icon_name)))
  {
    g_debug ("changing %s -> %s", app->priv->icon_name, update->priv->icon_name);
      
    if (app->priv->icon_name)
      g_free (app->priv->icon_name);
  
    app->priv->icon_name = g_strdup (update->priv->icon_name);
    retval = TRUE;
  }
  
  if ((!update->priv->service && app->priv->service) ||
      (update->priv->service && !app->priv->service) ||
      (update->priv->service && app->priv->service && !g_str_equal (update->priv->service,app->priv->service)))
  {
    g_debug ("changing %s -> %s", app->priv->service, update->priv->service);
    if (app->priv->service)
      g_free (app->priv->service);
  
    app->priv->service = g_strdup (update->priv->service);
    retval = TRUE;
  }

  if ((!update->priv->app_name && app->priv->app_name) ||
      (update->priv->app_name && !app->priv->app_name) ||
      (update->priv->app_name && app->priv->app_name && !g_str_equal (update->priv->app_name,app->priv->app_name)))
  {
    g_debug ("changing %s -> %s", app->priv->app_name, update->priv->app_name);
    if (app->priv->app_name)
      g_free (app->priv->app_name);
  
    app->priv->app_name = g_strdup (update->priv->app_name);
    retval = TRUE;
  }

  if ((!update->priv->exec_name && app->priv->exec_name) ||
      (update->priv->exec_name && !app->priv->exec_name) ||
      (update->priv->exec_name && app->priv->exec_name && !g_str_equal (update->priv->exec_name,app->priv->exec_name)))
  {
    g_debug ("changing %s -> %s", app->priv->exec_name, update->priv->exec_name);
    if (app->priv->exec_name)
      g_free (app->priv->exec_name);
  
    app->priv->exec_name  = g_strdup (update->priv->exec_name);
    retval = TRUE;
  }

  return retval;
}




static gboolean
hd_wm_application_has_windows_find_func (gpointer key,
					   gpointer value,
					   gpointer user_data)
{
  HDWMWindow *win;  

  win = (HDWMWindow*)value;

  g_debug ("Checking %p vs %p", 
	 hd_wm_window_get_application(win),
	 (HDWMApplication *)user_data);

  if (hd_wm_window_get_application(win) == (HDWMApplication *)user_data)
    return TRUE;

  return FALSE;
}

gboolean
hd_wm_application_has_windows (HDWMApplication *app)
{
  return (g_hash_table_find (hd_wm_get_windows(),
			     hd_wm_application_has_windows_find_func,
			     (gpointer)app) != NULL);
}

gboolean
hd_wm_application_has_hibernating_windows (HDWMApplication *app)
{
  return (g_hash_table_find (hd_wm_get_hibernating_windows(),
			     hd_wm_application_has_windows_find_func,
			     (gpointer)app) != NULL);
}

gboolean
hd_wm_application_has_any_windows (HDWMApplication *app)
{
  g_return_val_if_fail(app, FALSE);
  GHashTable * ht = hd_wm_application_is_hibernating (app) ?
    hd_wm_get_hibernating_windows() :
    hd_wm_get_windows();

  g_return_val_if_fail(ht, FALSE);
  
  return (g_hash_table_find (ht,
			     hd_wm_application_has_windows_find_func,
			     (gpointer)app) != NULL);
}

gboolean
hd_wm_application_is_hibernating (HDWMApplication *app)
{
  return HDWM_APPLICATION_IS_SET_FLAG(app, HDWM_APPLICATION_HIBERNATING);
}

void
hd_wm_application_set_hibernate (HDWMApplication *app,
				   gboolean          hibernate)
{
  HN_MARK();
  if(hibernate)
    HDWM_APPLICATION_SET_FLAG(app, HDWM_APPLICATION_HIBERNATING);
  else
    HDWM_APPLICATION_UNSET_FLAG(app, HDWM_APPLICATION_HIBERNATING);
}


gboolean
hd_wm_application_is_able_to_hibernate (HDWMApplication *app)
{
  if (hd_wm_application_is_dummy (app))
    return FALSE;

  return HDWM_APPLICATION_IS_SET_FLAG(app, HDWM_APPLICATION_CAN_HIBERNATE);
}

void
hd_wm_application_set_able_to_hibernate (HDWMApplication *app,
					   gboolean          hibernate)
{
  if(hibernate)
    HDWM_APPLICATION_SET_FLAG(app, HDWM_APPLICATION_CAN_HIBERNATE);
  else
    HDWM_APPLICATION_UNSET_FLAG(app, HDWM_APPLICATION_CAN_HIBERNATE);
}


const gchar*
hd_wm_application_get_service (HDWMApplication *app)
{
  return app->priv->service;
}

const gchar*
hd_wm_application_get_exec (HDWMApplication *app)
{
  return app->priv->exec_name;
}

void 
hd_wm_application_dummy_set_name (HDWMApplication *app, const gchar *name)
{
  if (!hd_wm_application_is_dummy (app))
    return;

  g_free (app->priv->app_name);
  g_free (app->priv->class_name);
    
  app->priv->app_name   = g_strdup (name);
  app->priv->class_name = g_strdup (name);
}	

const gchar*
hd_wm_application_get_name (HDWMApplication *app)
{
  return app->priv->app_name;
}

const gchar*
hd_wm_application_get_localized_name (HDWMApplication *app)
{
  return (app->priv->text_domain ? dgettext(app->priv->text_domain,app->priv->app_name):
                             app->priv->app_name);
}

const gchar*
hd_wm_application_get_icon_name (HDWMApplication *app)
{
  return app->priv->icon_name;
}

const gchar*
hd_wm_application_get_class_name (HDWMApplication *app)
{
  return app->priv->class_name;
}


gboolean
hd_wm_application_is_minimised (HDWMApplication *app)
{
  return HDWM_APPLICATION_IS_SET_FLAG(app, HDWM_APPLICATION_MINIMISED);
}

gboolean
hd_wm_application_has_startup_notify (HDWMApplication *app)
{
  return HDWM_APPLICATION_IS_SET_FLAG(app, HDWM_APPLICATION_STARTUP_NOTIFY);
}


void
hd_wm_application_set_minimised (HDWMApplication *app,
				   gboolean          minimised)
{
  if(minimised)
    HDWM_APPLICATION_SET_FLAG(app, HDWM_APPLICATION_MINIMISED);
  else
    HDWM_APPLICATION_UNSET_FLAG(app, HDWM_APPLICATION_MINIMISED);
}

void
hd_wm_application_died_dialog_show(HDWMApplication *app)
{
  GtkWidget *dialog;
  gchar *text;
  text = g_strdup_printf(dgettext("ke-recv", "memr_ni_application_closed_no_resources"),
			 app->priv->app_name ? _(app->priv->app_name) : "");
  dialog = hildon_note_new_information(NULL, text);
  gtk_widget_show_all(dialog);
  gtk_dialog_run(GTK_DIALOG(dialog));
  gtk_widget_destroy(dialog);
  g_free(text);
}

/* Launch Banner Dialog */

/* FIXME: rename namespace to watched app */
void 
hd_wm_application_launch_banner_show (HDWMApplication *app) 
{
  HDWMLaunchBannerInfo *info;
  guint                 interval;
  gchar                *lapp_name;

  g_return_if_fail(app);
  
  interval = HDWM_APPLICATION_LAUNCH_BANNER_CHECK_INTERVAL * 1000;

  info = g_new0(HDWMLaunchBannerInfo, 1);

  info->app = app;

  gettimeofday( &info->launch_time, NULL );

  gdk_error_trap_push(); 	/* Needed ? */

  lapp_name = (app->priv->text_domain?dgettext(app->priv->text_domain,app->priv->app_name):
                                gettext(app->priv->app_name));

  info->msg = g_strdup_printf(_(hd_wm_application_is_hibernating(app) ?
                                HDWM_APPLICATION_LAUNCH_BANNER_MSG_RESUMING :
                                HDWM_APPLICATION_LAUNCH_BANNER_MSG_LOADING ),
                              lapp_name ? _(lapp_name) : "" );
        
  info->banner = hildon_banner_show_animation(NULL, NULL, info->msg);
  gdk_error_trap_pop();

  g_timeout_add(interval, hd_wm_application_launch_banner_timeout, info);

}

void 
hd_wm_application_launch_banner_close (GtkWidget            *parent,
					 HDWMLaunchBannerInfo *info)
{
  if (!(info && info->msg))
    return;

  if(info->banner)
    gtk_widget_destroy(info->banner);

  hd_wm_launch_banner_info_free(info);
}

gboolean 
hd_wm_application_launch_banner_timeout (gpointer data)
{
  HDWMLaunchBannerInfo *info = data;
  struct timeval        current_time;
  long unsigned int     t1, t2;
  guint                 time_left;
  gulong                current_banner_timeout = 0;

  /* Added by Karoliina Salminen 26092005 
   * Addition to low memory situation awareness, the following 
   * multiplies the launch banner timeout with the timeout 
   * multiplier found from environment variable 
   */
  if(hd_wm_is_lowmem_situation()) 
    current_banner_timeout
      = hd_wm_get_lowmem_banner_timeout()*hd_wm_get_lowmem_timeout_multiplier();
  
  else 
    current_banner_timeout = hd_wm_get_lowmem_banner_timeout();

  /* End of addition 26092005 */

#if 0 // needed ???
  if ( find_service_from_tree( hnwm->callbacks.model, 
			       &iter, 
			       info->service_name ) > 0) 
    {
    } else {
      /* This should never happen. Bail out! */
      return FALSE;
    }
#endif	

  gettimeofday( &current_time, NULL );

  t1 = (long unsigned int) info->launch_time.tv_sec;
  t2 = (long unsigned int) current_time.tv_sec;
  time_left = (guint) (t2 - t1);
	
  /* The following uses now current_banner_timeout instead of 
   * lowmem_banner_timeout, changed by
   * Karoliina Salminen 26092005 
   */
  if (time_left >= current_banner_timeout 
      || hd_wm_application_has_windows (info->app))
    {
      /* Close the banner */
      hd_wm_application_launch_banner_close( NULL, info );
      
      return FALSE;
    }
  
  return TRUE;
}

void
hd_wm_application_set_ping_timeout_note(HDWMApplication *app, GtkWidget *note)
{
	app->priv->ping_timeout_note = note;
}

GtkWidget*
hd_wm_application_get_ping_timeout_note(HDWMApplication *app)
{
	return app->priv->ping_timeout_note;
}

HDWMWindow *
hd_wm_application_get_active_window (HDWMApplication *app)
{
  return app->priv->active_window;
}

void
hd_wm_application_set_active_window (HDWMApplication *app, HDWMWindow * win)
{
  app->priv->active_window = win;
}

gboolean
hd_wm_application_is_dummy(HDWMApplication *app)
{
  g_return_val_if_fail(app, TRUE);
  return HDWM_APPLICATION_IS_SET_FLAG(app, HDWM_APPLICATION_DUMMY);
}

gboolean
hd_wm_application_is_launching (HDWMApplication *app)
{
  return HDWM_APPLICATION_IS_SET_FLAG(app, HDWM_APPLICATION_LAUNCHING);
}

void
hd_wm_application_set_launching (HDWMApplication *app,
                                   gboolean launching)
{
  if(launching)
    HDWM_APPLICATION_SET_FLAG(app, HDWM_APPLICATION_LAUNCHING);
  else
    HDWM_APPLICATION_UNSET_FLAG(app, HDWM_APPLICATION_LAUNCHING);
}

HDEntryInfo *
hd_wm_application_get_info (HDWMApplication *app)
{
  if (!app->priv->info)
    app->priv->info = hd_entry_info_new_from_app (app);

  return app->priv->info;
}

#if 0
/* TODO -- remove me if not needed in the end */
struct _app_windows_count_data
{
  HDWMApplication * app;
  guint              count;
};

static void
hd_wm_application_my_window_foreach_func (gpointer key,
                                            gpointer value,
                                            gpointer user_data)
{
  HDWMWindow *win;  
  win = (HDWMWindow*)value;

  struct _app_windows_count_data * wcd
    = (struct _app_windows_count_data *) user_data;
  
  if (hd_wm_window_get_application(win) == wcd->app)
    wcd->count++;
}

gint
hd_wm_application_n_windows (HDWMApplication *app)
{
  struct _app_windows_count_data wcd;
  GHashTable * ht;

  g_return_val_if_fail(app, 0);

  ht = hd_wm_application_is_hibernating (app) ?
    hd_wm_get_hibernating_windows() :
    hd_wm_get_windows();

  g_return_val_if_fail(ht, 0);
  
  wcd.app = app;
  wcd.count = 0;

  g_hash_table_foreach(ht, hd_wm_application_my_window_foreach_func, &wcd);

  return wcd.count;
}

#endif

gboolean
hd_wm_application_is_active (HDWMApplication *app)
{
  g_return_val_if_fail(app && app->priv->active_window, FALSE);

  if(hd_wm_get_active_window() == app->priv->active_window)
    return TRUE;

  return FALSE;
}

const gchar *
hd_wm_application_get_extra_icon (HDWMApplication *app)
{
  g_return_val_if_fail(app, NULL);
  
  return app->priv->extra_icon;
}

void
hd_wm_application_set_extra_icon (HDWMApplication *app, const gchar *icon)
{
  g_return_if_fail(app);

  g_free (app->priv->extra_icon);

  app->priv->extra_icon = g_strdup(icon);
}
