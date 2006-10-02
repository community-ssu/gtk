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
 * SECTION:hailappview_tests
 * @short_description: unit tests for Atk support of #HildonAppview
 * @see_also: #HailAppView, #HildonAppview
 *
 * Checks accessibility support for #HildonAppview, provided
 * in hail by #HailAppView
 */


#include <glib.h>
#include <gtk/gtkwidget.h>
#include <hildon-widgets/hildon-appview.h>
#include <hildon-widgets/hildon-app.h>
#include <hail/hailappview.h>
#include <gtk/gtklabel.h>

#include <string.h>
#include <assert.h>

static const gchar * test_label = "test label";


static const gchar * test_app_view_title = "app view title";

/**
 * get_hildon_app_view:
 *
 * obtains a #HildonAppView instance ready to use in tests
 *
 * Return value: a #HildonAppView
 */
GtkWidget * 
get_hildon_app_view (void)
{
  GtkWidget * app_view = NULL;

  app_view = hildon_appview_new (test_app_view_title);
  return app_view;
}

/**
 * test_app_view_get_accessible:
 *
 * should get the accessible object for hildon-caption
 *
 * Return value: 1 if the test is passed.
 */
int test_app_view_get_accessible ()
{
    GtkWidget * app_view = NULL;
    AtkObject * accessible = NULL;

    app_view = get_hildon_app_view ();
    assert (HILDON_IS_APPVIEW (app_view));

    accessible = gtk_widget_get_accessible(app_view);
    assert (HAIL_IS_APP_VIEW (accessible));

    return 1;
}

/**
 * test_app_view_get_children:
 *
 * should get the expected accessible children of the app view
 *
 * Return value: 1 if the test is passed.
 */
int test_app_view_get_children(void)
{
    GtkWidget * app_view = NULL;
    AtkObject * accessible = NULL;
    GtkWidget * label = NULL;

    app_view = get_hildon_app_view ();
    assert (HILDON_IS_APPVIEW (app_view));

    accessible = gtk_widget_get_accessible(app_view);
    assert (HAIL_IS_APP_VIEW (accessible));

    /* check that there are 2 children (toolbar and menu) */
    assert (atk_object_get_n_accessible_children(accessible) == 2);

    label = gtk_label_new(test_label);
    gtk_container_add(GTK_CONTAINER(app_view), label);

    /* check that now there are 3 children (the contained widget too) */
    assert (atk_object_get_n_accessible_children(accessible) == 3);
  
  return 1;
}

/**
 * test_app_view_get_bin:
 *
 * should get the embedded widget  of the app view
 *
 * Return value: 1 if the test is passed.
 */
int test_app_view_get_bin(void)
{
    GtkWidget * app_view = NULL;
    AtkObject * accessible = NULL;
    GtkWidget * label = NULL;
    AtkObject * accessible_child = NULL;

    app_view = get_hildon_app_view ();
    assert (HILDON_IS_APPVIEW (app_view));

    accessible = gtk_widget_get_accessible(app_view);
    assert (HAIL_IS_APP_VIEW (accessible));

    label = gtk_label_new(test_label);
    gtk_container_add(GTK_CONTAINER(app_view), label);

    accessible_child = atk_object_ref_accessible_child(accessible, 0);
    assert(GTK_ACCESSIBLE(accessible_child)->widget == label);
  
  return 1;
}

/**
 * test_app_view_get_toolbars:
 *
 * should get the toolbars  of the app view
 *
 * Return value: 1 if the test is passed.
 */
int test_app_view_get_toolbars(void)
{
    GtkWidget * app_view = NULL;
    AtkObject * accessible = NULL;
    GtkWidget * label = NULL;
    AtkObject * accessible_child = NULL;
    AtkObject * accessible_toolbar = NULL;

    app_view = get_hildon_app_view ();
    assert (HILDON_IS_APPVIEW (app_view));

    accessible = gtk_widget_get_accessible(app_view);
    assert (HAIL_IS_APP_VIEW (accessible));

    /* get accessible of the toolbar container */
    accessible_toolbar = gtk_widget_get_accessible(HILDON_APPVIEW(app_view)->vbox);
    assert (ATK_IS_OBJECT(accessible_toolbar));

    accessible_child = atk_object_ref_accessible_child(accessible, 0);
    assert(accessible_child == accessible_toolbar);
    g_object_unref(accessible_child);

    label = gtk_label_new(test_label);
    gtk_container_add(GTK_CONTAINER(app_view), label);

    accessible_child = atk_object_ref_accessible_child(accessible, 1);
    assert(accessible_child == accessible_toolbar);
    g_object_unref(accessible_child);
  
  return 1;
}

/**
 * test_app_view_get_menu:
 *
 * Tries to get the menu of the app view
 *
 * Return value: 1 if the test is passed.
 */
int test_app_view_get_menu(void)
{
    GtkWidget * app_view = NULL;
    AtkObject * accessible = NULL;
    GtkWidget * label = NULL;
    AtkObject * accessible_child = NULL;
    AtkObject * accessible_menu = NULL;

    app_view = get_hildon_app_view ();
    assert (HILDON_IS_APPVIEW (app_view));

    accessible = gtk_widget_get_accessible(app_view);
    assert (HAIL_IS_APP_VIEW (accessible));

    /* get accessible of the toolbar container */
    accessible_menu = gtk_widget_get_accessible(GTK_WIDGET(hildon_appview_get_menu(HILDON_APPVIEW(app_view))));
    assert (ATK_IS_OBJECT(accessible_menu));

    accessible_child = atk_object_ref_accessible_child(accessible, 1);
    assert(accessible_child == accessible_menu);
    g_object_unref(accessible_child);

    label = gtk_label_new(test_label);
    gtk_container_add(GTK_CONTAINER(app_view), label);

    accessible_child = atk_object_ref_accessible_child(accessible, 2);
    assert(accessible_child == accessible_menu);
    g_object_unref(accessible_child);
  return 1;
}

/**
 * test_app_view_fullscreen_actions:
 *
 * Checks the fullscreen actions. It gets a #HildonAppView embedded in a #HildonApp. Then
 * it checks that without fullscreen key allowed, we cannot get an action. After this,
 * it changes the state of fullscreen with do_action to check if it's updated correctly
 *
 * Return value: 1 if the test is passed.
 */
int test_app_view_fullscreen_actions(void)
{
   GtkWidget * app_view = NULL;
    AtkObject * accessible = NULL;
    GtkWidget * app = NULL;

    app_view = get_hildon_app_view ();
    assert (HILDON_IS_APPVIEW (app_view));

    app = hildon_app_new ();
    assert(HILDON_IS_APP(app));

    hildon_app_set_appview(HILDON_APP(app), HILDON_APPVIEW(app_view));
    gtk_widget_show(app);

    accessible = gtk_widget_get_accessible(app_view);
    assert (HAIL_IS_APP_VIEW (accessible));

    assert (!hildon_appview_get_fullscreen_key_allowed(HILDON_APPVIEW(app_view)));
    assert (atk_action_get_n_actions(ATK_ACTION(accessible)) == 0);

    hildon_appview_set_fullscreen_key_allowed(HILDON_APPVIEW(app_view), TRUE);
    assert (atk_action_get_n_actions(ATK_ACTION(accessible)) == 1);

    assert(!hildon_appview_get_fullscreen(HILDON_APPVIEW(app_view)));
    assert (strcmp(atk_action_get_name(ATK_ACTION(accessible),0),"fullscreen")==0);
    assert (atk_action_do_action(ATK_ACTION(accessible),0));

    assert(hildon_appview_get_fullscreen(HILDON_APPVIEW(app_view)));
    assert (strcmp(atk_action_get_name(ATK_ACTION(accessible),0),"unfullscreen")==0);
    assert (atk_action_do_action(ATK_ACTION(accessible),0));
    assert (!hildon_appview_get_fullscreen(HILDON_APPVIEW(app_view)));

    return 1;
}

/**
 * test_app_view_get_role:
 *
 * check that the role for a HildonAppView is a #ATK_ROLE_FRAME
 *
 * Return value: 1 if the test is passed.
 */
int test_app_view_get_role(void)
{
   GtkWidget * app_view = NULL;
    AtkObject * accessible = NULL;

    app_view = get_hildon_app_view ();
    assert (HILDON_IS_APPVIEW (app_view));

    accessible = gtk_widget_get_accessible(app_view);
    assert (HAIL_IS_APP_VIEW (accessible));

    assert(atk_object_get_role(accessible) == ATK_ROLE_FRAME);

    return 1;
}

