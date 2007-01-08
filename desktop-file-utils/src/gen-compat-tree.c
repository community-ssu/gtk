
/* The vfolder stuff is a bunch of cruft that needs to die die die,
 * only the new menu spec is interesting going forward. i.e. menu-*.[hc]
 * is good, vfolder-*.[hc] is bad, this file is crufted up with both
 * of them.
 */

#include <config.h>

#include <glib.h>
#include <popt.h>

#include "desktop_file.h"
#include "validate.h"
#include "vfolder-parser.h"
#include "vfolder-query.h"
#include "menu-process.h"
#include "menu-tree-cache.h"
#include "menu-util.h"

#include <libintl.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <locale.h>

#define _(x) gettext ((x))
#define N_(x) x

static char *target_dir = NULL;
static gboolean do_print = FALSE;
static gboolean do_verbose = FALSE;
static char *only_show_in_desktop = NULL;
static gboolean print_available = FALSE;
static gboolean do_print_test_results = FALSE;
static DesktopEntryTreeCache *tree_cache = NULL;

static void parse_options_callback (poptContext              ctx,
                                    enum poptCallbackReason  reason,
                                    const struct poptOption *opt,
                                    const char              *arg,
                                    void                    *data);
static void process_one_file       (const char              *filename);

enum {
  OPTION_DIR,
  OPTION_PRINT,
  OPTION_VERBOSE,
  OPTION_DESKTOP,
  OPTION_PRINT_AVAILABLE,
  OPTION_TEST_RESULTS,
  OPTION_LAST
};

struct poptOption options[] = {
  {
    NULL, 
    '\0', 
    POPT_ARG_CALLBACK,
    parse_options_callback, 
    0, 
    NULL, 
    NULL
  },
  { 
    NULL, 
    '\0', 
    POPT_ARG_INCLUDE_TABLE, 
    poptHelpOptions,
    0, 
    N_("Help options"), 
    NULL 
  },
  {
    "dir",
    '\0',
    POPT_ARG_STRING,
    NULL,
    OPTION_DIR,
    N_("Specify the directory where the compat tree should be generated."),
    NULL
  },
  {
    "print",
    '\0',
    POPT_ARG_NONE,
    NULL,
    OPTION_PRINT,
    N_("Print a human-readable representation of the menu to standard output."),
    NULL
  },
  {
    "test-results",
    '\0',
    POPT_ARG_NONE,
    NULL,
    OPTION_TEST_RESULTS,
    N_("Print menu in test results format for menu test suite to standard output."),
    NULL
  },
  {
    "desktop",
    '\0',
    POPT_ARG_STRING,
    NULL,
    OPTION_DESKTOP,
    N_("Specify the current desktop, for purposes of OnlyShowIn."),
    NULL
  },
  {
    "print-available",
    '\0',
    POPT_ARG_NONE,
    NULL,
    OPTION_PRINT_AVAILABLE,
    N_("Print the set of desktop files used for a given menu file."),
    NULL
  },
  {
    "verbose",
    '\0',
    POPT_ARG_NONE,
    NULL,
    OPTION_VERBOSE,
    N_("Print verbose debug information."),
    NULL
  },
  {
    NULL,
    '\0',
    0,
    NULL,
    0,
    NULL,
    NULL
  }
};

static void
parse_options_callback (poptContext              ctx,
                        enum poptCallbackReason  reason,
                        const struct poptOption *opt,
                        const char              *arg,
                        void                    *data)
{
  const char *str;
  
  if (reason != POPT_CALLBACK_REASON_OPTION)
    return;

  switch (opt->val & POPT_ARG_MASK)
    {
    case OPTION_DIR:
      if (target_dir)
        {
          g_printerr (_("Can only specify %s once\n"), "--dir");

          exit (1);
        }

      str = poptGetOptArg (ctx);
      target_dir = g_strdup (str);
      break;

    case OPTION_PRINT:
      if (do_print)
        {
          g_printerr (_("Can only specify %s once\n"), "--print");

          exit (1);
        }
      do_print = TRUE;
      break;

    case OPTION_VERBOSE:
      if (do_verbose)
        {
          g_printerr (_("Can only specify %s once\n"), "--verbose");

          exit (1);
        }

      do_verbose = TRUE;
      vfolder_set_verbose_queries (TRUE);
      menu_set_verbose_queries (TRUE);
      break;

    case OPTION_DESKTOP:
      if (only_show_in_desktop)
        {
          g_printerr (_("Can only specify %s once\n"), "--desktop");

          exit (1);
        }

      only_show_in_desktop = g_strdup (poptGetOptArg (ctx));
      break;
    case OPTION_PRINT_AVAILABLE:
      if (print_available)
        {
          g_printerr (_("Can only specify %s once\n"), "--print-available");

          exit (1);
        }
      print_available = TRUE;
      break;

    case OPTION_TEST_RESULTS:
      if (do_print_test_results)
        {
          g_printerr (_("Can only specify %s once\n"), "--test-results");

          exit (1);
        }
      do_print_test_results = TRUE;
      break;
      
    default:
      break;
    }
}

int
main (int argc, char **argv)
{
  poptContext ctx;
  int nextopt;
  const char** args;
  int i;
  
  setlocale (LC_ALL, "");
  
  ctx = poptGetContext ("desktop-menu-tool", argc, (const char **) argv, options, 0);

  poptReadDefaultConfig (ctx, TRUE);

  while ((nextopt = poptGetNextOpt (ctx)) > 0)
    /*nothing*/;

  if (nextopt != -1)
    {
      g_printerr (_("Error on option %s: %s.\nRun '%s --help' to see a full list of available command line options.\n"),
                  poptBadOption (ctx, 0),
                  poptStrerror (nextopt),
                  argv[0]);
      return 1;
    }

  if (target_dir == NULL && !do_print && !print_available && !do_print_test_results)
    {
      g_printerr (_("Must specify --dir option for target directory or --print option to print the menu or --print-available to print available desktop files.\n"));
      return 1;
    }

  if (only_show_in_desktop)
    vfolder_set_only_show_in_desktop (only_show_in_desktop);
  
  args = poptGetArgs (ctx);

  i = 0;
  while (args && args[i])
    {
      process_one_file (args[i]);
      ++i;
    }

  if (i == 0) 
    { 
	    if ( do_print_test_results) {
		    XdgPathInfo xdg;
		    char * menufile;

		    init_xdg_paths (&xdg);
		    menufile = g_build_filename (xdg.first_system_config,
                                           "menus", "applications.menu", NULL);
		    process_one_file (menufile);
		    g_free (menufile);
	    } else {
	      g_printerr (_("Must specify one menu file to parse\n"));
	    }

      return 1;
    }
  
  poptFreeContext (ctx);
        
  return 0;
}

static void
start_element_handler (GMarkupParseContext  *context,
                       const gchar          *element_name,
                       const gchar         **attribute_names,
                       const gchar         **attribute_values,
                       gpointer              user_data,
                       GError              **error)
{
  char **root_element = user_data;

  g_assert (*root_element == NULL);

  *root_element = g_strdup (element_name);

  g_set_error (error, G_MARKUP_ERROR,
               G_MARKUP_ERROR_INVALID_CONTENT,
               "Fake error to force gmarkup to abort parsing");
}

static GMarkupParser scan_elements_funcs = {
  start_element_handler,
  NULL,
  NULL,
  NULL,
  NULL
};

static char*
markup_get_root_node_name (const char  *filename,
                           GError     **error)
{
  GMarkupParseContext *context;
  char *root_element;
  FILE *f;
#define BUFLEN 128
  char buf[BUFLEN];
  size_t bytes_read;
  
  root_element = NULL;

  f = fopen (filename, "r");
  if (f == NULL)
    {
      g_set_error (error, G_FILE_ERROR,
                   g_file_error_from_errno (errno),
                   _("Failed to open \"%s\": %s\n"),
                   filename, g_strerror (errno));
      return NULL;
    }
  
  context = g_markup_parse_context_new (&scan_elements_funcs,
                                        0, &root_element, NULL);


  bytes_read = fread (buf, 1, BUFLEN, f);
  while (bytes_read > 0)
    {
      g_assert (root_element == NULL);
      
      if (!g_markup_parse_context_parse (context,
                                         buf, bytes_read,
                                         NULL))
        goto out;
      
      bytes_read = fread (buf, 1, BUFLEN, f);
    }

  if (ferror (f))
    {
      g_set_error (error, G_FILE_ERROR,
                   g_file_error_from_errno (errno),
                   _("Failed to read from \"%s\": %s\n"),
                   filename, g_strerror (errno));
      goto out;
    }

  if (!g_markup_parse_context_end_parse (context, NULL))
    goto out;

  goto out;

 out:

  fclose (f);
  
  if (context)
    g_markup_parse_context_free (context);

  /* note we may return NULL without setting error if the file
   * simply lacked a root element
   */
  if (root_element != NULL)
    return root_element;
  else
    return NULL;
}

static void
process_one_file (const char *filename)
{
  GError *err;
  char *root_element;

  /* the filename may be just "applications.menu" to be searched for
   * in XDG_CONFIG_DIRS, so we don't fail fatally if we can't open the
   * root element, just assume <Menu>
   */

  root_element = NULL;
  if (g_path_is_absolute (filename))
    {
      err = NULL;
      root_element = markup_get_root_node_name (filename, &err);
      
      if (err != NULL)
        g_error_free (err);
    }

  if (root_element == NULL)
    root_element = g_strdup ("Menu");
  
  if (strcmp (root_element, "VFolderInfo") == 0)
    {
      Vfolder *folder;

      err = NULL;
      folder = vfolder_load (filename, &err);
      if (err)
        {
          g_printerr (_("Failed to load %s: %s\n"),
                      filename, err->message);
          g_error_free (err);

          exit (1);
        }

      if (folder)
        {
          DesktopFileTree *tree;

          tree = desktop_file_tree_new (folder);

          if (print_available)
            desktop_file_tree_dump_desktop_list (tree);
          
          if (do_print)
            desktop_file_tree_print (tree,
                                     DESKTOP_FILE_TREE_PRINT_NAME |
                                     DESKTOP_FILE_TREE_PRINT_GENERIC_NAME); 

          if (target_dir)
            desktop_file_tree_write_symlink_dir (tree, target_dir);
          
          desktop_file_tree_free (tree);
          
          vfolder_free (folder);
        }
    }
  else if (strcmp (root_element, "Menu") == 0)
    {
      DesktopEntryTree *tree;

      if (tree_cache == NULL)
        tree_cache = desktop_entry_tree_cache_new (NULL);
      
      err = NULL;
      tree = desktop_entry_tree_cache_lookup (tree_cache,
                                              filename,
                                              FALSE, &err);
      
      if (err)
        {
          g_printerr (_("Failed to load %s: %s\n"),
                      filename, err->message);
          g_error_free (err);

          exit (1);
        }

      g_assert (tree != NULL);
      
      if (print_available)
        desktop_entry_tree_dump_desktop_list (tree);
      
      if (do_print)
        desktop_entry_tree_print (tree,
                                  DESKTOP_ENTRY_TREE_PRINT_NAME |
                                  DESKTOP_ENTRY_TREE_PRINT_GENERIC_NAME); 

      if (do_print_test_results)
        desktop_entry_tree_print (tree,
                                  DESKTOP_ENTRY_TREE_PRINT_TEST_RESULTS);
      
      if (target_dir)
        desktop_entry_tree_write_symlink_dir (tree, target_dir);

#if 0
      /* for memprof */
      sleep (30);
#endif
      
      desktop_entry_tree_unref (tree);      
    }
  else
    {
      g_printerr (_("In file \"%s\", root element <%s> is unknown\n"),
                  filename, root_element);
      exit (1);
    }

  g_free (root_element);

#if 0
  /* for memprof */
  g_print ("At end after freeing stuff\n");
  sleep (30);
#endif
}
