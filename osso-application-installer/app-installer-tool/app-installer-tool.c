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

#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <glib.h>
#include <libgnomevfs/gnome-vfs.h>

#undef DEBUG

/* Prototypes
 */

void setup_environment (void);
void redirect_stdout_to_stderr (gpointer unused);
int run_cmd_save_output (gchar **argv,
			 gchar **stdout_string,
			 gchar **stderr_string);
int run_cmd (gchar **argv);
int get_package_name_from_file (gchar *file, gchar **package);
int package_already_installed (gchar *package, int *is_installed);
int dump_failed_relations (gchar *output, gchar *me);

int do_list (void);
int do_describe_file (gchar *file);
int do_describe_package (gchar *package);
int do_install (gchar *file);
int do_remove (gchar *package, int silent);
int do_copy (gchar *source, gchar *target);

/* Setup the environment for dpkg.  It expects to be able to find
   programs from /sbin.  XXX - We replace the PATH environment
   variable completely but maybe it would be nicer to just add to
   it...

   We also set the locale to "C" since we parse the output of dpkg.
*/

void
setup_environment ()
{
  setenv ("PATH", "/sbin:/bin:/usr/sbin:/usr/bin", 1);
  setenv ("LC_ALL", "C", 1);
}

/* Run ARGV[0] with the given arguments and return its exit code when
   it terminates normally.  When ARGV[0] terminates because of a
   signal or can not be run at all, display a message to stderr and
   return -1.  When STDOUT_STRING is non-NULL, it will get the stdout
   from the program.  You need to free *STDOUT_STRING eventually.
   When STDOUT_STRING is NULL, the stdout of the program will be
   directed to stderr.

   When STDERR_STRING is non-NULL, it will get the stderr from the
   program (including the possibly redirect stdout).  Otherwise,
   stderr goes to our stderr.  You need to free *STDERR_STRING
   eventually.
*/

void
redirect_stdout_to_stderr (gpointer unused)
{
  if (dup2 (2, 1) < 0)
    perror ("can not redirect stdout to stderr");
}

int
run_cmd_save_output (gchar **argv,
		     gchar **stdout_string, gchar **stderr_string)
{
  gint exit_status;
  GError *error = NULL;

#ifdef DEBUG
  {
    int i;
    fprintf (stderr, "[");
    for (i = 0; argv[i]; i++)
      fprintf (stderr, " %s", argv[i]);
    fprintf (stderr, " ]\n");
  }
#endif

  g_spawn_sync (NULL,
		argv,
		NULL,
		0,
		stdout_string? NULL : redirect_stdout_to_stderr,
		NULL,
		stdout_string,
		stderr_string,
		&exit_status,
		&error);

  if (error)
    {
      fprintf (stderr, "can not run %s: %s\n", argv[0], error->message);
      g_error_free (error);
      return -1;
    }
  
  if (WIFEXITED (exit_status))
    {
#ifdef DEBUG
      fprintf (stderr, "[ exit %d ]\n", WEXITSTATUS (exit_status));
#endif
      return WEXITSTATUS (exit_status);
    }
  
  if (WIFSIGNALED (exit_status))
    {
      fprintf (stderr, "%s caught signal %d%s.\n",
	       argv[0], WTERMSIG (exit_status),
	       (WCOREDUMP (exit_status)? " (dumped core)" : ""));
      return -1;
    }

  if (WIFSTOPPED (exit_status))
    {
      /* This should better not happen.  Hopefully g_spawn_sync does
	 not use WUNTRACED in its call to waitpid or equivalent.  We
	 still handle this case for extra clarity in the error
	 message.
      */
      fprintf (stderr, "%s stopped with signal %d.\n",
	       argv[0], WSTOPSIG (exit_status));
      return -1;
    }
	       
  fprintf (stderr, "can not run %s for unknown reasons.\n", argv[0]);
  return -1;
}

int
run_cmd (gchar **argv)
{
  return run_cmd_save_output (argv, NULL, NULL);
}

/* Get the package name from a .deb file and return 0 when this
   succeeded.  You need to free the name eventually, even for failure.
*/

int
get_package_name_from_file (gchar *file, gchar **package)
{
  int result;
  int len;
  gchar *args[] = {
    "/usr/bin/dpkg-deb",
    "-f", file, "Package",
    NULL
  };

  result = run_cmd_save_output (args, package, NULL);
  if (result == 0)
    {
      /* Remove trailing newline. */
      len = strlen (*package);
      if ((*package)[len-1] == '\n')
	(*package)[len-1] = '\0';
    }
  return result;
}

/* Figure out whether a package is already installed.
 */

int
package_already_installed (gchar *package, int *is_installed)
{
  int result;
  gchar *output;
  gchar *args[] = {
    "/usr/bin/dpkg-query",
    "--admindir=/var/lib/install/var/lib/dpkg",
    "--showformat=${Status}",
    "-W", package,
    NULL
  };

  /* dpkg-query exits with code 1 when there is no information about
     the package and with code 2 when some other error occured.
  */

  result = run_cmd_save_output (args, &output, NULL);
  if (result == 1)
    {
      *is_installed = 0;
      result = 0;
    }
  else if (result == 0)
    {
      gchar *install_status = strrchr (output, ' ');
      *is_installed = 
	install_status && !strcmp (install_status+1, "installed");
    }
  free (output);
  return result;
}

/* List all installed packages.
 */

int
do_list (void)
{
  int result;
  gchar *output, *line, *next_line;
  gchar *args[] = {
    "/usr/bin/dpkg-query",
    "--admindir=/var/lib/install/var/lib/dpkg",
    "--showformat=${Package}\\t${Version}\\t${Installed-Size}\\t${Status}\\n",
    "-W",
    NULL
  };

  result = run_cmd_save_output (args, &output, NULL);
  
  /* Loop over the lines in STDOUT and output them with a transformed
     ${Status} field.
   */
  for (line = output; *line; line = next_line)
    {
      gchar *status;

      next_line = strchr (line, '\n');
      if (next_line)
	*next_line++ = '\0';
      else
	next_line = strchr (line, '\0');

      status = strrchr (line, '\t');
      if (status)
	{
	  status += 1;
	  fwrite (line, status - line, 1, stdout);
	  status = strrchr (status, ' ');
	  if (status && !strcmp (status+1, "installed"))
	    puts ("ok");
	  else
	    puts ("broken");
	}
    }

  free (output);
  return result;
}

static gchar *
parse_field (gchar *ptr, gchar *prefix, gchar **value)
{
  int len;

  if (ptr == NULL)
    return NULL;

  len = strlen (prefix);
  if (strncmp (ptr, prefix, len) == 0)
    *value = ptr + len;

  while (1)
    {
      ptr = strchr (ptr, '\n');
      if (ptr == NULL)
	return NULL;
      if (ptr[1] == ' ')
	ptr++;
      else
	{
	  *ptr++ = '\0';
	  return ptr;
	}
    }
}


int
do_describe_file (gchar *file)
{
  int result;
  gchar *output;
  gchar *args[] = {
    "/usr/bin/dpkg-deb",
    "-f", file,
    "Package",
    "Version",
    "Installed-Size",
    "Description",
    NULL
  };

  result = run_cmd_save_output (args, &output, NULL);
  if (result == 0)
    {
      gchar *package = NULL, *version = NULL, *size = NULL;
      gchar *description = NULL;
      gchar *ptr;

      /* The fields are output in the order of the control file, so we
	 really have to parse the output.
      */
      ptr = output;
      while (*ptr)
	{
	  ptr = parse_field (ptr, "Package: ", &package);
	  ptr = parse_field (ptr, "Version: ", &version);
	  ptr = parse_field (ptr, "Installed-Size: ", &size);
	  ptr = parse_field (ptr, "Description: ", &description);
	  
	  g_assert (ptr != NULL);
	}

      printf ("%s\n", package);
      printf ("%s\n", version);
      printf ("%s\n", size);
      printf ("%s\n", description);
    }

  free (output);
  return result;
}

int
do_describe_package (gchar *package)
{
  int result;
  gchar *output;
  gchar *args[] = {
    "/usr/bin/dpkg-query",
    "--admindir=/var/lib/install/var/lib/dpkg",
    "--showformat=${Description}\\n",
    "-W", package,
    NULL
  };

  result = run_cmd_save_output (args, &output, NULL);
  if (result == 0)
    fputs (output, stdout);
  free (output);
  return result;
}

/* Carve out information about failed relations from OUTPUT and print
   it to stdout in the documented format.  Failed relations are
   unsatisfied dependencies and conflicts.  Return non-zero when
   something has been printed to stdout, zero otherwise.
*/

int
dump_failed_relations (gchar *output, gchar *me)
{
  gchar *line, *next_line;
  int found_something = 0;

  /* We expect a very rigid format here and do not interpret version
     information at all.
  */
  for (line = output; *line; line = next_line)
    {
      char p1[256], p2[256];
		      
      next_line = strchr (line, '\n');
      if (next_line)
	*next_line++ = '\0';
      else
	next_line = strchr (line, '\0');
      
      if (sscanf (line," %255s depends on %255[^;]", p1, p2) == 2
	  && !strcmp (p1, me))
	{
	  found_something = 1;
	  printf ("depends %s\n", p2);
	}
      else if (sscanf (line," %255s conflicts with %255s", p1, p2) == 2
	  && !strcmp (p1, me))
	{
	  found_something = 1;
	  printf ("conflicts %s\n", p2);
	}
      else if (sscanf (line," %255s conflicts with %255s", p1, p2) == 2
	  && !strcmp (p2, me))
	{
	  found_something = 1;
	  printf ("conflicts %s\n", p1);
	}
      else if (sscanf (line, " %255s depends on %255[^.]", p1, p2) == 2
	       && !strcmp (p2, me))
	{
	  found_something = 1;
	  printf ("depended %s\n", p1);
	}
    }

  return found_something;
}

int
do_install (gchar *file)
{
  int result, is_installed;
  gchar *package, *output;
  gchar *install_args[] = {
    "/usr/bin/fakeroot",
    "/usr/bin/dpkg",
    "--root=/var/lib/install",
    "--install", file,
    NULL
  };

  result = get_package_name_from_file (file, &package);
  if (result != 0)
    {
      puts ("failed");
      goto done;
    }

  result = package_already_installed (package, &is_installed);
  if (result != 0)
    {
      puts ("failed");
      goto done;
    }

  if (is_installed)
    {
      puts ("exists");
      result = 1;
      goto done;
    }

  result = run_cmd_save_output (install_args, NULL, &output);
  fputs (output, stderr);

  if (result != 0)
    {
      if (!dump_failed_relations (output, package))
	puts ("failed");
      
      fprintf (stderr,
	       "Installation of %s failed, removing package %s.\n",
	       file, package);
      if (do_remove (package, 1) != 0)
	fprintf (stderr, "removing failed!\n");
    }
  free (output);

 done:
  free (package);
  return result;
}

int
do_remove (gchar *package, int silent)
{
  gchar *output;
  int result;
  gchar *args[] = {
    "/usr/bin/fakeroot",
    "/usr/bin/dpkg",
    "--root=/var/lib/install",
    "--purge", package,
    NULL
  };

  result = run_cmd_save_output (args, NULL, &output);
  fputs (output, stderr);
  
  if (!silent)
    {
      if (result != 0 && !dump_failed_relations (output, package))
	puts ("failed");
    }
  free (output);
  return result;
}

static gboolean
copy_progress (GnomeVFSXferProgressInfo *info, gpointer raw_data)
{
  if (info->file_size > 0)
    printf ("%g\n", ((double) info->bytes_copied) / info->file_size);;
  return TRUE;
}

int
do_copy (gchar *source, gchar *target)
{
  GnomeVFSURI *source_uri, *target_uri;
  GnomeVFSResult result;

  if (!gnome_vfs_init ())
    {
      fprintf (stderr, "Can not initialize Gnome VFS\n");
      return 1;
    }

  source_uri = gnome_vfs_uri_new (source);
  if (source_uri == NULL)
    {
      fprintf (stderr, "Unsupported URI: %s.\n", source);
      return 1;
    }

  target_uri = gnome_vfs_uri_new (target);
  if (target_uri == NULL)
    {
      fprintf (stderr, "Unsupported URI: %s.\n", target);
      return 1;
    }

  result = gnome_vfs_xfer_uri (source_uri,
			       target_uri,
			       GNOME_VFS_XFER_DEFAULT,
			       GNOME_VFS_XFER_ERROR_MODE_ABORT,
			       GNOME_VFS_XFER_OVERWRITE_MODE_REPLACE,
			       copy_progress,
			       NULL);

  if (result != GNOME_VFS_OK)
    {
      fprintf (stderr, "Copying %s to %s failed: %s\n",
	       source, target, gnome_vfs_result_to_string (result));
      return 1;
    }

  return 0;
}

int
main (int argc, char **argv)
{
  int result;

  setup_environment ();

  if (argc == 2 && !strcmp (argv[1], "list"))
    result = do_list ();
  else if (argc == 3 && !strcmp (argv[1], "describe-file"))
    result = do_describe_file (argv[2]);
  else if (argc == 3 && !strcmp (argv[1], "describe-package"))
    result = do_describe_package (argv[2]);
  else if (argc == 3 && !strcmp (argv[1], "install"))
    result = do_install (argv[2]);
  else if (argc == 3 && !strcmp (argv[1], "remove"))
    result = do_remove (argv[2], 0);
  else if (argc == 4 && !strcmp (argv[1], "copy"))
    result = do_copy (argv[2], argv[3]);
  else
    {
      fputs ("usage: app-installer-tool list\n",                       stderr);
      fputs ("       app-installer-tool describe-file <file>\n",       stderr);
      fputs ("       app-installer-tool describe-package <package>\n", stderr);
      fputs ("       app-installer-tool install <file>\n",             stderr);
      fputs ("       app-installer-tool remove <package>\n",           stderr);
      fputs ("       app-installer-tool copy <source-uri> <target>\n", stderr);
      exit (1);
    }

  if (result == -1)
    exit (255);
  else
    exit (result);
}
