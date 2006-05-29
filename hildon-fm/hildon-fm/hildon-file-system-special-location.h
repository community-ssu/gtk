#ifndef __HILDON_FILE_SYSTEM_SPECIAL_LOCATION_H__
#define __HILDON_FILE_SYSTEM_SPECIAL_LOCATION_H__

#include <gtk/gtkwidget.h>
#include "hildon-file-system-common.h"

G_BEGIN_DECLS

#define HILDON_TYPE_FILE_SYSTEM_SPECIAL_LOCATION \
        (hildon_file_system_special_location_get_type ())
#define HILDON_FILE_SYSTEM_SPECIAL_LOCATION(obj) \
        (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
         HILDON_TYPE_FILE_SYSTEM_SPECIAL_LOCATION, \
         HildonFileSystemSpecialLocation))
#define HILDON_FILE_SYSTEM_SPECIAL_LOCATION_CLASS(klass) \
        (G_TYPE_CHECK_CLASS_CAST ((klass), \
         HILDON_TYPE_FILE_SYSTEM_SPECIAL_LOCATION, \
         HildonFileSystemSpecialLocationClass))
#define HILDON_IS_FILE_SYSTEM_SPECIAL_LOCATION(obj) \
        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
         HILDON_TYPE_FILE_SYSTEM_SPECIAL_LOCATION))
#define HILDON_IS_FILE_SYSTEM_SPECIAL_LOCATION_CLASS(klass) \
        (G_TYPE_CHECK_CLASS_TYPE ((klass), \
         HILDON_TYPE_FILE_SYSTEM_SPECIAL_LOCATION))
#define HILDON_FILE_SYSTEM_SPECIAL_LOCATION_GET_CLASS(obj) \
        (G_TYPE_INSTANCE_GET_CLASS ((obj), \
         HILDON_TYPE_FILE_SYSTEM_SPECIAL_LOCATION, \
         HildonFileSystemSpecialLocationClass))


typedef struct _HildonFileSystemSpecialLocation HildonFileSystemSpecialLocation;
typedef struct _HildonFileSystemSpecialLocationClass HildonFileSystemSpecialLocationClass;


struct _HildonFileSystemSpecialLocation
{
    GObject parent_instance;
    gchar *basepath;    /* Path as uri */
    gchar *fixed_icon;  /* Icon name as string. NULL for no fixed icon */
    gchar *fixed_title; /* Text for fixed display name as string */
    const gchar *failed_access_message; /* Default failed accessa message */
    gint  sort_weight;  /* How the location behaves while sorting */
    HildonFileSystemModelItemType compatibility_type; /* For backwards compatibility */
};

struct _HildonFileSystemSpecialLocationClass
{
    GObjectClass parent_class;

    /* private */
    gchar* (*get_display_name) (HildonFileSystemSpecialLocation *location, GtkFileSystem *fs);
    gchar* (*get_extra_info) (HildonFileSystemSpecialLocation *location);
    GdkPixbuf* (*get_icon) (HildonFileSystemSpecialLocation *location,
                            GtkFileSystem *fs, GtkWidget *ref_widget, int size);
    gboolean (*is_available) (HildonFileSystemSpecialLocation *location);
    gboolean (*is_visible) (HildonFileSystemSpecialLocation *location);
    gchar* (*get_unavailable_reason) (HildonFileSystemSpecialLocation *location);
    gboolean (*requires_access) (HildonFileSystemSpecialLocation *location);
    gboolean (*failed_access) (HildonFileSystemSpecialLocation *location);
    HildonFileSystemSpecialLocation* (*create_child_location)
        (HildonFileSystemSpecialLocation *location, gchar *uri);
    void (*volumes_changed) (HildonFileSystemSpecialLocation *location,
                             GtkFileSystem *fs);

    /* signals */
    void (*changed) (GObject *obj);
    void (*connection_state) (GObject *obj);
};

GType hildon_file_system_special_location_get_type (void) G_GNUC_CONST;


/* Title that should be used for the location. If the virtual function is not
 * defined, then NULL is returned (which in turn can be intepreted as fallback
 * to GtkFileInfo) */
gchar *hildon_file_system_special_location_get_display_name (HildonFileSystemSpecialLocation *location, GtkFileSystem *fs);

/* Title that should be used for the location. If the virtual function is not
 * defined, then NULL is returned (which in turn can be intepreted as fallback
 * to GtkFileInfo) */
gchar *hildon_file_system_special_location_get_extra_info (HildonFileSystemSpecialLocation *location);

/* Note! We cannot return just icon name, since some icons are created using
 * gtk_file_system_render_icon, which doesn't provide icon names. Returns
 * fixed icon, if such is available. */
GdkPixbuf *hildon_file_system_special_location_get_icon (HildonFileSystemSpecialLocation *location, GtkFileSystem *fs, GtkWidget *ref_widget, int size);

/* Whether or not the location should be dimmed. May depend on flightmode,
 * mmc, usb-cable...
 * If subclass doesn't define this method the default result is TRUE */
gboolean hildon_file_system_special_location_is_available (HildonFileSystemSpecialLocation *location);

/* Whether or not the location should be shown at all. For example, undefined
 * old-style gateways should not be shown at all. By default all locations
 * are visible. */
gboolean hildon_file_system_special_location_is_visible (HildonFileSystemSpecialLocation *location);

/* Why the location should be dimmed. Used in infoprints. If the subclass
 * doesn't define this method, the default result is NULL, since by
 * default the location is available. */
gchar *hildon_file_system_special_location_get_unavailable_reason (HildonFileSystemSpecialLocation *location);

/* Whether or nor the model should be accessed before actually using it.
 * Returns FALSE by default. */
gboolean hildon_file_system_special_location_requires_access (HildonFileSystemSpecialLocation *location);

/* Called by model to hint the device that operation to access location
 * contents failed (for example: no BT connection). Location object can
 * ignore this, but may also want to be removed from the model */
gboolean hildon_file_system_special_location_failed_access (HildonFileSystemSpecialLocation *location);

/* If the given path is an immediate child of the location and location thinks
 * that the path is actually a device as well, it creates a new dynamic
 * location.
 * This functionality is used when dynamic device are appended to the tree.
 * If not, NULL is returned. If child_type == 0 or subclass doesn't implement
 * this, NULL is returned always.
 * THIS IS CALLED BY _hildon_file_system_get_special_location ONLY */
HildonFileSystemSpecialLocation *
hildon_file_system_special_location_create_child_location (HildonFileSystemSpecialLocation *location, gchar *uri);

void
hildon_file_system_special_location_volumes_changed (HildonFileSystemSpecialLocation *location, GtkFileSystem *fs);

/* Convenience function for setting fixed name. If fixed name is enough, name
 * related virtual functions are not needed to be overwritten by subclasses.*/
void hildon_file_system_special_location_set_display_name (HildonFileSystemSpecialLocation *location, const gchar *name);

/* Convenience function for setting fixed icon. If fixed icon is enough,
 * subclasses do not need to overwrite icon related methods.
 * Note that we set icon name as string, but receive it as GdkPixBuf.
 * This small inconvenience is bacause of GtkFileSystem iface */
void hildon_file_system_special_location_set_icon (HildonFileSystemSpecialLocation *location, const gchar *icon_name);


G_END_DECLS

#endif
