/* 
 * This file is part of libhildonwm
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

#include <signal.h>
#include <X11/Xatom.h>
#include <gdk/gdkx.h>

#ifdef HAVE_LIBHILDON
#include <hildon/hildon-defines.h>
#include <hildon/hildon-banner.h>
#include <hildon/hildon-note.h>
#else
#include <hildon-widgets/hildon-defines.h>
#include <hildon-widgets/hildon-banner.h>
#include <hildon-widgets/hildon-note.h>
#endif


#include "hd-wm-types.h"
#include "hd-wm-memory.h"
#include "hd-wm-window.h"
#include "hd-wm-application.h"

gboolean config_do_bgkill;
gboolean config_lowmem_dim;

static gboolean
hd_wm_memory_get_env_is_set (const gchar *env, gboolean valbydefault)
{
  const gchar *val = g_getenv (env);

  if (val)
    return (g_str_equal (val, "yes"));
  else
    return valbydefault;
}

void 
hd_wm_memory_get_env_vars (void)
{
  config_do_bgkill  = hd_wm_memory_get_env_is_set ("NAVIGATOR_DO_BGKILL", TRUE);
  config_lowmem_dim = hd_wm_memory_get_env_is_set ("NAVIGATOR_LOWMEM_DIM", FALSE);
}

gboolean 
hd_wm_memory_get_limits (guint *pages_used,
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
      g_warning ("We could not read lowmem page stats.\n");
    }

  if (lowmem_allowed_f)
    fclose(lowmem_allowed_f);

  if (pages_used_f)
    fclose(pages_used_f);

  return result;
}

/* helper struct to hold things we need to pass to
 * hd_wm_memory_kill_all_watched_foreach_func () below
 */
typedef struct
{
  gboolean             only_able_to_hibernate;
  HDWMApplication   * top_app;
} _memory_foreach_data;


static void
hd_wm_memory_kill_all_watched_foreach_func (gpointer key,
					    gpointer value,
					    gpointer userdata)
{
  HDWMApplication     * app;
  HDWMWindow    * win;
  _memory_foreach_data * d;
  
  d   = (_memory_foreach_data*) userdata;
  win = (HDWMWindow *)value;
  app = hd_wm_window_get_application(win);

  if (d->only_able_to_hibernate)
    {
      hd_wm_debug("'%s' able_to_hibernate? '%s' , hiberanting? '%s'",
	     hd_wm_window_get_name (win),
	     hd_wm_application_is_able_to_hibernate(app) ? "yes" : "no",
	     hd_wm_application_is_hibernating (app)       ? "yes" : "no");
      
      if (app != d->top_app &&
          hd_wm_application_is_able_to_hibernate(app) &&
          !hd_wm_application_is_hibernating (app)) 
	{
	  hd_wm_debug("hd_wm_window_attempt_pid_kill() for '%s'",
		 hd_wm_window_get_name (win));
          
          /* mark the application as hibernating -- we do not know how many
           * more windows it might have, so we need to set this on each one
           * of them
           *
           * we have to do this unconditionally before sending SIGTERM to the
           * app otherwise if the app exits very quickly, the client window
           * list is updated before we have chance to mark the app as
           * hibernating (NB, we cannot reset the hibernation flag here if the
           * SIGTERM fails because there is still the SIGKILL in the pipeline.
           */
          hd_wm_application_set_hibernate (app, TRUE);
          
          if (!hd_wm_window_attempt_signal_kill (win, SIGTERM, TRUE))
 	    {
              g_warning ("sigterm failed");
            }
	}
    }
  else
    {
      /* Totally kill everything */
      hd_wm_debug("killing everything, currently '%s'",
             hd_wm_window_get_name (win));
      hd_wm_window_attempt_signal_kill (win, SIGTERM, FALSE);
    }

  hd_wm_debug("### leave ###");
}

/* FIXME: rename kill to hibernate - kill is misleading */
int 
hd_wm_memory_kill_all_watched (gboolean only_kill_able_to_hibernate)
{
  _memory_foreach_data   d;
  HDWMWindow    * top_win = NULL;
  Window               * top_xwin;

  /* init the foreach data */
  d.only_able_to_hibernate = only_kill_able_to_hibernate;
  d.top_app                = NULL;

  /* locate the application associated with the top-level window
   * we pass this into the foreach function, to avoid doing the lookup on
   * each iteration
   *
   * Get the xid of the top level window
   */
  top_xwin 
	=  hd_wm_util_get_win_prop_data_and_validate (GDK_ROOT_WINDOW(),
						      hd_wm_get_atom(HD_ATOM_MB_CURRENT_APP_WINDOW),
						      XA_WINDOW,
						      32,
						      0,
						      NULL);

  /* see if we have a matching watched window */
  if (top_xwin)
    {
      top_win = g_hash_table_lookup (hd_wm_get_windows(),
                                     (gconstpointer) top_xwin);

      XFree (top_xwin);
      top_xwin = NULL;
    }
  
  /* if so, get the corresponding application */
  if (top_win)
    {
      d.top_app = hd_wm_window_get_application (top_win);
      hd_wm_debug ("Top level application [%s]",
              hd_wm_application_get_name (d.top_app));
    }
  
  /* now hibernate our applications */
  g_hash_table_foreach ( hd_wm_get_windows(),
                         hd_wm_memory_kill_all_watched_foreach_func,
                         (gpointer)&d);

  hd_wm_memory_update_lowmem_ui(hd_wm_is_lowmem_situation());

  return 0; 
}

void                     /* NOTE: callback from app switcher */
hd_wm_memory_bgkill_func(gboolean is_on) 
{
  if (!config_do_bgkill) /* NOTE: var extern in hildon-navigator-main.h  */
    return;

  hd_wm_set_bg_kill_situation(is_on);
      
  if (is_on == TRUE)
  {
	hd_wm_memory_kill_all_watched(TRUE);
  }
}

 void                     /* NOTE: callback from app switcher */
hd_wm_memory_lowmem_func (gboolean is_on)
{
  if (hd_wm_is_lowmem_situation() != is_on)
  {
    hd_wm_set_lowmem_situation (is_on);

    /* The 'lowmem' situation always includes the 'bgkill' situation,
       but the 'bgkill' signal is not generated in all configurations.
       So we just call the bgkill_handler here. */
    hd_wm_memory_bgkill_func (is_on);
    hd_wm_memory_update_lowmem_ui (is_on);
      
    if (is_on)
    {
      hd_wm_debug ("We enter in lowmem situation");
    }
  }
}

/* FIXME: This is defined in maemo-af-desktop-main.c and we shouldn't
   be using it of course. Cleaning this up can be done when nothing
   else is left to clean up... */

 void 
hd_wm_memory_update_lowmem_ui (gboolean lowmem)
{
  /* If dimming is disabled, we don't do anything here. Also see
     APPLICATION_SWITCHER_UPDATE_LOWMEM_SITUATION. */
  if (!config_lowmem_dim)
    return;
  
  hd_wm_debug ("We have to set sensitiveness of others menu here!");
/*
  gtk_widget_set_sensitive(hn_window_get_others_menu(tasknav),!lowmem);
*/
#if 0
  /* the new AS listens directly to the dbus signals */
  application_switcher_update_lowmem_situation(tasknav.app_switcher, lowmem);
#endif
}

int  /* FIXME: What does this do, does anything use it ? */
hd_wm_memory_kill_lru( void )
{
#if 0
  menuitem_comp_t  menu_comp;
  GtkWidget       *widgetptr = NULL;
  
  HN_MARK();

  widgetptr = application_switcher_get_killable_item(hd_wm_get_app_switcher());
  
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
      HDWMWindow     *win = NULL;


      win = hd_wm_lookup_window_via_service (menu_comp.wm_class);

      if (win)
	{
	  HDWMApplication      *app;

	  app = hd_wm_window_get_application (win);
	  
	  if (hd_wm_application_is_able_to_hibernate (app))
	    hd_wm_window_attempt_signal_kill (win, SIGTERM);
	}
    }
#endif
  return 0;
}
