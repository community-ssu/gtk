/* iconview-hildon.c: autotest the different Hildon GtkIconView modes.
 *
 * Copyright (C) 2008  Nokia Corporation
 * Author: Kristian Rietveld <kris@imendio.com>
 */

#include <gtk/gtk.h>

/* Fixture */
typedef struct
{
  GtkWidget *window;
  GtkWidget *icon_view;
  GtkTreeModel *model;
}
HildonIconViewFixture;

static GtkTreeModel *
create_model (void)
{
  int i;
  GtkListStore *store;

  store = gtk_list_store_new (1, G_TYPE_STRING);

  for (i = 0; i < 50; i++)
    {
      gchar *str;

      str = g_strdup_printf ("\nRow %d\n", i);
      gtk_list_store_insert_with_values (store, NULL, i, 0, str, -1);
      g_free (str);
    }

  return GTK_TREE_MODEL (store);
}

static void
hildon_icon_view_fixture_setup (HildonIconViewFixture *fixture,
                                gboolean               multi,
                                gboolean               edit,
                                gconstpointer          test_data)
{
  HildonUIMode mode;
  GtkCellRenderer *renderer;

  fixture->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  if (edit)
    mode = HILDON_UI_MODE_EDIT;
  else
    mode = HILDON_UI_MODE_NORMAL;

  fixture->model = create_model ();
  fixture->icon_view = g_object_new (GTK_TYPE_ICON_VIEW,
                                     "model", fixture->model,
                                     "name", "fremantle-widget",
                                     "hildon-ui-mode", mode,
                                     NULL);
  g_object_unref (fixture->model);

  if (multi)
    gtk_icon_view_set_selection_mode (GTK_ICON_VIEW (fixture->icon_view),
                                      GTK_SELECTION_MULTIPLE);
  else if (mode != HILDON_UI_MODE_NORMAL)
    gtk_icon_view_set_selection_mode (GTK_ICON_VIEW (fixture->icon_view),
                                      GTK_SELECTION_SINGLE);

  renderer = gtk_cell_renderer_text_new ();
  g_object_set (renderer,
                "xalign", 0.5,
                "weight", PANGO_WEIGHT_BOLD,
                NULL);

  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (fixture->icon_view),
                              renderer,
                              FALSE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (fixture->icon_view),
                                  renderer,
                                  "text", 0,
                                  NULL);

  gtk_container_add (GTK_CONTAINER (fixture->window), fixture->icon_view);
  gtk_widget_show_all (fixture->window);
}

static void
hildon_icon_view_fixture_single_setup (HildonIconViewFixture *fixture,
                                       gconstpointer          test_data)
{
  hildon_icon_view_fixture_setup (fixture, FALSE, FALSE, test_data);
}

static void
hildon_icon_view_fixture_multi_setup (HildonIconViewFixture *fixture,
                                      gconstpointer          test_data)
{
  hildon_icon_view_fixture_setup (fixture, TRUE, FALSE, test_data);
}

static void
hildon_icon_view_fixture_edit_single_setup (HildonIconViewFixture *fixture,
                                            gconstpointer          test_data)
{
  hildon_icon_view_fixture_setup (fixture, FALSE, TRUE, test_data);
}

static void
hildon_icon_view_fixture_edit_multi_setup (HildonIconViewFixture *fixture,
                                           gconstpointer          test_data)
{
  hildon_icon_view_fixture_setup (fixture, TRUE, TRUE, test_data);
}


static void
hildon_icon_view_fixture_teardown (HildonIconViewFixture *fixture,
                                   gconstpointer          test_data)
{
  gtk_widget_destroy (fixture->window);
}

/* Helpers */
static void
count_selected_func (GtkIconView *icon_view,
                     GtkTreePath *path,
                     gpointer     data)
{
  gint *count = data;

  (*count)++;
}

static gint
gtk_icon_view_count_selected_rows (GtkIconView *icon_view)
{
  gint count = 0;

  gtk_icon_view_selected_foreach (icon_view, count_selected_func, &count);

  return count;
}

/* Tests */
static void
normal_selection_none (HildonIconViewFixture *fixture,
                       gconstpointer          test_data)
{
  /* Verify the selection mode is forced to be NONE */
  g_assert (gtk_icon_view_get_selection_mode (GTK_ICON_VIEW (fixture->icon_view)) == GTK_SELECTION_NONE);
  g_assert (gtk_icon_view_count_selected_rows (GTK_ICON_VIEW (fixture->icon_view)) == 0);
}

static void
edit_selection_single (HildonIconViewFixture *fixture,
                       gconstpointer          test_data)
{
  GtkTreePath *path;
  GtkTreeIter iter;

  /* One item must be selected */
  g_assert (gtk_icon_view_count_selected_rows (GTK_ICON_VIEW (fixture->icon_view)) == 1);

  /* Selection should move */
  path = gtk_tree_path_new_from_indices (10, -1);
  gtk_icon_view_select_path (GTK_ICON_VIEW (fixture->icon_view), path);
  g_assert (gtk_icon_view_count_selected_rows (GTK_ICON_VIEW (fixture->icon_view)) == 1);
  g_assert (gtk_icon_view_path_is_selected (GTK_ICON_VIEW (fixture->icon_view), path));

  /* When selected item is deleted, first item should get selection */
  gtk_tree_model_get_iter (fixture->model, &iter, path);
  gtk_tree_path_free (path);

  gtk_list_store_remove (GTK_LIST_STORE (fixture->model), &iter);
  g_assert (gtk_icon_view_count_selected_rows (GTK_ICON_VIEW (fixture->icon_view)) == 1);

  path = gtk_tree_path_new_from_indices (0, -1);
  g_assert (gtk_icon_view_path_is_selected (GTK_ICON_VIEW (fixture->icon_view), path));
  gtk_tree_path_free (path);
}

static void
edit_selection_multi (HildonIconViewFixture *fixture,
                      gconstpointer          test_data)
{
  GtkTreePath *path;
  GtkTreeIter iter;

  /* One item must be selected */
  g_assert (gtk_icon_view_count_selected_rows (GTK_ICON_VIEW (fixture->icon_view)) == 1);

  /* Selection should move */
  path = gtk_tree_path_new_from_indices (10, -1);
  gtk_icon_view_select_path (GTK_ICON_VIEW (fixture->icon_view), path);
  g_assert (gtk_icon_view_count_selected_rows (GTK_ICON_VIEW (fixture->icon_view)) == 2);
  g_assert (gtk_icon_view_path_is_selected (GTK_ICON_VIEW (fixture->icon_view), path));

  /* When selected item is deleted, first item should get selection */
  gtk_tree_model_get_iter (fixture->model, &iter, path);
  gtk_tree_path_free (path);

  gtk_list_store_remove (GTK_LIST_STORE (fixture->model), &iter);
  g_assert (gtk_icon_view_count_selected_rows (GTK_ICON_VIEW (fixture->icon_view)) == 1);

  /* Path 0 was selected and should still be selected */
  path = gtk_tree_path_new_from_indices (0, -1);
  g_assert (gtk_icon_view_path_is_selected (GTK_ICON_VIEW (fixture->icon_view), path));

  /* Deleting path 0 should make the "new" path 0 selected */
  gtk_tree_model_get_iter (fixture->model, &iter, path);
  gtk_list_store_remove (GTK_LIST_STORE (fixture->model), &iter);
  g_assert (gtk_icon_view_count_selected_rows (GTK_ICON_VIEW (fixture->icon_view)) == 1);

  g_assert (gtk_icon_view_path_is_selected (GTK_ICON_VIEW (fixture->icon_view), path));
  gtk_tree_path_free (path);
}


int
main (int argc, char **argv)
{
  gtk_test_init (&argc, &argv);

  gtk_rc_parse_string ("style \"fremantle-widget\" {\n"
                       "  GtkWidget::hildon-mode = 1\n" 
                       "} widget \"*.fremantle-widget\" style \"fremantle-widget\"");

  g_test_add ("/iconview/hildon/normal-single-selection-none",
              HildonIconViewFixture, NULL,
              hildon_icon_view_fixture_single_setup,
              normal_selection_none,
              hildon_icon_view_fixture_teardown);
  g_test_add ("/iconview/hildon/normal-multi-selection-none",
              HildonIconViewFixture, NULL,
              hildon_icon_view_fixture_multi_setup,
              normal_selection_none,
              hildon_icon_view_fixture_teardown);

  g_test_add ("/iconview/hildon/edit-single-selection-test",
              HildonIconViewFixture, NULL,
              hildon_icon_view_fixture_edit_single_setup,
              edit_selection_single,
              hildon_icon_view_fixture_teardown);
  g_test_add ("/iconview/hildon/edit-multi-selection-test",
              HildonIconViewFixture, NULL,
              hildon_icon_view_fixture_edit_multi_setup,
              edit_selection_multi,
              hildon_icon_view_fixture_teardown);

  return g_test_run ();
}
