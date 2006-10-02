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
 * SECTION:hailcaption_tests
 * @short_description: unit tests for Atk support of #HildonCaption
 * @see_also: #HailCaption, #HildonCaption
 *
 * Checks accessibility support for #HildonCaption, provided
 * in hail by #HailCaption
 */

#include <gtk/gtkentry.h>
#include <hildon-widgets/hildon-caption.h>
#include <hail/hailcaption.h>
#include <gtk/gtkwidget.h>
#include <gtk/gtkwindow.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkimage.h>
#include <gtk/gtkstock.h>

#include <string.h>
#include <assert.h>

static const gchar * test_label = "test label";
static const gchar * test_label_with_separator = "test label:";
static const gchar * test_label2 = "test label 2";
static const gchar * test_label2_with_separator = "test label 2:";
static const gchar * test_label2_with_separator2 = "test label 2;";
static const gchar * separator = ":";
static const gchar * separator2 = ";";


static GtkWidget * get_hildon_caption (const gchar * label);

static GtkWidget * get_hildon_caption (const gchar * label)
{
    GtkWidget * caption = NULL;
    GtkWidget * entry = NULL;

    entry = gtk_entry_new();

    caption = hildon_caption_new (NULL, label, entry, NULL, HILDON_CAPTION_MANDATORY);

    return caption;
}

/**
 * test_caption_get_accessible:
 *
 * this tests should get the accessible object for a #HildonCaption
 *
 * Return value: 1 if test is passed
 */
int 
test_caption_get_accessible ()
{
    GtkWidget * caption = NULL;
    AtkObject * accessible = NULL;

    caption = get_hildon_caption (test_label);
    assert (HILDON_IS_CAPTION (caption));

    accessible = gtk_widget_get_accessible(caption);
    assert (HAIL_IS_CAPTION (accessible));

    return 1;
}

/**
 * test_caption_follows_hildon_caption_label:
 *
 * this test changes the #HildonCaption label, and check the
 * caption accessible name follows the label content.
 *
 * Return value: 1 if test is passed
 */
int 
test_caption_follows_hildon_caption_label ()
{
  GtkWidget * caption = NULL;
  AtkObject * accessible = NULL;

  caption = get_hildon_caption (test_label);
  assert (HILDON_IS_CAPTION (caption));

  hildon_caption_set_separator(HILDON_CAPTION(caption), separator);

  accessible = gtk_widget_get_accessible (caption);
  assert (HAIL_IS_CAPTION (accessible));

  assert (strcmp(atk_object_get_name(ATK_OBJECT(accessible)), test_label_with_separator) == 0);

  hildon_caption_set_label(HILDON_CAPTION(caption), test_label2);
  assert (strcmp(atk_object_get_name(ATK_OBJECT(accessible)), test_label2_with_separator) == 0);

  hildon_caption_set_separator(HILDON_CAPTION(caption), separator2);
  assert (strcmp(atk_object_get_name(ATK_OBJECT(accessible)), test_label2_with_separator2) == 0);

  return 1;
}

/**
 * test_caption_get_control:
 *
 * this test gets the child of the accessible and check it's
 * the expected accessible of the control inside the caption.
 *
 * Return value: 1 if test is passed
 */
int
test_caption_get_control(void)
{
  GtkWidget * caption = NULL;
  AtkObject * accessible = NULL;
  AtkObject * accessible_entry = NULL;
  GtkWidget * entry = NULL;

  entry = gtk_entry_new();
  caption = hildon_caption_new(NULL, test_label, entry, NULL, HILDON_CAPTION_MANDATORY);

  accessible = gtk_widget_get_accessible (caption);

  /* check there's only one child */
  assert (atk_object_get_n_accessible_children(accessible)== 1);

  accessible_entry = (atk_object_ref_accessible_child (accessible, 0));

  /* check that the child name is "control" as we didn't provide a name to the accessible */
  assert (strcmp(atk_object_get_name(accessible_entry), "control")==0);

  /* check that the child is an accessible object */
  assert (ATK_IS_OBJECT(accessible_entry));

  /* check that the child is the expected one */
  assert (GTK_ACCESSIBLE(accessible_entry)->widget == entry);

  g_object_unref(accessible_entry);

  return 1;
}

/**
 * test_caption_get_label_relation:
 *
 * Check that the accessible labels the accessible of the embedded control
 *
 * Return value: 1 if test is passed
 */
int 
test_caption_get_label_relation(void)
{
  GtkWidget * caption = NULL;
  AtkObject * accessible = NULL;
  AtkObject * accessible_entry = NULL;
  GtkWidget * entry = NULL;
  AtkRelationSet * relation_set = NULL;
  AtkRelation * relation = NULL;
  GPtrArray * relation_array = NULL;

  entry = gtk_entry_new();
  caption = hildon_caption_new(NULL, test_label, entry, NULL, HILDON_CAPTION_MANDATORY);

  accessible = gtk_widget_get_accessible (caption);

  /* check there's only one child */
  assert (atk_object_get_n_accessible_children(accessible)== 1);

  accessible_entry = (atk_object_ref_accessible_child (accessible, 0));

  /* check that the child is an accessible object */
  assert (ATK_IS_OBJECT(accessible_entry));

  relation_set = atk_object_ref_relation_set (accessible);
  assert (atk_relation_set_get_n_relations(relation_set)==1);
  relation = atk_relation_set_get_relation_by_type(relation_set, ATK_RELATION_LABEL_FOR);

  /* check that the relation exists, and it refers to the expected accessible objects */
  assert (ATK_IS_RELATION(relation));
  relation_array = atk_relation_get_target(relation);
  assert (relation_array != NULL);
  assert (g_ptr_array_index(relation_array, 0) == accessible_entry);
  assert (ATK_IS_OBJECT(g_ptr_array_index(relation_array,0)));

  g_object_unref(relation_set);

  return 1;
}

/**
 * test_caption_get_role:
 *
 * Check that the role of the caption is panel
 *
 * Return value: 1 if test is passed
 */
int
test_caption_get_role(void)
{
  GtkWidget * caption = NULL;
  AtkObject * accessible = NULL;
  
  caption = get_hildon_caption (test_label);
  assert (HILDON_IS_CAPTION (caption));
  
  accessible = gtk_widget_get_accessible(caption);
  assert (HAIL_IS_CAPTION (accessible));
  
  assert (atk_object_get_role(accessible) == ATK_ROLE_PANEL);  

  return 1;
}

/**
 * test_caption_required_state:
 *
 * Check that the state "required" is managed the same way mandatory status is managed
 * in hildon caption
 *
 * Return value: 1 if test is passed
 */
int
test_caption_required_state (void)
{
    GtkWidget * caption = NULL;
    AtkObject * accessible = NULL;
    AtkStateSet * state_set = NULL;

    caption = get_hildon_caption (test_label);
    assert (HILDON_IS_CAPTION (caption));
    gtk_widget_show(caption);

    assert (hildon_caption_get_status(HILDON_CAPTION(caption))==HILDON_CAPTION_MANDATORY);

    accessible = gtk_widget_get_accessible(caption);
    assert (HAIL_IS_CAPTION (accessible));

    state_set = atk_object_ref_state_set(accessible);
    assert(atk_state_set_contains_state(state_set, ATK_STATE_REQUIRED));

    hildon_caption_set_status (HILDON_CAPTION(caption), HILDON_CAPTION_OPTIONAL);
    assert(hildon_caption_get_status(HILDON_CAPTION(caption)) == HILDON_CAPTION_OPTIONAL);

    state_set = atk_object_ref_state_set(accessible);
    assert(!atk_state_set_contains_state(state_set, ATK_STATE_REQUIRED));

  return 1;
}

/**
 * test_caption_component:
 *
 * Check the #AtkComponent interface of #HailCaption
 *
 * Return value: 1 if test is passed
 */
int
test_caption_component (void)
{
  GtkWidget * caption = NULL;
  GtkWidget * label = NULL;
  GtkWidget * window = NULL;
  AtkObject * accessible = NULL;
  AtkObject * accessible_child = NULL;
  AtkObject * accessible_tmp = NULL;
  gint caption_x, caption_y, label_x, label_y;
  gint caption_height, caption_width, label_height, label_width;

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  label = gtk_label_new("Test label");
  caption = hildon_caption_new(NULL, "Test caption", label, NULL, HILDON_CAPTION_MANDATORY);
  gtk_container_add(GTK_CONTAINER(window), caption);
  gtk_widget_show_all(window);

  accessible = gtk_widget_get_accessible(caption);
  assert (atk_object_get_n_accessible_children(accessible)==1);
  accessible_child = atk_object_ref_accessible_child(accessible, 0);
  assert (HAIL_IS_CAPTION(accessible));

  assert (ATK_IS_COMPONENT(accessible));
  assert (ATK_IS_COMPONENT(accessible_child));
  
  atk_component_get_extents(ATK_COMPONENT(accessible), &caption_x, &caption_y,
			    &caption_width, &caption_height, ATK_XY_SCREEN);
  atk_component_get_extents(ATK_COMPONENT(accessible_child), &label_x, &label_y,
			    &label_width, &label_height, ATK_XY_SCREEN);
  assert (caption_x >= 0);
  assert (label_x >= 0);

  accessible_tmp = atk_component_ref_accessible_at_point(ATK_COMPONENT(accessible),
							 label_x, label_y,
							 ATK_XY_SCREEN);
  assert (accessible_tmp == accessible_child);
  g_object_unref(accessible_tmp);
  
  accessible_tmp = atk_component_ref_accessible_at_point(ATK_COMPONENT(accessible),
							 label_x-1, label_y-1,
							 ATK_XY_SCREEN);
  assert (accessible_tmp == NULL);
  
  return 1;
}

/**
 * test_caption_image:
 *
 * Check the #AtkImage interface of #HailCaption
 *
 * Return value: 1 if test is passed
 */
int
test_caption_image (void)
{
  GtkWidget * caption = NULL;
  GtkWidget * label = NULL;
  GtkWidget * window = NULL;
  GtkWidget * icon = NULL;
  AtkObject * accessible = NULL;
  AtkObject * accessible_image = NULL;
  gint x, y;
  gint x2, y2;

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  label = gtk_label_new("Test label");
  icon = gtk_image_new_from_stock(GTK_STOCK_OPEN, GTK_ICON_SIZE_BUTTON);
  caption = hildon_caption_new(NULL, "Test caption", label, icon, HILDON_CAPTION_MANDATORY);
  gtk_container_add(GTK_CONTAINER(window), caption);
  gtk_widget_show_all(window);

  accessible = gtk_widget_get_accessible(caption);
  assert (atk_object_get_n_accessible_children(accessible)==1);
  assert (HAIL_IS_CAPTION(accessible));
  assert (ATK_IS_IMAGE(accessible));

  assert (atk_image_get_image_description(ATK_IMAGE(accessible))== NULL);

  assert (atk_image_set_image_description(ATK_IMAGE(accessible), "Test description"));
  assert (strcmp(atk_image_get_image_description(ATK_IMAGE(accessible)), "Test description")==0);

  /* the atk image position has been get from the hidden atk image interface in the caption */
  accessible_image = gtk_widget_get_accessible(icon);
  atk_image_get_image_size(ATK_IMAGE(accessible_image), &x, &y);
  atk_image_get_image_size(ATK_IMAGE(accessible), &x2, &y2);
  assert ((x==x2)&&(y==y2));

  return 1;
}

