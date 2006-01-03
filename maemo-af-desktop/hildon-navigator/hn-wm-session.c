/*
 * This file is part of maemo-af-desktop
 *
 * Copyright (C) 2005 Nokia Corporation.
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

#include "hn-wm-session.h"

extern HNWM *hnwm;

#define LOCATION_VAR    "STATESAVEDIR"
#define FALLBACK_PREFIX "/tmp/state"
#define MAX_SESSION_SIZE 2048

struct state_data
{
    gchar running_apps[MAX_SESSION_SIZE];
} 
statedata;


static gboolean 
send_sigterms(gpointer data)
{
  int      i;
  gboolean success = TRUE;
  GArray  *pidlist = (GArray *)data;
  
  for (i = 0; i< pidlist->len; i++)
    {
      int retval;
      
      retval = kill(g_array_index(pidlist, pid_t, i), SIGTERM);
      
      if (retval < 0)
	{
	  osso_log(LOG_WARNING, "HN: Could not kill an application");
	  success = FALSE;
	}
    }
  
  if (hnwm->timer_id != 0)
    g_source_remove(hnwm->timer_id);
  
  hnwm->timer_id = 0;
  
  /* Free the pidlist, although the memory would be free'd by
     the HN shutdown anyway. */
  if (pidlist != NULL)
    {
      g_array_free(pidlist, TRUE);
    }
  
  return success;
}

int 
hn_wm_session_save (GArray *arguments, gpointer data)
{
  gint            i, state_position = 0;
  GList          *mitems = NULL;
  GArray         *pids = g_array_new(FALSE, FALSE, sizeof(pid_t));
  osso_state_t    session_state;
  menuitem_comp_t menu_comp;
  osso_context_t *osso;
  osso_manager_t *man = (osso_manager_t *)data;
  
  osso = get_context(man);

  /* Assuming 32-bit unsigned int; 2^32 views should suffice... */
  gchar view_id[10];

  /* Acquire list of menu items from the Application Switcher */
  mitems = application_switcher_get_menuitems(hnwm->app_switcher);

  if (mitems == NULL)
    {
      osso_log(LOG_WARNING, "Unable to save session state.");
      return FALSE;
    }

  memset(&statedata, 0, sizeof(struct state_data));

  for (i = g_list_length(mitems)-1 ; i >= 2; i--)
    {
      menu_comp.menu_ptr  = g_list_nth_data(mitems, i);
      menu_comp.wm_class  = NULL;
      menu_comp.service   = NULL;
      menu_comp.window_id = 0;
      
      /* We should also get the PIDs here... */
      
      /* FIXME: what does this do ?
	 gtk_tree_model_foreach(Callbacks.model, menuitem_match_helper,
	 &menu_comp);
      */
      
      if (menu_comp.service != NULL)
        {
	  /* +12 comes from the linefeed, space and the view ID */
	  if ((strlen(menu_comp.service) + 12 + state_position) >
	      MAX_SESSION_SIZE)
            {
	      osso_log(LOG_WARNING, "Can't add entries anymore");
	      break;
            }
	  else
            {
	      strcat(statedata.running_apps, menu_comp.service);
	      strncat(statedata.running_apps, " ", 1);
	      memset(&(view_id), '\0', sizeof(view_id));
	      g_snprintf(view_id, 10, "%lu", menu_comp.view_id);
	      strncat(statedata.running_apps, view_id, strlen(view_id));
	      strcat(statedata.running_apps, "\n");
	      state_position = 
		state_position + strlen(menu_comp.service) +
		strlen(view_id);
	      
	      if (menu_comp.window_id != 0)
                {
		  guchar       *pid_result = NULL;
		  int           actual_format;
		  unsigned long nitems, bytes_after;
		  Atom          actual_type;
		  
		  gdk_error_trap_push();
		  
		  XGetWindowProperty(GDK_DISPLAY(), 
				     (Window)menu_comp.window_id, hnwm->atoms[HN_ATOM_NET_WM_PID],
				     0, 32, False, XA_CARDINAL, &actual_type,
				     &actual_format, &nitems, &bytes_after,
				     (unsigned char **)&pid_result);
		  
		  gdk_error_trap_pop();
		  
		  if (pid_result != NULL)
		    g_array_append_vals(pids, pid_result, 1);
                }
            }
        }
    }
  session_state.state_size = sizeof(struct state_data);
  session_state.state_data = &statedata;
  osso_state_write(osso, &session_state);
  
  /* We will have to set a timer that sends the sigterms after the exit
     DBUS signals have been sent and before HN is shut down */
  
  hnwm->timer_id = g_timeout_add(KILL_DELAY, send_sigterms, pids);
  
  /* At the moment, this change is not reverted anywhere as saving
     session always impiles immediate shutdown/restart */
  
  hnwm->about_to_shutdown = TRUE;
  
  /* We reply automatically for the appkiller, thanks to LibOSSO */        
  return TRUE;
}

void
hn_wm_session_restore (osso_manager_t *man)
{
  osso_return_t   ret;
  osso_state_t    state;
  osso_context_t *osso = get_context(man);
  struct stat      buf;
  gint            i = 0;
  gchar          *path = NULL;
  gchar         **entries = NULL, *last_app = NULL, *tmpdir_path = NULL;

  memset(&statedata, 0, sizeof(struct state_data));
  state.state_data = &statedata;
  state.state_size = sizeof(struct state_data);
    
  /* Read the state */
  ret = osso_state_read(osso, &state);
    
  if (ret != OSSO_OK)
    return;

  /* Launch the applications */
  entries = g_strsplit(statedata.running_apps, "\n", 100);

  while(entries[i] != NULL && strlen(entries[i]) > 0)
    {
      gchar **launch = g_strsplit(entries[i], " ", 2);

      /* Should we check whether this has already been launched? */
      hn_wm_top_service(launch[0]);

      if (entries[i+1] == NULL || strlen(entries[i+1]) <= 1 )
        {
	  last_app = g_strdup(launch[0]);
        }
      i++;

      g_strfreev(launch);
    }
    
  /* The application switcher may be left on somewhat inconsistent state
   * due to the undeterministic app startup times etc. This is
   * supposedly OK for now, but reordering code might be added below
   * this someday... 
   */
  g_strfreev(entries); 

  /* Finally, unlink the state file. This should be upgraded when
     bug # is fixed... */

  tmpdir_path = getenv(LOCATION_VAR);
    
  if (tmpdir_path != NULL)
    path = g_strconcat(tmpdir_path, "/",
		       "tasknav", "/",
		       "0.1", NULL);
  else
    path = g_strconcat(FALLBACK_PREFIX,
                           "tasknav", "/", "0.1", "/",
                           NULL);

  if (path == NULL)
    return;
  
  if (stat(path, &buf) != 0) 
    {
      g_free(path);
      return;
    }
  
  if (S_ISREG(buf.st_mode)) 
    unlink(path);
  
  g_free(path);
  
  return;
}

/* wrapper around hn_wm_session_restore() for calling via timeout */
gboolean
hn_wm_session_restore_via_timeout (gpointer user_data)
{
  osso_manager_t *man = (osso_manager_t *)user_data;

  hn_wm_session_restore(man);

  return FALSE;
}
