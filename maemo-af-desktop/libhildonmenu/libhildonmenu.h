#ifndef __LIBMENU_H__
#define __LIBMENU_H__

#include <libxml/xmlreader.h>
#include <gtk/gtk.h>
#include <libintl.h>

G_BEGIN_DECLS

/* [Desktop Entry] */
#define DESKTOP_ENTRY_TYPE_FIELD        "Type"
#define DESKTOP_ENTRY_ICON_FIELD        "Icon"
#define DESKTOP_ENTRY_NAME_FIELD        "Name"
#define DESKTOP_ENTRY_COMMENT_FIELD     "Comment"
#define DESKTOP_ENTRY_EXEC_FIELD        "Exec"
#define DESKTOP_ENTRY_SERVICE_FIELD     "X-Osso-Service"
#define DESKTOP_ENTRY_TEXT_DOMAIN_FIELD "X-Text-Domain"

#define SEPARATOR_STRING   "SEPARATOR"
#define EXTRAS_MENU_STRING "tana_fi_extras"

/* Default directory for .desktop files */
#define DEFAULT_APPS_DIR "/usr/share/applications/"
#define ICON_FAVOURITES  "qgn_list_gene_favor"
#define ICON_FOLDER      "qgn_list_gene_fldr_cls"
#define ICON_DEFAULT_APP "qgn_list_gene_default_app"
#define ICON_SIZE        26
/* Apparently 64 is what we get for the "scalable" size. Should really be -1.*/
#define ICON_THUMB_SIZE  64

#define EMBLEM_EXPANDER_OPEN   "qgn_list_gene_fldr_exp"
#define EMBLEM_EXPANDER_CLOSED "qgn_list_gene_fldr_clp"

/* This string is only displayed in the task navigator applet */
#define FAVOURITES_LOGICAL_STRING "tncpa_li_of_favourites"
#define FAVOURITES_NAME  dgettext("osso-applet-tasknavigator", FAVOURITES_LOGICAL_STRING)

/* Default systemwide menu */
#define SYSTEMWIDE_MENU_FILE "/etc/xdg/menus/applications.menu"

/* User specific menu. $HOME is prepended to this! */
#define USER_MENU_FILE ".osso/menus/applications.menu"


/* TreeModel items */
enum {
	TREE_MODEL_NAME = 0,
	TREE_MODEL_ICON,
	TREE_MODEL_THUMB_ICON,
	TREE_MODEL_EMBLEM_EXPANDER_OPEN,
	TREE_MODEL_EMBLEM_EXPANDER_CLOSED,
	TREE_MODEL_EXEC,
	TREE_MODEL_SERVICE,
	TREE_MODEL_DESKTOP_ID,
	TREE_MODEL_COMMENT,
	TREE_MODEL_TEXT_DOMAIN,
	TREE_MODEL_COLUMNS
};

/* Menu types */
enum {
	USER_MENU = 0,        /* User specific menu */
	SYSTEMWIDE_MENU,      /* The (default) systemwide menu */
	MENU_TYPES            /* Number of menu types */
};



/* Function for getting an icon */
GdkPixbuf *get_icon(const char *icon_name, int icon_size);

/* Function to load an icon with a fallback if exact size is not present */
GdkPixbuf *get_icon_with_fallback(const char *icon_name,
                                  int icon_size,
                                  GdkPixbuf *fallback);


/* Function for getting the menu contents */
GtkTreeModel *get_menu_contents(void);

/* Function for finding the first and last folders on root level
 * i.e. under Favourites.
 *
 * model        - The tree model which to search for the folders
 * first_folder - GtkTreePath to be set to point to the first folder
 * last_folder  - GtkTreePath to be set to point to the last folder
 */
void find_first_and_last_root_level_folders( GtkTreeModel *model,
		GtkTreePath **first_folder, GtkTreePath **last_folder );

/* Function for setting the separators to correct positions 
 *
 * model - The tree model of the menu in which we want to set the separators.
 *
 * Returns TRUE on success, FALSE on failure.
 */
gboolean set_separators( GtkTreeModel *model );

/* Function for setting the menu contents, i.e. writing it to the file */
gboolean set_menu_contents( GtkTreeModel *model );

G_END_DECLS

#endif
