#include <hail/hailfactory.h>
#include <hail-hildon-desktop.h>

#include "hailimagemenuitem.h"
#include "haildesktopitem.h"
#include "haildesktoppanel.h"
#include "haildesktoppanelitem.h"
#include "haildesktopwindow.h"
#include "hailhomewindow.h"
#include "haildesktoppanelwindow.h"
#include "hailtogglebutton.h"
#include "hailhomearea.h"
#include "hailhometitlebar.h"

#include <gtk/gtkimagemenuitem.h>
#include <libhildondesktop/hildon-desktop-item.h>
#include <libhildondesktop/hildon-desktop-panel.h>
#include <libhildondesktop/hildon-desktop-panel-item.h>
#include <libhildondesktop/hildon-desktop-window.h>
#include <libhildondesktop/hildon-home-window.h>
#include <libhildondesktop/hildon-desktop-panel-window.h>
#include <gtk/gtktogglebutton.h>
#include <libhildondesktop/hildon-home-area.h>
#include <libhildondesktop/hildon-home-titlebar.h>

/* Hail factories class definition */
HAIL_ACCESSIBLE_FACTORY (HAIL_TYPE_IMAGE_MENU_ITEM, hail_image_menu_item, hail_image_menu_item_new)
HAIL_ACCESSIBLE_FACTORY (HAIL_TYPE_DESKTOP_ITEM, hail_desktop_item, hail_desktop_item_new)
HAIL_ACCESSIBLE_FACTORY (HAIL_TYPE_DESKTOP_PANEL, hail_desktop_panel, hail_desktop_panel_new)
HAIL_ACCESSIBLE_FACTORY (HAIL_TYPE_DESKTOP_PANEL_ITEM, hail_desktop_panel_item, hail_desktop_panel_item_new)
HAIL_ACCESSIBLE_FACTORY (HAIL_TYPE_DESKTOP_WINDOW, hail_desktop_window, hail_desktop_window_new)
HAIL_ACCESSIBLE_FACTORY (HAIL_TYPE_HOME_WINDOW, hail_home_window, hail_home_window_new)
HAIL_ACCESSIBLE_FACTORY (HAIL_TYPE_DESKTOP_PANEL_WINDOW, hail_desktop_panel_window, hail_desktop_panel_window_new)
HAIL_ACCESSIBLE_FACTORY (HAIL_TYPE_TOGGLE_BUTTON, hail_toggle_button, hail_toggle_button_new)
HAIL_ACCESSIBLE_FACTORY (HAIL_TYPE_HOME_AREA, hail_home_area, hail_home_area_new)
HAIL_ACCESSIBLE_FACTORY (HAIL_TYPE_HOME_TITLEBAR, hail_home_titlebar, hail_home_titlebar_new)

void
hail_hildon_desktop_init (void)
{
  HAIL_WIDGET_SET_FACTORY (GTK_TYPE_IMAGE_MENU_ITEM, hail_image_menu_item);
  HAIL_WIDGET_SET_FACTORY (HILDON_DESKTOP_TYPE_ITEM, hail_desktop_item);
  HAIL_WIDGET_SET_FACTORY (HILDON_DESKTOP_TYPE_PANEL, hail_desktop_panel);
  HAIL_WIDGET_SET_FACTORY (HILDON_DESKTOP_TYPE_PANEL_ITEM, hail_desktop_panel_item);
  HAIL_WIDGET_SET_FACTORY (HILDON_DESKTOP_TYPE_WINDOW, hail_desktop_window);
  HAIL_WIDGET_SET_FACTORY (HILDON_TYPE_HOME_WINDOW, hail_home_window);
  HAIL_WIDGET_SET_FACTORY (HILDON_DESKTOP_TYPE_PANEL_WINDOW, hail_desktop_panel_window);
  HAIL_WIDGET_SET_FACTORY (GTK_TYPE_TOGGLE_BUTTON, hail_toggle_button);
  HAIL_WIDGET_SET_FACTORY (HILDON_TYPE_HOME_AREA, hail_home_area);
  HAIL_WIDGET_SET_FACTORY (HILDON_TYPE_HOME_TITLEBAR, hail_home_titlebar);
}
