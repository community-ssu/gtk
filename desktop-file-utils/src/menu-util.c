/* Random utility functions for menu code */

/*
 * Copyright (C) 2003 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "menu-util.h"
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <stdio.h>

#include <libintl.h>
#define _(x) gettext ((x))
#define N_(x) x

gboolean
g_create_dir (const char    *dir,
              unsigned int   mode,
              GError       **err)
{
  char *parent;
  
  menu_verbose ("Creating directory \"%s\" mode %o\n", dir, mode);
  
  parent = g_path_get_dirname (dir);

  menu_verbose ("Parent dir is \"%s\"\n", parent);

  if (!g_file_test (parent, G_FILE_TEST_IS_DIR))
    {
      if (!g_create_dir (parent, mode, err))
        {
          menu_verbose ("Failed to create parent dir\n");
          g_free (parent);
          return FALSE;
        }
    }

  g_free (parent);
  
  if (mkdir (dir, mode) < 0)
    {
      if (errno != EEXIST)
        {
          g_set_error (err, G_FILE_ERROR_FAILED,
                       g_file_error_from_errno (errno),
                       _("Could not make directory \"%s\": %s"),
                       dir, g_strerror (errno));
          menu_verbose ("Error: \"%s\"\n", err ? (*err)->message : "(no err requested)");
          return FALSE;
        }
    }
  
  return TRUE;
}

static gboolean
write_string (int         fd,
              const char *data,
              size_t      len,
              const char *filename,
              GError    **error)
{
  size_t total_written;
  size_t bytes_written;

  total_written = 0;
  
 again:

  bytes_written = write (fd, data + total_written, len - total_written);

  if (bytes_written < 0)
    {
      if (errno == EINTR)
        goto again;
      else
        {
          g_set_error (error, G_FILE_ERROR,
                       g_file_error_from_errno (errno),
                       _("Failed to write file \"%s\": %s\n"),
                       filename, g_strerror (errno));
          return FALSE;
        }
    }
  else
    {
      total_written += bytes_written;
      if (total_written < len)
        goto again;
    }
  
  return TRUE;
}

gboolean
g_file_save_atomically (const char *filename,
                        const char *str,
                        int         len,
                        GError    **error)
{
  char *tmpfile;
  int fd;
  gboolean retval;

  retval = FALSE;
  
  if (len < 0)
    len = strlen (str);
  
  tmpfile = g_strconcat (filename, ".tmp-XXXXXX", NULL);
  
  fd = g_mkstemp (tmpfile);
  if (fd < 0)
    {
      g_set_error (error, G_FILE_ERROR,
                   g_file_error_from_errno (errno),
                   _("Could not create file \"%s\": %s\n"),
                   tmpfile, g_strerror (errno));
      goto out;
    }
  
  if (!write_string (fd, str, len, tmpfile, error))
    goto out;
  
  if (close (fd) < 0)
    {
      g_set_error (error, G_FILE_ERROR,
                   g_file_error_from_errno (errno),
                   _("Failed to close file \"%s\": %s\n"),
                   tmpfile, g_strerror (errno));
      goto out;
    }

  if (rename (tmpfile, filename) < 0)
    {
      g_set_error (error, G_FILE_ERROR,
                   g_file_error_from_errno (errno),
                   _("Failed to move file \"%s\" to \"%s\": %s\n"),
                   tmpfile, filename, g_strerror (errno));
      goto out;
    }

  g_free (tmpfile);
  tmpfile = NULL; /* so we won't try to unlink it */

  retval = TRUE;
  
 out:
  if (tmpfile)
    unlink (tmpfile); /* ignore failures */
  g_free (tmpfile);
  return retval;
}

static int
utf8_fputs (const char *str,
            FILE       *f)
{
  char *l;
  int ret;
  
  l = g_locale_from_utf8 (str, -1, NULL, NULL, NULL);

  if (l == NULL)
    ret = fputs (str, f); /* just print it anyway, better than nothing */
  else
    ret = fputs (l, f);

  g_free (l);

  return ret;
}

void
menu_verbose (const char *format,
              ...)
{
  va_list args;
  char *str;
  static gboolean verbose = FALSE;
  static gboolean initted = FALSE;
  
  if (!initted)
    {
      verbose = g_getenv ("DFU_MENU_VERBOSE") != NULL;
      initted = TRUE;
    }

  if (!verbose)
    return;
    
  va_start (args, format);
  str = g_strdup_vprintf (format, args);
  va_end (args);

  utf8_fputs (str, stderr);
  fflush (stderr);
  
  g_free (str);
}

void
g_string_append_random_bytes (GString    *str,
                              int         n_bytes)
{
  int i;
  i = 0;
  while (i < n_bytes)
    {
      g_string_append_c (str,
                         g_random_int_range (0, 256));
      ++i;
    }
}

void
g_string_append_random_ascii (GString *str,
                              int      n_bytes)
{
  static const char letters[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijklmnopqrstuvwxyz";
  unsigned int i;

  g_string_append_random_bytes (str, n_bytes);
  
  i = str->len - n_bytes;
  while (i < str->len)
    {
      str->str[i] = letters[str->str[i] % (sizeof (letters) - 1)];
      ++i;
    }
}

static char**
parse_search_path_and_prepend (const char *path,
                               const char *prepend_this)
{
  char **retval;
  int i;
  int len;

  if (path != NULL)
    {
      menu_verbose ("Parsing path \"%s\"\n", path);
      
      retval = g_strsplit (path, ":", -1);

      for (len = 0; retval[len] != NULL; len++)
        ;

      menu_verbose ("%d elements after split\n", len);
        
      i = 0;
      while (i < len) {
        /* delete empty strings */
        if (*(retval[i]) == '\0')
          {
            menu_verbose ("Deleting element %d\n", i);
            
            g_free (retval[i]);
            g_memmove (&retval[i],
                       &retval[i + 1],
                       (len - i) * sizeof (retval[0]));
            --len;
          }
        else
          {
            menu_verbose ("Keeping element %d\n", i);
            ++i;
          }
      }

      if (prepend_this != NULL)
        {
          menu_verbose ("Prepending \"%s\"\n", prepend_this);
          
          retval = g_renew (char*, retval, len + 2);
          g_memmove (&retval[1],
                     &retval[0],
                     (len + 1) * sizeof (retval[0]));
          retval[0] = g_strdup (prepend_this);
        }
    }
  else
    {
      menu_verbose ("Using \"%s\" as only path element\n", prepend_this);
      retval = g_new0 (char*, 2);
      if (prepend_this)
        retval[0] = g_strdup (prepend_this);
    }
        
  return retval;
}                        

void
init_xdg_paths (XdgPathInfo *info_p)
{
  static XdgPathInfo info = { NULL, NULL, NULL, NULL };

  if (info.data_home == NULL)
    {
      const char *p;

      p = g_getenv ("XDG_DATA_HOME");
      if (p != NULL && *p != '\0')
        info.data_home = g_strdup (p);
      else
        info.data_home = g_build_filename (g_get_home_dir (),
                                           ".local", "share",
                                           NULL);
                
      p = g_getenv ("XDG_CONFIG_HOME");
      if (p != NULL && *p != '\0')
        info.config_home = g_strdup (p);
      else
        info.config_home = g_build_filename (g_get_home_dir (),
                                             ".config", NULL);
                        
      p = g_getenv ("XDG_DATA_DIRS");
      if (p == NULL || *p == '\0')
        p = PREFIX"/local/share:"DATADIR;
      info.data_dirs = parse_search_path_and_prepend (p, info.data_home);
      info.first_system_data = info.data_dirs[1];
      
      p = g_getenv ("XDG_CONFIG_DIRS");
      if (p == NULL || *p == '\0')
        p = SYSCONFDIR"/xdg";
      info.config_dirs = parse_search_path_and_prepend (p, info.config_home);
      info.first_system_config = info.config_dirs[1];
      
#ifndef DFU_MENU_DISABLE_VERBOSE
      {
        int q;
        q = 0;
        while (info.data_dirs[q])
          {
            menu_verbose ("Data Path Entry: %s\n", info.data_dirs[q]);
            ++q;
          }
        q = 0;
        while (info.config_dirs[q])
          {
            menu_verbose ("Conf Path Entry: %s\n", info.config_dirs[q]);
            ++q;
          }
      }
#endif /* Verbose */
    }

  *info_p = info;
}
