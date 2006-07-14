#ifndef __HILDON_THUMB_MENU_ITEM_H__
#define __HILDON_THUMB_MENU_ITEM_H__


#include <gdk/gdk.h>
#include <gtk/gtkimagemenuitem.h>

G_BEGIN_DECLS

/* The thumb press is indicated by button id 8 */
#define THUMB_BUTTON_ID 8

#define HILDON_TYPE_THUMB_MENU_ITEM            (hildon_thumb_menu_item_get_type ())
#define HILDON_THUMB_MENU_ITEM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), HILDON_TYPE_THUMB_MENU_ITEM, HildonThumbMenuItem))
#define HILDON_THUMB_MENU_ITEM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), HILDON_TYPE_THUMB_MENU_ITEM, HildonThumbMenuItemClass))
#define HILDON_IS_THUMB_MENU_ITEM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HILDON_TYPE_THUMB_MENU_ITEM))
#define HILDON_IS_THUMB_MENU_ITEM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), HILDON_TYPE_THUMB_MENU_ITEM))
#define HILDON_THUMB_MENU_ITEM_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), HILDON_TYPE_THUMB_MENU_ITEM, HildonThumbMenuItemClass))


typedef struct _HildonThumbMenuItem       HildonThumbMenuItem;
typedef struct _HildonThumbMenuItemClass  HildonThumbMenuItemClass;

struct _HildonThumbMenuItem
{
  GtkImageMenuItem menu_item;
};

struct _HildonThumbMenuItemClass
{
  GtkImageMenuItemClass parent_class;
};


GType	   hildon_thumb_menu_item_get_type (void) G_GNUC_CONST;
GtkWidget* hildon_thumb_menu_item_new (void);
GtkWidget* hildon_thumb_menu_item_new_with_labels (const gchar *label,
                                                   const gchar *thumb_label,
                                                   const gchar *comment);
GtkWidget* hildon_thumb_menu_item_new_with_mnemonic (const gchar *label,
                                                     const gchar *thumb_label,
                                                     const gchar *comment);
void hildon_thumb_menu_item_set_images (HildonThumbMenuItem *thumb_menu_item,
                                        GtkWidget *image,
                                        GtkWidget *thumb_image);
void hildon_menu_set_thumb_mode (GtkMenu *menu, gboolean thumb_mode);

G_END_DECLS

#endif /* __HILDON_THUMB_MENU_ITEM_H__ */
#ifndef __HILDON_THUMB_MENU_ITEM_H__
#define __HILDON_THUMB_MENU_ITEM_H__


#include <gdk/gdk.h>
#include <gtk/gtkimagemenuitem.h>

G_BEGIN_DECLS

/* The thumb press is indicated by button id 8 */
#define THUMB_BUTTON_ID 8

#define HILDON_TYPE_THUMB_MENU_ITEM            (hildon_thumb_menu_item_get_type ())
#define HILDON_THUMB_MENU_ITEM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), HILDON_TYPE_THUMB_MENU_ITEM, HildonThumbMenuItem))
#define HILDON_THUMB_MENU_ITEM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), HILDON_TYPE_THUMB_MENU_ITEM, HildonThumbMenuItemClass))
#define HILDON_IS_THUMB_MENU_ITEM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HILDON_TYPE_THUMB_MENU_ITEM))
#define HILDON_IS_THUMB_MENU_ITEM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), HILDON_TYPE_THUMB_MENU_ITEM))
#define HILDON_THUMB_MENU_ITEM_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), HILDON_TYPE_THUMB_MENU_ITEM, HildonThumbMenuItemClass))


typedef struct _HildonThumbMenuItem       HildonThumbMenuItem;
typedef struct _HildonThumbMenuItemClass  HildonThumbMenuItemClass;

struct _HildonThumbMenuItem
{
  GtkImageMenuItem menu_item;
};

struct _HildonThumbMenuItemClass
{
  GtkImageMenuItemClass parent_class;
};


GType	   hildon_thumb_menu_item_get_type (void) G_GNUC_CONST;
GtkWidget* hildon_thumb_menu_item_new (void);
GtkWidget* hildon_thumb_menu_item_new_with_labels (const gchar *label,
                                                   const gchar *thumb_label,
                                                   const gchar *comment);
GtkWidget* hildon_thumb_menu_item_new_with_mnemonic (const gchar *label,
                                                     const gchar *thumb_label,
                                                     const gchar *comment);
void hildon_thumb_menu_item_set_images (HildonThumbMenuItem *thumb_menu_item,
                                        GtkWidget *image,
                                        GtkWidget *thumb_image);
void hildon_menu_set_thumb_mode (GtkMenu *menu, gboolean thumb_mode);

G_END_DECLS

#endif /* __HILDON_THUMB_MENU_ITEM_H__ */
