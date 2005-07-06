/*
 * This file is part of hildon-fm package
 *
 * Copyright (C) 2005 Nokia Corporation.
 *
 * Contact: Luc Pionchon <luc.pionchon@nokia.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
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
  hildon-file-system-private.c

  Functions in this paclage are internal helpers of the
  HildonFileSystem and should not be called by
  applications.
*/

#include <libintl.h>
#include <string.h>
#include <gtk/gtkicontheme.h>
#include <osso-log.h>
#include "hildon-file-system-private.h"
#include "hildon-file-system-settings.h"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

extern GtkFileSystem *gtk_file_system_unix_new();

#define _(String) dgettext(PACKAGE, String)
#define N_(String) String

typedef struct
{
  const char *icon_name;
  const char *display_name;
  char *path;
} LocationType; 

/* Keep this table in sync with public enumeration found in
    hildon-file-selection.h */
static LocationType locations[] = {
  { NULL, NULL, NULL }, 
  { "qgn_list_gene_unknown_file", NULL, NULL }, 
  { "qgn_list_filesys_common_fldr", NULL, NULL }, 
  { "qgn_list_filesys_image_fldr", N_("sfil_li_folder_images"), NULL },
  { "qgn_list_filesys_video_fldr", N_("sfil_li_folder_video_clips"), NULL },
  { "qgn_list_filesys_audio_fldr", N_("sfil_li_folder_sound_clips"), NULL },
  { "qgn_list_filesys_doc_fldr", N_("sfil_li_folder_documents"), NULL },
  { "qgn_list_filesys_games_fldr", N_("sfil_li_folder_games"), NULL },
  { "qgn_list_filesys_mmc_root", N_("sfil_li_mmc_local_device"), NULL },
 /* This is used, if GConf for some reason don't contain name */
  { "qgn_list_filesys_divc_gw", N_("sfil_li_gateway_root"), NULL },
  { "qgn_list_filesys_divc_cls", N_("sfil_li_folder_root"), NULL }
};

/* Paths for system folders and devices */
static const char *LOCAL_DEVICE_ROOT = "MyDocs";    /* Under $HOME */
static const char *IMAGES_SAFE = ".images";         /* Under local device */
static const char *VIDEOS_SAFE = ".videos";
static const char *SOUNDS_SAFE = ".sounds";
static const char *DOCUMENTS_SAFE = ".documents";
static const char *GAMES_SAFE = ".games";

/* Let's make sure that we survise with folder names both ending and not ending to slash */
gboolean 
_hildon_file_system_compare_ignore_last_separator(const char *a, const char *b)
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

void _hildon_file_system_ensure_locations(void)
{
  const gchar *env;
  gchar *root;

  if (locations[HILDON_FILE_SYSTEM_MODEL_LOCAL_DEVICE].path != NULL)
    return;

  env = g_getenv("MYDOCSDIR");
  if (env && env[0])
    root = g_strdup(env);
  else
  {
    /* g_getenv uses $HOME as last resort. Normally it returns home 
        directory defined in passwd database. We want to use $HOME
        when possible. */
    env = g_getenv("HOME");
    if (env && env[0])
      root = g_build_path(G_DIR_SEPARATOR_S, env, LOCAL_DEVICE_ROOT, NULL);
    else
      root = g_build_path(G_DIR_SEPARATOR_S, g_get_home_dir(), LOCAL_DEVICE_ROOT, NULL);
  }

  locations[HILDON_FILE_SYSTEM_MODEL_LOCAL_DEVICE].path = root;
  locations[HILDON_FILE_SYSTEM_MODEL_SAFE_FOLDER_IMAGES].path = 
    g_build_path(G_DIR_SEPARATOR_S, root, IMAGES_SAFE, NULL);
  locations[HILDON_FILE_SYSTEM_MODEL_SAFE_FOLDER_VIDEOS].path = 
    g_build_path(G_DIR_SEPARATOR_S, root, VIDEOS_SAFE, NULL);
  locations[HILDON_FILE_SYSTEM_MODEL_SAFE_FOLDER_SOUNDS].path = 
    g_build_path(G_DIR_SEPARATOR_S, root, SOUNDS_SAFE, NULL);
  locations[HILDON_FILE_SYSTEM_MODEL_SAFE_FOLDER_DOCUMENTS].path = 
    g_build_path(G_DIR_SEPARATOR_S, root, DOCUMENTS_SAFE, NULL);
  locations[HILDON_FILE_SYSTEM_MODEL_SAFE_FOLDER_GAMES].path =
    g_build_path(G_DIR_SEPARATOR_S, root, DOCUMENTS_SAFE, GAMES_SAFE, NULL);

  env = g_getenv("MMC_MOUNTPOINT");
  if (env == NULL || env[0] == 0)
    env = "/media/mmc1";    /* Fallback location */

  locations[HILDON_FILE_SYSTEM_MODEL_MMC].path = g_strdup(env);
}

gint 
_hildon_file_system_get_special_location(GtkFileSystem *fs, const GtkFilePath *path)
{
  gchar *local_path;
  gint i, result = HILDON_FILE_SYSTEM_MODEL_UNKNOWN;

  local_path = gtk_file_system_path_to_filename(fs, path);
  if (local_path)
  {
    for (i = 3; i < G_N_ELEMENTS(locations); i++)
      if (locations[i].path && 
          _hildon_file_system_compare_ignore_last_separator(local_path, locations[i].path))
      {
        result = i;
        break;
      }

    g_free(local_path);  
  }

  return result;
}

GdkPixbuf *
_hildon_file_system_create_image(GtkFileSystem *fs, 
      GtkWidget *ref_widget, GtkFilePath *path, 
      HildonFileSystemModelItemType type, gint size)
{
    GdkPixbuf *pixbuf = NULL;

    if (ref_widget)
    {
      if (type <= HILDON_FILE_SYSTEM_MODEL_FOLDER)    
      {
        pixbuf = gtk_file_system_render_icon(fs, path, ref_widget, size, NULL);
      }
      else if (type == HILDON_FILE_SYSTEM_MODEL_GATEWAY)
      {
        GtkFileSystemVolume *vol;

        /* We do need the path. Every gateway device has different one */
        g_assert(path != NULL);

        vol = _hildon_file_system_get_volume_for_location(fs, 
            HILDON_FILE_SYSTEM_MODEL_GATEWAY, path);

        if (vol)
        {
          pixbuf = gtk_file_system_volume_render_icon(fs, vol, ref_widget, size, NULL);
          gtk_file_system_volume_free(fs, vol);
        }
      }  

      if (!pixbuf)
          pixbuf = gtk_icon_theme_load_icon(gtk_icon_theme_get_default(), 
                        locations[type].icon_name, size, 0, NULL);
    }

    return pixbuf;
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
  const GtkFilePath *path, HildonFileSystemModelItemType type,
  GtkFileInfo *info)
{
  gchar *str;

  if (type == HILDON_FILE_SYSTEM_MODEL_GATEWAY ||
      type == HILDON_FILE_SYSTEM_MODEL_MMC)
  {
    GtkFileSystemVolume *vol;

    vol = _hildon_file_system_get_volume_for_location(fs, type, path);

    if (vol)
    {
      str = gtk_file_system_volume_get_display_name(fs, vol);
      gtk_file_system_volume_free(fs, vol);
      if (str) return str;
    }
  }  
  else if (type == HILDON_FILE_SYSTEM_MODEL_LOCAL_DEVICE)
  {
    HildonFileSystemSettings *settings;
    gchar *name;
    
    settings = _hildon_file_system_settings_get_instance();
    g_object_get(settings, "btname", &name, NULL);

    if (name)
    {
      if (name[0]) return name;
      g_free(name);
    }
  }

  if (type > HILDON_FILE_SYSTEM_MODEL_FOLDER)
  {
    const gchar *name;
    g_return_val_if_fail(locations[type].display_name, NULL);
    name = _(locations[type].display_name);
    g_return_val_if_fail(name, NULL);

    return g_strdup(name);
  }

  if (info)  
  {
    const gchar *name;
    g_return_val_if_fail(info, NULL);
    name = gtk_file_info_get_display_name(info);
    g_return_val_if_fail(name, NULL);

    return g_strdup(name);
  }

  return g_strdup(get_custom_root_name(path));
}

gchar *
_hildon_file_system_create_display_name(GtkFileSystem *fs, 
  const GtkFilePath *path, HildonFileSystemModelItemType type, 
  GtkFileInfo *info)
{
  gchar *str, *dot;
  const gchar *mime_type = NULL;

  str = _hildon_file_system_create_file_name(fs, path, type, info);   

  if (info)
    mime_type = gtk_file_info_get_mime_type(info);

  if (mime_type && type < HILDON_FILE_SYSTEM_MODEL_FOLDER &&
    g_ascii_strcasecmp(mime_type, "application/octet-stream") != 0)
  {  /* Unrecognized types are not touched */
     dot = g_strrstr(str, ".");
     if (dot && dot != str)
       *dot = 0;
  }
  return str;
}

GtkFilePath *_hildon_file_system_path_for_location(GtkFileSystem *fs, 
  HildonFileSystemModelItemType type)
{
  g_assert(0 <= type && type < G_N_ELEMENTS(locations));
  return gtk_file_system_filename_to_path(fs, locations[type].path);
}

/* You can omit either type or base */
GtkFileSystemVolume *
_hildon_file_system_get_volume_for_location(GtkFileSystem *fs, 
  HildonFileSystemModelItemType type,
  const GtkFilePath *base)
{
    GSList *volumes, *iter;
    GtkFilePath *mount_path, *path;
    GtkFileSystemVolume *vol, *result = NULL;
    const char *path_a, *path_b;

    /* We cannot just use get_volume_for_path, because it won't
        work work with URIs other than file:// */
    volumes = gtk_file_system_list_volumes(fs);

    mount_path = base ? gtk_file_path_copy(base) : 
        _hildon_file_system_path_for_location(fs, type);
    if (mount_path == NULL) return NULL;

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
        result = _gtk_file_system_create(name);
        if (!GTK_IS_FILE_SYSTEM(result))
            ULOG_WARN("Cannot create \"%s\" backend", name);
    }
    if (use_fallback && !GTK_IS_FILE_SYSTEM(result))
        result = gtk_file_system_unix_new();

    g_free(default_name);
    return result;
}
