/* -*- mode:C; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
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

/* -*- mode:C; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#include <signal.h>
#include <libosso.h>
#include <log-functions.h>
#include <X11/Xatom.h>
#include <gdk/gdkx.h>
#include <hildon-widgets/hildon-defines.h>
#include <hildon-widgets/hildon-banner.h>
#include <hildon-widgets/hildon-note.h>

#include "hn-wm-types.h"
#include "hn-wm-memory.h"
#include "hn-wm-watched-window.h"
#include "hn-wm-watchable-app.h"
#include "hildon-navigator.h"
#include "others-menu.h"
#include "hildon-navigator-main.h"
#include "osso-manager.h"
#include "others-menu.h"
#include "hildon-navigator-interface.h"


gboolean 
hn_wm_memory_get_limits (guint *pages_used,
			 guint *pages_available)
{
  guint    lowmem_allowed;
  gboolean result;
  FILE    *lowmem_allowed_f, *pages_used_f;

  result = FALSE;

  lowmem_allowed_f = fopen(LOWMEM_PROC_ALLOWED, "r");
  pages_used_f     = fopen(LOWMEM_PROC_USED, "r");

  if (lowmem_allowed_f != NULL && pages_used_f != NULL)
    {
      fscanf(lowmem_allowed_f, "%u", &lowmem_allowed);
      fscanf(pages_used_f, "%u", pages_used);

      if (*pages_used < lowmem_allowed)
	*pages_available = lowmem_allowed - *pages_used;
      else
	*pages_available = 0;

      result = TRUE;
    }
  else
    {
      osso_log(LOG_ERR, "We could not read lowmem page stats.\n");
    }

  if (lowmem_allowed_f)
    fclose(lowmem_allowed_f);

  if (pages_used_f)
    fclose(pages_used_f);

  return result;
}

static gboolean 
hn_wm_memory_kill_all_watched_foreach_func (gpointer key,
					    gpointer value,
					    gpointer userdata)
{
  HNWMWatchableApp    *app;
  HNWMWatchedWindow   *win;
  gboolean             only_kill_able_to_hibernate;
  Window              *top_xwin;

  HN_DBG("### enter ###");

  only_kill_able_to_hibernate = *(gboolean*)(userdata);
  win                         = (HNWMWatchedWindow *)value;
  app                         = hn_wm_watched_window_get_app(win);

  if (only_kill_able_to_hibernate)
    {
      gboolean return_val = FALSE;
      HN_DBG("called with 'only_kill_able_to_hibernate'");

      top_xwin 
	=  hn_wm_util_get_win_prop_data_and_validate (GDK_ROOT_WINDOW(),
						      hn_wm_get_atom(HN_ATOM_MB_CURRENT_APP_WINDOW),
						      XA_WINDOW,
						      32,
						      0,
						      NULL);
      
      HN_DBG("'%s' able_to_hibernate? '%s' , hiberanting? '%s', Top win? '%s'",
	     hn_wm_watched_window_get_name (win),
	     hn_wm_watchable_app_is_able_to_hibernate(app) ? "yes" : "no",
	     hn_wm_watchable_app_is_hibernating (app)       ? "yes" : "no",
	     (hn_wm_watched_window_get_x_win (win) == *top_xwin) ? "yes" : "no" );

      if (hn_wm_watchable_app_is_able_to_hibernate(app) 
	  && hn_wm_watched_window_get_x_win (win) != *top_xwin) 
	{
	  HN_DBG("hn_wm_watched_window_attempt_pid_kill() for '%s'",
		 hn_wm_watched_window_get_name (win));

          if (hn_wm_watched_window_attempt_signal_kill (win, SIGTERM, TRUE))
 	    {
              HN_DBG("app->hibernating now '%s'",
                     hn_wm_watchable_app_is_hibernating(app) ? "true":"false");

              /* move the window into hibernation hash */
              g_hash_table_insert (hn_wm_get_hibernating_windows(),
                                   g_strdup (hn_wm_watched_window_get_hibernation_key(win)),
                                   win);

              /* need to reset the window xid as well */
              hn_wm_watched_window_reset_x_win (win);

              /* mark the application as hibernating -- we do not know how many
               * more windows it might have, so we need to set this on each one
               * of them
               */
              hn_wm_watchable_app_set_hibernate (app, TRUE);

              /* free the key */
              g_free (key);
              
              /* remove the current entry from the watched windows hash */
              return_val = TRUE;
            }
	}

      if (top_xwin)
	XFree(top_xwin);

      HN_DBG("### leave ###");

      return return_val;
    }
  else
    {
      /* Totally kill everthing and remove from our hash */
      HN_DBG("killing everything, currently '%s'", hn_wm_watched_window_get_name (win));
      hn_wm_watched_window_attempt_signal_kill (win, SIGTERM, FALSE);

      /* we are called by g_hash_foreach_steal -- have to explicitely destroy
       * the window here and the associated key
       */
      hn_wm_watched_window_destroy (win);
      g_free (key);
      
      HN_DBG("### leave ###");
      return TRUE;
    }
}

/* FIXME: rename kill to hibernate - kill is misleading */
int 
hn_wm_memory_kill_all_watched (gboolean only_kill_able_to_hibernate)
{
  /* use _steal rather than _remove as we need to keep the window object
   * in the hibernating windows hash
   */
  g_hash_table_foreach_steal ( hn_wm_get_watched_windows(),
                               hn_wm_memory_kill_all_watched_foreach_func,
                               (gpointer)&only_kill_able_to_hibernate);

  hn_wm_memory_update_lowmem_ui(hn_wm_is_lowmem_situation());

  return 0; 
}

 void 			/* NOTE: callback from app switcher */
hn_wm_shutdown_func(void)
{
    hn_wm_memory_kill_all_watched(FALSE);
}

 void                     /* NOTE: callback from app switcher */
hn_wm_memory_bgkill_func(gboolean is_on) 
{
  if (!config_do_bgkill) /* NOTE: var extern in hildon-navigator-main.h  */
    return;

  hn_wm_set_bg_kill_situation(is_on);
      
  if (is_on == TRUE)
  {
	hn_wm_memory_kill_all_watched(TRUE);
  }
}

 void                     /* NOTE: callback from app switcher */
hn_wm_memory_lowmem_func(gboolean is_on)
{
  if (hn_wm_is_lowmem_situation() != is_on)
    {
      hn_wm_set_lowmem_situation(is_on);

      /* The 'lowmem' situation always includes the 'bgkill' situation,
	 but the 'bgkill' signal is not generated in all configurations.
	 So we just call the bgkill_handler here. */
      hn_wm_memory_bgkill_func(is_on);

      hn_wm_memory_update_lowmem_ui(is_on);
      
      /* NOTE: config_lowmem_notify_enter extern in hildon-navigator-main.h */
      if (is_on && config_lowmem_notify_enter)
	{
	  /* NOTE: again in hildon-navigator-main.h 
	  if (config_lowmem_pavlov_dialog)
	    {
	      tm_wm_memory_show_pavlov_dialog();
	    }
	  */
	}
    }
}

/* FIXME: This is defined in maemo-af-desktop-main.c and we shouldn't
   be using it of course. Cleaning this up can be done when nothing
   else is left to clean up... */
extern Navigator tasknav;

 void 
hn_wm_memory_update_lowmem_ui (gboolean lowmem)
{
  /* If dimming is disabled, we don't do anything here. Also see
     APPLICATION_SWITCHER_UPDATE_LOWMEM_SITUATION. */
  if (!config_lowmem_dim)
    return;

  gtk_widget_set_sensitive(others_menu_get_button(tasknav.others_menu),
			   !lowmem);
  /*
  gtk_widget_set_sensitive(tasknav.bookmark_button, !lowmem);
  gtk_widget_set_sensitive(tasknav.mail_button, !lowmem);
  */

#if 0
  /* the new AS listens directly to the dbus signals */
  application_switcher_update_lowmem_situation(tasknav.app_switcher, lowmem);
#endif
}

int  /* FIXME: What does this do, does anything use it ? */
hn_wm_memory_kill_lru( void )
{
#if 0
  menuitem_comp_t  menu_comp;
  GtkWidget       *widgetptr = NULL;
  
  HN_MARK();

  widgetptr = application_switcher_get_killable_item(hn_wm_get_app_switcher());
  
  if (widgetptr == NULL) {
    return 1;
  }
  
  menu_comp.menu_ptr = widgetptr;
  menu_comp.wm_class = NULL;
  
  /* FIXME: what does this do ?
  gtk_tree_model_foreach (Callbacks.model, 
			  menuitem_match_helper,
			  &menu_comp);
  */

  if (menu_comp.wm_class != NULL) 
    {
      HNWMWatchedWindow     *win = NULL;


      win = hn_wm_lookup_watched_window_via_service (menu_comp.wm_class);

      if (win)
	{
	  HNWMWatchableApp      *app;

	  app = hn_wm_watched_window_get_app (win);
	  
	  if (hn_wm_watchable_app_is_able_to_hibernate (app))
	    hn_wm_watched_window_attempt_signal_kill (win, SIGTERM);
	}
    }
#endif
  return 0;
}
