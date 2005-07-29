/** 
    @file core.c

    Core functionality of Application Installer.
    <p>
*/

/*
 * This file is part of osso-application-installer
 *
 * Copyright (C) 2005 Nokia Corporation.
 *
 * Contact: Marius Vollmer <marius.vollmer@nokia.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

#define _GNU_SOURCE

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <glib.h>

#include "core.h"
#include "ui/represent.h"

/* OVERVIEW

   The real work is done by the program /usr/bin/app-installer-tool
   and this file basically only invokes that program to do things like
   retrieve the list of installed packages, install new ones, and
   remove installed ones.

   The app-installer-tool is used in a number of ways:

   - Sort of synchronously, where nothing can be done in the GUI until
     the program has terminated.  This is used to get the list of
     installed packages.  (Redraws are still handled, see below.)
     
   - Semi-Asynchronously, where the GUI waits for the program to
     terminate and shows a pulsating 'I am still alive' indication and
     the operation can be cancelled.  The output of the
     app-installer-tool is gathered and analzyed after it has
     terminated.  This is used for installation and uninstallation of
     packages.  There is no real progress information available from
     the app-installer-tool for these operations, unfortunately.

   - Really asynchronously, where the GUI interprets the output from
     the app-installer-tool as soon as it becomes available.  This is
     used for copying a remote file to local storage.  The
     app-installer-tool can provide real progress information for that
     and we use it to keep a progress bar alive.

   The third way if of course the most general and the other two ways
   are in fact implemented as special cases of it.  This means that
   the app-installer-tool is in fact always execute asynchronously,
   and the GUI will never starve.

   The function SPAWN_APP_INSTALLER_TOOL implements the most general
   way of invoking the app-installer-tool, and RUN_APP_INSTALLER_TOOL
   uses it to run it sort-of-synchronously.

   XXX - These three ways should probably be unified so that even
   getting the list of installed packages gets a progress bar and can
   be canceled.  The dialog for that would of course only be shown
   after a certain timeout.

   XXX - cancellation is not implemented.
*/


/* This function starts /usr/bin/app-installer-tool so that it runs
   the background.

   When AS_INSTALL is true, app-installer-tool will in fact be invoked
   thru sudo und run as the user "install".
 
   ARG1, ARG2, and ARG3 are up to three arguments that are passed to
   app-installer-tool.  Set the unused arguments to NULL.

   When this invokation of app-intstaller-tool has terminated or could
   not be started at all, CALLBACK is called.  The SUCCESS parameter
   of CALLBACK is non-zero when app-intstaller-tool exited with a code
   of zero, non-zero otherwise.  The STDOUT_STRING and STDERR_STRING
   parameters contain the stdout and stderr output of the invokation,
   respectively.  (See below for how this interacts with the 'line
   callbacks', however.)  Both strings are freed when CALLBACK
   returns.

   When SUCCESS is zero, the STDERR_STRING passed to CALLBACK will
   contain suitable error messages such as "Caught signal 11 (core
   dumped)".  These error messages will not be seen by
   STDERR_LINE_CALLBACK.

   When STDOUT_LINE_CALLBACK is non-zero, it is called whenever a new
   complete line is available from the stdout of the invokation.  The
   LINE parameter points to this zero-terminated line.  This pointer
   is only valid during the call to STDOUT_LINE_CALLBACK.  When the
   STDOUT_LINE_CALLBACK is used, the STDOUT_STRING passed to CALLBACK
   will consist of the rest of stdout when it does not end with a
   newline character.

   STDERR_LINE_CALLBACK is the same as STDOUT_LINE_CALLBACK, only for
   stderr.

   CALLBACK_DATA is the data passed to all three callbacks above.

   CALLBACK is either called from the main event loop, or it might be
   called directly from SPAWN_APP_INSTALLER_TOOL when an error
   occurred before app-installer-tool could be spawned.  When CALLBACK
   is called directly, SPAWN_APP_INSTALLER_TOOL returns zero.
   Otherwise, it returns non-zero.
*/
static int
spawn_app_installer_tool (AppData *app_data,
			  int as_install,
			  gchar *arg1, gchar *arg2, gchar *arg3,
			  void (*callback) (gpointer data,
					    int success,
					    gchar *stdout_string,
					    gchar *stderr_string),
			  void (*stdout_line_callback) (gpointer data,
							gchar *line),
			  void (*stderr_line_callback) (gpointer data,
							gchar *line),
			  gpointer callback_data);

typedef struct {
  void (*callback) (gpointer, int, gchar *, gchar *);
  void (*line_callback[2]) (gpointer, gchar *);
  gpointer callback_data;

  GIOChannel *channels[2];
  GString *strings[2];
  gboolean exited;
  int status;
} spawn_data;

static void
check_completion (spawn_data *data)
{
  /* The invocation is done when we have reaped the exit status and both
     the stdout and stderr channels are at EOF.
  */

  if (data->channels[0] == NULL
      && data->channels[1] == NULL
      && data->exited)
    {
      int success;
      
      if (WIFEXITED (data->status))
	{
	  success = (WEXITSTATUS (data->status) == 0);
	}
      else if (WIFSIGNALED (data->status))
	{
	  g_string_append_printf (data->strings[1],
				  "Caught signal %d%s.\n",
				  WTERMSIG (data->status),
				  (WCOREDUMP (data->status)?
				   " (core dumped)" : ""));
	  success = 0;
	}
      else if (WIFSTOPPED (data->status))
	{
	  /* This should better not happen.  Hopefully glib does not
	     use WUNTRACED in its call to waitpid or equivalent.  We
	     still handle this case for extra clarity in the error
	     message.
	  */
	  g_string_append_printf (data->strings[1],
				  "Stopped with signal %d.\n",
				  WSTOPSIG (data->status));
	  success = 0;
	}
      else
	{
	  g_string_append_printf (data->strings[1],
				  "Unknown reason for failure.\n");
	  success = 0;
	}

      data->callback (data->callback_data, success,
		      data->strings[0]->str,
		      data->strings[1]->str);
      g_string_free (data->strings[0], 1);
      g_string_free (data->strings[1], 1);
      g_free (data);
    }
}

static void
reap_process (GPid pid, int status, gpointer raw_data)
{
  spawn_data *data = (spawn_data *)raw_data;
  data->status = status;
  data->exited = 1;
  check_completion (data);
}

static gboolean
read_into_string (GIOChannel *channel, GIOCondition cond, gpointer raw_data)
{
  spawn_data *data = (spawn_data *)raw_data;
  gchar buf[256];
  gsize count;
  GIOStatus status;
  int id;
  
  if (data->channels[0] == channel)
    id = 0;
  else if (data->channels[1] == channel)
    id = 1;
  else
    return FALSE;

#if 0
  /* XXX - this blocks sometime.  Maybe setting the encoding to NULL
           will work, but for now we just do it the old school way...
  */
  status = g_io_channel_read_chars (channel, buf, 256, &count, NULL);
#else
  {
    int fd = g_io_channel_unix_get_fd (channel);
    int n = read (fd, buf, 256);
    if (n > 0)
      {
	status = G_IO_STATUS_NORMAL;
	count = n;
      }
    else
      {
	status = G_IO_STATUS_EOF;
	count = 0;
      }
  }
#endif

  if (status == G_IO_STATUS_NORMAL)
    {
      g_string_append_len (data->strings[id], buf, count);
      if (data->line_callback[id])
	{
	  GString *string = data->strings[id];
	  gchar *line_end;

	  while ((line_end = strchr (string->str, '\n')))
	    {
	      *line_end = '\0';
	      data->line_callback[id] (data->callback_data, string->str);
	      g_string_erase (string, 0, line_end - string->str + 1);
	    }
	}
      return TRUE;
    }
  else
    {
      g_io_channel_shutdown (channel, 0, NULL);
      data->channels[id] = NULL;
      check_completion (data);
      return FALSE;
    }
}

void
add_string_reader (int fd, int id, spawn_data *data)
{
  data->strings[id] = g_string_new ("");
  data->channels[id] = g_io_channel_unix_new (fd);
  g_io_add_watch (data->channels[id], 
		  G_IO_IN | G_IO_HUP | G_IO_ERR,
		  read_into_string, data);
  g_io_channel_unref (data->channels[id]);
}

static int
spawn_app_installer_tool (AppData *app_data,
			  int as_install,
			  gchar *arg1, gchar *arg2, gchar *arg3,
			  void (*callback) (gpointer data,
					    int success,
					    gchar *stdout_string,
					    gchar *stderr_string),
			  void (*stdout_line_callback) (gpointer data,
							gchar *line),
			  void (*stderr_line_callback) (gpointer data,
							gchar *line),
			  gpointer callback_data)
{
  gchar *argv[8], **arg = argv;
  spawn_data *data;
  GError *error = NULL;
  GPid child_pid;
  int stdout_fd, stderr_fd;

#if USE_SUDO
  if (as_install)
    {
      *arg++ = "/usr/bin/sudo";
      *arg++ = "-u";
      *arg++ = "install";
    }
#endif

  *arg++ = "/usr/bin/app-installer-tool";
  *arg++ = arg1;
  *arg++ = arg2;
  *arg++ = arg3;
  *arg++ = NULL;

#ifdef DEBUG
  {
    int i;
    fprintf (stderr, "[");
    for (i = 0; argv[i]; i++)
      fprintf (stderr, " %s", argv[i]);
    fprintf (stderr, " ]\n");
  }
#endif

  g_spawn_async_with_pipes (NULL,
			    argv,
			    NULL,
			    G_SPAWN_DO_NOT_REAP_CHILD,
			    NULL,
			    NULL,
			    &child_pid,
			    NULL,
			    &stdout_fd,
			    &stderr_fd,
			    &error);

  if (error)
    {
      callback (callback_data, 0, "", error->message);
      g_error_free (error);
      return 0;
    }

  data = g_new (spawn_data, 1);
  data->callback = callback;
  data->callback_data = callback_data;
  data->line_callback[0] = stdout_line_callback;
  data->line_callback[1] = stderr_line_callback;

  g_child_watch_add (child_pid, reap_process, data);
  add_string_reader (stdout_fd, 0, data);
  add_string_reader (stderr_fd, 1, data);

  return 1;
}

/* Run the app-installer-tool as the user "install" with the arguments
   ARG1, ARG2, and ARG3 (which be NULL if they are unused) and wait
   for it to terminate.

   Stdout from the invocation is returned in *STDOUT_STRING which must
   be non-NULL.  When STDERR_STRING is non-NULL, it gets the stderr
   from the invocation.  You need to free both strings.

   The return value is non-zero when the invocation succeeded and zero
   when it failed.  In the case of failure and when ERROR_DESCRIPTION
   is non-NULL, present_report_with_details is used to show stderr to
   the user.
*/
static int
run_app_installer_tool (AppData *app_data,
			gchar *arg1, gchar *arg2, gchar *arg3,
			gchar **stdout_string, gchar **stderr_string,
			gchar *error_description);

typedef struct {
  AppData *app_data;
  gchar ***stdout_string;
  gchar ***stderr_string;
  gchar *error_description;
  int success;
  GMainLoop *loop;
} run_data;

static void
run_callback (gpointer raw_data,
	      int success,
	      gchar *stdout_string, gchar *stderr_string)
{
  run_data *data = (run_data *)raw_data;

  data->success = success;

  if (!success && data->error_description)
    present_report_with_details (data->app_data,
				 _("ai_ti_general_error"),
				 data->error_description,
				 stderr_string);

  **(data->stdout_string) = g_strdup (stdout_string);
  **(data->stderr_string) = g_strdup (stderr_string);

  if (data->loop)
    g_main_loop_quit (data->loop);
}

static int
run_app_installer_tool (AppData *app_data,
			gchar *arg1, gchar *arg2, gchar *arg3,
			gchar **stdout_string, gchar **stderr_string,
			gchar *error_description)
{
  gchar *my_stderr_string = NULL;
  run_data data;

  if (stderr_string == NULL)
    stderr_string = &my_stderr_string;

  data.app_data = app_data;
  data.stdout_string = &stdout_string;
  data.stderr_string = &stderr_string;
  data.error_description = error_description;
  if (spawn_app_installer_tool (app_data, 1,
				arg1, arg2, arg3,
				run_callback,
				NULL,
				NULL,
				&data))
    {
      data.loop = g_main_loop_new (NULL, 0);
      g_main_loop_run (data.loop);
    }

  if (stderr_string == &my_stderr_string)
    free (my_stderr_string);

  return data.success;
}

static gchar *
parse_delimited (gchar *ptr, gchar del, gchar **token)
{
  if (ptr == NULL)
    return NULL;
  *token = ptr;
  ptr = strchr (ptr, del);
  if (ptr)
    *ptr++ = '\0';
  return ptr;
}

GtkTreeModel *
list_packages (AppData *app_data)
{
  GtkListStore *store;
  GtkTreeIter iter;
  gchar *stdout_string, *ptr;
  GdkPixbuf *default_icon;

  if (!run_app_installer_tool (app_data,
			       "list", NULL, NULL,
			       &stdout_string, NULL,
			       _("ai_ti_listing_failed")))
    return NULL;

  default_icon = gtk_icon_theme_load_icon (gtk_icon_theme_get_default (),
					   "qgn_list_gene_default_app",
					   26,
					   0,
					   NULL);

  store = gtk_list_store_new (NUM_COLUMNS,
			      G_TYPE_OBJECT,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              G_TYPE_STRING);

  /* Parse stdout_string.
   */
  ptr = stdout_string;
  while (*ptr)
    {
      gchar *name, *version, *size, *status;
      
      ptr = parse_delimited (ptr, '\t', &name);
      ptr = parse_delimited (ptr, '\t', &version);
      ptr = parse_delimited (ptr, '\t', &size);
      ptr = parse_delimited (ptr, '\n', &status);

      g_assert (ptr != NULL); /* XXX - do not crash here */

      if (!strcmp (name, META_PACKAGE))
	continue;

      /* XXX - find a better way to present broken packages.
       */
      if (strcmp (status, "ok"))
	version = g_strdup_printf ("broken - v%s", version);
      else
	version = g_strdup_printf ("v%s", version);
      size = g_strdup_printf ("%skB", size);

      gtk_list_store_append (store, &iter);
      gtk_list_store_set (store, &iter,
			  COLUMN_ICON, default_icon, 
			  COLUMN_NAME, name,
			  COLUMN_VERSION, version,
			  COLUMN_SIZE, size,
			  -1);

      free (version);
      free (size);
    }

  g_object_unref (default_icon);
  free (stdout_string);
  return GTK_TREE_MODEL (store);
}

gboolean
any_packages_installed (GtkTreeModel *model)
{
  GtkTreeIter iter;
  return model && gtk_tree_model_get_iter_first (model, &iter);
}

void
add_columns (AppUIData *app_ui_data, GtkTreeView *treeview)
{
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;
 
  /* Icon column */
  renderer = gtk_cell_renderer_pixbuf_new ();
  column = gtk_tree_view_column_new_with_attributes("Icon",
						    renderer,
						    "pixbuf",
						    COLUMN_ICON,
						    NULL);
  gtk_tree_view_column_set_sort_column_id (column, COLUMN_ICON);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
  gtk_tree_view_column_set_fixed_width (column, COLUMN_ICON_WIDTH);
  gtk_tree_view_append_column (treeview, column);

  /* Name column */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Name",
						     renderer,
						     "text",
						     COLUMN_NAME,
						     NULL);
  gtk_tree_view_column_set_sort_column_id (column, COLUMN_NAME);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  g_object_set (column, "expand", 1, NULL);
  gtk_tree_view_append_column( treeview, column);
  app_ui_data->name_column = column;

  /* Version column */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Version",
						     renderer,
						     "text",
						     COLUMN_VERSION,
						     NULL);
  gtk_tree_view_column_set_sort_column_id (column, COLUMN_VERSION);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_column_set_alignment (column, 1.0);
  gtk_tree_view_append_column (treeview, column);
  app_ui_data->version_column = column;

  /* Size column */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Size",
						     renderer,
						     "text",
						     COLUMN_SIZE,
						     NULL);
  gtk_tree_view_column_set_sort_column_id (column, COLUMN_SIZE);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_column_set_alignment (column, 1.0);
  gtk_tree_view_append_column (treeview, column);
  app_ui_data->size_column = column;
}

/* Return information about the package file FILE.  You need to free
   the returned structure eventually with package_info_free.  When the
   information can not be retrieved, all strings in the returned
   structure are NULL.
 */
PackageInfo
package_file_info (AppData *app_data, gchar *file)
{
  gchar *stdout_string;
  PackageInfo info;

  if (run_app_installer_tool (app_data,
			      "describe-file", file, NULL,
			      &stdout_string, NULL,
			      _("ai_ti_describe_file_failed")))
    {
      gchar *ptr, *package, *version, *size;

      ptr = stdout_string;
      ptr = parse_delimited (ptr, '\n', &package);
      ptr = parse_delimited (ptr, '\n', &version);
      ptr = parse_delimited (ptr, '\n', &size);

      /* XXX - do not crash here.
       */
      g_assert (ptr != NULL);

      info.name = g_string_new (package);
      info.version = g_string_new (version);
      info.size = g_string_new (size);
      info.description = g_string_new (ptr);
      free (stdout_string);
    }
  else
    {
      info.name = NULL;
      info.size = NULL;
      info.version = NULL;
      info.description = NULL;
    }

  return info;
}

void
package_info_free (PackageInfo *info)
{
  g_string_free (info->name, 1);
  g_string_free (info->version, 1);
  g_string_free (info->size, 1);
  g_string_free (info->description, 1);
}

/* Return the description of PACKAGE.  You need to free the returned
   string eventually.  This function never returns NULL.
*/
gchar *
package_description (AppData *app_data, gchar *package)
{
  gchar *stdout_string;
  if (run_app_installer_tool (app_data,
			      "describe-package", package, NULL,
			      &stdout_string, NULL,
			      _("ai_ti_describe_package_failed")))
    return stdout_string;
  else
    return g_strdup ("");
}

static gchar *
has_prefix (gchar *ptr, gchar *prefix)
{
  int len = strlen (prefix);
  if (!strncmp (ptr, prefix, len))
    return ptr + len;
  else
    return NULL;
}

static void
append_list_strings (GString *string, GSList *list)
{
  while (list)
    {
      g_string_append_printf (string, " %s\n", (gchar *)list->data);
      list = list->next;
    }
}

static GString *
format_relationship_failures (gchar *header, gchar *output)
{
  GString *report;
  GSList *depends = NULL, *depended = NULL, *conflicts = NULL;
  int exists = 0;
  gchar *ptr;

  report = g_string_new (header);
  g_string_append (report, "\n");

  ptr = output;
  while (ptr && *ptr)
    {
      gchar *val;
      if ((val = has_prefix (ptr, "depends ")))
	depends = g_slist_prepend (depends, val);
      else if ((val = has_prefix (ptr, "depended ")))
	depended = g_slist_prepend (depended, val);
      else if ((val = has_prefix (ptr, "conflicts ")))
	conflicts = g_slist_prepend (conflicts, val);
      else if (has_prefix (ptr, "exists"))
	exists = 1;
      ptr = strchr (ptr, '\n');
      if (ptr)
	*ptr++ = '\0';
    }

  if (exists)
    g_string_append (report, _("ai_ti_already_installed\n"));
  if (depends)
    {
      g_string_append (report, _("ai_ti_depends_on\n"));
      append_list_strings (report, depends);
    }
  if (depended)
    {
      g_string_append (report, _("ai_ti_depended_on_by\n"));
      append_list_strings (report, depended);
    }
  if (conflicts)
    {
      g_string_append (report, _("ai_ti_conflicts_with\n"));
      append_list_strings (report, conflicts);
    }

  g_slist_free (depends);
  g_slist_free (depended);
  g_slist_free (conflicts);

  return report;
}

#define FUZZ_FACTOR 1.0

typedef struct {
  AppData *app_data;
  gchar *package;
  GMainLoop *loop;
  guint timeout;

  progress_dialog *dialog;
  int initial_used_kb, package_size_kb;

  gchar *progress_title;
  gchar *report_title;
  gchar *success_text;
  gchar *failure_text;
} installer_data;

static void
installer_callback (gpointer raw_data,
		    int success,
		    gchar *output, gchar *details)
{
  installer_data *data = (installer_data *)raw_data;
  GString *report;

  if (data->loop)
    {
      g_source_remove (data->timeout);
      if (data->dialog)
	ui_close_progress_dialog (data->dialog);
    }
      
  if (success)
    {
      report = g_string_new ("");
      g_string_printf (report, data->success_text, data->package);
    }
  else
    {
      gchar *header = g_strdup_printf (data->failure_text, data->package);
      report = format_relationship_failures (header, output);
      free (header);
    }
      
  present_report_with_details (data->app_data,
			       data->report_title,
			       report->str,
			       details);
  g_string_free (report, 1);

  if (data->loop)
    g_main_loop_quit (data->loop);
}

static int
kb_used_in_var_lib_install (void)
{
#if 0
  /* This is the method as used by dpkg to figure out Installed-Size.
     So we use it as well to get more accurate results.  Just looking
     at statvfs, for example, is very off due to JFFS2 compression and
     stuff.  Still, using "du" is off because of the differences
     between the filesystem used during packaging and the one used on
     the target.

     You might think that spawning a process for this is too much
     overhead.  But think again.  This is Unix, you are supposed to do
     things like this.

     - Well, it does not seem to work too well and g_spawn_sync often
       fails in fork with ENOMEM...
  */
  
  gchar *args[] = {
    "/usr/bin/du", "-sk", "/var/lib/install",
    NULL
  };
  gchar *stdout_string = NULL;
  GError *error = NULL;

  int used_k = -1;

  if (g_spawn_sync (NULL,
		    args,
		    NULL,
		    0,
		    NULL,
		    NULL,
		    &stdout_string,
		    NULL,
		    NULL,
		    &error))
    {
      used_k = atoi (stdout_string);
    }
  else
    {
      ULOG_ERR ("used_k: %s\n", error->message);
      g_error_free (error);
    }

  free (stdout_string);  return used_k;
#else
  /* A lighter weight approach that is not as accurate.
   */
  struct statvfs buf;

  if (statvfs("/", &buf) != 0)
    {
      ULOG_ERR ("statvfs: %s\n", strerror (errno));
      return -1;
    }

  return (buf.f_bsize * (buf.f_blocks - buf.f_bavail)) / 1024;
#endif
}

static gboolean
installer_progress (gpointer raw_data)
{
  installer_data *data = (installer_data *)raw_data;
  GtkWindow *main_dialog =
    GTK_WINDOW (data->app_data->app_ui_data->main_dialog);
  int now_used_kb;
  double frac;

  if (data->dialog == NULL)
    data->dialog =
      ui_create_progress_dialog (data->app_data, data->progress_title);

#if 0
  /* XXX - the progress indication is not at all accurate and we need
           real cooperation from app-installer-tool for it anyway.  So
           we just pulse the progress bar for now.
  */
  now_used_kb = kb_used_in_var_lib_install ();
  if (now_used_kb > 0 && data->initial_used_kb > 0)
    {
      frac = FUZZ_FACTOR * (((double) (now_used_kb - data->initial_used_kb))
			    / data->package_size_kb);
#if 0
      fprintf (stderr, "initial %d, now %d, size %d, frac %f\n",
	       data->initial_used_kb, now_used_kb, data->package_size_kb,
	       frac);
#endif
      ui_set_progress_dialog (data->dialog, frac);
    }
#else
  ui_set_progress_dialog (data->dialog, -1.0);
#endif

  return TRUE;
}

void
install_package (gchar *deb, AppData *app_data) 
{
  PackageInfo info;
  installer_data data;

  info = package_file_info (app_data, deb);

  if (info.name == NULL)
    return;

  if (confirm_install (app_data, &info))
    {
      data.loop = NULL;
      data.app_data = app_data;
      data.package = info.name->str;
      data.dialog = NULL;
      data.initial_used_kb = kb_used_in_var_lib_install ();
      data.package_size_kb = atoi (info.size->str);

      data.progress_title = "Installing";
      data.report_title = _("ai_ti_installation_report"),
      data.success_text = _("ai_ti_application_installed_text");
      data.failure_text = _("ai_ti_installation_failed_text");

      if (spawn_app_installer_tool (app_data, 1,
				    "install", deb, NULL,
				    installer_callback,
				    NULL,
				    NULL,
				    &data))
	{
	  data.loop = g_main_loop_new (NULL, 0);
	  data.timeout = g_timeout_add (500, installer_progress, &data);
	  g_main_loop_run (data.loop);
	}
    }

  package_info_free (&info);
}

typedef struct {
  AppData *app_data;
  gchar *local;
  GMainLoop *loop;
  int showing_progress;
} copy_data;

static void
copy_callback (gpointer raw_data,
	       int success,
	       gchar *output, gchar *details)
{
  copy_data *data = (copy_data *)raw_data;

  if (data->showing_progress)
    gtk_banner_close (GTK_WINDOW (data->app_data->app_ui_data->main_dialog));

  if (success)
    install_package (data->local, data->app_data);
  else
    {
      present_report_with_details (data->app_data,
				   _("ai_ti_install_report"),
				   _("ai_ti_copying_failed"),
				   details);
    }

  if (data->loop)
    g_main_loop_quit (data->loop);
}

static void
copy_stdout_callback (gpointer raw_data,
		      gchar *line)
{
  copy_data *data = (copy_data *)raw_data;
  GtkWindow *main_dialog =
    GTK_WINDOW (data->app_data->app_ui_data->main_dialog);
  double frac = atof (line);

  if (!data->showing_progress)
    {
      gtk_banner_show_bar (main_dialog, _("ai_ti_copying"));
      data->showing_progress = 1;
    }

  gtk_banner_set_fraction (main_dialog, frac);
}

void
install_package_from_uri (gchar *uri, AppData *app_data)
{
  int success = 1;
  gchar *details = NULL;

  GnomeVFSURI *vfs_uri = NULL;
  gchar *scheme;

  vfs_uri = gnome_vfs_uri_new (uri);
  if (vfs_uri == NULL)
    {
      details = g_strdup_printf (_("ai_ti_unsupported_uri"), uri);
      success = 0;
      goto done;
    }

  /* The app-installer-tool can access all "file://" URIs, whether
     they are considered local by GnomeVFS or not.  (GnomeVFS
     considers a file:// URI pointing to a NFS mounted volume as
     remote, but we can read that just fine of course.)
  */

  scheme = gnome_vfs_uri_get_scheme (vfs_uri);
  if (scheme && !strcmp (scheme, "file"))
    install_package (gnome_vfs_uri_get_path (vfs_uri), app_data);
  else
    {
      /* We need to copy.
       */
      char template[] = "/tmp/osso-ai-XXXXXX", *tempdir;
      gchar *basename, *local;
      copy_data data;

      tempdir = mkdtemp (template);
      if (tempdir == NULL)
	{
	  details = g_strdup_printf (_("ai_ti_can_not_create_tempdir"),
				     strerror (errno));
	  success = 0;
	  goto done;
	}

      basename = gnome_vfs_uri_extract_short_path_name (vfs_uri);
      local = g_strdup_printf ("%s/%s", tempdir, basename);
      free (basename);

      fprintf (stderr, "copy %s %s\n", uri, local);

      data.app_data = app_data;
      data.local = local;
      data.loop = NULL;
      data.showing_progress = 0;
      if (spawn_app_installer_tool (app_data, 0,
				    "copy", uri, local,
				    copy_callback,
				    copy_stdout_callback,
				    NULL,
				    &data))
	{
	  data.loop = g_main_loop_new (NULL, 0);
	  fprintf (stderr, "looping\n");
	  g_main_loop_run (data.loop);
	}

      /* We don't put up dialogs for errors that happen now.  From the
	 point of the user, the installation has been completed and he
	 has seen the report already.
      */
      success = 1;
      if (unlink (local) < 0)
	ULOG_ERR ("error unlinking %s: %s\n", local, strerror (errno));
      if (rmdir (tempdir) < 0)
	ULOG_ERR ("error removing %s: %s\n", tempdir, strerror (errno));
      free (local);
    }

 done:
  if (!success)
    {
      gchar *report = g_strdup_printf (_("ai_ti_installation_failed_text"),
				       uri);
      present_report_with_details (app_data,
				   _("ai_ti_installation_report"),
				   report,
				   details);
      free (report);
    }
  gnome_vfs_uri_unref (vfs_uri);
  g_free (details);
}

void
uninstall_package (gchar *package, gchar *size, AppData *app_data)
{
  installer_data data;

  if (confirm_uninstall (app_data, package)) 
    {
      data.loop = NULL;
      data.app_data = app_data;
      data.package = package;
      data.dialog = NULL;
      data.initial_used_kb = kb_used_in_var_lib_install ();
      data.package_size_kb = -atoi (size);

      data.progress_title = "Uninstalling";
      data.report_title = _("ai_ti_uninstallation_report"),
      data.success_text = _("ai_ti_application_uninstalled_text");
      data.failure_text = _("ai_ti_uninstallation_failed_text");

      if (spawn_app_installer_tool (app_data, 1,
				    "remove", package, NULL,
				    installer_callback,
				    NULL,
				    NULL,
				    &data))
	{
	  data.loop = g_main_loop_new (NULL, 0);
	  data.timeout = g_timeout_add (500, installer_progress, &data);
	  g_main_loop_run (data.loop);
	}
    }
}

gint space_left_on_device(void)
{
  gint free_space_in_kb = -1;
  struct statvfs buf;

  if (statvfs("/", &buf) != 0)
    ULOG_DEBUG("statvfs failed!");

  free_space_in_kb = (buf.f_bsize * buf.f_bfree) / 1024;
  ULOG_DEBUG("free space on device: %d kb\n", free_space_in_kb);

  return free_space_in_kb;
}
