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
 * SECTION:hailapp_tests
 * @short_description: unit tests for Atk support of #HildonApp
 * @see_also: #HildonApp
 *
 * Checks accessibility support for #HildonApp
 */

#include <glib.h>
#include <gtk/gtkwidget.h>
#include <gtk/gtklabel.h>
#include <hildon-widgets/hildon-app.h>
#include <atk/atkcomponent.h>

#include <assert.h>
#include <string.h>
#include <hailapp_tests.h>
#include <hailappview_tests.h>

static const gchar * test_label = "test label";

static GtkWidget * get_hildon_app (void);

static GtkWidget * get_hildon_app (void)
{
  GtkWidget * app = NULL;

  app = hildon_app_new ();
  return app;
}

/**
 * test_app_get_accessible:
 *
 * should get the accessible object for #HildonApp
 *
 * Return value: 1 if the test is passed
 */
int test_app_get_accessible ()
{
    GtkWidget * app = NULL;
    AtkObject * accessible = NULL;

    app = get_hildon_app ();
    assert (HILDON_IS_APP (app));

    accessible = gtk_widget_get_accessible(app);
    assert (G_TYPE_CHECK_INSTANCE_TYPE(accessible, g_type_from_name("GailWindow")));
    return 1;
}

/**
 * test_app_get_children:
 *
 * check that there's no children when no #HildonAppView has been embedded
 * and check that there's one when there's a #HildonAppView  inside
 *
 * Return value: 1 if the test is passed
 */
int test_app_get_children(void)
{
    GtkWidget * app = NULL;
    AtkObject * accessible = NULL;
    GtkWidget * app_view = NULL;

    app = get_hildon_app ();
    assert (HILDON_IS_APP (app));

    accessible = gtk_widget_get_accessible(app);
    assert (G_TYPE_CHECK_INSTANCE_TYPE(accessible, g_type_from_name("GailWindow")));

    /* check that there are no children (no appview yet) */
    assert (atk_object_get_n_accessible_children(accessible) == 0);

    app_view = get_hildon_app_view();
    hildon_app_set_appview (HILDON_APP(app), HILDON_APPVIEW(app_view));

    /* check that now there is a children (the contained widget too) */
    assert (atk_object_get_n_accessible_children(accessible) == 1);
  
  return 1;
}

/**
 * test_app_get_name:
 *
 * check that the name of the #HildonApp is taken right through Atk
 *
 * Return value: 1 if the test is passed
 */
int test_app_get_name(void)
{
  GtkWidget * app = NULL;
  AtkObject * accessible = NULL;
  GtkWidget * app_view1 = NULL;
  
  app = get_hildon_app ();
  assert (HILDON_IS_APP (app));
  
  accessible = gtk_widget_get_accessible(app);
  assert (G_TYPE_CHECK_INSTANCE_TYPE(accessible, g_type_from_name("GailWindow")));

  app_view1 = get_hildon_app_view();
  hildon_app_set_appview (HILDON_APP(app), HILDON_APPVIEW(app_view1));

  gtk_widget_show(app);

  hildon_app_set_title(HILDON_APP(app), test_label);
  assert (GTK_WIDGET_REALIZED(app));
  assert(gtk_window_get_title(GTK_WINDOW(app)) != NULL);
  assert(atk_object_get_name(ATK_OBJECT(accessible)) != NULL);
  assert (strcmp(atk_object_get_name(ATK_OBJECT(accessible)), test_label) == 0);
  

  return 1;
}

/* this function is a signal handler to detect if the signal is emitted */
void test_app_children_changed_signal_handler (AtkObject * object,
					       guint child_index,
					       gpointer child_pointer,
					       gpointer user_data)
{
  gboolean * is_ok = NULL;
  is_ok = (gboolean *) user_data;
  *is_ok = 0;

  if (!G_TYPE_CHECK_INSTANCE_TYPE(object, g_type_from_name("GailWindow")))
    return;

  *is_ok = 1;
}

/**
 * test_app_children_changed:
 *
 * Check that when we change the HildonAppView embedded in HildonApp, the correspondant 
 * children-changed event is called
 *
 * Return value: 1 if the test is passed
 */
int test_app_children_changed (void)
{
    GtkWidget * app = NULL;
    AtkObject * accessible = NULL;
    GtkWidget * app_view1 = NULL;
    GtkWidget * app_view2 = NULL;
    AtkObject * accessible_app_view1 = NULL;
    AtkObject * accessible_app_view2 = NULL;
    AtkObject * accessible_ref = NULL;
    gboolean add_called = FALSE;

    app = get_hildon_app ();
    assert (HILDON_IS_APP (app));

    accessible = gtk_widget_get_accessible(app);
    assert (G_TYPE_CHECK_INSTANCE_TYPE(accessible, g_type_from_name("GailWindow")));

    g_signal_connect(accessible, "children-changed::add", 
		     (GCallback) test_app_children_changed_signal_handler,
		     &add_called);

    app_view1 = get_hildon_app_view();
    app_view2 = get_hildon_app_view();
    accessible_app_view1 = gtk_widget_get_accessible(app_view1);
    accessible_app_view2 = gtk_widget_get_accessible(app_view2);
    assert(accessible_app_view1 != accessible_app_view2);

    hildon_app_set_appview (HILDON_APP(app), HILDON_APPVIEW(app_view1));
    accessible_ref = atk_object_ref_accessible_child(accessible, 0);
    assert (accessible_ref == accessible_app_view1);
    g_object_unref(accessible_ref);
    
    hildon_app_set_appview (HILDON_APP(app), HILDON_APPVIEW(app_view2));
    accessible_ref = atk_object_ref_accessible_child(accessible, 1);
    assert (accessible_ref == accessible_app_view2);
    g_object_unref(accessible_ref);
    
    assert (atk_object_get_n_accessible_children(accessible) == 2);

    assert (add_called);
  
  return 1;
}

/**
 * test_app_role:
 *
 * HailApp role should be #ATK_ROLE_FRAME
 *
 * Return value: 1 if the test is passed
 */
int test_app_role(void)
{
  GtkWidget * app = NULL;
  AtkObject * accessible = NULL;
  
  app = get_hildon_app ();
  assert (HILDON_IS_APP (app));
  
  accessible = gtk_widget_get_accessible(app);
  assert (G_TYPE_CHECK_INSTANCE_TYPE(accessible, g_type_from_name("GailWindow")));

  assert(atk_object_get_role(accessible) == ATK_ROLE_FRAME);

  return 1;
}

