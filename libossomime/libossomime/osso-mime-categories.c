#include <config.h>
#include "osso-mime.h"
#include <libgnomevfs/gnome-vfs-mime-info.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/time.h>

static GHashTable *category_hash;

const gchar *
osso_mime_get_category_name (OssoMimeCategory category)
{
	switch (category) {
	case OSSO_MIME_CATEGORY_AUDIO:
		return "audio";
	case OSSO_MIME_CATEGORY_BOOKMARKS:
		return "bookmarks";
	case OSSO_MIME_CATEGORY_CONTACTS:
		return "contacts";
	case OSSO_MIME_CATEGORY_DOCUMENTS:
		return "documents";
	case OSSO_MIME_CATEGORY_EMAILS:
		return "emails";
	case OSSO_MIME_CATEGORY_IMAGES:
		return "images";
	case OSSO_MIME_CATEGORY_VIDEO:
		return "video"; 
	default:
		return NULL;
	}
}

OssoMimeCategory
osso_mime_get_category_from_name (const gchar *category)
{
	int i;

	struct {
		const gchar *name;
		OssoMimeCategory category;
	} categories [] = {
		{ "audio",     OSSO_MIME_CATEGORY_AUDIO },    
		{ "bookmarks", OSSO_MIME_CATEGORY_BOOKMARKS },
		{ "contacts",  OSSO_MIME_CATEGORY_CONTACTS },
		{ "documents", OSSO_MIME_CATEGORY_DOCUMENTS },
		{ "emails",    OSSO_MIME_CATEGORY_EMAILS },
		{ "images",    OSSO_MIME_CATEGORY_IMAGES },
		{ "video",     OSSO_MIME_CATEGORY_VIDEO },
	};
	
	for (i = 0; i < G_N_ELEMENTS (categories); i++) {
		if (strcmp (category, categories[i].name) == 0) {
			return categories[i].category;
		}
	}
	
	return OSSO_MIME_CATEGORY_OTHER;
}

OssoMimeCategory
osso_mime_get_category_for_mime_type (const gchar *mime_type)
{
	const gchar *category;

	category = gnome_vfs_mime_get_value (mime_type, "category");

	if (!category) {
		return OSSO_MIME_CATEGORY_OTHER;
	}

	return osso_mime_get_category_from_name (category);
}

typedef struct XdgDirTimeList XdgDirTimeList;

struct XdgDirTimeList
{
	time_t mtime;
	char *directory_name;
	int checked;
	XdgDirTimeList *next;
};

static XdgDirTimeList *dir_time_list = NULL;
static time_t last_stat_time = 0;
static int need_reread = TRUE;

enum
{
	XDG_CHECKED_UNCHECKED,
	XDG_CHECKED_VALID,
	XDG_CHECKED_INVALID
};

/* Function called by xdg_run_command_on_dirs.  If it returns TRUE, further
 * directories aren't looked at */
typedef int (*XdgDirectoryFunc) (const char *directory,
				 void       *user_data);

static XdgDirTimeList *
xdg_dir_time_list_new (void)
{
	XdgDirTimeList *retval;

	retval = calloc (1, sizeof (XdgDirTimeList));
	retval->checked = XDG_CHECKED_UNCHECKED;

	return retval;
}

static void
xdg_dir_time_list_free (XdgDirTimeList *list)
{
	XdgDirTimeList *next;

	while (list) {
		next = list->next;
		free (list->directory_name);
		free (list);
		list = next;
	}
}

static void
read_categories (const gchar *file_name)
{
	gsize    length;
	gchar   *contents, *line;
	gchar   *pos;
	gchar  **types;
	GList   *type_list = NULL;
	int      i;
	GError  *error = NULL;
	gchar   *category;

	if (!g_file_get_contents (file_name, &contents, &length, &error)) {
		g_warning ("Could not open categories file: '%s'\n", 
			   error->message);
		g_error_free (error);
	}

        pos = contents;
	while (TRUE) {
		gchar *tmp;

                if (*pos == '#') {
                        pos = strchr (pos, '\n');
	
			if (!pos) {
				break;
			} else {
				pos++;
				continue;
			}
		}

		tmp = pos;
		pos = strchr (tmp, ':');

		if (!pos) {
			break;
		}

		category = g_strndup (tmp, pos - tmp);

		pos++;
    
		line = strchr (pos, '\n');

		if (!line) {
			tmp = g_strdup (pos);
		} else {
			tmp = g_strndup (pos, line - pos);
		}
    
		types = g_strsplit (tmp, " ", -1);

		for (i = 0; types[i] != NULL; i++) {
			if (types[i][0] == '\0') {
                                /* Free here since we just free the types 
                                 * pointer below.
                                 */
                                g_free (types[i]);
				continue;
			}
	
			type_list = g_list_prepend (type_list, types[i]);
		}

                g_hash_table_insert (category_hash, category, type_list);
		type_list = NULL;

                /* All elements have been put in list or freed above */
		g_free (types);
		g_free (tmp);

		if (!line) {
			break;
		}

		pos = line + 1;
	}

        g_free (contents);
}

static int
xdg_mime_init_from_directory (const char *directory)
{
	char           *file_name;
	struct          stat st;
	XdgDirTimeList *list;

	assert (directory != NULL);

	file_name = malloc (strlen (directory) + strlen ("/mime/categories") + 1);
	strcpy (file_name, directory); 
	strcat (file_name, "/mime/categories");

	if (stat (file_name, &st) == 0) {
		read_categories (file_name);
		
		list = xdg_dir_time_list_new ();
		list->directory_name = file_name;
		list->mtime = st.st_mtime;
		list->next = dir_time_list;
		dir_time_list = list;
	} else {
		free (file_name);
	}

	return FALSE; /* Keep processing */
}

/* Runs a command on all the directories in the search path */
static void
xdg_run_command_on_dirs (XdgDirectoryFunc  func,
			 void             *user_data)
{
	const char *xdg_data_home;
	const char *xdg_data_dirs;
	const char *ptr;

	xdg_data_home = getenv ("XDG_DATA_HOME");
	if (xdg_data_home) {
		if ((func) (xdg_data_home, user_data)) {
			return;
		}
	} else {
		const char *home;
		
		home = getenv ("HOME");
		if (home != NULL) {
			char *guessed_xdg_home;
			int stop_processing;

			guessed_xdg_home = malloc (strlen (home) + strlen ("/.local/share/") + 1);
			strcpy (guessed_xdg_home, home);
			strcat (guessed_xdg_home, "/.local/share/");
			stop_processing = (func) (guessed_xdg_home, user_data);
			free (guessed_xdg_home);

			if (stop_processing) {
				return;
			}
		}
	}

	xdg_data_dirs = getenv ("XDG_DATA_DIRS");
	if (xdg_data_dirs == NULL) {
		xdg_data_dirs = "/usr/local/share/:/usr/share/";
	}

	ptr = xdg_data_dirs;

	while (*ptr != '\000') {
		const char *end_ptr;
		char *dir;
		int len;
		int stop_processing;

		end_ptr = ptr;
		
		while (*end_ptr != ':' && *end_ptr != '\000') {
			end_ptr ++;
		}

		if (end_ptr == ptr) {
			ptr++;
			continue;
		}

		if (*end_ptr == ':') {
			len = end_ptr - ptr;
		} else {
			len = end_ptr - ptr + 1;
		}
		
		dir = malloc (len + 1);
		strncpy (dir, ptr, len);
		dir[len] = '\0';
		stop_processing = (func) (dir, user_data);
		free (dir);
		
		if (stop_processing) {
			return;
		}
		ptr = end_ptr;
	}
}

/* Checks file_path to make sure it has the same mtime as last time it was
 * checked.  If it has a different mtime, or if the file doesn't exist, it
 * returns FALSE.
 *
 * FIXME: This doesn't protect against permission changes.
 */
static int
xdg_check_file (const char *file_path)
{
	struct stat st;

	/* If the file exists */
	if (stat (file_path, &st) == 0) {
		XdgDirTimeList *list;
      
		for (list = dir_time_list; list; list = list->next) {
			if (! strcmp (list->directory_name, file_path) &&
			    st.st_mtime == list->mtime)
			{
				if (list->checked == XDG_CHECKED_UNCHECKED) {
					list->checked = XDG_CHECKED_VALID;
				}
				else if (list->checked == XDG_CHECKED_VALID) {
					list->checked = XDG_CHECKED_INVALID;
				}

				return (list->checked != XDG_CHECKED_VALID);
			}
		}
		
		return TRUE;
	}
	
	return FALSE;
}

static int
xdg_check_dir (const char *directory,
	       int        *invalid_dir_list)
{
	int   invalid;
	char *file_name;
	
	assert (directory != NULL);

	/* Check the globs file */
	file_name = malloc (strlen (directory) + strlen ("/mime/globs") + 1);
	strcpy (file_name, directory); strcat (file_name, "/mime/globs");
	invalid = xdg_check_file (file_name);
	free (file_name);
  
	if (invalid) {
		*invalid_dir_list = TRUE;
		return TRUE;
	}

	/* Check the magic file */
	file_name = malloc (strlen (directory) + strlen ("/mime/magic") + 1);
	strcpy (file_name, directory); strcat (file_name, "/mime/magic");
	invalid = xdg_check_file (file_name);
	free (file_name);
	if (invalid) {
		*invalid_dir_list = TRUE;
		return TRUE;
	}

	return FALSE; /* Keep processing */
}

/* Walks through all the mime files stat()ing them to see if they've changed.
 * Returns TRUE if they have. */
static int
xdg_check_dirs (void)
{
	XdgDirTimeList *list;
	int             invalid_dir_list = FALSE;

	for (list = dir_time_list; list; list = list->next) {
		list->checked = XDG_CHECKED_UNCHECKED;
	}
	
	xdg_run_command_on_dirs ((XdgDirectoryFunc) xdg_check_dir,
				 &invalid_dir_list);

	if (invalid_dir_list) {
		return TRUE;
	}

	for (list = dir_time_list; list; list = list->next) {
		if (list->checked != XDG_CHECKED_VALID) {
			return TRUE;
		}
	}

  return FALSE;
}

/* We want to avoid stat()ing on every single mime call, so we only look for
 * newer files every 5 seconds.  This will return TRUE if we need to reread the
 * mime data from disk.
 */
static int
xdg_check_time_and_dirs (void)
{
	struct timeval tv;
	time_t current_time;
	int    retval = FALSE;

	gettimeofday (&tv, NULL);
	current_time = tv.tv_sec;

	if (current_time >= last_stat_time + 5) {
		retval = xdg_check_dirs ();
		last_stat_time = current_time;
	}
	
	return retval;
}

static void
xdg_mime_shutdown (void)
{
	/* FIXME: Need to make this (and the whole library) thread safe */
	if (dir_time_list) {
		xdg_dir_time_list_free (dir_time_list);
		dir_time_list = NULL;
	}

	if (category_hash) {
		g_hash_table_destroy (category_hash);
		category_hash = NULL;
	}

	need_reread = TRUE;
}

static void
string_list_free (GList *list)
{
	g_list_foreach (list, (GFunc)g_free, NULL);
	g_list_free (list);
}

static void
xdg_mime_init (void)
{
	if (xdg_check_time_and_dirs ()) {
		xdg_mime_shutdown ();
	}
  
	if (need_reread) {
		category_hash = g_hash_table_new_full (g_str_hash, 
						       g_str_equal, 
						       g_free, 
						       (GDestroyNotify)string_list_free);
		
		xdg_run_command_on_dirs ((XdgDirectoryFunc) xdg_mime_init_from_directory,
					 NULL);
		
		need_reread = FALSE;
	}
}

static GList *
dup_string_list (GList *list)
{
	GList *new_list = NULL;

	for (; list != NULL; list = list->next) {
		new_list = g_list_prepend (new_list, g_strdup (list->data));
	}

	return g_list_reverse (new_list);
}

GList *
osso_mime_get_mime_types_for_category (OssoMimeCategory category)
{
	const gchar *category_name;
	GList       *list;

	if (category == OSSO_MIME_CATEGORY_OTHER) {
		return NULL;
	}

	xdg_mime_init ();

        category_name = osso_mime_get_category_name (category);
        
        if (category_name) {
                list = g_hash_table_lookup (category_hash, category_name);

                return dup_string_list (list);
        }

        return NULL;
}

void 
osso_mime_types_list_free (GList *list)
{
        g_list_foreach (list, (GFunc)g_free, NULL);
	g_list_free (list);
}
