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
 
/*
 * hildon-file-system-private.c
 *
 * Functions in this package are internal helpers of the
 * HildonFileSystem and should not be called by
 * applications.
 */

#include <libintl.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtkicontheme.h>
#include "hildon-file-system-private.h"
#include "hildon-file-system-settings.h"

#include "hildon-file-common-private.h"
#include "hildon-file-system-local-device.h"
#include "hildon-file-system-mmc.h"
#include "hildon-file-system-upnp.h"
#include "hildon-file-system-obex.h"
#include "hildon-file-system-old-gateway.h"

extern GtkFileSystem *gtk_file_system_unix_new();

/* Let's make sure that we survise with folder names both ending and
 * not ending to slash */
gboolean _hildon_file_system_compare_ignore_last_separator(const char *a,
                                                           const char *b)
{        
    gint len_a, len_b;

    len_a = strlen(a);
    len_b = strlen(b);

    if (len_a > 1 && a[len_a - 1] == G_DIR_SEPARATOR)
      len_a--;
    if (len_b > 1 && b[len_b - 1] == G_DIR_SEPARATOR)
      len_b--;

    if (len_a != len_b)
      return FALSE;

    return g_ascii_strncasecmp(a, b, len_a) == 0;
}

/*** Setting up devices ************************************************************/

/* If safe folders would be dynamic locations under local device, this logic
   could be moved into local device class */
static gchar *get_local_device_root_path(void)
{
    const gchar *env;

    env = g_getenv("MYDOCSDIR");
    if (env && env[0])
        return g_strdup(env);

    /* g_getenv uses $HOME as last resort. Normally it returns home 
       directory defined in passwd database. We want to use $HOME
       when possible. */
    env = g_getenv("HOME");

    return g_build_path(G_DIR_SEPARATOR_S, 
              (env && env[0]) ? env : g_get_home_dir(), 
              "MyDocs", NULL);
}

static GNode *setup_safe_folder(GNode *parent, const gchar *root_path, 
    const gchar *relative_path, const gchar *title, const gchar *icon,
    HildonFileSystemModelItemType type)
{
    HildonFileSystemSpecialLocation *location;
    gchar *local_path, *uri;
    GNode *result = NULL;

    g_assert(parent != NULL);

    local_path = g_build_path(G_DIR_SEPARATOR_S, root_path, relative_path, NULL);
    uri = g_filename_to_uri(local_path, NULL, NULL);

    if (uri) {
        location = g_object_new(HILDON_TYPE_FILE_SYSTEM_SPECIAL_LOCATION, NULL);
        hildon_file_system_special_location_set_icon(location, icon);
        hildon_file_system_special_location_set_display_name(location, title);
        location->basepath = uri;
        location->sort_weight = SORT_WEIGHT_FOLDER;
        location->compatibility_type = type;
        result = g_node_new(location);
        g_node_append(parent, result);
    }

    g_free(local_path);

    return result;
}

static void setup_mmc(GNode *parent, const gchar *path, gboolean internal)
{
    HildonFileSystemSpecialLocation *location;
    gchar *uri;

    if (path && path[0]) {
        GValue val = {0, };
        g_value_init(&val, G_TYPE_BOOLEAN);

        uri = g_filename_to_uri(path, NULL, NULL);
        if (uri) {
            location = g_object_new(HILDON_TYPE_FILE_SYSTEM_MMC, NULL);
            g_value_set_boolean(&val, internal);
            g_object_set_property(G_OBJECT(location), "internal-card",
                                  &val);
            location->basepath = uri;
            g_node_append_data(parent, location);
        }
    }
}

GNode *_hildon_file_system_get_locations(GtkFileSystem *fs)
{
    static GNode *locations = NULL;

    if (G_UNLIKELY(locations == NULL))
    {
        HildonFileSystemSpecialLocation *location;
        const gchar *env;
        gchar *rootpath;
        GNode *rootnode;

        locations = g_node_new(NULL); /* Invisible root node above everything else */
        rootpath =  get_local_device_root_path();

        /* Setup local device */
        location = g_object_new(HILDON_TYPE_FILE_SYSTEM_LOCAL_DEVICE, NULL);
        location->basepath = g_filename_to_uri(rootpath, NULL, NULL);
        rootnode = g_node_new(location);
        g_node_append(locations, rootnode);

        /* Setup safe folders */
        setup_safe_folder(rootnode, rootpath, ".images", 
            _("sfil_li_folder_images"), "qgn_list_filesys_image_fldr",
            HILDON_FILE_SYSTEM_MODEL_SAFE_FOLDER_IMAGES);
        setup_safe_folder(rootnode, rootpath, ".videos", 
            _("sfil_li_folder_video_clips"), "qgn_list_filesys_video_fldr",
            HILDON_FILE_SYSTEM_MODEL_SAFE_FOLDER_VIDEOS);
        setup_safe_folder(rootnode, rootpath, ".sounds", 
            _("sfil_li_folder_sound_clips"), "qgn_list_filesys_audio_fldr",
            HILDON_FILE_SYSTEM_MODEL_SAFE_FOLDER_SOUNDS);
        setup_safe_folder(rootnode, rootpath, ".documents", 
            _("sfil_li_folder_documents"), "qgn_list_filesys_doc_fldr",
            HILDON_FILE_SYSTEM_MODEL_SAFE_FOLDER_DOCUMENTS);
        setup_safe_folder(rootnode, rootpath, ".games", 
            _("sfil_li_folder_games"), "qgn_list_filesys_games_fldr",
            HILDON_FILE_SYSTEM_MODEL_SAFE_FOLDER_GAMES);

        g_free(rootpath);

        /* Setup MMC device(s) */
        setup_mmc(rootnode, g_getenv("MMC_MOUNTPOINT"), FALSE);
        setup_mmc(rootnode, g_getenv("INTERNAL_MMC_MOUNTPOINT"), TRUE);

        /* Setup uPnP devices */
        env = g_getenv("UPNP_ROOT");
        if (env && env[0]) {
            location = g_object_new(HILDON_TYPE_FILE_SYSTEM_UPNP, NULL);
            location->basepath = g_strdup(env);
            g_node_append_data(locations, location);
        }    

        if (!g_getenv("DISABLE_GATEWAY")) {
            /* Setup gateway */
            location = g_object_new(HILDON_TYPE_FILE_SYSTEM_OLD_GATEWAY, NULL);
            g_node_append_data(locations, location);
        } else {
            /* Setup multiple Bluetooth device support */
            env = g_getenv("HILDON_FM_OBEX_ROOT");
            if (env && env[0]) {
                location = g_object_new(HILDON_TYPE_FILE_SYSTEM_OBEX, NULL);
                location->basepath = g_strdup(env);
                g_node_append_data(locations, location);
            }    
        }
    }

    return locations;
}

typedef struct {
    gchar *uri;
    gint len_uri;
    HildonFileSystemSpecialLocation *result;
    gboolean is_child;
} CallbackData;

static gboolean get_special_location_callback(GNode *node, gpointer data)
{
    HildonFileSystemSpecialLocation *candidate = node->data;
    CallbackData *searched = data;

    if (candidate) { 
        /* Check if the searched uri exactly matches this location OR
          is under this location. It might be a dynamic device in that case */
        gint len_cand = strlen(candidate->basepath);

        if (len_cand > 1 && candidate->basepath[len_cand - 1] == G_DIR_SEPARATOR)
          len_cand--;

        if (searched->len_uri >= len_cand && g_ascii_strncasecmp(searched->uri, 
                candidate->basepath, len_cand) == 0) 
        {
            if (searched->len_uri == len_cand) {
                searched->result = g_object_ref(candidate);
                searched->is_child = FALSE;
            } else if (searched->uri[len_cand] == G_DIR_SEPARATOR) {
                searched->result =
                    hildon_file_system_special_location_create_child_location(
                    candidate, searched->uri);
                searched->is_child = TRUE;
                ULOG_INFO("Checking if %s considers %s as dynamic device? Result = %p",
                    candidate->basepath, searched->uri, (gpointer) searched->result);
            }

            return searched->result != NULL;
        }
    }

    return FALSE;
}

HildonFileSystemSpecialLocation *
_hildon_file_system_get_special_location(GtkFileSystem *fs,
                                         const GtkFilePath *path)
{
    CallbackData data;
    GNode *locations;

    locations = _hildon_file_system_get_locations(fs);
    data.uri = gtk_file_system_path_to_uri(fs, path);
    data.result = NULL;
    
    if (data.uri) {
        /* Let's precalculate the length for the entire search */
        data.len_uri = strlen(data.uri);
        if (data.len_uri > 1 && data.uri[data.len_uri - 1] == G_DIR_SEPARATOR)
          data.len_uri--;

        g_node_traverse(locations, G_PRE_ORDER, G_TRAVERSE_ALL, -1, 
            get_special_location_callback, &data);
        g_free(data.uri);
    }

    return data.result;
}

typedef struct
{
  const gchar *name;
  gint size;
} CacheElement;

static GHashTable *get_cache(GtkIconTheme *theme);

static void cache_element_free(gpointer a)
{
  if (a)
  {
    CacheElement *item = a;
    g_free((gchar *) item->name);
    g_free(item);
  }
}

static gboolean cache_element_equal(gconstpointer a, gconstpointer b)
{
  const CacheElement *ea, *eb;

  ea = (CacheElement *) a;
  eb = (CacheElement *) b;

  return ea->size == eb->size && g_str_equal(ea->name, eb->name);
}

static guint cache_element_hash(gconstpointer a)
{
  const CacheElement *e = a;

  return g_str_hash(e->name) ^ e->size;
}

static gboolean find_finalized_icon(gpointer key, gpointer value, 
  gpointer data)
{
  return value == data;  
}

static void icon_finalized(gpointer data, GObject *finalized_icon)
{
  GHashTable *hash = get_cache(GTK_ICON_THEME(data));

  ULOG_INFO("%s: %p", __FUNCTION__, (gpointer) finalized_icon);
  g_hash_table_foreach_remove(hash, find_finalized_icon, finalized_icon);

  if (g_hash_table_size(hash) == 0)
  {
    /* Setting data to NULL causes gobject to call installed finalizer */
    g_object_set_data(G_OBJECT(data), "hildon-file-system-icon-cache", NULL);
  }
}

static void unref_all_helper(gpointer key, gpointer value, gpointer data)
{
  g_object_weak_unref(G_OBJECT(value), icon_finalized, NULL);  
}

static void cache_finalize(gpointer data)
{
  GHashTable *cache = data;

  ULOG_INFO(__FUNCTION__);

  g_hash_table_foreach(cache, unref_all_helper, NULL); 
  g_hash_table_destroy(cache);
}

static GHashTable *get_cache(GtkIconTheme *theme)
{
  GHashTable *cache;

  cache = g_object_get_data(G_OBJECT(theme), "hildon-file-system-icon-cache");
  if (!cache)
  {
    cache = g_hash_table_new_full (cache_element_hash, cache_element_equal,
                                   cache_element_free, NULL);
    g_object_set_data_full(G_OBJECT(theme), "hildon-file-system-icon-cache",
      cache, cache_finalize);
  }

  return cache;
}

static GdkPixbuf *_hildon_file_system_lookup_icon_cached(GtkIconTheme *theme,
  const gchar *name, gint size)
{
  CacheElement key;

  key.name = name;
  key.size = size;

  return g_hash_table_lookup(get_cache(theme), &key);
}

static void _hildon_file_system_insert_icon(GtkIconTheme *theme, 
  const gchar *name, gint size, GdkPixbuf *icon)
{
  CacheElement *key;
  GHashTable *hash;

  key = g_new(CacheElement, 1);
  key->name = g_strdup(name);
  key->size = size;
  hash = get_cache(theme);

  g_hash_table_insert(hash, key, icon);
  g_object_weak_ref(G_OBJECT(icon), icon_finalized, theme);
}

GdkPixbuf *_hildon_file_system_load_icon_cached(GtkIconTheme *theme,
						const gchar *name,
						gint size)
{
  GdkPixbuf *pixbuf;

  pixbuf = _hildon_file_system_lookup_icon_cached(theme, name, size);  

  if (!pixbuf)
  {
    ULOG_INFO("Cache miss, loading %s at %d pix", name, size);
    pixbuf = gtk_icon_theme_load_icon(theme, name, size, 0, NULL);
    if (!pixbuf) return NULL;

    _hildon_file_system_insert_icon(theme, name, size, pixbuf);
  }
  else
    g_object_ref(pixbuf);

  return pixbuf;
}

GdkPixbuf *
_hildon_file_system_create_image(GtkFileSystem *fs, 
      GtkWidget *ref_widget, GtkFilePath *path, 
      HildonFileSystemSpecialLocation *location, gint size)
{
    if (!ref_widget)
        return NULL;

    if (location) {
        GdkPixbuf *pixbuf;
        pixbuf = hildon_file_system_special_location_get_icon(location,
               fs, ref_widget, size);
        if (pixbuf) return pixbuf;
    }

    return gtk_file_system_render_icon(fs, path, ref_widget, size, NULL);
}

static const gchar *get_custom_root_name(const GtkFilePath *path)
{
  const gchar *s, *name;
  gssize len;

  g_assert(path != NULL);
  s = gtk_file_path_get_string(path);
  len = strlen(s);

  while (TRUE)
  {
    name = g_strrstr_len(s, len, "/");

    if (!name)
      return s;
    if (name[1] != 0)
      return &name[1];  /* This looks weird, but is safe */
    len = name - s;
  }
}

gchar *
_hildon_file_system_create_file_name(GtkFileSystem *fs,
  const GtkFilePath *path, HildonFileSystemSpecialLocation *location, 
  GtkFileInfo *info)
{
  if (location) {
    char *name;
    name = hildon_file_system_special_location_get_display_name(location, fs);
    if (name) return name;
  }

  if (info)  
    return g_strdup(gtk_file_info_get_display_name(info));

  return g_strdup(get_custom_root_name(path));
}

gchar *
_hildon_file_system_create_display_name(GtkFileSystem *fs,
  const GtkFilePath *path, HildonFileSystemSpecialLocation *location, 
  GtkFileInfo *info)
{
  gchar *str, *dot;

  str = _hildon_file_system_create_file_name(fs, path, location, info);   

  if (info && !gtk_file_info_get_is_folder(info))
  {
      dot = _hildon_file_system_search_extension(str, 
                gtk_file_info_get_mime_type(info));
      if (dot && dot != str)
        *dot = 0;
  }

  return str;
}

GtkFilePath *_hildon_file_system_path_for_location(GtkFileSystem *fs, 
  HildonFileSystemSpecialLocation *location)
{
  g_assert(HILDON_IS_FILE_SYSTEM_SPECIAL_LOCATION(location));
  return gtk_file_system_uri_to_path(fs, location->basepath);
}

/* You can omit either type or base */
GtkFileSystemVolume *
_hildon_file_system_get_volume_for_location(GtkFileSystem *fs, 
    HildonFileSystemSpecialLocation *location)
{
    GSList *volumes, *iter;
    GtkFilePath *mount_path, *path;
    GtkFileSystemVolume *vol, *result = NULL;
    const char *path_a, *path_b;

    /* We cannot just use get_volume_for_path, because it won't
        work work with URIs other than file:// */
    volumes = gtk_file_system_list_volumes(fs);
    mount_path = _hildon_file_system_path_for_location(fs, location);
    path_a = gtk_file_path_get_string(mount_path);

    for (iter = volumes; iter; iter = g_slist_next(iter))
      if ((vol = iter->data) != NULL)   /* Hmmm, it seems to be possible that this list contains NULL items!! */
      {
        path = gtk_file_system_volume_get_base_path(fs, vol);
        path_b = gtk_file_path_get_string(path);

        if (!result &&
           _hildon_file_system_compare_ignore_last_separator(path_a, path_b))
          result = vol;
        else
          gtk_file_system_volume_free(fs, vol); 

        gtk_file_path_free(path);
      }

    gtk_file_path_free(mount_path);
    g_slist_free(volumes);

    return result;
}

GtkFileSystem *hildon_file_system_create_backend(const gchar *name, gboolean use_fallback)
{
    GtkFileSystem *result = NULL;
    gchar *default_name = NULL;

    /* Let's load a backend module. If user has given a name, we'll try to 
       load it. Otherwise we'll try the default module. As a last resort
       we'll create normal unix backend (if faalback is asked) */

    if (!name) {
        static gboolean not_installed = TRUE;
        GtkSettings *settings;        

        if (not_installed)
        {
          gtk_settings_install_property
            (g_param_spec_string("gtk-file-chooser-backend",
                             "Default file chooser backend",
                             "Name of the GtkFileChooser backend to "
                             "use by default",
                             NULL, G_PARAM_READWRITE));

          not_installed = FALSE;
        }

        settings = gtk_settings_get_default();
        g_object_get(settings, "gtk-file-chooser-backend",
                     &default_name, NULL);
        name = default_name;
    }
    if (name) {
        result = hildon_gtk_file_system_create(name);
        if (!GTK_IS_FILE_SYSTEM(result))
            ULOG_WARN("Cannot create \"%s\" backend", name);
    }
    if (use_fallback && !GTK_IS_FILE_SYSTEM(result))
        result = gtk_file_system_unix_new();

    g_free(default_name);
    return result;
}

/* 
   Giving MIME type is optional (when saving files we have to remove extension
   as well, but we do not have mime-information available). 
   * If not given, then extension db if just searched completely. 
   * If given, then only matching mime-types are searched 
*/

/* known types are stored into list of these structs (with longest types first) */
typedef struct
{
  gchar *extension;
  gchar *mime;
} MimeType;

static gint mime_list_insert(gconstpointer a, gconstpointer b)
{
  return strlen(((MimeType *) b)->extension) - 
         strlen(((MimeType *) a)->extension);
}

gchar *_hildon_file_system_search_extension(gchar *name, const gchar *mime)
{
  static GSList *types = NULL;
  MimeType *type;
  GSList *iter;
  gint len;

  /* Unrecognized types are not touched */
  if (mime && g_ascii_strcasecmp(mime, "application/octet-stream") == 0)
    return NULL;

  /* Initialize suffix hash table from /usr/share/mime/globs */
  if (!types)
  {
    FILE *f;
    gchar line[256];
    gchar *sep;

    f = fopen("/usr/share/mime/globs", "rt");
    if (f)
    {
      while (fgets(line, sizeof(line), f))
      {
        if (line[0] == 0 || line[0] == '#') continue;
        /* fgets leaves newline into buffer */
        len = strlen(line);
        if (line[len - 1] == '\n') line[len - 1] = 0;
        sep = strstr(line, ":*.");
      	if (sep == NULL) continue;
        *sep = 0; /* Clear colon */ 

        type = g_new(MimeType, 1);
        type->extension = g_strdup(sep + 2);
        type->mime = g_strdup(line);
        types = g_slist_insert_sorted(types, type, mime_list_insert);
      }

      fclose(f);

/*      for (iter = types; iter; iter = iter->next)
      {
        type = iter->data;
        fprintf(stderr, "%s: %s\n", type->extension, type->mime);
      }*/
    }
  }

  /* Now we must search possible extensions from the list that match
     suffix of the given name. If mime type is given, it also has to match */
  len = strlen(name);
  for (iter = types; iter; iter = iter->next)
  {
    type = iter->data;

    if (!mime || g_ascii_strcasecmp(mime, type->mime) == 0)
    {
      gchar *candidate = name + len - strlen(type->extension);

      if (name <= candidate && 
          g_ascii_strcasecmp(candidate, type->extension) == 0)
        return candidate;
    }
  }

  /* If we didn't find a match (but type was recognized), then we just
     return the part after last dot. If we didn't have type information,
     then we do not touch the name. */
  return mime ? g_strrstr(name, ".") : NULL;
}

enum {
 STATE_START,
 STATE_OPEN,
 STATE_END,
 STATE_CLOSE
};

/* Checks whether the string contains valid autonumber and
   returns the value. Negative if not valid autonumber. */
long _hildon_file_system_parse_autonumber(const char *start)
{
  gint state = STATE_START;
  long value = 0;
  char *endp;

  while (*start) {
    if (state == STATE_START){
      if (*start == '(') state = STATE_OPEN;
      else if (g_ascii_isspace(*start)) state = STATE_START;
      else return -1;
    }
    else if (state == STATE_OPEN){
      if (g_ascii_isspace(*start)) state = STATE_OPEN;
      else if (g_ascii_isalnum(*start)) {
        value = strtol(start, &endp, 10);
        start = endp;
        state = STATE_END;
        continue; /* start already points to first non-number char */
      }
      else return -1;
    }
    else if (state == STATE_END){
      if (*start == ')') state = STATE_CLOSE;
      else if (g_ascii_isspace(*start)) state = STATE_END;
      else return -1;
    }
    else if (state == STATE_CLOSE){
      if (g_ascii_isspace(*start)) state = STATE_CLOSE;
      else return -1;
    }
    start++;
  }

  return (state == STATE_CLOSE ? value : -1);
}

/* Let's check if the name body already contains autonumber.
 * If this is a case then we'll remove the previous one. */
void _hildon_file_system_remove_autonumber(char *name)
{
  char *par = g_strrstr(name, "(");

  if (par && par > name && _hildon_file_system_parse_autonumber(par) >= 0)
  {
    *par = 0;

    /* Autonumber can have a space before paranthesis.
     * we only remove one, because autonumbering only adds one. */
    par--;
    if (par > name && g_ascii_isspace(*par))
      *par = 0;
  }
}
