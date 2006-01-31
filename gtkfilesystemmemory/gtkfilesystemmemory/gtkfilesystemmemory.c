/* gtkfilesystemmemory.c
 *
 * Copyright (C) 2005 Nokia Corporation.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 *
 * TODO - Before the converting functions were public there was only
 *        conversions from GtkTreePath to GtkFilePath.
 *        Most probably there is cases where to use straight conversion to
 *        GtkTreeIter or from it. This modification does not make any radical
 *        changes, it just optimizes a bit. 
 */

#include <gtk/gtktreestore.h>

#define GTK_FILE_SYSTEM_ENABLE_UNSUPPORTED
#undef __GNUC__ /* This is needed because compile option -pedantic
                   disables GNU extensions and code don't detect this */

#include <gtk/gtkfilesystem.h>

#include "gtkfilesystemmemoryprivate.h"
#include "../gtkfilesystemmemorypublic/gtkfilesystemmemory.h"

#include <time.h>

#include <errno.h>
#include <math.h>                            
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define MAX_PATH_LENGHT 256


typedef struct _GtkFileSystemMemoryClass GtkFileSystemMemoryClass;


#define GTK_FILE_SYSTEM_MEMORY_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GTK_TYPE_FILE_SYSTEM_MEMORY, GtkFileSystemMemoryClass))
#define GTK_IS_FILE_SYSTEM_MEMORY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTK_TYPE_FILE_SYSTEM_MEMORY))
#define GTK_FILE_SYSTEM_MEMORY_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GTK_TYPE_FILE_SYSTEM_MEMORY, GtkFileSystemMemoryClass))

static GType type_gtk_file_system_memory = 0;
static GType type_gtk_file_folder_memory = 0;


struct _GtkFileSystemMemoryClass
{
  GtkTreeStoreClass parent_class;
};


/*#define PRINT_FUNCTION_NAME 1*/
/* Set SOME_TESTING_STUFF is you want to use gtkfilechooserdialog.  */
/*#define SOME_TESTING_STUFF 1 */

#if PRINT_FUNCTION_NAME
  #define DTEXT(str) fprintf( stderr, str )
#else
  #define DTEXT(str)
#endif

#define GTK_TYPE_FILE_FOLDER_MEMORY (gtk_file_folder_memory_get_type())
#define GTK_FILE_FOLDER_MEMORY(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_TYPE_FILE_FOLDER_MEMORY, GtkFileFolderMemory))
#define GTK_IS_FILE_FOLDER_MEMORY(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_TYPE_FILE_FOLDER_MEMORY))
#define GTK_FILE_FOLDER_MEMORY_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GTK_TYPE_FILE_FOLDER_MEMORY, GtkFileFolderMemoryClass))
#define GTK_IS_FILE_FOLDER_MEMORY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTK_TYPE_FILE_FOLDER_MEMORY))
#define GTK_FILE_FOLDER_MEMORY_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GTK_TYPE_FILE_FOLDER_MEMORY, GtkFileFolderMemoryClass))

typedef struct _GtkFileFolderMemory GtkFileFolderMemory;
typedef struct _GtkFileFolderMemoryClass GtkFileFolderMemoryClass;

struct _GtkFileFolderMemoryClass
{
  GObjectClass parent_class;
};

struct _GtkFileFolderMemory
{
  GObject parent_instance;
  
  GtkTreeModel *model;
  GtkTreeRowReference *data;
  gboolean appended;
};



static GtkTreeStore *system_parent_class = NULL;
static GObjectClass *folder_parent_class = NULL;


static void
gtk_file_system_memory_class_init( GtkFileSystemMemoryClass *class );
static void
gtk_file_system_memory_iface_init( GtkFileSystemIface *iface );
static void
gtk_file_system_memory_init( GtkFileSystemMemory *impl );
static void
gtk_file_system_memory_finalize( GObject *object );




static GSList 
*gtk_file_system_memory_list_volumes( GtkFileSystem  *file_system );
static GtkFileSystemVolume *
gtk_file_system_memory_get_volume_for_path( GtkFileSystem *file_system,
									                          const GtkFilePath *path );
static GtkFileFolder *
gtk_file_system_memory_get_folder( GtkFileSystem *file_system,
							                     const GtkFilePath *path, GtkFileInfoType
                                   types, GError **error );
static gboolean
gtk_file_system_memory_create_folder( GtkFileSystem *file_system,
							                        const GtkFilePath *path,
                                      GError **error );
static void
gtk_file_system_memory_volume_free( GtkFileSystem  *file_system,
						                        GtkFileSystemVolume *volume );
static GtkFilePath *
gtk_file_system_memory_volume_get_base_path( GtkFileSystem *file_system,
								                             GtkFileSystemVolume *volume );
static gboolean
gtk_file_system_memory_volume_get_is_mounted( GtkFileSystem *file_system,
								                              GtkFileSystemVolume *volume );
static gboolean
gtk_file_system_memory_volume_mount( GtkFileSystem *file_system,
								                     GtkFileSystemVolume *volume,
                                     GError **error );
static gchar *
gtk_file_system_memory_volume_get_display_name( GtkFileSystem *file_system,
								                                GtkFileSystemVolume *volume );
static GdkPixbuf *
gtk_file_system_memory_volume_render_icon( GtkFileSystem *file_system,
								                           GtkFileSystemVolume *volume,
                                           GtkWidget *widget, gint pixel_size, 
                                           GError **error );
static gboolean
gtk_file_system_memory_get_parent( GtkFileSystem *file_system,
							                     const GtkFilePath *path,
                                   GtkFilePath **parent, GError **error );
static GtkFilePath *
gtk_file_system_memory_make_path( GtkFileSystem *file_system,
							                    const GtkFilePath *base_path,
                                  const gchar *display_name, GError **error );
static gboolean
gtk_file_system_memory_parse( GtkFileSystem *file_system,
							                const GtkFilePath *base_path, const gchar *str,
                              GtkFilePath **folder, gchar **file_part,
                              GError **error );
static gchar *
gtk_file_system_memory_path_to_uri( GtkFileSystem *file_system,
								                    const GtkFilePath *path );
static gchar *
gtk_file_system_memory_path_to_filename( GtkFileSystem *file_system,
								                         const GtkFilePath *path );
static GtkFilePath *
gtk_file_system_memory_uri_to_path( GtkFileSystem *file_system,
								                    const gchar *uri );
static GtkFilePath *
gtk_file_system_memory_filename_to_path( GtkFileSystem *file_system,
								                         const gchar *filename );
static GdkPixbuf *
gtk_file_system_memory_render_icon( GtkFileSystem *file_system,
							                      const GtkFilePath *path, GtkWidget *widget,
                                    gint pixel_size, GError **error );
static gboolean
gtk_file_system_memory_insert_bookmark( GtkFileSystem *file_system,
							                          const GtkFilePath *path, gint position,
                                        GError **error );
static gboolean
gtk_file_system_memory_remove_bookmark( GtkFileSystem *file_system,
							                          const GtkFilePath *path,
                                        GError **error );
static GSList *
gtk_file_system_memory_list_bookmarks( GtkFileSystem *file_system );




static GType
gtk_file_folder_memory_get_type( void );
static void
gtk_file_folder_memory_class_init( GtkFileFolderMemoryClass *class );
static void
gtk_file_folder_memory_iface_init( GtkFileFolderIface *iface );
static void
gtk_file_folder_memory_init( GtkFileFolderMemory *impl );
static void
gtk_file_folder_memory_finalize( GObject *object );




static GtkFileInfo *
gtk_file_folder_memory_get_info( GtkFileFolder *folder, const GtkFilePath *path,
                                 GError **error );
static gboolean
gtk_file_folder_memory_list_children( GtkFileFolder *folder, GSList **children,
                                      GError **error );
static gboolean
gtk_file_folder_memory_is_finished_loading( GtkFileFolder *folder );




void signal_row_inserted( GtkTreeModel *treemodel, GtkTreePath *path,
                          GtkTreeIter *iter, gpointer data);

void signal_row_changed( GtkTreeModel *treemodel, GtkTreePath *path,
                          GtkTreeIter *iter, gpointer data);

void signal_row_deleted( GtkTreeModel *treemodel, GtkTreePath *path,
                         gpointer data);

                         
void
signal_row_reference_inserted( GtkTreeModel *treemodel, GtkTreePath *path,
                               GtkTreeIter *iter, GtkFileFolderMemory *ffm );
void
signal_row_reference_changed( GtkTreeModel *treemodel, GtkTreePath *path,
                               GtkTreeIter *iter, GtkFileFolderMemory *ffm );
void
signal_row_reference_deleted( GtkTreeModel *treemodel, GtkTreePath *path,
                               GtkFileFolderMemory *ffm );

void
fs_module_init( GTypeModule *module );
void
fs_module_exit( void );
GtkFileSystem *
fs_module_create( void );


void
fs_module_init( GTypeModule *module )
{
  {
    static const GTypeInfo file_system_memory_info =
    {
      sizeof (GtkFileSystemMemoryClass),
      NULL,		/* base_init */
      NULL,		/* base_finalize */
      (GClassInitFunc) gtk_file_system_memory_class_init,
      NULL,		/* class_finalize */
      NULL,		/* class_data */
      sizeof (GtkFileSystemMemory),
      0,		/* n_preallocs */
      (GInstanceInitFunc) gtk_file_system_memory_init,
    };

    static const GInterfaceInfo file_system_info =
    { /* interface_init */
      (GInterfaceInitFunc) gtk_file_system_memory_iface_init, 
      NULL,			                              /* interface_finalize */
      NULL			                              /* interface_data */
    };

    type_gtk_file_system_memory = g_type_module_register_type( module,
				                             GTK_TYPE_TREE_STORE, "GtkFileSystemMemory",
                                     &file_system_memory_info, 0 );

    g_type_module_add_interface( module, GTK_TYPE_FILE_SYSTEM_MEMORY,
				                         GTK_TYPE_FILE_SYSTEM, &file_system_info );
  }

  {
    static const GTypeInfo file_folder_memory_info =
    {
      sizeof (GtkFileFolderMemoryClass),
      NULL,		/* base_init */
      NULL,		/* base_finalize */
      (GClassInitFunc) gtk_file_folder_memory_class_init,
      NULL,		/* class_finalize */
      NULL,		/* class_data */
      sizeof (GtkFileFolderMemory),
      0,		/* n_preallocs */
      (GInstanceInitFunc) gtk_file_folder_memory_init,
    };
    
    static const GInterfaceInfo file_folder_info =
    { /* interface_init */
      (GInterfaceInitFunc) gtk_file_folder_memory_iface_init, 
      NULL,			                              /* interface_finalize */
      NULL			                              /* interface_data */
    };

    type_gtk_file_folder_memory = g_type_module_register_type( module,
		                  						  G_TYPE_OBJECT, "GtkFileFolderMemory",
                                    &file_folder_memory_info, 0 );

    g_type_module_add_interface( module, type_gtk_file_folder_memory,
				                         GTK_TYPE_FILE_FOLDER, &file_folder_info );
  }
}


void 
fs_module_exit( void )
{
}


GtkFileSystem *     
fs_module_create( void )
{
  return gtk_file_system_memory_new();
}




/*Copy from the glib*/
static int
unescape_character (const char *scanner)
{
  int first_digit;
  int second_digit;

  DTEXT( ":"__FUNCTION__"\n" );

  first_digit = g_ascii_xdigit_value (scanner[0]);
  if (first_digit < 0) 
    return -1;
  
  second_digit = g_ascii_xdigit_value (scanner[1]);
  if (second_digit < 0) 
    return -1;
  
  return (first_digit << 4) | second_digit;
}

/*Almost identical copy from the glib*/
static gchar *
g_unescape_uri_string (const char *escaped)
{
  const gchar *in, *in_end;
  gchar *out, *result;
  int c, len;

  DTEXT( ":"__FUNCTION__"\n" );

  if (escaped == NULL)
    return NULL;

  len = strlen (escaped);

  result = g_malloc (len + 1);
  
  out = result;
  for (in = escaped, in_end = escaped + len; in < in_end; in++)
    {
      c = *in;

      if (c == '%')
	{
	  /* catch partial escape sequences past the end of the substring */
	  if (in + 3 > in_end)
	    break;

	  c = unescape_character (in + 1);

	  /* catch bad escape sequences and NUL characters */
	  if (c <= 0)
	    break;

          /* catch escaped ASCII. GLib has more complex checks on when
             escaping is needed for characters. We'll just dumb it down to
             alphanumeric. */
	  if (c <= 0x7F && isalnum(c))
	    break;

	  in += 2;
	}

      *out++ = c;
    }
  
  g_assert (out - result <= len);
  *out = '\0';

  if (in != in_end || !g_utf8_validate (result, -1, NULL))
    {
      g_free(result);
      return NULL;
    }

  return result;
}





GType
gtk_file_system_memory_get_type( void )
{
  return type_gtk_file_system_memory;
}

static GType
gtk_file_folder_memory_get_type( void )
{
  return type_gtk_file_folder_memory;
}

/**
 * gtk_file_system_memory_new:
 *
 * Creates a new #GtkFileSystemMemory object. #GtkFileSystemMemory
 * implements the #GtkFileSystem interface using the memory file system lib.
 *
 * Return value: the new #GtkFileSystemMemory object
 **/
GtkFileSystem *
gtk_file_system_memory_new( void )
{
  GtkFileSystemMemory *mfs;

  mfs = g_object_new( GTK_TYPE_FILE_SYSTEM_MEMORY, NULL );

  DTEXT( ":"__FUNCTION__"\n" );

  return GTK_FILE_SYSTEM( mfs );
}


static void
gtk_file_system_memory_class_init( GtkFileSystemMemoryClass *class )
{
  GObjectClass *gobject_class = G_OBJECT_CLASS( class );
  system_parent_class = g_type_class_peek_parent( class );

  DTEXT( ":"__FUNCTION__"\n" );

  gobject_class->finalize = gtk_file_system_memory_finalize;
}


static void
gtk_file_system_memory_iface_init( GtkFileSystemIface *iface )
{
  iface->list_volumes = gtk_file_system_memory_list_volumes;
  iface->get_volume_for_path = gtk_file_system_memory_get_volume_for_path;
  iface->get_folder = gtk_file_system_memory_get_folder;
  iface->create_folder = gtk_file_system_memory_create_folder;
  iface->volume_free = gtk_file_system_memory_volume_free;
  iface->volume_get_base_path = gtk_file_system_memory_volume_get_base_path;
  iface->volume_get_is_mounted = gtk_file_system_memory_volume_get_is_mounted;
  iface->volume_mount = gtk_file_system_memory_volume_mount;
  iface->volume_get_display_name = 
                        gtk_file_system_memory_volume_get_display_name;
  iface->volume_render_icon = gtk_file_system_memory_volume_render_icon;
  iface->get_parent = gtk_file_system_memory_get_parent;
  iface->make_path = gtk_file_system_memory_make_path;
  iface->parse = gtk_file_system_memory_parse;
  iface->path_to_uri = gtk_file_system_memory_path_to_uri;
  iface->path_to_filename = gtk_file_system_memory_path_to_filename;
  iface->uri_to_path = gtk_file_system_memory_uri_to_path;
  iface->filename_to_path = gtk_file_system_memory_filename_to_path;
  iface->render_icon = gtk_file_system_memory_render_icon;
  iface->insert_bookmark = gtk_file_system_memory_insert_bookmark;
  iface->remove_bookmark = gtk_file_system_memory_remove_bookmark;
  iface->list_bookmarks = gtk_file_system_memory_list_bookmarks;
}


static void
gtk_file_system_memory_init(GtkFileSystemMemory *mfs)
{
  GType types[7];

  types[0] = GDK_TYPE_PIXBUF;
  types[1] = G_TYPE_STRING;
  types[2] = G_TYPE_STRING;
  types[3] = G_TYPE_INT;
  types[4] = G_TYPE_INT;
  types[5] = G_TYPE_BOOLEAN;
  types[6] = G_TYPE_BOOLEAN;

  gtk_tree_store_set_column_types( GTK_TREE_STORE(mfs), G_N_ELEMENTS(types), types );

  mfs->bookmarks = NULL;
  mfs->file_paths_to_be_deleted = NULL;

  g_signal_connect( G_OBJECT(mfs), "row-inserted", 
                    G_CALLBACK(signal_row_inserted), NULL );
  g_signal_connect( G_OBJECT(mfs), "row-deleted",
                    G_CALLBACK(signal_row_deleted), NULL );
  g_signal_connect( G_OBJECT(mfs), "row-changed",
                    G_CALLBACK(signal_row_changed), NULL );

#ifdef SOME_TESTING_STUFF
{
  GtkTreeIter iter, parent, root;
  GtkTreeStore *store = GTK_TREE_STORE( mfs );

  gtk_tree_store_append( store, &iter, NULL );
  gtk_tree_store_set( store, &iter,
                      GTK_FILE_SYSTEM_MEMORY_COLUMN_ICON, NULL,
                      GTK_FILE_SYSTEM_MEMORY_COLUMN_NAME, "/",
                      GTK_FILE_SYSTEM_MEMORY_COLUMN_MIME, "x-directory/normal",
                      GTK_FILE_SYSTEM_MEMORY_COLUMN_MOD_TIME, time(NULL),
                      GTK_FILE_SYSTEM_MEMORY_COLUMN_SIZE, 0,
                      GTK_FILE_SYSTEM_MEMORY_COLUMN_IS_HIDDEN, FALSE,
                      GTK_FILE_SYSTEM_MEMORY_COLUMN_IS_FOLDER, TRUE, -1);
                                                  
  root = iter;

  gtk_tree_store_append( store, &iter, &root );
  gtk_tree_store_set( store, &iter,
                      GTK_FILE_SYSTEM_MEMORY_COLUMN_ICON, NULL,
                      GTK_FILE_SYSTEM_MEMORY_COLUMN_NAME, "home",
                      GTK_FILE_SYSTEM_MEMORY_COLUMN_MIME, "x-directory/normal",
                      GTK_FILE_SYSTEM_MEMORY_COLUMN_MOD_TIME, time(NULL),
                      GTK_FILE_SYSTEM_MEMORY_COLUMN_SIZE, 0,
                      GTK_FILE_SYSTEM_MEMORY_COLUMN_IS_HIDDEN, FALSE,
                      GTK_FILE_SYSTEM_MEMORY_COLUMN_IS_FOLDER, TRUE, -1);
  
  parent = iter;
  
  gtk_tree_store_append( store, &iter, &parent );
  gtk_tree_store_set( store, &iter,
                      GTK_FILE_SYSTEM_MEMORY_COLUMN_ICON, NULL,
                      GTK_FILE_SYSTEM_MEMORY_COLUMN_NAME, "rkivilah",
                      GTK_FILE_SYSTEM_MEMORY_COLUMN_MIME, "x-directory/normal",
                      GTK_FILE_SYSTEM_MEMORY_COLUMN_MOD_TIME, time(NULL),
                      GTK_FILE_SYSTEM_MEMORY_COLUMN_SIZE, 0,
                      GTK_FILE_SYSTEM_MEMORY_COLUMN_IS_HIDDEN, FALSE,
                      GTK_FILE_SYSTEM_MEMORY_COLUMN_IS_FOLDER, TRUE, -1);
  
  parent = iter;
  
  gtk_tree_store_append( store, &iter, &parent );
  gtk_tree_store_set( store, &iter,
                      GTK_FILE_SYSTEM_MEMORY_COLUMN_ICON, NULL,
                      GTK_FILE_SYSTEM_MEMORY_COLUMN_NAME, "File.txt",
                      GTK_FILE_SYSTEM_MEMORY_COLUMN_MIME,
                      "text/x-tex",
                      GTK_FILE_SYSTEM_MEMORY_COLUMN_MOD_TIME, time(NULL),
                      GTK_FILE_SYSTEM_MEMORY_COLUMN_SIZE, 0,
                      GTK_FILE_SYSTEM_MEMORY_COLUMN_IS_HIDDEN, FALSE,
                      GTK_FILE_SYSTEM_MEMORY_COLUMN_IS_FOLDER, FALSE, -1);
  
  gtk_tree_store_append( store, &iter, &parent );
  gtk_tree_store_set( store, &iter,
                      GTK_FILE_SYSTEM_MEMORY_COLUMN_ICON, NULL,
                      GTK_FILE_SYSTEM_MEMORY_COLUMN_NAME, "Desktop",
                      GTK_FILE_SYSTEM_MEMORY_COLUMN_MIME, "x-directory/normal",
                      GTK_FILE_SYSTEM_MEMORY_COLUMN_MOD_TIME, time(NULL),
                      GTK_FILE_SYSTEM_MEMORY_COLUMN_SIZE, 0,
                      GTK_FILE_SYSTEM_MEMORY_COLUMN_IS_HIDDEN, FALSE,
                      GTK_FILE_SYSTEM_MEMORY_COLUMN_IS_FOLDER, TRUE, -1);
                      
  gtk_tree_store_append( store, &iter, &root );
  gtk_tree_store_set( store, &iter,
                      GTK_FILE_SYSTEM_MEMORY_COLUMN_ICON, NULL,
                      GTK_FILE_SYSTEM_MEMORY_COLUMN_NAME, "work",
                      GTK_FILE_SYSTEM_MEMORY_COLUMN_MIME, "x-directory/normal",
                      GTK_FILE_SYSTEM_MEMORY_COLUMN_MOD_TIME, time(NULL),
                      GTK_FILE_SYSTEM_MEMORY_COLUMN_SIZE, 0,
                      GTK_FILE_SYSTEM_MEMORY_COLUMN_IS_HIDDEN, FALSE,
                      GTK_FILE_SYSTEM_MEMORY_COLUMN_IS_FOLDER, TRUE, -1);
                                                  
  parent = iter;

  gtk_tree_store_append( store, &iter, &parent );
  gtk_tree_store_set( store, &iter,
                      GTK_FILE_SYSTEM_MEMORY_COLUMN_ICON, NULL,
                      GTK_FILE_SYSTEM_MEMORY_COLUMN_NAME, "teema4",
                      GTK_FILE_SYSTEM_MEMORY_COLUMN_MIME, "x-directory/normal",
                      GTK_FILE_SYSTEM_MEMORY_COLUMN_MOD_TIME, time(NULL),
                      GTK_FILE_SYSTEM_MEMORY_COLUMN_SIZE, 0,
                      GTK_FILE_SYSTEM_MEMORY_COLUMN_IS_HIDDEN, FALSE,
                      GTK_FILE_SYSTEM_MEMORY_COLUMN_IS_FOLDER, TRUE, -1);
  
  parent = iter;
  
  gtk_tree_store_append( store, &iter, &parent );
  gtk_tree_store_set( store, &iter,
                      GTK_FILE_SYSTEM_MEMORY_COLUMN_ICON, NULL,
                      GTK_FILE_SYSTEM_MEMORY_COLUMN_NAME, "gtkfilesystemmemory",
                      GTK_FILE_SYSTEM_MEMORY_COLUMN_MIME, "x-directory/normal",
                      GTK_FILE_SYSTEM_MEMORY_COLUMN_MOD_TIME, time(NULL),
                      GTK_FILE_SYSTEM_MEMORY_COLUMN_SIZE, 0,
                      GTK_FILE_SYSTEM_MEMORY_COLUMN_IS_HIDDEN, FALSE,
                      GTK_FILE_SYSTEM_MEMORY_COLUMN_IS_FOLDER, TRUE, -1);
  
  parent = iter;
  
  gtk_tree_store_append( store, &iter, &parent );
  gtk_tree_store_set( store, &iter,
                      GTK_FILE_SYSTEM_MEMORY_COLUMN_ICON, NULL,
                      GTK_FILE_SYSTEM_MEMORY_COLUMN_NAME, "fstest",
                      GTK_FILE_SYSTEM_MEMORY_COLUMN_MIME,
                      "application/octet-stream",
                      GTK_FILE_SYSTEM_MEMORY_COLUMN_MOD_TIME, time(NULL),
                      GTK_FILE_SYSTEM_MEMORY_COLUMN_SIZE, 0,
                      GTK_FILE_SYSTEM_MEMORY_COLUMN_IS_HIDDEN, FALSE,
                      GTK_FILE_SYSTEM_MEMORY_COLUMN_IS_FOLDER, FALSE, -1);
}
  #endif

  DTEXT( ":"__FUNCTION__"\n" );

}


static void
gtk_file_system_memory_finalize(GObject *object)
{
  GtkFileSystemMemory *mfs = GTK_FILE_SYSTEM_MEMORY( object );
  GObjectClass *object_class = G_OBJECT_CLASS(system_parent_class); 

  DTEXT( ":"__FUNCTION__"\n" );

  g_slist_free(mfs->bookmarks);
  object_class->finalize( object );
}


void signal_row_inserted( GtkTreeModel *treemodel, GtkTreePath *path,
                          GtkTreeIter *iter, gpointer data)
{
  DTEXT( ":"__FUNCTION__"\n" );
}

void signal_row_changed( GtkTreeModel *treemodel, GtkTreePath *path,
                          GtkTreeIter *iter, gpointer data)
{
  DTEXT( ":"__FUNCTION__"\n" );

  if( gtk_tree_path_get_depth( path ) == 1 )
    g_signal_emit_by_name( GTK_FILE_SYSTEM(treemodel), "volumes-changed" );
}

void signal_row_deleted( GtkTreeModel *treemodel, GtkTreePath *path,
                         gpointer data)
{
  DTEXT( ":"__FUNCTION__"\n" );
}


void
signal_row_reference_inserted( GtkTreeModel *treemodel, GtkTreePath *path,
                               GtkTreeIter *iter, GtkFileFolderMemory *ffm )
{
  DTEXT( ":"__FUNCTION__"\n" );
  ffm->appended = TRUE;
}

void
signal_row_reference_changed( GtkTreeModel *treemodel, GtkTreePath *path,
                              GtkTreeIter *iter, GtkFileFolderMemory *ffm )
{
  GtkTreePath *folder_path = NULL;
  GtkTreeRowReference *row = ffm->data;

  DTEXT( ":"__FUNCTION__"\n" );

  if( !gtk_tree_row_reference_valid(row) )
    return;

  folder_path = gtk_tree_row_reference_get_path( row );
  
  if( gtk_tree_path_is_ancestor( folder_path, path ) && 
    (gtk_tree_path_get_depth(path) == gtk_tree_path_get_depth(folder_path) + 1))
  {
    GSList *paths = NULL;
    paths = g_slist_append( paths, 
                          (gchar*)gtk_file_system_memory_tree_path_to_file_path( 
                          GTK_FILE_SYSTEM(treemodel), path ) );
    if( ffm->appended )
    {
      g_signal_emit_by_name(ffm, "files-added", paths);
      ffm->appended = FALSE;
    }
    else
      g_signal_emit_by_name( ffm, "files-changed", paths );

    g_slist_free( paths );
  }
}

void
signal_row_reference_deleted( GtkTreeModel *treemodel, GtkTreePath *path,
                              GtkFileFolderMemory *ffm )
{
  GtkTreePath *folder_path = NULL;
  GtkTreePath *row_path = NULL;
  GtkTreeRowReference *row = ffm->data;
  GtkFileSystemMemory *fsm = GTK_FILE_SYSTEM_MEMORY(treemodel);

  DTEXT( ":"__FUNCTION__"\n" );

  if( !fsm->parent_path || !fsm->file_paths_to_be_deleted )
    return;

  folder_path = fsm->parent_path;
  row_path = gtk_tree_row_reference_get_path( row );

  if( !gtk_tree_path_compare(folder_path, row_path) )
  {
    GSList *paths = fsm->file_paths_to_be_deleted;
    g_signal_emit_by_name( ffm, "files-removed", paths );
    g_slist_free( paths );
    fsm->file_paths_to_be_deleted = NULL;
    gtk_tree_path_free( folder_path );
    fsm->parent_path = NULL;
  }
}


static void
gtk_file_folder_memory_init( GtkFileFolderMemory *impl )
{
  impl->model = NULL;
  impl->data = NULL;
}

static void
gtk_file_folder_memory_class_init( GtkFileFolderMemoryClass *class )
{
  GObjectClass *gobject_class = G_OBJECT_CLASS( class );

  folder_parent_class = g_type_class_peek_parent( class );

  gobject_class->finalize = gtk_file_folder_memory_finalize;
}

static void
gtk_file_folder_memory_iface_init( GtkFileFolderIface *iface )
{
  iface->get_info = gtk_file_folder_memory_get_info;
  iface->list_children = gtk_file_folder_memory_list_children;
  iface->is_finished_loading = gtk_file_folder_memory_is_finished_loading;
}

static void
gtk_file_folder_memory_finalize( GObject *object )
{
  GtkTreePath *path = NULL;
  GtkFileFolderMemory *folder = GTK_FILE_FOLDER_MEMORY( object );

  DTEXT( ":"__FUNCTION__"\n" );

  path = gtk_tree_row_reference_get_path( folder->data );
  gtk_tree_path_free( path );
  gtk_tree_row_reference_free( folder->data );
}




static GtkFileInfo *
gtk_file_folder_memory_get_info( GtkFileFolder *folder, const GtkFilePath *path,
                                 GError **error )
{
  GtkTreeIter iter;
  gboolean is_hidden = FALSE;
  gboolean is_folder = FALSE;
  gchar *name = NULL;
  gint64 mod_time = 0;
  gint64 size = 0;
  gchar *mime = NULL;
  GtkTreePath *tree_path = NULL;
  GtkFileInfo *info = NULL;
  GtkFileFolderMemory *ffm = GTK_FILE_FOLDER_MEMORY(folder);

  DTEXT( ":"__FUNCTION__"\n" );

  if( path == NULL )
  {
    tree_path = gtk_tree_row_reference_get_path(ffm->data);

    if( gtk_tree_path_get_depth(tree_path) > 1 )
    {
      g_set_error( error, GTK_FILE_SYSTEM_ERROR, 
                  GTK_FILE_SYSTEM_ERROR_NONEXISTENT,
                  "Path %s is not valid", (gchar*)path );
      gtk_tree_path_free( tree_path );
      return NULL;
    }
  }
  else if( !(tree_path =
             gtk_file_system_memory_file_path_to_tree_path(
             GTK_FILE_SYSTEM(ffm->model), path)) )
  {
    g_set_error( error, GTK_FILE_SYSTEM_ERROR, 
                 GTK_FILE_SYSTEM_ERROR_NONEXISTENT,
                 "Path %s is not valid", (gchar*)path );
    return NULL;
  }

  gtk_tree_model_get_iter( ffm->model, &iter, tree_path );
  
  gtk_tree_model_get( ffm->model, &iter,
                      GTK_FILE_SYSTEM_MEMORY_COLUMN_IS_FOLDER, &is_folder,
                      GTK_FILE_SYSTEM_MEMORY_COLUMN_IS_HIDDEN, &is_hidden,
                      GTK_FILE_SYSTEM_MEMORY_COLUMN_NAME, &name,
                      GTK_FILE_SYSTEM_MEMORY_COLUMN_MOD_TIME, &mod_time,
                      GTK_FILE_SYSTEM_MEMORY_COLUMN_MIME, &mime,
                      GTK_FILE_SYSTEM_MEMORY_COLUMN_SIZE, &size, -1 );

  info = gtk_file_info_new();
  gtk_file_info_set_display_name( info, name );
  gtk_file_info_set_is_folder( info, is_folder );
  gtk_file_info_set_is_hidden( info, is_hidden );
  gtk_file_info_set_mime_type( info, "x-directory/normal" );
  gtk_file_info_set_modification_time( info, mod_time );
  gtk_file_info_set_size( info, size );
  gtk_file_info_get_display_key( info );
  
  g_free( name );
  g_free( mime );
  gtk_tree_path_free( tree_path );

  return info;
}

static gboolean
gtk_file_folder_memory_list_children( GtkFileFolder *folder, GSList **children,
					                            GError **error )
{
  GtkTreeIter iter;
  GSList *list = NULL;
  GtkTreePath *path = NULL;
  GtkTreeModel *model = NULL;
  gboolean is_folder = FALSE;
  GtkFileFolderMemory *ffm = GTK_FILE_FOLDER_MEMORY(folder);
  GtkTreeRowReference *row = ffm->data;

  DTEXT( ":"__FUNCTION__"\n" );

  *children = NULL;

  path = gtk_tree_row_reference_get_path( row );
  if( !path )
  {
    g_set_error( error, GTK_FILE_SYSTEM_ERROR, 
                 GTK_FILE_SYSTEM_ERROR_NONEXISTENT,
                 "Path %s is not a valid directory", (gchar*)path );
    return FALSE;
  }
  
  model = ffm->model;
  
  gtk_tree_model_get_iter( model, &iter, path );
  gtk_tree_model_get( model, &iter, GTK_FILE_SYSTEM_MEMORY_COLUMN_IS_FOLDER,
                      &is_folder, -1 );

  if( !is_folder )
  {
    g_set_error( error, GTK_FILE_SYSTEM_ERROR, 
                 GTK_FILE_SYSTEM_ERROR_NOT_FOLDER,
                 "%s is not a folder", (gchar*)path );
    gtk_tree_path_free( path );
    return FALSE;
  }
  else if( !gtk_tree_model_iter_has_child(model, &iter) )
  {
    gtk_tree_path_free( path );
    return TRUE;
  }

  gtk_tree_path_down( path );
  gtk_tree_model_get_iter( model, &iter, path );
    
  do
  {
    list = g_slist_append( list, gtk_file_system_memory_tree_path_to_file_path( 
              GTK_FILE_SYSTEM(ffm->model), path) );
    gtk_tree_path_next(path);
  } while( gtk_tree_model_get_iter( model, &iter, path ) );
  gtk_tree_path_free( path );
  *children = list;
  return TRUE;
}

static gboolean
gtk_file_folder_memory_is_finished_loading( GtkFileFolder *folder )
{
  DTEXT( ":"__FUNCTION__"\n" );
  return TRUE;
}




static GSList *
gtk_file_system_memory_list_volumes( GtkFileSystem *file_system )
{
  GSList *list = NULL;
  GtkTreeIter iter;
  GtkTreePath *path = NULL;
  GtkTreeRowReference *row = NULL;
  GtkTreeModel *model = GTK_TREE_MODEL( file_system );

  DTEXT( ":"__FUNCTION__"\n" );

  if( !gtk_tree_model_get_iter_first( model, &iter ) )
    return NULL;

  do
  {
    path = gtk_tree_model_get_path( model, &iter );
    row = gtk_tree_row_reference_new( model, path );
    list = g_slist_append( list, row );
    
  } while( gtk_tree_model_iter_next( model, &iter ) );

  return list;
}


static GtkFileSystemVolume *
gtk_file_system_memory_get_volume_for_path( GtkFileSystem *file_system,
					                                  const GtkFilePath *path )
{
  GtkTreeModel *model = NULL;
  GtkTreePath *tree_path = NULL;
  GtkTreeRowReference *row = NULL;
  GtkTreeIter iter;
  gchar **first = NULL;
  gchar *str = NULL;

  DTEXT( ":"__FUNCTION__"\n" );

  g_return_val_if_fail( path, NULL );
  
  model = GTK_TREE_MODEL(file_system);
  
  if( !gtk_tree_model_get_iter_first( model, &iter ) )
    return NULL;
  
  first = g_strsplit( (gchar*)path, "/", 1 );
  
  if( ((gchar*)path)[0] == '/' )
    do
    {
      gtk_tree_model_get( model, &iter, GTK_FILE_SYSTEM_MEMORY_COLUMN_NAME, 
                          &str, -1 );
      if( !strcmp( first[0], str ) )
      {
        g_free(str);
        break;
      }

      g_free(str);
      
    } while( gtk_tree_model_iter_next( model, &iter) );
  
  g_free(first[0]);
  g_free(first[1]);
  g_free(first);
  
  tree_path = gtk_tree_model_get_path( model, &iter );
  row = gtk_tree_row_reference_new( model, tree_path );
  gtk_tree_path_free( tree_path );
  
  return (GtkFileSystemVolume*)row;
}

/* FIXME: Some kind of cache (=hashmap) for created folders would be nice */
static GtkFileFolder *
gtk_file_system_memory_get_folder( GtkFileSystem *file_system,
				                           const GtkFilePath *path,
                                   GtkFileInfoType types, GError **error )
{
  GtkFileFolderMemory *ffm;
  GtkTreeModel *model;
  GtkTreePath *tree_path;

  DTEXT( ":"__FUNCTION__"\n" );

  if( !(tree_path = gtk_file_system_memory_file_path_to_tree_path( file_system,
                                                                   path )) )
  {
    g_set_error( error, GTK_FILE_SYSTEM_ERROR, 
                 GTK_FILE_SYSTEM_ERROR_NOT_FOLDER,
                 "Path %s is not valid", (gchar*)path );
    return NULL;
  }

  model = GTK_TREE_MODEL(file_system);
  
  ffm = g_object_new( GTK_TYPE_FILE_FOLDER_MEMORY, NULL );
  ffm->data = gtk_tree_row_reference_new( model, tree_path );
  ffm->model = model;

  g_signal_connect_object( G_OBJECT(model), "row-changed",
                    G_CALLBACK(signal_row_reference_changed), ffm, 0 );
  g_signal_connect_object( G_OBJECT(model), "row-inserted",
                    G_CALLBACK(signal_row_reference_inserted), ffm, 0 );
  g_signal_connect_object( G_OBJECT(model), "row-deleted",
                    G_CALLBACK(signal_row_reference_deleted), ffm, 0 );

  gtk_tree_path_free( tree_path );
                    
  return GTK_FILE_FOLDER(ffm);
}


static gboolean
gtk_file_system_memory_create_folder( GtkFileSystem *file_system,
					                            const GtkFilePath *path, GError **error)
{
  gint path_len = 0, slash = 0, len = 0;
  gchar *str = NULL, *current = NULL, *to_path = NULL, *new_folder = NULL;
  GtkTreePath *tree_path = NULL;
  GtkTreeStore *store;
  GtkTreeIter iter, ipath;

  DTEXT( ":"__FUNCTION__"\n" );

  if( (tree_path = gtk_file_system_memory_file_path_to_tree_path( file_system, 
                                                                  path )) )
  {
    g_set_error( error, GTK_FILE_SYSTEM_ERROR, 
                 GTK_FILE_SYSTEM_ERROR_ALREADY_EXISTS,
                 "Folder %s already exists", (gchar*)path );
    return FALSE;
  }
  
  str = (gchar*)path;
  path_len = strlen(str);
  current = &str[path_len-1];

  if( *(current) == '/' )
  {
    current--;
    slash = 1;
  }

  while( *(current) != '/' )
    current--;
  current++;
  
  to_path = g_strndup( current, path_len - (current - str) - slash );
  new_folder = to_path;
  len = current - str - slash;

  if( len )
    to_path = g_strndup( str, len );
  else
    to_path = g_strndup( str, 1 );

  gtk_tree_path_free( tree_path );
  if( !(tree_path = gtk_file_system_memory_file_path_to_tree_path(file_system,
       (GtkFilePath*)to_path)) )
  {
    g_set_error( error, GTK_FILE_SYSTEM_ERROR, 
                 GTK_FILE_SYSTEM_ERROR_BAD_FILENAME,
                 "Path to %s is not valid", (gchar*)path );
    g_free( new_folder );
    g_free( to_path );             
    return FALSE;
  }

  store = GTK_TREE_STORE( file_system );
  
  gtk_tree_model_get_iter( GTK_TREE_MODEL(store), &ipath, tree_path );
  gtk_tree_store_append( store, &iter, &ipath );
  gtk_tree_store_set( store, &iter,
                      GTK_FILE_SYSTEM_MEMORY_COLUMN_ICON, NULL,
                      GTK_FILE_SYSTEM_MEMORY_COLUMN_NAME, new_folder,
                      GTK_FILE_SYSTEM_MEMORY_COLUMN_MIME, "x-directory/normal",
                      GTK_FILE_SYSTEM_MEMORY_COLUMN_MOD_TIME, time(NULL),
                      GTK_FILE_SYSTEM_MEMORY_COLUMN_SIZE, 0,
                      GTK_FILE_SYSTEM_MEMORY_COLUMN_IS_HIDDEN, FALSE,
                      GTK_FILE_SYSTEM_MEMORY_COLUMN_IS_FOLDER, TRUE,
                      -1 );

  g_free( to_path );
  g_free( new_folder );
  gtk_tree_path_free( tree_path );
  return TRUE;
}


static void
gtk_file_system_memory_volume_free( GtkFileSystem *file_system,
				                            GtkFileSystemVolume *volume )
{
  DTEXT( ":"__FUNCTION__"\n" );
  gtk_tree_row_reference_free( (GtkTreeRowReference*)volume );
}


static GtkFilePath *
gtk_file_system_memory_volume_get_base_path( GtkFileSystem *file_system,
						                                 GtkFileSystemVolume  *volume )
{
  DTEXT( ":"__FUNCTION__"\n" );
  return (GtkFilePath*)gtk_file_system_memory_volume_get_display_name(
                                                         file_system, volume );
}


static gboolean
gtk_file_system_memory_volume_get_is_mounted( GtkFileSystem  *file_system,
                                              GtkFileSystemVolume  *volume )
{
  DTEXT( ":"__FUNCTION__"\n" );
  return TRUE;
}


static gboolean
gtk_file_system_memory_volume_mount( GtkFileSystem *file_system,
					                           GtkFileSystemVolume *volume,
                                     GError **error )
{
  DTEXT( ":"__FUNCTION__"\n" );
  /* DONTKNOW - Should we emit a signal or not? */
  /*g_signal_emit_by_name (file_system, "volumes-changed", NULL);*/
  return TRUE;
}


static gchar *
gtk_file_system_memory_volume_get_display_name( GtkFileSystem *file_system,
						                                    GtkFileSystemVolume *volume )
{
  GtkTreePath *path = NULL;
  GtkTreeIter iter;
  gchar *str = NULL;
  GtkTreeModel *model;
  GtkTreeRowReference *row = (GtkTreeRowReference*)volume;

  DTEXT( ":"__FUNCTION__"\n" );

  if( !(path = gtk_tree_row_reference_get_path(row)) )
    return NULL;
  
  while( gtk_tree_path_get_depth( path ) > 1 )
    gtk_tree_path_up(path);
  
  model = GTK_TREE_MODEL(file_system);
  
  if(!gtk_tree_model_get_iter( model, &iter, path ))
    return NULL;
  
  gtk_tree_path_free( path );
  
  gtk_tree_model_get( model, &iter, GTK_FILE_SYSTEM_MEMORY_COLUMN_NAME, &str,
                      -1 );
  return str;
}


static GdkPixbuf *
gtk_file_system_memory_volume_render_icon( GtkFileSystem *file_system,
					                                 GtkFileSystemVolume *volume,
                                           GtkWidget *widget, gint pixel_size,
                                           GError **error )
{
  GdkPixbuf *pbuf = NULL;
  GtkFilePath *path = NULL;
  GtkTreePath *tree_path = gtk_tree_row_reference_get_path(
      					   (GtkTreeRowReference*)volume );

  DTEXT( ":"__FUNCTION__"\n" );

  if( !tree_path )
  {
    g_set_error( error, GTK_FILE_SYSTEM_ERROR,
                 GTK_FILE_SYSTEM_ERROR_BAD_FILENAME,
                 "%s is not a valid path", (gchar*)path );
    return NULL;
  }
                                            
  path = gtk_file_system_memory_tree_path_to_file_path( file_system, tree_path);
  
  pbuf = gtk_file_system_memory_render_icon( file_system, path, widget,
                                             pixel_size, error );

  g_free( (gchar*)path );
  gtk_tree_path_free( tree_path );
  return pbuf;
}


static gboolean
gtk_file_system_memory_get_parent( GtkFileSystem *file_system,
				                           const GtkFilePath *path,
                                   GtkFilePath **parent, GError **error )
{
  GtkTreePath *tree_path = NULL;

  DTEXT( ":"__FUNCTION__"\n" );

  if( !(tree_path = gtk_file_system_memory_file_path_to_tree_path( file_system,
                                                                   path )) )
  {
    g_set_error( error, GTK_FILE_SYSTEM_ERROR, 
                 GTK_FILE_SYSTEM_ERROR_NONEXISTENT,
                 "Failed to get parent, %s is not a valid path", (gchar*)path );

    *parent = NULL;
    return FALSE;
  }

  if( gtk_tree_path_get_depth(tree_path) == 1 )
  {
    *parent = NULL;
    gtk_tree_path_free( tree_path );
    return TRUE;
  }

  gtk_tree_path_up( tree_path );
  *parent = gtk_file_system_memory_tree_path_to_file_path( file_system,
                                                           tree_path );
  gtk_tree_path_free( tree_path );
  
  return TRUE;
}


static GtkFilePath *
gtk_file_system_memory_make_path( GtkFileSystem *file_system,
				                          const GtkFilePath *base_path,
                                  const gchar *display_name, GError **error )
{
  gint len = 0;
  GtkFilePath *result = NULL;
  const gchar *path = gtk_file_path_get_string(base_path);

  DTEXT( ":"__FUNCTION__"\n" );

  len = strlen(path)-1;

  if( path[len] != '/' && display_name[0] != '/' )
    result = (GtkFilePath*)g_strconcat( (gchar*)base_path, "/", display_name,
                                      NULL );
  else if( path[len] == '/' && display_name[0] == '/' )
    result = (GtkFilePath*)g_strconcat( (gchar*)base_path, display_name + 1,
                                      NULL);
  else
    result = (GtkFilePath*)g_strconcat( (gchar*)base_path, display_name, NULL );
  
  return result;
}

/* TODO -- This is not parsing anything*/
static gboolean
gtk_file_system_memory_parse( GtkFileSystem *file_system,
				                      const GtkFilePath *base_path,
                              const gchar *str, GtkFilePath **folder,
                              gchar **file_part, GError **error )
{
  GtkTreePath *tree_path = NULL;

  DTEXT( ":"__FUNCTION__"\n" );

  if( !(tree_path = gtk_file_system_memory_file_path_to_tree_path( file_system,
                                                                   base_path)) )
  {
    g_set_error( error, GTK_FILE_SYSTEM_ERROR, 
                 GTK_FILE_SYSTEM_ERROR_BAD_FILENAME,
                 "%s is not a valid path", (gchar*)base_path );
    return FALSE;
  }
  
  *folder = (GtkFilePath*)g_strdup( (gchar*)base_path );
  *file_part = g_strdup( str );
  gtk_tree_path_free(tree_path);

  return TRUE;
}


static gchar *
gtk_file_system_memory_path_to_uri( GtkFileSystem *file_system,
				                            const GtkFilePath *path )
{
  DTEXT( ":"__FUNCTION__"\n" );
  return g_filename_to_uri( gtk_file_path_get_string(path), NULL, NULL );
}


static gchar *
gtk_file_system_memory_path_to_filename( GtkFileSystem *file_system,
					                               const GtkFilePath *path )
{
  DTEXT( ":"__FUNCTION__"\n" );
  return g_strdup( gtk_file_path_get_string(path) );
}


static GtkFilePath *
gtk_file_system_memory_uri_to_path( GtkFileSystem *file_system,
                                    const gchar *uri )
{
  GtkFilePath *path = NULL;
  gchar *filename = strchr( uri, ':' );

  DTEXT( ":"__FUNCTION__"\n" );

  if (filename)
    {
      if( (filename = g_unescape_uri_string( filename )) )
      {
        path = gtk_file_path_new_dup(filename+3);
        g_free( filename );
      }
    }

  return path;
}


static GtkFilePath *
gtk_file_system_memory_filename_to_path( GtkFileSystem *file_system,
					                               const gchar *filename )
{
  DTEXT( ":"__FUNCTION__"\n" );
  return gtk_file_path_new_dup(filename);
}


static GdkPixbuf *
gtk_file_system_memory_render_icon( GtkFileSystem *file_system,
				                            const GtkFilePath *path, GtkWidget *widget,
                                    gint pixel_size, GError **error )
{
  GdkPixbuf *pbuf;
  GtkTreeIter iter;
  GtkTreeModel *model;
  GtkTreePath *tree_path;

  DTEXT( ":"__FUNCTION__"\n" );

  if( !(tree_path = gtk_file_system_memory_file_path_to_tree_path(
       file_system, path )) )
  {
    g_set_error( error, GTK_FILE_SYSTEM_ERROR, 
                 GTK_FILE_SYSTEM_ERROR_BAD_FILENAME,
                 "%s is not a valid path", (gchar*)path );
    return NULL;
  }

  model = GTK_TREE_MODEL( file_system );

  gtk_tree_model_get_iter( model, &iter, tree_path );
  
  gtk_tree_model_get( model, &iter, GTK_FILE_SYSTEM_MEMORY_COLUMN_ICON, &pbuf,
                      -1 );
  
  gtk_tree_path_free( tree_path );
  
  return pbuf;
}


static gboolean
gtk_file_system_memory_insert_bookmark( GtkFileSystem *file_system,
					                              const GtkFilePath *path, gint position,
                                        GError **error )
{
  GtkTreePath *tree_path = NULL;
  GtkFileSystemMemory *fsm = GTK_FILE_SYSTEM_MEMORY(file_system);

  DTEXT( ":"__FUNCTION__"\n" );

  tree_path = gtk_file_system_memory_file_path_to_tree_path( file_system, path );
  
  if( !tree_path )
  {
    g_set_error( error, GTK_FILE_SYSTEM_ERROR, 
                 GTK_FILE_SYSTEM_ERROR_BAD_FILENAME,
                 "%s is not a valid path", (gchar*)path );
    return FALSE;
  }

  gtk_tree_path_free( tree_path );
  fsm->bookmarks = g_slist_append( fsm->bookmarks, g_strdup((gchar*)path) );
  g_signal_emit_by_name (file_system, "bookmarks-changed", 0);
  
  return TRUE;
}


static gboolean
gtk_file_system_memory_remove_bookmark( GtkFileSystem *file_system,
					                              const GtkFilePath *path,
                                        GError **error )
{
  GSList *start = NULL;
  GSList *current = NULL;
  start = current = GTK_FILE_SYSTEM_MEMORY(file_system)->bookmarks;

  DTEXT( ":"__FUNCTION__"\n" );

  while( current )
  {
    if( !strcmp( (gchar*)current->data, (gchar*)path ) )
    {
      start = g_slist_delete_link( start, current );
      GTK_FILE_SYSTEM_MEMORY(file_system)->bookmarks = start;
      g_signal_emit_by_name (file_system, "bookmarks-changed", 0);
      return TRUE;
    }
    current = g_slist_next(current);
  }
  
  g_set_error( error, GTK_FILE_SYSTEM_ERROR, 
                 GTK_FILE_SYSTEM_ERROR_NONEXISTENT,
                 "%s is not a valid bookmark", (gchar*)path );
  
  return FALSE;
}


static GSList *
gtk_file_system_memory_list_bookmarks( GtkFileSystem *file_system )
{
  GSList *current = GTK_FILE_SYSTEM_MEMORY(file_system)->bookmarks;
  GSList *new = NULL;

  DTEXT( ":"__FUNCTION__"\n" );

  if( current )
    do
    {
      new = g_slist_append( new, g_strdup(current->data) );
      
    } while( (current = g_slist_next(current)) );

  return new; 
}

