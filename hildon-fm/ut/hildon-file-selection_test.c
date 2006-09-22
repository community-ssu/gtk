/*
 * This file is part of hildon-fm package
 *
 * Copyright (C) 2005-2006 Nokia Corporation.  All rights reserved.
 *
 * Contact: Marius Vollmer <marius.vollmer@nokia.com>
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

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>

#include <gtk/gtk.h>
#include <glib.h>
#include "../hildon-fm/hildon-file-selection.h"
#include "../hildon-fm/hildon-file-system-info.h"

#include <outo.h>

char* outo_name = "hildon-fm tests";

void init_test(void);
int test001(void);
int test002(void);
int test003(void);
int test004(void);
int test005(void);
int test006(void);
int test007(void);
int test008(void);
int test009(void);
int test010(void);

/*prototypes to keep the compiler happy*/
testcase *get_tests(void);

/* this has to be like this (not static). outo
   calls for this! */
testcase *get_tests(void);

void init_test(void)
{
    int plop = 0;
    g_type_init();
    if (!gtk_init_check(&plop, NULL)) {
        fprintf(stderr, "Gtk initialisation failed\n");
    } else {
        printf("init succeeded\n");
    }
}

int test001(void)
{
  HildonFileSystemModel *model;
  HildonFileSelection *fs;

  model = g_object_new(HILDON_TYPE_FILE_SYSTEM_MODEL, NULL);
  assert(HILDON_IS_FILE_SYSTEM_MODEL(model));
  fs = HILDON_FILE_SELECTION(hildon_file_selection_new_with_model(model));
  assert(HILDON_IS_FILE_SELECTION(fs));

  return 1;
}

int test002(void)
{
  HildonFileSystemModel *model;
  HildonFileSelection *fs;
  GtkWidget *window;

  window = gtk_window_new(GTK_WINDOW_POPUP);
  model = g_object_new(HILDON_TYPE_FILE_SYSTEM_MODEL, NULL);
  fs = HILDON_FILE_SELECTION(hildon_file_selection_new_with_model(model));
  gtk_container_add(GTK_CONTAINER(window),GTK_WIDGET(fs));
  gtk_widget_show_all(window);

  hildon_file_selection_set_mode(fs, HILDON_FILE_SELECTION_MODE_THUMBNAILS);
  assert(hildon_file_selection_get_mode(fs) ==
         HILDON_FILE_SELECTION_MODE_THUMBNAILS);

  gtk_widget_destroy(window);

  return 1;  
}

int test003(void){ 
  GtkSortType order;
  HildonFileSelectionSortKey key;
  HildonFileSelection *fs;
  HildonFileSystemModel *model;

  model = g_object_new(HILDON_TYPE_FILE_SYSTEM_MODEL, NULL);
  fs = HILDON_FILE_SELECTION(hildon_file_selection_new_with_model(model));

  hildon_file_selection_set_sort_key(fs, HILDON_FILE_SELECTION_SORT_SIZE,
                                     GTK_SORT_ASCENDING);
  hildon_file_selection_get_sort_key(fs,&key,&order);

  assert(key == HILDON_FILE_SELECTION_SORT_SIZE);
  assert(order == GTK_SORT_ASCENDING);

  return 1;
}

int test004(void) {
  HildonFileSelection *fs;
  HildonFileSystemModel *model;
  GtkFilePath *path, *path2;
  GtkTreeIter iter;
  gboolean ret;
 
  model = g_object_new(HILDON_TYPE_FILE_SYSTEM_MODEL, NULL);
  fs = HILDON_FILE_SELECTION(hildon_file_selection_new_with_model(model));

  path = hildon_file_selection_get_current_folder(fs);  
  ret = hildon_file_selection_get_current_folder_iter(fs, &iter);
  assert(ret);
  gtk_tree_model_get(GTK_TREE_MODEL(model), &iter,
                     HILDON_FILE_SYSTEM_MODEL_COLUMN_GTK_PATH, &path2, -1);
  g_assert(gtk_file_path_compare(path, path2) == 0);

  return 1;
}
 
int test005(void){ /* set_filter/get_filter */
  GtkFileFilter *filter, *filter2;
  HildonFileSelection *fs;
  HildonFileSystemModel *model;

  model = g_object_new(HILDON_TYPE_FILE_SYSTEM_MODEL, NULL);
  fs = HILDON_FILE_SELECTION(hildon_file_selection_new_with_model(model));

  filter = gtk_file_filter_new();
  gtk_file_filter_add_mime_type(filter, "image/png");

  hildon_file_selection_set_filter(fs, filter);

  filter2 = hildon_file_selection_get_filter(fs);

  assert(filter2 == filter);

  return 1;
}

int test006(void){
  HildonFileSelection *fs;
  HildonFileSystemModel *model;
                                                                                                                   
  model = g_object_new(HILDON_TYPE_FILE_SYSTEM_MODEL, NULL);
  fs = HILDON_FILE_SELECTION(hildon_file_selection_new_with_model(model));
                                                                                                                   
  hildon_file_selection_set_select_multiple(fs, TRUE);
  g_assert(hildon_file_selection_get_select_multiple(fs) == TRUE);

  hildon_file_selection_select_all(fs);
/*  g_assert(hildon_file_selection_get_selected_paths(fs) != NULL);*/
  hildon_file_selection_unselect_all(fs);
  g_assert(hildon_file_selection_get_selected_paths(fs) == NULL);

  return 1;
}

int test007(void)
{
  HildonFileSystemModel *model;
  gchar *local_path;
  GtkTreeIter iter;
  gboolean result;
  HildonFileSystemModelItemType type; 
                                                                                                                
  model = g_object_new(HILDON_TYPE_FILE_SYSTEM_MODEL, NULL);
  local_path = g_strdup_printf("%s/MyDocs", g_get_home_dir());
  result = hildon_file_system_model_search_local_path(model, local_path,
                                                      &iter, NULL, TRUE);
  assert(result);
  gtk_tree_model_get(GTK_TREE_MODEL(model), &iter,
                     HILDON_FILE_SYSTEM_MODEL_COLUMN_TYPE, &type, -1);
  assert(type >= HILDON_FILE_SYSTEM_MODEL_GATEWAY);

  return 1;  
}

int test008(void)
{
  HildonFileSystemModel *model;
  GtkTreeIter iter;
  gboolean result;
  gchar *name;

  model = g_object_new(HILDON_TYPE_FILE_SYSTEM_MODEL, NULL);
  result = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(model), &iter);
  assert(result);
  name = hildon_file_system_model_new_item(model, &iter, "test", ".txt");  
  assert(name);

  return 1;
}

static gpointer cb_data;
static GMainLoop *loop;
static HildonFileSystemInfoHandle *async_handle;

static void info_callback(HildonFileSystemInfoHandle *handle,
                          HildonFileSystemInfo *info,
                          const GError *error, gpointer data)
{
  assert(handle == async_handle);
  assert(info != NULL);
  assert(error == NULL);
  assert(data == cb_data);

  g_main_loop_quit(loop);
}

int test009(void)
{
  HildonFileSelection *fs;
  HildonFileSystemModel *model;
  const gchar *uri = "file:///";

  model = g_object_new(HILDON_TYPE_FILE_SYSTEM_MODEL, NULL);
  assert(model != NULL);
  fs = HILDON_FILE_SELECTION(hildon_file_selection_new_with_model(model));
  assert(fs != NULL);

  async_handle = hildon_file_system_info_async_new(uri, info_callback,
                                                   cb_data);
  assert(async_handle != NULL);

  loop = g_main_loop_new(NULL, TRUE);
  assert(loop != NULL);
  g_main_loop_run(loop);

  return 1;
}

static void do_not_call_me_info_callback(HildonFileSystemInfoHandle *handle,
                          HildonFileSystemInfo *info,
                          const GError *error, gpointer data)
{
  assert(0);
}

static gboolean timeout_func(gpointer data)
{
  g_main_loop_quit(loop);
  return FALSE;
}

int test010(void)
{
  HildonFileSelection *fs;
  HildonFileSystemModel *model;
  const gchar *uri = "file:///";

  model = g_object_new(HILDON_TYPE_FILE_SYSTEM_MODEL, NULL);
  assert(model != NULL);
  fs = HILDON_FILE_SELECTION(hildon_file_selection_new_with_model(model));
  assert(fs != NULL);

  async_handle = hildon_file_system_info_async_new(uri,
                     do_not_call_me_info_callback, cb_data);
  assert(async_handle != NULL);

  loop = g_main_loop_new(NULL, TRUE);
  assert(loop != NULL);

  hildon_file_system_info_async_cancel(async_handle);
  g_timeout_add(1000, timeout_func, NULL);
  
  g_main_loop_run(loop);

  return 1;
}

/*use EXPECT_ASSERT for the tests that are _meant_ to throw assert so they are 
*considered passed when they throw assert and failed when they do not
*/

testcase tcases[] =
{
  {*test001, "file_selection: new", EXPECT_OK},
  {*test002, "file_selection: set/get_mode", EXPECT_OK},
  {*test003, "file_selection: set/get_sort_key", EXPECT_OK},
  {*test004, "file_selection: set/get_current_folder", EXPECT_OK},
  {*test005, "file_selection: set/get filter", EXPECT_OK},
  {*test006, "file_selection: selections", EXPECT_OK},
  {*test007, "file_system_model: search path", EXPECT_OK},
  {*test008, "file_system_model: autonaming", EXPECT_OK},
  {*test009, "file_system_info: async_new", EXPECT_OK},
  {*test010, "file_system_info: async_cancel", EXPECT_OK},
  {0} /*REMEMBER THE TERMINATING NULL*/
};

testcase* get_tests()
{
/*    g_log_set_always_fatal (G_LOG_LEVEL_ERROR | G_LOG_LEVEL_CRITICAL | G_LOG_LEVEL_WARNING);*/
    return tcases;
}

