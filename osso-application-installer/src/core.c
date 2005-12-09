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

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <glib.h>
#include <gconf/gconf-client.h>
#include <obex-vfs-utils/ovu-xfer.h>

#include "core.h"
#include "ui/represent.h"
#include "applicationinstaller-i18n.h"

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
     the app-installer-tool as soon as it becomes available.  This
     used to be used for copying a remote file to local storage.  The
     app-installer-tool can provide real progress information for that
     and we use it to keep a progress bar alive.  (Nowadays, we do the
     copy ourselves, since we have to use a ovu_async_xfer anyway and
     I finally figured out how to do it...)

   The third way is of course the most general and the other two ways
   are in fact implemented as special cases of it.  This means that
   the app-installer-tool is in fact always executed asynchronously,
   and the GUI will never starve.

   The function SPAWN_APP_INSTALLER_TOOL implements the most general
   way of invoking the app-installer-tool, and RUN_APP_INSTALLER_TOOL
   uses it to run it sort-of-synchronously.

   XXX - These three ways should probably be unified so that even
   getting the list of installed packages gets a progress bar and can
   be canceled.  The dialog for that would of course only be shown
   after a certain timeout.
*/


/* This function starts /usr/bin/app-installer-tool so that it runs in
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
   is called directly, SPAWN_APP_INSTALLER_TOOL returns NULL.
   Otherwise, it returns a handle that can be used with
   CANCEL_APP_INSTALLER_TOOL to interrupt the process.

   The handle needs to be freed with free_app_installer_tool.
*/
typedef struct tool_handle tool_handle;
static tool_handle *
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

struct tool_handle {
  GPid pid;

  void (*callback) (gpointer, int, gchar *, gchar *);
  void (*line_callback[2]) (gpointer, gchar *);
  gpointer callback_data;

  int stdin_fd;
  GIOChannel *channels[2];
  GString *strings[2];
  gboolean exited;
  int status;
};

static void
check_completion (tool_handle *data)
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
    }
}

static void
reap_process (GPid pid, int status, gpointer raw_data)
{
  tool_handle *data = (tool_handle *)raw_data;
  data->status = status;
  data->exited = 1;
  check_completion (data);
}

static gboolean
read_into_string (GIOChannel *channel, GIOCondition cond, gpointer raw_data)
{
  tool_handle *data = (tool_handle *)raw_data;
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
add_string_reader (int fd, int id, tool_handle *data)
{
  data->strings[id] = g_string_new ("");
  data->channels[id] = g_io_channel_unix_new (fd);
  g_io_add_watch (data->channels[id], 
		  G_IO_IN | G_IO_HUP | G_IO_ERR,
		  read_into_string, data);
  g_io_channel_unref (data->channels[id]);
}

static tool_handle *
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
  tool_handle *data;
  GError *error = NULL;
  GPid child_pid;
  int stdin_fd, stdout_fd, stderr_fd;

#if USE_SUDO
  if (as_install)
    {
      /* Only use sudo when we are not running inside Scratchbox.
       */
      struct stat info;
      if (stat ("/targets/links/scratchbox.config", &info))
	{
	  *arg++ = "/usr/bin/sudo";
	  *arg++ = "-u";
	  *arg++ = "install";
	}
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
			    &stdin_fd,
			    &stdout_fd,
			    &stderr_fd,
			    &error);

  if (error)
    {
      callback (callback_data, 0, "", error->message);
      g_error_free (error);
      return NULL;
    }

  data = g_new (tool_handle, 1);
  data->pid = child_pid;
  data->stdin_fd = stdin_fd;
  data->callback = callback;
  data->callback_data = callback_data;
  data->line_callback[0] = stdout_line_callback;
  data->line_callback[1] = stderr_line_callback;

  g_child_watch_add (child_pid, reap_process, data);
  add_string_reader (stdout_fd, 0, data);
  add_string_reader (stderr_fd, 1, data);

  return data;
}

static void
cancel_app_installer_tool (tool_handle *data)
{
  /* Closing stdin is detected by app-installer-tool and it will exit
     as soon as possible.
  */
  close (data->stdin_fd);
  data->stdin_fd = -1;
}

static void
free_app_installer_tool (tool_handle *data)
{
  if (data->stdin_fd > 0)
    close (data->stdin_fd);
  g_spawn_close_pid (data->pid);
  g_free (data);
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
   the user.  (XXX-UI32 - stderr is not actually shown.)
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
    {
      /* XXX-UI32 - Do not suppress stderr_output.
       */
      present_report_with_details (data->app_data,
				   data->error_description,
				   NULL);
    }

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
  tool_handle *handle;
  run_data data;

  if (stderr_string == NULL)
    stderr_string = &my_stderr_string;

  data.loop = NULL;
  data.app_data = app_data;
  data.stdout_string = &stdout_string;
  data.stderr_string = &stderr_string;
  data.error_description = error_description;
  handle = spawn_app_installer_tool (app_data, 1,
				     arg1, arg2, arg3,
				     run_callback,
				     NULL,
				     NULL,
				     &data);
  if (handle)
    {
      data.loop = g_main_loop_new (NULL, 0);
      g_main_loop_run (data.loop);
      g_main_loop_unref (data.loop);
      free_app_installer_tool (handle);
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

static int
all_white_space (gchar *text)
{
  while (*text)
    if (!isspace (*text++))
      return 0;
  return 1;
}

GtkTreeModel *
list_packages (AppData *app_data)
{
  GtkListStore *store;
  GtkTreeIter iter;
  gchar *stdout_string, *ptr;
  GdkPixbuf *default_icon;
  GtkIconTheme *icon_theme;
  gchar *version_fmt, *size_fmt;

  /* XXX-NLS - once these logical ids appear in l10n files, we can use
               the simpler _(...) markup instead of gettext_try_many.
  */
  _("ai_ti_version_column");
  version_fmt = gettext_try_many ("ai_ti_version_column",
				  "v%s",
				  NULL);
  _("ai_ti_size_column");
  size_fmt = gettext_try_many ("ai_ti_size_column",
			       "%skB",
			       NULL);

  if (!run_app_installer_tool (app_data,
			       "list", NULL, NULL,
			       &stdout_string, NULL,
			       /* XXX-NLS - be more descriptive */
			       dgettext ("osso-filemanager",
					 "sfil_ni_operation_failed")))
    return NULL;

  icon_theme = gtk_icon_theme_get_default ();
  default_icon = gtk_icon_theme_load_icon (icon_theme,
					   "qgn_list_gene_default_app",
					   26,
					   0,
					   NULL);

  store = gtk_list_store_new (NUM_COLUMNS,
			      G_TYPE_OBJECT,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
			      G_TYPE_BOOLEAN);

  /* Parse stdout_string.
   */
  ptr = stdout_string;
  while (*ptr)
    {
      gchar *name, *version, *size, *status, *icon_name;
      gboolean broken;
      GdkPixbuf *icon;

      ptr = parse_delimited (ptr, '\t', &name);
      ptr = parse_delimited (ptr, '\t', &version);
      ptr = parse_delimited (ptr, '\t', &size);
      ptr = parse_delimited (ptr, '\n', &status);

      g_assert (ptr != NULL); /* XXX - do not crash here */

      if (!strcmp (name, META_PACKAGE))
	continue;

      version = g_strdup_printf (version_fmt, version);
      if (!all_white_space (size))
	size = g_strdup_printf (size_fmt, size);
      else
	size = g_strdup ("");
      broken = strcmp (status, "ok");

      icon_name = g_strdup_printf ("pkg_%s", name);
      icon = gtk_icon_theme_load_icon (icon_theme,
				       icon_name,
				       26,
				       0,
				       NULL);
      g_free (icon_name);

      gtk_list_store_append (store, &iter);
      gtk_list_store_set (store, &iter,
			  COLUMN_ICON, icon? icon : default_icon, 
			  COLUMN_NAME, name,
			  COLUMN_VERSION, version,
			  COLUMN_SIZE, size,
			  COLUMN_BROKEN, broken,
			  -1);
      
      if (icon)
	g_object_unref (icon);

      free (version);
      free (size);
    }

  g_object_unref (default_icon);
  free (stdout_string);
  return GTK_TREE_MODEL (store);
}

gboolean
package_already_installed (AppData *app_data, gchar *package)
{
  GtkTreeIter iter;
  gboolean have_iter;
  GtkTreeView *view = GTK_TREE_VIEW (app_data->app_ui_data->treeview);
  GtkTreeModel *model = GTK_TREE_MODEL (gtk_tree_view_get_model (view));
  
  if (model == NULL)
    return FALSE;

  for (have_iter = gtk_tree_model_get_iter_first (model, &iter);
       have_iter;
       have_iter = gtk_tree_model_iter_next (model, &iter))
    {
      gchar *name;
      gboolean broken, found_it;

      gtk_tree_model_get (model, &iter,
			  COLUMN_NAME, &name,
			  COLUMN_BROKEN, &broken,
			  -1);
      found_it = (strcmp (name, package) == 0 && !broken);
      g_free (name);
      if (found_it)
	return TRUE;
    }

  return FALSE;
}

gboolean
any_packages_installed (GtkTreeModel *model)
{
  GtkTreeIter iter;
  return model && gtk_tree_model_get_iter_first (model, &iter);
}

static void
text_cell_data_func (GtkTreeViewColumn *tree_column,
		     GtkCellRenderer *cell,
		     GtkTreeModel *tree_model,
		     GtkTreeIter *iter,
		     gpointer data)
{
  gboolean broken;
  gtk_tree_model_get (tree_model, iter,
		      COLUMN_BROKEN, &broken,
		      -1);
  if (broken)
    {
      /* XXX-UI32 - maybe find a better way to present broken
                    packages. */
      g_object_set (cell, "strikethrough", TRUE, NULL);
    }
  else
    {
      /* Use the default. */
      g_object_set (cell, "strikethrough-set", FALSE, NULL);
    }
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
  gtk_tree_view_column_set_cell_data_func (column,
					   renderer,
					   text_cell_data_func,
					   NULL,
					   NULL);
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
  gtk_tree_view_column_set_cell_data_func (column,
					   renderer,
					   text_cell_data_func,
					   NULL,
					   NULL);
  g_object_set (renderer, "xalign", 1.0, NULL);
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
  gtk_tree_view_column_set_cell_data_func (column,
					   renderer,
					   text_cell_data_func,
					   NULL,
					   NULL);
  g_object_set (renderer, "xalign", 1.0, NULL);
  gtk_tree_view_append_column (treeview, column);
  app_ui_data->size_column = column;
}


static void
report_installation_failure (AppData *app_data,
			     gchar *source,
			     gchar *details_fmt,
			     ...)
{
  gchar *report, *details;
  va_list ap;

  va_start (ap, details_fmt);
  report = g_strdup_printf (SUPPRESS_FORMAT_WARNING
			    (_("ai_ti_installation_failed_text")),
			    source);
  details = g_strdup_vprintf (details_fmt, ap);
  present_report_with_details (app_data, report, details);
  g_free (details);
  g_free (report);
  va_end (ap);
}


/* Return information about the package file FILE.  You need to free
   the returned structure eventually with package_info_free.  When the
   information can not be retrieved, all strings in the returned
   structure are NULL.

   The string REJECTION_REASON gives the reason why this package
   should be rejected.  It is used by confirm_install.
 */
static PackageInfo
package_file_info (AppData *app_data, gchar *file)
{
  gchar *stdout_string;
  PackageInfo info;

  info.name = NULL;
  info.size = NULL;
  info.version = NULL;
  info.description = NULL;
  info.rejection_reason = NULL;

  if (run_app_installer_tool (app_data,
			      "describe-file", file, NULL,
			      &stdout_string, NULL,
			      NULL))
    {
      gchar *ptr, *package, *version, *size, *depends, *arch;
      gchar *tok;
      gboolean depends_on_maemo;

      ptr = stdout_string;
      ptr = parse_delimited (ptr, '\n', &package);
      ptr = parse_delimited (ptr, '\n', &version);
      ptr = parse_delimited (ptr, '\n', &size);
      ptr = parse_delimited (ptr, '\n', &depends);
      ptr = parse_delimited (ptr, '\n', &arch);

      if (ptr == NULL)
	{
	  /* XXX-UI32 - do not suppress stderr output. */
	  report_installation_failure (app_data,
				       basename (file),
				       _("ai_error_corrupted"));
	  goto done;
	}

      /* Figure out whether the package properly depends on maemo.
         The maemo package itself does not need to depend on itself,
         tho. 

	 XXX - the parsing is a bit coarse here...
       */
      depends_on_maemo = 0;
      if (strcmp (package, META_PACKAGE) != 0)
	{
	  while ((tok = strsep (&depends, " \t\r\n,")))
	    if (strcmp (tok, META_PACKAGE) == 0)
	      depends_on_maemo = 1;
	}
      else
	depends_on_maemo = 1;
	  
      if (!depends_on_maemo)
	{
	  report_installation_failure (app_data,
				       package,
				       _("ai_error_builtin"),
				       package);
	}
      else 
	{
	  info.name = g_string_new (package);
	  info.version = g_string_new (version);
	  info.size = g_string_new (size);
	  info.description = g_string_new (ptr);
	  
	  if (strcmp (arch, "all") && strcmp (arch, DEB_HOST_ARCH))
	    info.rejection_reason = _("ai_error_incompatible");
	}
    }
  else
    /* XXX-UI32 - do not suppress stderr output. */
    report_installation_failure (app_data,
				 basename (file),
				 _("ai_error_corrupted"));
    
 done:
  free (stdout_string);
  return info;
}

void
package_info_free (PackageInfo *info)
{
  g_string_free (info->name, 1);
  g_string_free (info->version, 1);
  g_string_free (info->size, 1);
  g_string_free (info->description, 1);
  /* rejection_reason is a static string. */
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
			      /* XXX-NLS - be more descriptive */
			      dgettext ("osso-filemanager",
					"sfil_ni_operation_failed")))
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
      g_string_append_printf (string, "%s\n", (gchar *)list->data);
      list = list->next;
    }
}

static gchar *
format_relationship_failures (gchar *footer, gchar *output)
{
  GString *report;
  GSList *depends = NULL, *depended = NULL, *conflicts = NULL;
  int full = 0;
  gchar *ptr;

  report = g_string_new ("");

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
      else if (has_prefix (ptr, "full"))
	full = 1;
      ptr = strchr (ptr, '\n');
      if (ptr)
	*ptr++ = '\0';
    }

  if (full)
    {
      g_string_append (report, _("ai_info_notenoughmemory"));
      g_string_append (report, "\n");
    }

  while (depends)
    {
      g_string_append_printf (report,
			      SUPPRESS_FORMAT_WARNING 
			      (_("ai_error_componentmissing")),
			      (gchar *)(depends->data));
      g_string_append (report, "\n");
      depends = depends->next;
    }
  if (depended)
    {
      /* XXX-NLS - ai_ti_depended_on_by */
      /* XXX - use proper .po plural mechanism */
      if (depended->next)
	g_string_append 
	  (report, _("ai_ti_dependency_conflict_text_plural"));
      else
	g_string_append
	  (report, _("ai_ti_dependency_conflict_text"));
      g_string_append (report, "\n");
      append_list_strings (report, depended);
    }
  if (conflicts)
    {
      /* XXX-NLS - the most ugly hack yet.  We want to say "It
	           conflicts with the following installed packages:"
	           but we can not since we don't have a logical id for
	           it.  So we just display the second sentence of
	           "ai_ti_dependency_conflict" which is "Delete
	           following package first and then try again".

		   I checked that this hack works with the
		   translations for de_DE en_GB en_US es_ES es_MX
		   fr_FR it_IT.
      */
      gchar *text;
      if (conflicts->next)
	text = _("ai_ti_dependency_conflict_text_plural");
      else
	text = _("ai_ti_dependency_conflict_text");
      text = strchr (text, '.');
      if (text)
	{
	  while (isspace (*++text))
	    ;
	  g_string_append (report, text);
	}
      else
	g_string_append (report,
			 "It conflicts with the "
			 "following installed packages:");
      g_string_append (report, "\n");
      append_list_strings (report, conflicts);
    }

  g_slist_free (depends);
  g_slist_free (depended);
  g_slist_free (conflicts);

  if (footer)
    {
      if (report->len > 0)
	g_string_append (report, "\n");
      g_string_append (report, footer);
    }

  ptr = report->str;
  g_string_free (report, 0);

  return ptr;
}

#define FUZZ_FACTOR 1.0

typedef struct {
  tool_handle *tool;

  AppData *app_data;
  gchar *package;
  GMainLoop *loop;
  guint timeout;

  progress_dialog *dialog;
  int initial_used_kb, package_size_kb;

  gchar *progress_title;
  gchar *success_text;
  gchar *failure_text;
  /* XXX-UI32 - never suppress details. */
  gboolean dont_show_details;
  gboolean mmc_cover_open;
} installer_data;

static void
installer_callback (gpointer raw_data,
		    int success,
		    gchar *output, gchar *stderr_output)
{
  installer_data *data = (installer_data *)raw_data;
  GString *report;
  gchar *details = NULL;

  /* XXX-UI32 - do not suppress stderr_output
   */
  stderr_output = NULL;

  if (data->loop)
    {
      g_source_remove (data->timeout);
      if (data->dialog)
	ui_close_progress_dialog (data->dialog);
    }

  report = g_string_new ("");

  if (success)
    g_string_printf (report, data->success_text, data->package);
  else
    {
      g_string_printf (report, data->failure_text, data->package);
      if (!data->dont_show_details)
	{
	  details = format_relationship_failures (stderr_output, output);
	  
	  if (data->mmc_cover_open)
	    {
	      gchar *more_details =
		g_strdup_printf ("%s\n\n%s",
				 _("ai_error_memorycardopen"),
				 details);
	      g_free (details);
	      details = more_details;
	    }
	}
    }

  /* XXX-UI32 - if this operation failed, and we are asked to show
                details, but we don't have any, we show "Incompatible
                package".
  */
  if (!success && !data->dont_show_details
      && (details == NULL || all_white_space (details)))
    {
      g_free (details);
      details = g_strdup (_("ai_error_incompatible"));
    }

  present_report_with_details (data->app_data,
			       report->str,
			       details);

  g_string_free (report, 1);
  g_free (details);

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

  if (data->dialog == NULL)
    data->dialog =
      ui_create_progress_dialog (data->app_data,
				 data->progress_title,
				 NULL, NULL);

#if 0
  /* XXX - the progress indication is not at all accurate and we need
           real cooperation from app-installer-tool for it anyway.  So
           we just pulse the progress bar for now.
  */
  int now_used_kb = kb_used_in_var_lib_install ();
  if (now_used_kb > 0 && data->initial_used_kb > 0)
    {
      double frac = 
	FUZZ_FACTOR * (((double) (now_used_kb - data->initial_used_kb))
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

static void
mmc_cover_changed (GConfClient *client, guint unused, GConfEntry *entry,
		   gpointer raw_data)
{
  installer_data *data = (installer_data *)raw_data;
  if (gconf_value_get_bool (entry->value))
    {
      data->mmc_cover_open = 1;
      cancel_app_installer_tool (data->tool);
    }
}

static gboolean
file_is_on_mmc (const gchar *abs_filename)
{
  const gchar *mount_point = getenv ("MMC_MOUNTPOINT");

  if (mount_point == NULL)
    mount_point = "/media/mmc1";
  return g_str_has_prefix (abs_filename, mount_point);
}

void
install_package (gchar *deb, AppData *app_data) 
{
  PackageInfo info;
  installer_data data;

  info = package_file_info (app_data, deb);

  if (info.name == NULL)
    return;

  /* Don't allow upgrades, except of the special "maemo" package.
   */
  if (strcmp (info.name->str, META_PACKAGE) != 0
      && package_already_installed (app_data, info.name->str))
    {
      /* XXX-NLS - some versions of the osso-application-installer
	           erroneously used ai_ti_alreadyinstalled and this
	           has made it into some versions of the l10n files.
      */
      _("ai_error_alreadyinstalled");
      present_report_with_details (app_data,
				   gettext_try_many
				   ("ai_error_alreadyinstalled",
				    "ai_ti_alreadyinstalled",
				    NULL),
				   NULL);
    }
  else if (confirm_install (app_data, &info))
    {
      data.loop = NULL;
      data.app_data = app_data;
      data.package = info.name->str;
      data.dialog = NULL;
      data.initial_used_kb = kb_used_in_var_lib_install ();
      data.package_size_kb = atoi (info.size->str);

      data.progress_title = _("ai_ti_installing_installing");
      data.success_text = _("ai_ti_application_installed_text");
      data.failure_text = _("ai_ti_installation_failed_text");
      data.dont_show_details = 0;
      data.mmc_cover_open = 0;

      data.tool = spawn_app_installer_tool (app_data, 1,
					    "install", deb, NULL,
					    installer_callback,
					    NULL,
					    NULL,
					    &data);
      if (data.tool)
	{
	  GConfClient *client = NULL;
	  guint cnx_id;

	  if (file_is_on_mmc (deb))
	    {
	      client = gconf_client_get_default ();
	      gconf_client_add_dir (client,
				    "/system/osso/af",
				    GCONF_CLIENT_PRELOAD_NONE,
				    NULL);
	      cnx_id =
		gconf_client_notify_add (client,
					 "/system/osso/af/mmc-cover-open",
					 mmc_cover_changed,
					 &data,
					 NULL,
					 NULL);
	    }

	  data.loop = g_main_loop_new (NULL, 0);
	  data.timeout = g_timeout_add (500, installer_progress, &data);
	  g_main_loop_run (data.loop);
	  g_main_loop_unref (data.loop);

	  if (client)
	    {
	      gconf_client_notify_remove (client, cnx_id);
	      g_object_unref (client);
	    }

	  free_app_installer_tool (data.tool);
	}
    }

  package_info_free (&info);
}
	      
typedef struct {
  progress_dialog *progress_dialog;
  GMainLoop *loop;
  GnomeVFSResult result;
  gboolean cancelled;
} copy_data;

static gboolean
copy_progress (GnomeVFSAsyncHandle *handle,
	       GnomeVFSXferProgressInfo *info,
	       gpointer raw_data)
{
  copy_data *data = (copy_data *)raw_data;

#if 0
  fprintf (stderr, "phase %d, status %d, vfs_status %s\n",
	   info->phase, info->status,
	   gnome_vfs_result_to_string (info->vfs_status));
#endif

  if (info->file_size > 0 && data->progress_dialog)
    {
      float progress = ((float)info->bytes_copied) / info->file_size;
      ui_set_progress_dialog (data->progress_dialog, progress);
    }

  if (info->phase == GNOME_VFS_XFER_PHASE_COMPLETED && data->loop)
    g_main_loop_quit (data->loop);

  /* Produce an appropriate return value depending on the status.
   */
  if (info->status == GNOME_VFS_XFER_PROGRESS_STATUS_OK)
    {
      return !data->cancelled;
    }
  else if (info->status == GNOME_VFS_XFER_PROGRESS_STATUS_VFSERROR)
    {
      data->result = info->vfs_status;
      if (data->loop)
	g_main_loop_quit (data->loop);
      return GNOME_VFS_XFER_ERROR_ACTION_ABORT;
    }
  else
    {
      ULOG_CRIT ("unexpected status %d in copy_progress\n", info->status);
      return 1;
    }
}

static void
copy_cancel (gpointer raw_data)
{
  copy_data *data = (copy_data *)raw_data;
  data->cancelled = TRUE;
  g_main_loop_quit (data->loop);
}

static gboolean
do_copy (AppData *app_data,
	 gchar *source, GnomeVFSURI *source_uri,
	 gchar *target)
{
  GnomeVFSAsyncHandle *handle;
  GnomeVFSURI *target_uri;
  GList *source_uri_list, *target_uri_list;
  GnomeVFSResult result;

  /* XXX - there seems to be no good way to really stop copy_progress
           from being called; I just can not tame
           gnome_vfs_async_xfer, at least not in its ovu_async_xfer
           costume.  Thus, I simple punt the issue and use a static
           copy_data struct.
  */
  static copy_data data;

  target_uri = gnome_vfs_uri_new (target);
  if (target_uri == NULL)
    {
      /* XXX-NLS - be more descriptive. */
      present_error_details_fmt (app_data,
				 dgettext ("osso-filemanager",
					   "sfil_ni_operation_failed"),
				 NULL);
      return FALSE;
    }

  source_uri_list = g_list_append (NULL, (gpointer) source_uri);
  target_uri_list = g_list_append (NULL, (gpointer) target_uri);

  /* XXX-NLS - should have our own id for "Copying". 
   */
  data.progress_dialog =
    ui_create_progress_dialog (app_data,
			       dgettext ("osso-filemanager",
					 "sfil_pr_lb_copying_dev"),
			       copy_cancel,
			       &data);
  data.loop = g_main_loop_new (NULL, 0);
  data.result = GNOME_VFS_OK;
  data.cancelled = FALSE;

  result = ovu_async_xfer (&handle,
			   source_uri_list,
			   target_uri_list,
			   GNOME_VFS_XFER_DEFAULT,
			   GNOME_VFS_XFER_ERROR_MODE_QUERY,
			   GNOME_VFS_XFER_OVERWRITE_MODE_REPLACE,
			   GNOME_VFS_PRIORITY_DEFAULT,
			   copy_progress,
			   &data,
			   NULL,
			   NULL);

  if (result == GNOME_VFS_OK)
    {
      g_main_loop_run (data.loop);
      gnome_vfs_async_cancel (handle);
      g_main_loop_unref (data.loop);
      data.loop = NULL;
      result = data.result;
    }

  ui_close_progress_dialog (data.progress_dialog);
  data.progress_dialog = NULL;

  if (result != GNOME_VFS_OK)
    {
      /* XXX-NLS - be more descriptive */
      present_error_details (app_data,
			     dgettext ("osso-filemanager",
				       "sfil_ni_operation_failed"),
			     NULL,
			     NULL);
      return FALSE;
    }

  return !data.cancelled;
}

void
install_package_from_uri (gchar *uri, AppData *app_data)
{
  GnomeVFSURI *vfs_uri = NULL;
  const gchar *scheme;

  vfs_uri = gnome_vfs_uri_new (uri);
  if (vfs_uri == NULL)
    {
      /* XXX-NLS - be more descriptive */
      report_installation_failure (app_data, uri,
				   dgettext ("osso-filemanager",
					     "sfil_ni_operation_failed"));
      return;
    }

  /* The app-installer-tool can access all "file://" URIs, whether
     they are considered local by GnomeVFS or not.  (GnomeVFS
     considers a file:// URI pointing to a NFS mounted volume as
     remote, but we can read that just fine of course.)
  */

  scheme = gnome_vfs_uri_get_scheme (vfs_uri);
  if (scheme && !strcmp (scheme, "file"))
    {
      const gchar *path = gnome_vfs_uri_get_path (vfs_uri);
      gchar *unescaped_path = gnome_vfs_unescape_string (path, NULL);
      if (unescaped_path)
	{
	  install_package (unescaped_path, app_data);
	  free (unescaped_path);
	}
      else
	{
	  /* XXX-NLS - be more descriptive */
	  report_installation_failure (app_data, uri,
				       dgettext ("osso-filemanager",
						 "sfil_ni_operation_failed"));
	}
    }
  else
    {
      /* We need to copy.
       */
      char template[] = "/var/tmp/osso-ai-XXXXXX", *tempdir;
      gchar *basename, *local;

      /* Make a temporary directory and allow everyone to read it.
       */

      tempdir = mkdtemp (template);
      if (tempdir == NULL)
	{
	  /* XXX-NLS */
	  report_installation_failure (app_data, uri,
				       "Can not create %s: %s",
				       template, strerror (errno));
	}
      else if (chmod (tempdir, 0755) < 0)
	{
	  /* XXX-NLS */
	  report_installation_failure (app_data, uri,
				       "Can not chmod %s: %s",
				       tempdir, strerror (errno));
	}
      else
	{
	  basename = gnome_vfs_uri_extract_short_path_name (vfs_uri);
	  local = g_strdup_printf ("%s/%s", tempdir, basename);
	  free (basename);

	  if (do_copy (app_data, uri, vfs_uri, local))
	    install_package (local, app_data);

	  /* We don't put up dialogs for errors that happen now.  From the
	     point of the user, the installation has been completed and he
	     has seen the report already.
	  */
	  if (unlink (local) < 0)
	    ULOG_ERR ("error unlinking %s: %s\n", local, strerror (errno));
	  if (rmdir (tempdir) < 0)
	    ULOG_ERR ("error removing %s: %s\n", tempdir, strerror (errno));
	  free (local);
	}
    }

  gnome_vfs_uri_unref (vfs_uri);
}

static gboolean
check_dependencies (AppData *app_data, gchar *package)
{
  gchar *stdout_string, *dependencies_report;
  gboolean result;

  /* XXX-NLS - be more descriptive in case of failure. */
  if (!run_app_installer_tool (app_data,
			       "get-dependencies", package, NULL,
			       &stdout_string, NULL,
			       dgettext ("osso-filemanager",
					 "sfil_ni_operation_failed")))
    return FALSE;

  dependencies_report = format_relationship_failures (NULL, stdout_string);
  if (dependencies_report[0] != '\0')
    {
      present_error_details (app_data,
			     _("ai_ti_dependency_conflict_title"),
			     AI_HELP_KEY_DEPENDS,
			     dependencies_report);
      result = FALSE;
    }
  else
    result = TRUE;

  free (stdout_string);
  free (dependencies_report);
  return result;
}

void
uninstall_package (gchar *package, gchar *size, AppData *app_data)
{
  installer_data data;

  if (!check_dependencies (app_data, package))
    return;

  if (confirm_uninstall (app_data, package)) 
    {
      data.loop = NULL;
      data.app_data = app_data;
      data.package = package;
      data.dialog = NULL;
      data.initial_used_kb = kb_used_in_var_lib_install ();
      data.package_size_kb = -atoi (size);

      data.progress_title = _("ai_ti_uninstall_progress_uninstalling");
      data.success_text = _("ai_ti_application_uninstalled_text");
      data.failure_text = _("ai_ti_uninstallation_failed_text");
      data.dont_show_details = 1;
      data.mmc_cover_open = 0;

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
	  g_main_loop_unref (data.loop);
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
