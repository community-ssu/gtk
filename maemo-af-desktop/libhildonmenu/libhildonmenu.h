#ifndef __LIBMENU_H__
#define __LIBMENU_H__

#include <libxml/xmlreader.h>
#include <gtk/gtk.h>
#include <libintl.h>

G_BEGIN_DECLS

/* [Desktop Entry] */
#define DESKTOP_ENTRY_TYPE_FIELD     "Type"
#define DESKTOP_ENTRY_ICON_FIELD     "Icon"
#define DESKTOP_ENTRY_NAME_FIELD     "Name"
#define DESKTOP_ENTRY_EXEC_FIELD     "Exec"
#define DESKTOP_ENTRY_SERVICE_FIELD  "X-Osso-Service"

#define SEPARATOR_STRING "SEPARATOR"

/* Default directory for .desktop files */
#define DEFAULT_APPS_DIR "/usr/share/applications/"

#define ICON_FAVOURITES  "qgn_list_gene_favor"
#define ICON_FOLDER      "qgn_list_gene_fldr_cls"
#define ICON_DEFAULT_APP "qgn_list_gene_default_app"
#define ICON_SIZE        26

/* This string is only displayed in the task navigator applet */
#define FAVOURITES_NAME  dgettext("osso-applet-tasknavigator", "tncpa_li_of_favourites")

/* Default systemwide menu */
#define SYSTEMWIDE_MENU_FILE "/etc/xdg/menus/applications.menu"

/* User specific menu. $HOME is prepended to this! */
#define USER_MENU_FILE ".osso/menus/applications.menu"


/* TreeModel items */
enum {
	TREE_MODEL_NAME = 0,
	TREE_MODEL_ICON,
	TREE_MODEL_EXEC,
	TREE_MODEL_SERVICE,
	TREE_MODEL_DESKTOP_ID,
	TREE_MODEL_COLUMNS
};

/* Cell columns */
enum {
	CELL_NAME_COLUMN = 0, /* Name cell in TreeView. */
	CELL_ICON_COLUMN,     /* Icon cell in TreeView */
	CELL_COLUMNS          /* Number of items in this enum */
};

/* Menu types */
enum {
	USER_MENU = 0,        /* User specific menu */
	SYSTEMWIDE_MENU,      /* The (default) systemwide menu */
	MENU_TYPES            /* Number of menu types */
};



/* Function for getting an icon */
GdkPixbuf *get_icon(const char *icon_name, int icon_size);

/* Function for getting the menu contents */
GtkTreeModel *get_menu_contents(void);

/* Function for setting the menu contents, i.e. writing it to the file */
gboolean set_menu_contents( GtkTreeModel *model );

G_END_DECLS

#endif
