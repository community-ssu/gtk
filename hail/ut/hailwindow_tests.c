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
 * SECTION:hailwindow_tests
 * @short_description: unit tests for Atk support of #HildonWindow
 * @see_also: #HailWindow, #HildonWindow
 *
 * Checks accessibility support for #HildonWindow, provided
 * in hail by #HailWindow
 */


#include <glib.h>
#include <gtk/gtkwidget.h>
#include <gtk/gtkstock.h>
#include <gtk/gtklabel.h>
#include <gtk/gtktoolbutton.h>
#include <hildon-widgets/hildon-window.h>
#include <hildon-widgets/hildon-program.h>
#include <hail/hailwindow.h>

#include <string.h>
#include <assert.h>
#include <hail_tests_utils.h>

static const gchar * test_label = "test label";

/* private functions */
static GtkWidget * get_hildon_window (void);

/* obtains a instance of #HildonWindow */
static GtkWidget * get_hildon_window ()
{
  HildonProgram * program = NULL;
  GtkWidget * window = NULL;
  GtkWidget * label = NULL;

  program = hildon_program_get_instance();
  window = hildon_window_new();
  hildon_program_add_window(program, HILDON_WINDOW(window));

  label = gtk_label_new(test_label);
  gtk_container_add(GTK_CONTAINER(window), label);

  gtk_widget_show_all(window);

  return window;
}

/**
 * test_window_get_accessible:
 *
 * should get the accessible object for hildon-caption
 *
 * Return value: 1 if the test is passed.
 */
int test_window_get_accessible ()
{
    GtkWidget * window = NULL;
    AtkObject * accessible = NULL;

    window = get_hildon_window ();
    assert (HILDON_IS_WINDOW (window));

    accessible = gtk_widget_get_accessible(window);
    assert (HAIL_IS_WINDOW (accessible));

    return 1;
}

/**
 * test_window_get_children:
 *
 * should get the expected accessible children of the app view
 *
 * Return value: 1 if the test is passed.
 */
int test_window_get_children(void)
{
    GtkWidget * window = NULL;
    AtkObject * accessible = NULL;
    GtkWidget * label = NULL;

    window = hildon_window_new ();
    assert (HILDON_IS_WINDOW (window));

    accessible = gtk_widget_get_accessible(window);
    assert (HAIL_IS_WINDOW (accessible));

    gtk_widget_show_all(window);

    /* check that there is 1 children (toolbar container) */
    assert (atk_object_get_n_accessible_children(accessible) == 1);

    label = gtk_label_new(test_label);
    gtk_container_add(GTK_CONTAINER(window), label);

    gtk_widget_show(label);

    /* check that now there are 2 children (the contained widget too) */
    assert (atk_object_get_n_accessible_children(accessible) == 2);
  
  return 1;
}

/**
 * test_window_get_bin:
 *
 * should get the embedded widget  of the app view
 *
 * Return value: 1 if the test is passed.
 */
int test_window_get_bin(void)
{
    GtkWidget * window = NULL;
    AtkObject * accessible = NULL;
    GtkWidget * label = NULL;
    AtkObject * accessible_child = NULL;

    window = hildon_window_new ();
    assert (HILDON_IS_WINDOW (window));

    accessible = gtk_widget_get_accessible(window);
    assert (HAIL_IS_WINDOW (accessible));

    label = gtk_label_new(test_label);
    gtk_container_add(GTK_CONTAINER(window), label);

    gtk_widget_show_all(window);

    accessible_child = atk_object_ref_accessible_child(accessible, 0);
    assert(GTK_ACCESSIBLE(accessible_child)->widget == label);
  
  return 1;
}

/**
 * test_window_get_toolbars:
 *
 * This test tests the toolbars support in #HildonWindow and
 * #HildonProgram, through accessibility support. The steps of the
 * test are:
 * <itemizedlist>
 * <listitem>First it builds a #HildonWindow linked to the #HildonProgram,
 * and checks there's no toolbar</listitem>
 * <listitem>Then it defines a common toolbar for the program, and checks
 * the access to this toolbar</listitem>
 * <listitem>Finally it adds a toolbar to the window, and checks that the
 * 2 toolbars are exposed</listitem>
 * </itemizedlist>
 *
 * Return value: 1 if the test is passed.
 */
int test_window_get_toolbars(void)
{
  GtkWidget * window = NULL;
  AtkObject * accessible = NULL;
  AtkObject * accessible_box = NULL;
  AtkObject * accessible_toolbar = NULL;
  AtkObject * accessible_item = NULL;
  AtkObject * accessible_button = NULL;
  HildonProgram * program = NULL;
  GtkWidget * toolbar1 = NULL;
  GtkWidget * toolbar2 = NULL;
  GtkToolItem * toolitem1 = NULL;
  GtkToolItem * toolitem2 = NULL;

  window = get_hildon_window();
  assert(HILDON_IS_WINDOW(window));

  accessible = gtk_widget_get_accessible(window);
  assert(HAIL_IS_WINDOW(accessible));

  /* instances of the toolbars */
  toolbar1 = gtk_toolbar_new();
  toolitem1 = gtk_tool_button_new_from_stock(GTK_STOCK_OPEN);
  assert(GTK_IS_TOOL_ITEM(toolitem1));
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar1), toolitem1, -1);
  
  toolbar2 = gtk_toolbar_new();
  toolitem2 = gtk_tool_button_new_from_stock(GTK_STOCK_CLOSE);
  assert(GTK_IS_TOOL_ITEM(toolitem2));
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar2), toolitem2, -1);

  gtk_widget_show_all(window);

  /* check the status without toolbars */
  accessible_box = atk_object_ref_accessible_child(accessible, 1);
  assert(ATK_IS_OBJECT(accessible_box));
  assert(atk_object_get_role(accessible_box) == ATK_ROLE_FILLER);
  assert(atk_object_get_n_accessible_children(accessible_box)==0);

  /* add a common toolbar.  */
  program = hildon_program_get_instance();
  hildon_program_set_common_toolbar(program, GTK_TOOLBAR(toolbar1));
  gtk_widget_show_all(window);

  hail_tests_utils_button_waiter(1000);

  assert(atk_object_get_n_accessible_children(accessible_box)==1);
  accessible_toolbar = atk_object_ref_accessible_child(accessible_box, 0);
  assert(ATK_IS_OBJECT(accessible_toolbar));
  assert(atk_object_get_role(accessible_toolbar) == ATK_ROLE_TOOL_BAR);
  assert(atk_object_get_n_accessible_children(accessible_toolbar)==1);
  accessible_item = atk_object_ref_accessible_child(accessible_toolbar, 0);
  assert(ATK_IS_OBJECT(accessible_item));
  assert(atk_object_get_n_accessible_children(accessible_item)==1);
  accessible_button = atk_object_ref_accessible_child(accessible_item, 0);
  assert(ATK_IS_OBJECT(accessible_button));
  assert(atk_object_get_role(accessible_button) == ATK_ROLE_PUSH_BUTTON);
  assert(strcmp(atk_object_get_name(accessible_button),
		"Open")==0);
  g_object_unref(accessible_button);
  g_object_unref(accessible_item);
  
  /* add a common toolbar.  */
  hildon_window_add_toolbar(HILDON_WINDOW(window), GTK_TOOLBAR(toolbar2));
  gtk_widget_show_all(window);
  assert(atk_object_get_n_accessible_children(accessible_box)==2);

  accessible_toolbar = atk_object_ref_accessible_child(accessible_box, 0);
  assert(ATK_IS_OBJECT(accessible_toolbar));
  assert(atk_object_get_role(accessible_toolbar) == ATK_ROLE_TOOL_BAR);
  assert(atk_object_get_n_accessible_children(accessible_toolbar)==1);
  accessible_item = atk_object_ref_accessible_child(accessible_toolbar, 0);
  assert(ATK_IS_OBJECT(accessible_item));
  assert(atk_object_get_n_accessible_children(accessible_item)==1);
  accessible_button = atk_object_ref_accessible_child(accessible_item, 0);
  assert(ATK_IS_OBJECT(accessible_button));
  assert(atk_object_get_role(accessible_button) == ATK_ROLE_PUSH_BUTTON);
  assert(strcmp(atk_object_get_name(accessible_button),
		"Close")==0);
  g_object_unref(accessible_button);
  g_object_unref(accessible_item);
  
  return 1;
}

/**
 * test_window_get_menus:
 *
 * Tries to get the menu of the app view
 *
 * Return value: 1 if the test is passed.
 */
int test_window_get_menus(void)
{
    GtkWidget * window = NULL;
    AtkObject * accessible = NULL;
    AtkObject * accessible_child = NULL;
    AtkObject * accessible_child2 = NULL;
    GtkWidget * menu1 = NULL;
    GtkWidget * menu2 = NULL;
    GtkWidget * item = NULL;
    HildonProgram * program = NULL;

    window = get_hildon_window ();
    assert (HILDON_IS_WINDOW (window));

    accessible = gtk_widget_get_accessible(window);
    assert (HAIL_IS_WINDOW (accessible));

    /* prepare menus */
    menu1 = gtk_menu_new();
    item = gtk_menu_item_new_with_label("menu item 1");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu1), item);

    menu2 = gtk_menu_new();
    item = gtk_menu_item_new_with_label("menu item 2");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu2), item);

    /* first check: without any menu. It should return 2
     * as there are no menus, but a toolbar container and
     * the label inside the window */
    assert(atk_object_get_n_accessible_children(accessible)==2);

    /* second check: with the program menu. It should return 3
     * being 0 = label, 1 = toolbar container and 2 = menu item 1
     */
    program = hildon_program_get_instance();
    hildon_program_set_common_menu(program, GTK_MENU(menu1));
    assert(atk_object_get_n_accessible_children(accessible)==3);
    accessible_child = atk_object_ref_accessible_child(accessible, 2);
    assert(ATK_IS_OBJECT(accessible_child));
    assert(atk_object_get_role(accessible_child)==ATK_ROLE_MENU);
    assert(atk_object_get_n_accessible_children(accessible_child)==1);
    accessible_child2 = atk_object_ref_accessible_child(accessible_child, 0);
    assert(ATK_IS_OBJECT(accessible_child2));
    assert(strcmp(atk_object_get_name(accessible_child2), "menu item 1")==0);
    g_object_unref(accessible_child2);
    g_object_unref(accessible_child);

    /* third check: with the window menu. It should return 3
     * being 0 = label, 1 = toolbar container and 2 = menu item 1
     */
    hildon_window_set_menu(HILDON_WINDOW(window), GTK_MENU(menu2));
    assert(atk_object_get_n_accessible_children(accessible)==3);
    accessible_child = atk_object_ref_accessible_child(accessible, 2);
    assert(ATK_IS_OBJECT(accessible_child));
    assert(atk_object_get_role(accessible_child)==ATK_ROLE_MENU);
    assert(atk_object_get_n_accessible_children(accessible_child)==1);
    accessible_child2 = atk_object_ref_accessible_child(accessible_child, 0);
    assert(ATK_IS_OBJECT(accessible_child2));
    assert(strcmp(atk_object_get_name(accessible_child2), "menu item 2")==0);
    g_object_unref(accessible_child2);
    g_object_unref(accessible_child);

  return 1;
}

/**
 * test_window_get_role:
 *
 * check that the role for a #HildonWindow is a #ATK_ROLE_FRAME
 *
 * Return value: 1 if the test is passed.
 */
int test_window_get_role(void)
{
   GtkWidget * window = NULL;
    AtkObject * accessible = NULL;

    window = get_hildon_window ();
    assert (HILDON_IS_WINDOW (window));

    accessible = gtk_widget_get_accessible(window);
    assert (HAIL_IS_WINDOW (accessible));

    assert(atk_object_get_role(accessible) == ATK_ROLE_FRAME);

    return 1;
}

/**
 * test_window_get_scrolled_window:
 *
 * Check that, if we add a the widget inside the #HildonWindow
 * with scrollbar, it gets the expected accessibilty structure
 * (the widget inside the scrolled window inside the window).
 *
 * Return value: 1 if the test is passed.
 */
int test_window_get_scrolled_window(void)
{
  GtkWidget * window = NULL;
  AtkObject * accessible = NULL;
  AtkObject * accessible_scroll = NULL;
  AtkObject * accessible_scrolled = NULL;
  AtkObject * accessible_label = NULL;
  GtkWidget * label = NULL;

  window = hildon_window_new();
  hildon_program_add_window(hildon_program_get_instance(), HILDON_WINDOW(window));
  label = gtk_label_new("test label");
  hildon_window_add_with_scrollbar(HILDON_WINDOW(window), label);
    
  assert (HILDON_IS_WINDOW (window));
  gtk_widget_show_all(window);

  accessible = gtk_widget_get_accessible(window);
  assert (HAIL_IS_WINDOW (accessible));

  assert(atk_object_get_n_accessible_children(accessible)==2);
  accessible_scroll = atk_object_ref_accessible_child(accessible, 0);
  assert(ATK_IS_OBJECT(accessible_scroll));
  assert(atk_object_get_role(accessible_scroll)==ATK_ROLE_SCROLL_PANE);
  assert(atk_object_get_n_accessible_children(accessible_scroll)==1);

  accessible_scrolled = atk_object_ref_accessible_child(accessible_scroll, 0);
  assert(ATK_IS_OBJECT(accessible_scrolled));
  accessible_label = atk_object_ref_accessible_child(accessible_scrolled, 0);
  assert(ATK_IS_OBJECT(accessible_label));
  assert(strcmp(atk_object_get_name(accessible_label), "test label")==0);
  
  return 1;
}

