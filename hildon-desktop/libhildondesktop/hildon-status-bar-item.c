/*
 * This file is part of maemo-af-desktop
 *
 * Copyright (C) 2007 Nokia Corporation.
 *
 * Author: Moises Martinez <moises.martinez@nokia.com>
 * Contact: Karoliina Salminen <karoliina.t.salminen@nokia.com>
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

#include "hildon-status-bar-item.h" 

void 
hildon_status_bar_item_update (HildonStatusBarItem *item, gint value1, gint value2, const gchar *str)
{

}

void 
hildon_status_bar_item_update_conditional_cb (HildonStatusBarItem *item, gboolean user_data)
{
  g_assert (STATUSBAR_IS_ITEM (item));	
	
  g_object_set (item, "condition", user_data, NULL);
}

void 
hildon_status_bar_item_set_position (HildonStatusBarItem *item, gint position)
{
  g_assert (STATUSBAR_IS_ITEM (item));

  g_object_set (G_OBJECT (item), "position",position,NULL);
}

gint 
hildon_status_bar_item_get_position (HildonStatusBarItem *item)
{
  gint position;
	
  g_assert (STATUSBAR_IS_ITEM (item));

  g_object_get (G_OBJECT (item), "position", &position, NULL);

  return position;
}

gint 
hildon_status_bar_item_get_priority (HildonStatusBarItem *item)
{
  return 0;
}

const gchar *
hildon_status_bar_item_get_name (HildonStatusBarItem *item)
{
  gchar *name = NULL;

  g_assert (STATUSBAR_IS_ITEM (item));

  g_object_get (G_OBJECT (item), "name", &name, NULL);

  return name;
}

gboolean 
hildon_status_bar_item_get_conditional (HildonStatusBarItem *item)
{
  gboolean conditional;

  g_assert (STATUSBAR_IS_ITEM (item));

  g_object_get (G_OBJECT (item), "conditional", &conditional, NULL);

  return conditional;
}

gboolean 
hildon_status_bar_item_get_mandatory (HildonStatusBarItem *item)
{
  gboolean mandatory;

  g_assert (STATUSBAR_IS_ITEM (item));

  g_object_get (G_OBJECT (item), "mandatory", &mandatory, NULL);

  return mandatory;
}

void 
hildon_status_bar_item_set_button (HildonStatusBarItem *item, GtkWidget *button)
{
  g_assert (STATUSBAR_IS_ITEM (item));

  if (button)
  {	  
    if (GTK_BIN (item)->child)
      gtk_container_remove (GTK_CONTAINER (item),GTK_BIN (item)->child);

    gtk_container_add (GTK_CONTAINER (item), button);

    gtk_widget_show (button); 
  }
}

GtkWidget *
hildon_status_bar_item_get_button (HildonStatusBarItem *item)
{
  g_assert (STATUSBAR_IS_ITEM (item));

  return GTK_BIN (item)->child;
}
