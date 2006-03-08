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

/* This utility will check whether the application specified via a
   .desktop file is currently running.  If it is, it will pop up a
   dialog that explains this to the user and asks whether to continue.

   This program exits 0 when the uninstallation should proceed and 111
   when it should be cancelled.
*/

#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include <glib.h>
#include <gtk/gtk.h>
#include <hildon-widgets/hildon-note.h>

int
find_exec_pid (const char *file)
{
  char fullname[512];
  char linkbuf[512];
  int n;
  char *ptr;

  DIR *d = opendir ("/proc");
  if (d)
    {
      struct dirent *e;
      while ((e = readdir (d)))
	{
	  strcpy (fullname, "/proc/");
	  strcat (fullname, e->d_name);
	  strcat (fullname, "/exe");
	  if ((n = readlink (fullname, linkbuf, 512-1)) >= 0)
	    {
	      linkbuf[n] = '\0';
	      if ((ptr = strstr (linkbuf, file))
		  && strlen (ptr) == strlen (file))
		{
		  int pid = atoi (e->d_name);
		  closedir (d);
		  return pid;
		}
	    }
	}
      closedir (d);
    }

  return -1;
}

int
run_dialog (const char *name)
{
  GtkWidget *dialog;
  char *text = g_strdup_printf ("%s is currently running.\n"
				"Continue anyway?", name);

  dialog = hildon_note_new_confirmation (NULL, text);
  g_free (text);

  return gtk_dialog_run (GTK_DIALOG (dialog));
}

char *arg_name = NULL;
char *arg_exe = NULL;

static GOptionEntry entries[] = {
  { "application-name", 'a', 0, G_OPTION_ARG_STRING, &arg_name,
    "Application name", "app" },
  { "exe-file", 'x', 0, G_OPTION_ARG_STRING, &arg_exe,
    "Executable filename", "file" },
  { NULL }
};

int
main (int argc, char **argv)
{
  GError *error = NULL;
  GOptionContext *context;

  int pid, code = 0;

  context = g_option_context_new ("- check for running applications");
  g_option_context_add_main_entries (context, entries, NULL);
  g_option_context_add_group (context, gtk_get_option_group (TRUE));
  g_option_context_parse (context, &argc, &argv, &error);

  if (error)
    {
      fprintf (stderr, "check-application-for-uninstall: %s\n",
	       error->message);
      exit (1);
    }

  if (arg_name == NULL)
    {
      fprintf (stderr, "check-application-for-uninstall: Missing -a option\n");
      exit (1);
    }

  if (arg_exe == NULL)
    {
      fprintf (stderr, "check-application-for-uninstall: Missing -x option\n");
      exit (1);
    }

  pid = find_exec_pid (arg_exe);

  if (pid >= 0)
    code = (run_dialog (arg_name) == GTK_RESPONSE_OK)? 0 : 111;

  exit (code);
}
