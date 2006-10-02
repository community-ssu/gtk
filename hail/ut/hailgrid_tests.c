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
 * SECTION:hailgrid_tests
 * @short_description: unit tests for Atk support of #HildonGrid and #HildonGridItem
 * @see_also: #HailGrid, #HailGridItem, #HildonGrid, #HildonGridItem
 *
 * Unit tests to check accessibility support of #HildonGrid and #HildonGridItem,
 * provided in hail by #HailGrid and #HailGridItem
 */

#include <glib.h>
#include <gtk/gtkwidget.h>
#include <gtk/gtkwindow.h>
#include <atk/atkaction.h>
#include <atk/atkimage.h>
#include <hildon-widgets/hildon-grid.h>
#include <hildon-widgets/hildon-grid-item.h>
#include <hail/hailgrid.h>
#include <hail/hailgriditem.h>

#include <assert.h>
#include <string.h>
#include <hail_tests_utils.h>

#include <hailgrid_tests.h>

static void
test_grid_item_actions_grid_get_activate_handler (HildonGrid * grid,
						  HildonGridItem * item,
						  gpointer user_data);
static void
test_grid_item_actions_grid_get_popup_handler (HildonGrid * grid,
					       GtkWidget *item,
					       gpointer user_data);


/**
 * test_grid_get_accessibles:
 *
 * get the accessibles of the grid and the grid item widgets
 *
 * Return value: 1 if the test is passed
 */
int
test_grid_get_accessibles (void)
{
  GtkWidget * window = NULL;
  GtkWidget * grid = NULL;
  GtkWidget * item1 = NULL;
  GtkWidget * item2 = NULL;
  AtkObject * grid_a = NULL;
  AtkObject * item1_a = NULL;
  AtkObject * item2_a = NULL;

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  grid = hildon_grid_new ();
  assert (HILDON_IS_GRID(grid));
  gtk_container_add(GTK_CONTAINER(window), grid);
  grid_a = gtk_widget_get_accessible(grid);
  assert (HAIL_IS_GRID(grid_a));

  item1 = hildon_grid_item_new("qgn_list_gene_unknown_file");
  assert(HILDON_IS_GRID_ITEM(item1));
  item1_a = gtk_widget_get_accessible(item1);
  assert(HAIL_IS_GRID_ITEM(item1_a));

  assert (atk_object_get_n_accessible_children(grid_a)==0);

  gtk_container_add(GTK_CONTAINER(grid), item1);

  item2 = hildon_grid_item_new("qgn_list_gene_unknown_file");
  assert(HILDON_IS_GRID_ITEM(item2));
  item2_a = gtk_widget_get_accessible(item2);
  assert(HAIL_IS_GRID_ITEM(item2_a));

  gtk_container_add(GTK_CONTAINER(grid), item2);

  assert (atk_object_get_n_accessible_children(grid_a)==2);

  return 1;
}

/* signal handler for activate in test_grid_item_activate test */
static void
test_grid_item_actions_grid_get_activate_handler (HildonGrid * grid,
						  HildonGridItem * item,
						  gpointer user_data)
{
  gboolean * received = NULL;
  
  received = (gboolean *) user_data;

  *received = TRUE;
}

/* signal handler for popup in test_grid_item_activate test */
void
test_grid_item_actions_grid_get_popup_handler (HildonGrid * grid,
					       GtkWidget *item,
					       gpointer user_data)
{
  gboolean * received = NULL;
  
  received = (gboolean *) user_data;

  *received = TRUE;
}

/**
 * test_grid_item_activate:
 *
 * activate a grid item and try to check if it works as expected
 * (calling the activate event). It also tests the popup event
 *
 * Return value: 1 if the test is passed
 */
int 
test_grid_item_activate (void)
{
  GtkWidget * window = NULL;
  GtkWidget * grid = NULL;
  GtkWidget * item1 = NULL;
  GtkWidget * item2 = NULL;
  AtkObject * grid_a = NULL;
  AtkObject * item1_a = NULL;
  AtkObject * item2_a = NULL;
  gboolean received = FALSE;
  gboolean popup_received = FALSE;

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  grid = hildon_grid_new ();
  gtk_container_add(GTK_CONTAINER(window), grid);
  grid_a = gtk_widget_get_accessible(grid);

  item1 = hildon_grid_item_new("qgn_list_gene_unknown_file");
  item1_a = gtk_widget_get_accessible(item1);
  gtk_container_add(GTK_CONTAINER(grid), item1);

  item2 = hildon_grid_item_new("qgn_list_gene_unknown_file");
  item2_a = gtk_widget_get_accessible(item2);
  gtk_container_add(GTK_CONTAINER(grid), item2);

  assert (atk_object_get_n_accessible_children(grid_a)==2);

  g_signal_connect(grid, "activate-child", 
		   (GCallback)test_grid_item_actions_grid_get_activate_handler,
		   &received);

  gtk_widget_show_all(grid);

  assert (ATK_IS_ACTION(item1_a));
  assert(atk_action_do_action(ATK_ACTION(item1_a), 0));
  hail_tests_utils_button_waiter(1000);

  assert(received);

  g_signal_connect(grid, "popup-context-menu", 
		   (GCallback)test_grid_item_actions_grid_get_popup_handler,
		   &popup_received);

  assert (ATK_IS_ACTION(grid_a));
  assert(atk_action_do_action(ATK_ACTION(grid_a), 0));
  hail_tests_utils_button_waiter(1000);

  assert (popup_received);
  return 1;
}

/**
 * test_grid_item_image:
 *
 * Check #AtkImage support of #HildonGridItem
 *
 * Return value: 1 if the test is passed
 */
int 
test_grid_item_image (void)
{
  GtkWidget * window = NULL;
  GtkWidget * grid = NULL;
  GtkWidget * item1 = NULL;
  AtkObject * grid_a = NULL;
  AtkObject * item1_a = NULL;

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  grid = hildon_grid_new ();
  assert (HILDON_IS_GRID(grid));
  gtk_container_add(GTK_CONTAINER(window), grid);
  grid_a = gtk_widget_get_accessible(grid);
  assert (HAIL_IS_GRID(grid_a));
  gtk_widget_show_all(window);

  item1 = hildon_grid_item_new_with_label("qgn_list_gene_unknown_file", "Test label");
  assert(HILDON_IS_GRID_ITEM(item1));
  item1_a = gtk_widget_get_accessible(item1);
  assert(HAIL_IS_GRID_ITEM(item1_a));

  assert (atk_object_get_n_accessible_children(grid_a)==0);
  gtk_container_add(GTK_CONTAINER(grid), item1);

  assert (ATK_IS_IMAGE(item1_a));
  assert (strcmp(atk_image_get_image_description(ATK_IMAGE(item1_a)), "Test label")==0);
  hildon_grid_item_set_emblem_type(HILDON_GRID_ITEM(item1), "qgn_list_gene_unknown_file");
  assert (strcmp(atk_image_get_image_description(ATK_IMAGE(item1_a)), "Test label(qgn_list_gene_unknown_file)")==0);
  

  return 1;
}

/**
 * test_grid_component:
 *
 * Check #AtkComponent support of #HildonGrid
 *
 * Return value: 1 if the test is passed
 */
int
test_grid_component (void)
{
  GtkWidget * window = NULL;
  GtkWidget * grid = NULL;
  GtkWidget * item1 = NULL;
  AtkObject * grid_a = NULL;
  AtkObject * item1_a = NULL;
  AtkObject * tmp_a = NULL;
  gint x, y, width, height;

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  grid = hildon_grid_new ();
  assert (HILDON_IS_GRID(grid));
  grid_a = gtk_widget_get_accessible(grid);
  assert (HAIL_IS_GRID(grid_a));

  item1 = hildon_grid_item_new_with_label("qgn_list_gene_unknown_file", "Test label");
  assert(HILDON_IS_GRID_ITEM(item1));
  item1_a = gtk_widget_get_accessible(item1);
  hildon_grid_item_set_emblem_type(HILDON_GRID_ITEM(item1), "qgn_list_gene_unknown_file");
  assert(HAIL_IS_GRID_ITEM(item1_a));

  assert (atk_object_get_n_accessible_children(grid_a)==0);
  gtk_container_add(GTK_CONTAINER(grid), item1);
  gtk_container_add(GTK_CONTAINER(window), grid);
  gtk_widget_show_all(window);

  assert(ATK_IS_COMPONENT(item1_a));
  assert(ATK_IS_COMPONENT(grid_a));

  hail_tests_utils_button_waiter(600);

  atk_component_get_extents(ATK_COMPONENT(item1_a), &x, &y, &width, &height, ATK_XY_SCREEN);
  assert(atk_component_contains(ATK_COMPONENT(item1_a), x, y, ATK_XY_SCREEN));
  tmp_a = atk_component_ref_accessible_at_point(ATK_COMPONENT(grid_a), x, y, ATK_XY_SCREEN);
  assert(tmp_a != NULL);
  assert(tmp_a == item1_a);
  g_object_unref(tmp_a);

  tmp_a = atk_component_ref_accessible_at_point(ATK_COMPONENT(grid_a), x-1, y-1, ATK_XY_SCREEN);
  assert(tmp_a == NULL);

  return 1;
}


