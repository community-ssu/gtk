#include "validate.h"

int 
main (int argc, char *argv[])
{
  char *contents;
  GnomeDesktopFile *df;
  GError *error;
  char *filename;

  if (argc == 2)
    filename = argv[1];
  else
    {
      g_printerr ("Usage: %s <desktop-file>\n", argv[0]);
      exit (1);
    }
  
  if (!g_file_get_contents (filename, &contents,
			    NULL, NULL))
    {
      g_printerr ("%s: error reading desktop file\n", filename);
      return 1;
    }

  error = NULL;
  df = gnome_desktop_file_new_from_string (contents, &error);
  
  if (!df)
    {
      g_printerr ("%s: parse error: %s\n", filename, error->message);
      return 1;
    }

  if (desktop_file_validate (df, filename))
    return 0;
  else
    return 1;
}
