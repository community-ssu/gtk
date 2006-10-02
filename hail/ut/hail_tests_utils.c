/*
 * This file is part of hail
 *
 * Copyright (C) 2006 Nokia Corporation.
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

/**
 * SECTION:hail_tests_utils
 * @short_description: utilities for hail unit tests
 *
 * Utility functions, generaly used in hail unit tests
 */

#include <hail_tests_utils.h>
#include <gtk/gtkmain.h>
#include <string.h>

struct _AccessibleAction {
  gchar *name;
  AtkAction *accessible;
};

typedef struct _AccessibleAction AccessibleAction;                              



static gboolean button_waiter_handler (gpointer data);

static gboolean
button_waiter_handler (gpointer data)
{
  gboolean * returned = (gboolean *) data;

  *returned = TRUE;

  return FALSE;
}

/**
 * hail_tests_utils_button_waiter:
 * @interval: time in miliseconds to wait for
 *
 * enters gtk loop for the time specified.
 */
void 
hail_tests_utils_button_waiter (guint interval)
{
  gboolean returned = FALSE;

  g_timeout_add (interval, button_waiter_handler, &returned);
  while (!returned) {
    gtk_main_iteration();
  }
}

/**
 * hail_tests_utils_do_action_by_name:
 * @action: accessible object implementing #AtkAction interface.
 * @name: name of the action to do
 *
 * calls the action of the accessible object name by @name
 *
 * Return value: %TRUE if it was successful
 */
gboolean hail_tests_utils_do_action_by_name (AtkAction * action,
					     gchar * name)
{
  gint i = 0;
  gboolean return_value = FALSE;

  g_return_val_if_fail (ATK_IS_ACTION(action), FALSE);

  for (i = 0; i < atk_action_get_n_actions(action); i++) {
    if (strcmp(atk_action_get_name(action, i),name)==0) {
      return_value = atk_action_do_action(action,i);
      break;
    }
  }

  return return_value;

}

