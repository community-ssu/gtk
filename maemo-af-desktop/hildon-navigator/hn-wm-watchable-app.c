/* -*- mode:C; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/* 
 * This file is part of maemo-af-desktop
 *
 * Copyright (C) 2005, 2006 Nokia Corporation.
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


#include <string.h>
#include <libosso.h>
#include <log-functions.h>

#include <hildon-widgets/hildon-defines.h>
#include <hildon-widgets/hildon-banner.h>
#include <hildon-widgets/hildon-note.h>

#include "hn-wm.h"
#include "hn-wm-watchable-app.h"
#include "hn-wm-watched-window.h"
#include "hn-entry-info.h"
#include "hn-app-switcher.h"
#include "osso-manager.h"
#include "hildon-navigator.h"

typedef char HNWMWatchableAppFlags;

typedef enum 
{
  HNWM_APP_CAN_HIBERNATE  = 1 << 0,
  HNWM_APP_HIBERNATING    = 1 << 1,
  HNWM_APP_MINIMISED      = 1 << 2,
  HNWM_APP_STARTUP_NOTIFY = 1 << 3,
  HNWM_APP_DUMMY          = 1 << 4,
  HNWM_APP_LAUNCHING      = 1 << 5,
  /*
     Last dummy value for compile time check.
     If you need values > HNWM_APP_LAST, you have to change the
     definition of HNWMWatchableAppFlags to get more space.
  */
  HNWM_APP_LAST           = 1 << 7
}HNWMWatchableAppFlagsEnum;


/*
   compile time assert making sure that the storage space for our flags is
   big enough
*/
typedef char __app_compile_time_check_for_flags_size[
     (guint)(1<<(sizeof(HNWMWatchableAppFlags)*8))>(guint)HNWM_APP_LAST ? 1:-1
                                                    ];

#define HNWM_APP_SET_FLAG(a,f)    ((a)->flags |= (f))
#define HNWM_APP_UNSET_FLAG(a,f)  ((a)->flags &= ~(f))
#define HNWM_APP_IS_SET_FLAG(a,f) ((a)->flags & (f))

/* watchable apps */

struct HNWMWatchableApp
{
  gchar     *icon_name;
  gchar     *extra_icon;
  gchar     *service;
  gchar     *app_name; 		/* window title */
  gchar     *exec_name; 		/* class || exec field ? */
  gchar     *class_name;        
  gchar     *text_domain;        
  GtkWidget *ping_timeout_note; /* The note that is shown when the app quits responding */
  HNWMWatchedWindow    *active_window;
  HNWMWatchableAppFlags flags;
  HNEntryInfo          *info;
};



static gboolean
hn_wm_watchable_app_has_windows_find_func (gpointer key,
					   gpointer value,
					   gpointer user_data);

static void
hn_wm_launch_banner_info_free (HNWMLaunchBannerInfo* info)
{
  g_return_if_fail(info);
  
  g_free(info->msg);
  g_free(info);
}


HNWMWatchableApp*
hn_wm_watchable_app_new_dummy (void)
{
  HNWMWatchableApp *app;

  app = g_new0 (HNWMWatchableApp, 1);

  if (!app)
    {
      return NULL;
    }

  app->icon_name  = g_strdup("qgn_list_gene_default_app"); 
  app->app_name   = g_strdup("?");
  app->class_name = g_strdup("?");

  HNWM_APP_SET_FLAG (app, HNWM_APP_DUMMY);
  
  HN_DBG("Registered new non-watchable app\n\tapp_name: "
         "%s\n\tclass name: %s\n\texec name (service): %s", 
         app->app_name, app->class_name, app->service);

  return app;
}

HNWMWatchableApp*
hn_wm_watchable_app_new (const char * file)
{
  HNWMWatchableApp *app = NULL;
  gchar            *service, *icon_name, *startup_notify;
  gchar            *startup_wmclass, *exec_name, *app_name, *text_domain;
  GKeyFile         *key_file;

  g_return_val_if_fail(file && (key_file = g_key_file_new()), NULL);

  if (!g_key_file_load_from_file (key_file, file, G_KEY_FILE_NONE, NULL))
  {
    g_warning ("Could not load keyfile [%s]", file);
    g_key_file_free(key_file);
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
      osso_log(LOG_ERR, "HN: Desktop file has invalid fields");
      g_free(app_name);
      g_free(exec_name);
      g_free(startup_wmclass);
      return NULL;
    }
  
  /* DESKTOP_LAUNCH_FIELD maps to X-Osso-Service */
  service = g_key_file_get_value(key_file,
                                 "Desktop Entry",
                                 DESKTOP_LAUNCH_FIELD,
                                 NULL);

  if (service && !strchr (service, '.'))
    {
      /* unqualified service name; prefix com.nokia. */
      gchar * s = g_strconcat (SERVICE_PREFIX, service, NULL);
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
  
  app = g_new0 (HNWMWatchableApp, 1);

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

  app->icon_name      = icon_name; 
  app->service        = service;    
  app->app_name       = app_name; 
  app->exec_name      = exec_name;
  app->text_domain    = text_domain;

  HNWM_APP_SET_FLAG (app, HNWM_APP_STARTUP_NOTIFY); /* Default */

  if (startup_notify)
    {
      if(!g_ascii_strcasecmp(startup_notify, "false"))
        HNWM_APP_UNSET_FLAG (app, HNWM_APP_STARTUP_NOTIFY);

      g_free(startup_notify);
    }

  if (startup_wmclass != NULL)
    app->class_name = startup_wmclass;
  else
    app->class_name = g_path_get_basename(exec_name);

  app->active_window = NULL;
  HN_DBG("Registered new watchable app\n"
	 "\texec name: %s\n"
	 "\tapp name: %s\n"
	 "\tclass name: %s\n"
	 "\tservice: %s",
	 app->exec_name, app->app_name, app->class_name, app->service);

 out:
  g_key_file_free(key_file);
  return app;
}

gboolean
hn_wm_watchable_app_update (HNWMWatchableApp *app, HNWMWatchableApp *update)
{
  gboolean retval = FALSE;
  
  /* can only update if the two represent the same application */
  g_return_val_if_fail(app && update &&
                       app->class_name && update->class_name &&
                       !strcmp(app->class_name, update->class_name),
                       FALSE);

  /*
   * we only update fields that make sense to update -- update should not be
   * running application, so we do not update flags, etc
   */
  if((!update->icon_name && app->icon_name) ||
     (update->icon_name && !app->icon_name) ||
     (update->icon_name && app->icon_name && strcmp(update->icon_name,
                                                    app->icon_name)))
    {
      HN_DBG("changing %s -> %s", app->icon_name, update->icon_name);
      
      if(app->icon_name)
        g_free(app->icon_name);
  
      app->icon_name  = g_strdup(update->icon_name);
      retval = TRUE;
    }
  
  if((!update->service && app->service) ||
     (update->service && !app->service) ||
     (update->service && app->service && strcmp(update->service,
                                                app->service)))
    {
      HN_DBG("changing %s -> %s", app->service, update->service);
      if(app->service)
        g_free(app->service);
  
      app->service    = g_strdup(update->service);
      retval = TRUE;
    }

  if((!update->app_name && app->app_name) ||
     (update->app_name && !app->app_name) ||
     (update->app_name && app->app_name && strcmp(update->app_name,
                                                  app->app_name)))
    {
      HN_DBG("changing %s -> %s", app->app_name, update->app_name);
      if(app->app_name)
        g_free(app->app_name);
  
      app->app_name   = g_strdup(update->app_name);
      retval = TRUE;
    }

  if((!update->exec_name && app->exec_name) ||
     (update->exec_name && !app->exec_name) ||
     (update->exec_name && app->exec_name && strcmp(update->exec_name,
                                                    app->exec_name)))
    {
      HN_DBG("changing %s -> %s", app->exec_name, update->exec_name);
      if(app->exec_name)
        g_free(app->exec_name);
  
      app->exec_name  = g_strdup(update->exec_name);
      retval = TRUE;
    }

  return retval;
}




static gboolean
hn_wm_watchable_app_has_windows_find_func (gpointer key,
					   gpointer value,
					   gpointer user_data)
{
  HNWMWatchedWindow *win;  

  win = (HNWMWatchedWindow*)value;

  HN_DBG("Checking %p vs %p", 
	 hn_wm_watched_window_get_app(win),
	 (HNWMWatchableApp *)user_data);

  if (hn_wm_watched_window_get_app(win) == (HNWMWatchableApp *)user_data)
    return TRUE;

  return FALSE;
}

gboolean
hn_wm_watchable_app_has_windows (HNWMWatchableApp *app)
{
  return (g_hash_table_find (hn_wm_get_watched_windows(),
			     hn_wm_watchable_app_has_windows_find_func,
			     (gpointer)app) != NULL);
}

gboolean
hn_wm_watchable_app_has_hibernating_windows (HNWMWatchableApp *app)
{
  return (g_hash_table_find (hn_wm_get_hibernating_windows(),
			     hn_wm_watchable_app_has_windows_find_func,
			     (gpointer)app) != NULL);
}

gboolean
hn_wm_watchable_app_has_any_windows (HNWMWatchableApp *app)
{
  g_return_val_if_fail(app, FALSE);
  GHashTable * ht = hn_wm_watchable_app_is_hibernating (app) ?
    hn_wm_get_hibernating_windows() :
    hn_wm_get_watched_windows();

  g_return_val_if_fail(ht, FALSE);
  
  return (g_hash_table_find (ht,
			     hn_wm_watchable_app_has_windows_find_func,
			     (gpointer)app) != NULL);
}

gboolean
hn_wm_watchable_app_is_hibernating (HNWMWatchableApp *app)
{
  return HNWM_APP_IS_SET_FLAG(app, HNWM_APP_HIBERNATING);
}

void
hn_wm_watchable_app_set_hibernate (HNWMWatchableApp *app,
				   gboolean          hibernate)
{
  HN_MARK();
  if(hibernate)
    HNWM_APP_SET_FLAG(app, HNWM_APP_HIBERNATING);
  else
    HNWM_APP_UNSET_FLAG(app, HNWM_APP_HIBERNATING);
}


gboolean
hn_wm_watchable_app_is_able_to_hibernate (HNWMWatchableApp *app)
{
  return HNWM_APP_IS_SET_FLAG(app, HNWM_APP_CAN_HIBERNATE);
}

void
hn_wm_watchable_app_set_able_to_hibernate (HNWMWatchableApp *app,
					   gboolean          hibernate)
{
  if(hibernate)
    HNWM_APP_SET_FLAG(app, HNWM_APP_CAN_HIBERNATE);
  else
    HNWM_APP_UNSET_FLAG(app, HNWM_APP_CAN_HIBERNATE);
}


const gchar*
hn_wm_watchable_app_get_service (HNWMWatchableApp *app)
{
  return app->service;
}

const gchar*
hn_wm_watchable_app_get_exec (HNWMWatchableApp *app)
{
  return app->exec_name;
}

const gchar*
hn_wm_watchable_app_get_name (HNWMWatchableApp *app)
{
  return app->app_name;
}

const gchar*
hn_wm_watchable_app_get_icon_name (HNWMWatchableApp *app)
{
  return app->icon_name;
}

const gchar*
hn_wm_watchable_app_get_class_name (HNWMWatchableApp *app)
{
  return app->class_name;
}


gboolean
hn_wm_watchable_app_is_minimised (HNWMWatchableApp *app)
{
  return HNWM_APP_IS_SET_FLAG(app, HNWM_APP_MINIMISED);
}

gboolean
hn_wm_watchable_app_has_startup_notify (HNWMWatchableApp *app)
{
  return HNWM_APP_IS_SET_FLAG(app, HNWM_APP_STARTUP_NOTIFY);
}


void
hn_wm_watchable_app_set_minimised (HNWMWatchableApp *app,
				   gboolean          minimised)
{
  if(minimised)
    HNWM_APP_SET_FLAG(app, HNWM_APP_MINIMISED);
  else
    HNWM_APP_UNSET_FLAG(app, HNWM_APP_MINIMISED);
}

void
hn_wm_watchable_app_destroy (HNWMWatchableApp *app)
{
  g_return_if_fail(app);
  
  if(app->icon_name)
    g_free(app->icon_name);
  
  if(app->service)
    g_free(app->service);

  if(app->app_name)
    g_free(app->app_name);
  
  if(app->exec_name)
   	g_free(app->exec_name);

  if(app->class_name)
    g_free(app->class_name);

  if(app->extra_icon)
    g_free(app->extra_icon);
  
  if(app->text_domain)
    g_free(app->text_domain);
  
  if(app->info)
    hn_entry_info_free (app->info);

  g_free(app);
}

void
hn_wm_watchable_app_died_dialog_show(HNWMWatchableApp *app)
{
  GtkWidget *dialog;
  gchar *text;
  text = g_strdup_printf(dgettext("ke-recv", "memr_ni_application_closed_no_resources"),
			 app->app_name ? _(app->app_name) : "");
  dialog = hildon_note_new_information(NULL, text);
  gtk_widget_show_all(dialog);
  gtk_dialog_run(GTK_DIALOG(dialog));
  gtk_widget_destroy(dialog);
  g_free(text);
}

/* Launch Banner Dialog */

/* FIXME: rename namespace to watched app */
void 
hn_wm_watchable_app_launch_banner_show (HNWMWatchableApp *app) 
{
  HNWMLaunchBannerInfo *info;
  guint                 interval;
  gchar                *lapp_name;

  g_return_if_fail(app);
  
  interval = APP_LAUNCH_BANNER_CHECK_INTERVAL * 1000;

  info = g_new0(HNWMLaunchBannerInfo, 1);

  info->app = app;

  gettimeofday( &info->launch_time, NULL );

  gdk_error_trap_push(); 	/* Needed ? */

  lapp_name = (app->text_domain?dgettext(app->text_domain,app->app_name):
                                gettext(app->app_name));

  info->msg = g_strdup_printf(_(hn_wm_watchable_app_is_hibernating(app) ?
                                APP_LAUNCH_BANNER_MSG_RESUMING :
                                APP_LAUNCH_BANNER_MSG_LOADING ),
                              lapp_name ? _(lapp_name) : "" );
        
  info->banner = GTK_WIDGET(hildon_banner_show_animation(NULL, NULL, info->msg));
  gdk_error_trap_pop();

  g_timeout_add(interval, hn_wm_watchable_app_launch_banner_timeout, info);

}

void 
hn_wm_watchable_app_launch_banner_close (GtkWidget            *parent,
					 HNWMLaunchBannerInfo *info)
{
  if (!(info && info->msg))
    return;

  if(info->banner)
    gtk_widget_destroy(info->banner);

  hn_wm_launch_banner_info_free(info);
}

gboolean 
hn_wm_watchable_app_launch_banner_timeout (gpointer data)
{
  HNWMLaunchBannerInfo *info = data;
  struct timeval        current_time;
  long unsigned int     t1, t2;
  guint                 time_left;
  gulong                current_banner_timeout = 0;

  /* Added by Karoliina Salminen 26092005 
   * Addition to low memory situation awareness, the following 
   * multiplies the launch banner timeout with the timeout 
   * multiplier found from environment variable 
   */
  if(hn_wm_is_lowmem_situation()) 
    current_banner_timeout
      = hn_wm_get_lowmem_banner_timeout()*hn_wm_get_lowmem_timeout_multiplier();
  
  else 
    current_banner_timeout = hn_wm_get_lowmem_banner_timeout();

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
      || hn_wm_watchable_app_has_windows (info->app))
    {
      /* Close the banner */
      hn_wm_watchable_app_launch_banner_close( NULL, info );
      
      return FALSE;
    }
  
  return TRUE;
}

void
hn_wm_watchable_app_set_ping_timeout_note(HNWMWatchableApp *app, GtkWidget *note)
{
	app->ping_timeout_note = note;
}

GtkWidget*
hn_wm_watchable_app_get_ping_timeout_note(HNWMWatchableApp *app)
{
	return app->ping_timeout_note;
}

HNWMWatchedWindow *
hn_wm_watchable_app_get_active_window (HNWMWatchableApp *app)
{
  return app->active_window;
}

void
hn_wm_watchable_app_set_active_window (HNWMWatchableApp *app, HNWMWatchedWindow * win)
{
  app->active_window = win;
}

gboolean
hn_wm_watchable_app_is_dummy(HNWMWatchableApp *app)
{
  g_return_val_if_fail(app, TRUE);
  return HNWM_APP_IS_SET_FLAG(app, HNWM_APP_DUMMY);
}

gboolean
hn_wm_watchable_app_is_launching (HNWMWatchableApp *app)
{
  return HNWM_APP_IS_SET_FLAG(app, HNWM_APP_LAUNCHING);
}

void
hn_wm_watchable_app_set_launching (HNWMWatchableApp *app,
                                   gboolean launching)
{
  if(launching)
    HNWM_APP_SET_FLAG(app, HNWM_APP_LAUNCHING);
  else
    HNWM_APP_UNSET_FLAG(app, HNWM_APP_LAUNCHING);
}

HNEntryInfo *
hn_wm_watchable_app_get_info (HNWMWatchableApp *app)
{
  if (!app->info)
    app->info = hn_entry_info_new_from_app (app);

  return app->info;
}

#if 0
/* TODO -- remove me if not needed in the end */
struct _app_windows_count_data
{
  HNWMWatchableApp * app;
  guint              count;
};

static void
hn_wm_watchable_app_my_window_foreach_func (gpointer key,
                                            gpointer value,
                                            gpointer user_data)
{
  HNWMWatchedWindow *win;  
  win = (HNWMWatchedWindow*)value;

  struct _app_windows_count_data * wcd
    = (struct _app_windows_count_data *) user_data;
  
  if (hn_wm_watched_window_get_app(win) == wcd->app)
    wcd->count++;
}

gint
hn_wm_watchable_app_n_windows (HNWMWatchableApp *app)
{
  struct _app_windows_count_data wcd;
  GHashTable * ht;

  g_return_val_if_fail(app, 0);

  ht = hn_wm_watchable_app_is_hibernating (app) ?
    hn_wm_get_hibernating_windows() :
    hn_wm_get_watched_windows();

  g_return_val_if_fail(ht, 0);
  
  wcd.app = app;
  wcd.count = 0;

  g_hash_table_foreach(ht, hn_wm_watchable_app_my_window_foreach_func, &wcd);

  return wcd.count;
}

#endif

gboolean
hn_wm_watchable_app_is_active (HNWMWatchableApp *app)
{
  g_return_val_if_fail(app && app->active_window, FALSE);

  if(hn_wm_get_active_window() == app->active_window)
    return TRUE;

  return FALSE;
}

const gchar *
hn_wm_watchable_app_get_extra_icon (HNWMWatchableApp *app)
{
  g_return_val_if_fail(app, NULL);
  
  return app->extra_icon;
}

void
hn_wm_watchable_app_set_extra_icon (HNWMWatchableApp *app, const gchar *icon)
{
  g_return_if_fail(app);

  g_free(app->extra_icon);

  app->extra_icon = g_strdup(icon);
}
