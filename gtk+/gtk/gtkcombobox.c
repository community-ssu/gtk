/* gtkcombobox.c
 * Copyright (C) 2002, 2003  Kristian Rietveld <kris@gtk.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <config.h>
#include "gtkcombobox.h"

#include "gtkarrow.h"
#include "gtkbindings.h"
#include "gtkcelllayout.h"
#include "gtkcellrenderertext.h"
#include "gtkcellview.h"
#include "gtkeventbox.h"
#include "gtkframe.h"
#include "gtkhbox.h"
#include "gtkliststore.h"
#include "gtkmain.h"
#include "gtkmenu.h"
#include "gtkscrolledwindow.h"
#include "gtkseparatormenuitem.h"
#include "gtktearoffmenuitem.h"
#include "gtktogglebutton.h"
#include "gtktreeselection.h"
#include "gtkvseparator.h"
#include "gtkwindow.h"
#include "gtkprivate.h"

#include <gdk/gdkkeysyms.h>

#include <gobject/gvaluecollector.h>

#include <string.h>
#include <stdarg.h>

#include "gtkmarshalers.h"
#include "gtkintl.h"

#include "gtktreeprivate.h"
#include "gtkalias.h"

/* WELCOME, to THE house of evil code */

typedef struct _ComboCellInfo ComboCellInfo;
struct _ComboCellInfo
{
  GtkCellRenderer *cell;
  GSList *attributes;

  GtkCellLayoutDataFunc func;
  gpointer func_data;
  GDestroyNotify destroy;

  guint expand : 1;
  guint pack : 1;
};

#define GTK_COMBO_BOX_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GTK_TYPE_COMBO_BOX, GtkComboBoxPrivate))

struct _GtkComboBoxPrivate
{
  GtkTreeModel *model;

  gint col_column;
  gint row_column;

  gint wrap_width;

  GtkTreeRowReference *active_row;

  GtkWidget *tree_view;
  GtkTreeViewColumn *column;

  GtkWidget *cell_view;
  GtkWidget *cell_view_frame;

  GtkWidget *button;
  GtkWidget *box;
  GtkWidget *arrow;
  GtkWidget *separator;

  GtkWidget *popup_widget;
  GtkWidget *popup_window;
  GtkWidget *popup_frame;
  GtkWidget *scrolled_window;

  guint inserted_id;
  guint deleted_id;
  guint reordered_id;
  guint changed_id;
  guint popup_idle_id;
  guint scroll_timer;
  guint resize_idle_id;

  gint width;
  gint height;
  GSList *cells;

  guint popup_in_progress : 1;
  guint popup_shown : 1;
  guint add_tearoffs : 1;
  guint has_frame : 1;
  guint is_cell_renderer : 1;
  guint editing_canceled : 1;
  guint auto_scroll : 1;
  guint focus_on_click : 1;

  GtkTreeViewRowSeparatorFunc row_separator_func;
  gpointer                    row_separator_data;
  GtkDestroyNotify            row_separator_destroy;

  gchar *tearoff_title;
};

/* While debugging this evil code, I have learned that
 * there are actually 4 modes to this widget, which can
 * be characterized as follows
 * 
 * 1) menu mode, no child added
 *
 * tree_view -> NULL
 * cell_view -> GtkCellView, regular child
 * cell_view_frame -> NULL
 * button -> GtkToggleButton set_parent to combo
 * arrow -> GtkArrow set_parent to button
 * separator -> GtkVSepator set_parent to button
 * popup_widget -> GtkMenu
 * popup_window -> NULL
 * popup_frame -> NULL
 * scrolled_window -> NULL
 *
 * 2) menu mode, child added
 * 
 * tree_view -> NULL
 * cell_view -> NULL 
 * cell_view_frame -> NULL
 * button -> GtkToggleButton set_parent to combo
 * arrow -> GtkArrow, child of button
 * separator -> NULL
 * popup_widget -> GtkMenu
 * popup_window -> NULL
 * popup_frame -> NULL
 * scrolled_window -> NULL
 *
 * 3) list mode, no child added
 * 
 * tree_view -> GtkTreeView, child of popup_frame
 * cell_view -> GtkCellView, regular child
 * cell_view_frame -> GtkFrame, set parent to combo
 * button -> GtkToggleButton, set_parent to combo
 * arrow -> GtkArrow, child of button
 * separator -> NULL
 * popup_widget -> tree_view
 * popup_window -> GtkWindow
 * popup_frame -> GtkFrame, child of popup_window
 * scrolled_window -> GtkScrolledWindow, child of popup_frame
 *
 * 4) list mode, child added
 *
 * tree_view -> GtkTreeView, child of popup_frame
 * cell_view -> NULL
 * cell_view_frame -> NULL
 * button -> GtkToggleButton, set_parent to combo
 * arrow -> GtkArrow, child of button
 * separator -> NULL
 * popup_widget -> tree_view
 * popup_window -> GtkWindow
 * popup_frame -> GtkFrame, child of popup_window
 * scrolled_window -> GtkScrolledWindow, child of popup_frame
 * 
 */

enum {
  CHANGED,
  MOVE_ACTIVE,
  POPUP,
  LAST_SIGNAL
};

enum {
  PROP_0,
  PROP_MODEL,
  PROP_WRAP_WIDTH,
  PROP_ROW_SPAN_COLUMN,
  PROP_COLUMN_SPAN_COLUMN,
  PROP_ACTIVE,
  PROP_ADD_TEAROFFS,
  PROP_TEAROFF_TITLE,
  PROP_HAS_FRAME,
  PROP_FOCUS_ON_CLICK,
  PROP_POPUP_SHOWN
};

static guint combo_box_signals[LAST_SIGNAL] = {0,};

#define BONUS_PADDING 4
#define SCROLL_TIME  100

/* common */

static void     gtk_combo_box_cell_layout_init     (GtkCellLayoutIface *iface);
static void     gtk_combo_box_cell_editable_init   (GtkCellEditableIface *iface);
static void     gtk_combo_box_finalize             (GObject          *object);
static void     gtk_combo_box_destroy              (GtkObject        *object);

static void     gtk_combo_box_set_property         (GObject         *object,
                                                    guint            prop_id,
                                                    const GValue    *value,
                                                    GParamSpec      *spec);
static void     gtk_combo_box_get_property         (GObject         *object,
                                                    guint            prop_id,
                                                    GValue          *value,
                                                    GParamSpec      *spec);

static void     gtk_combo_box_state_changed        (GtkWidget        *widget,
			                            GtkStateType      previous);
static void     gtk_combo_box_grab_focus           (GtkWidget       *widget);
static void     gtk_combo_box_style_set            (GtkWidget       *widget,
                                                    GtkStyle        *previous);
static void     gtk_combo_box_button_toggled       (GtkWidget       *widget,
                                                    gpointer         data);
static void     gtk_combo_box_button_state_changed (GtkWidget       *widget,
			                            GtkStateType     previous,
						    gpointer         data);
static void     gtk_combo_box_add                  (GtkContainer    *container,
                                                    GtkWidget       *widget);
static void     gtk_combo_box_remove               (GtkContainer    *container,
                                                    GtkWidget       *widget);

static ComboCellInfo *gtk_combo_box_get_cell_info  (GtkComboBox      *combo_box,
                                                    GtkCellRenderer  *cell);

static void     gtk_combo_box_menu_show            (GtkWidget        *menu,
                                                    gpointer          user_data);
static void     gtk_combo_box_menu_hide            (GtkWidget        *menu,
                                                    gpointer          user_data);

static void     gtk_combo_box_set_popup_widget     (GtkComboBox      *combo_box,
                                                    GtkWidget        *popup);
static void     gtk_combo_box_menu_position_below  (GtkMenu          *menu,
                                                    gint             *x,
                                                    gint             *y,
                                                    gint             *push_in,
                                                    gpointer          user_data);
static void     gtk_combo_box_menu_position_over   (GtkMenu          *menu,
                                                    gint             *x,
                                                    gint             *y,
                                                    gint             *push_in,
                                                    gpointer          user_data);
static void     gtk_combo_box_menu_position        (GtkMenu          *menu,
                                                    gint             *x,
                                                    gint             *y,
                                                    gint             *push_in,
                                                    gpointer          user_data);

static gint     gtk_combo_box_calc_requested_width (GtkComboBox      *combo_box,
                                                    GtkTreePath      *path);
static void     gtk_combo_box_remeasure            (GtkComboBox      *combo_box);

static void     gtk_combo_box_unset_model          (GtkComboBox      *combo_box);

static void     gtk_combo_box_size_request         (GtkWidget        *widget,
                                                    GtkRequisition   *requisition);
static void     gtk_combo_box_size_allocate        (GtkWidget        *widget,
                                                    GtkAllocation    *allocation);
static void     gtk_combo_box_forall               (GtkContainer     *container,
                                                    gboolean          include_internals,
                                                    GtkCallback       callback,
                                                    gpointer          callback_data);
static gboolean gtk_combo_box_expose_event         (GtkWidget        *widget,
                                                    GdkEventExpose   *event);
static gboolean gtk_combo_box_scroll_event         (GtkWidget        *widget,
                                                    GdkEventScroll   *event);
static void     gtk_combo_box_set_active_internal  (GtkComboBox      *combo_box,
						    GtkTreePath      *path);

static void     gtk_combo_box_check_appearance     (GtkComboBox      *combo_box);
static gchar *  gtk_combo_box_real_get_active_text (GtkComboBox      *combo_box);
static void     gtk_combo_box_real_move_active     (GtkComboBox      *combo_box,
                                                    GtkScrollType     scroll);
static void     gtk_combo_box_real_popup           (GtkComboBox      *combo_box);

/* listening to the model */
static void     gtk_combo_box_model_row_inserted   (GtkTreeModel     *model,
						    GtkTreePath      *path,
						    GtkTreeIter      *iter,
						    gpointer          user_data);
static void     gtk_combo_box_model_row_deleted    (GtkTreeModel     *model,
						    GtkTreePath      *path,
						    gpointer          user_data);
static void     gtk_combo_box_model_rows_reordered (GtkTreeModel     *model,
						    GtkTreePath      *path,
						    GtkTreeIter      *iter,
						    gint             *new_order,
						    gpointer          user_data);
static void     gtk_combo_box_model_row_changed    (GtkTreeModel     *model,
						    GtkTreePath      *path,
						    GtkTreeIter      *iter,
						    gpointer          data);
static void     gtk_combo_box_model_row_expanded   (GtkTreeModel     *model,
						    GtkTreePath      *path,
						    GtkTreeIter      *iter,
						    gpointer          data);

/* list */
static void     gtk_combo_box_list_position        (GtkComboBox      *combo_box, 
						    gint             *x, 
						    gint             *y, 
						    gint             *width,
						    gint             *height);
static void     gtk_combo_box_list_setup           (GtkComboBox      *combo_box);
static void     gtk_combo_box_list_destroy         (GtkComboBox      *combo_box);

static gboolean gtk_combo_box_list_button_released (GtkWidget        *widget,
                                                    GdkEventButton   *event,
                                                    gpointer          data);
static gboolean gtk_combo_box_list_key_press       (GtkWidget        *widget,
                                                    GdkEventKey      *event,
                                                    gpointer          data);
static gboolean gtk_combo_box_list_enter_notify    (GtkWidget        *widget,
                                                    GdkEventCrossing *event,
                                                    gpointer          data);
static void     gtk_combo_box_list_auto_scroll     (GtkComboBox   *combo,
						    gint           x,
						    gint           y);
static gboolean gtk_combo_box_list_scroll_timeout  (GtkComboBox   *combo);
static gboolean gtk_combo_box_list_button_pressed  (GtkWidget        *widget,
                                                    GdkEventButton   *event,
                                                    gpointer          data);

static gboolean gtk_combo_box_list_select_func     (GtkTreeSelection *selection,
						    GtkTreeModel     *model,
						    GtkTreePath      *path,
						    gboolean          path_currently_selected,
						    gpointer          data);

static void     gtk_combo_box_list_row_changed     (GtkTreeModel     *model,
                                                    GtkTreePath      *path,
                                                    GtkTreeIter      *iter,
                                                    gpointer          data);
static void     gtk_combo_box_list_popup_resize    (GtkComboBox      *combo_box);

/* menu */
static void     gtk_combo_box_menu_setup           (GtkComboBox      *combo_box,
                                                    gboolean          add_children);
static void     gtk_combo_box_menu_fill            (GtkComboBox      *combo_box);
static void     gtk_combo_box_menu_fill_level      (GtkComboBox      *combo_box,
						    GtkWidget        *menu,
						    GtkTreeIter      *iter);
static void     gtk_combo_box_update_title         (GtkComboBox      *combo_box);
static void     gtk_combo_box_menu_destroy         (GtkComboBox      *combo_box);

static void     gtk_combo_box_relayout_item        (GtkComboBox      *combo_box,
						    GtkWidget        *item,
                                                    GtkTreeIter      *iter,
						    GtkWidget        *last);
static void     gtk_combo_box_relayout             (GtkComboBox      *combo_box);

static gboolean gtk_combo_box_menu_button_press    (GtkWidget        *widget,
                                                    GdkEventButton   *event,
                                                    gpointer          user_data);
static void     gtk_combo_box_menu_item_activate   (GtkWidget        *item,
                                                    gpointer          user_data);
static void     gtk_combo_box_menu_row_inserted    (GtkTreeModel     *model,
                                                    GtkTreePath      *path,
                                                    GtkTreeIter      *iter,
                                                    gpointer          user_data);
static void     gtk_combo_box_menu_row_deleted     (GtkTreeModel     *model,
                                                    GtkTreePath      *path,
                                                    gpointer          user_data);
static void     gtk_combo_box_menu_rows_reordered  (GtkTreeModel     *model,
						    GtkTreePath      *path,
						    GtkTreeIter      *iter,
						    gint             *new_order,
						    gpointer          user_data);
static void     gtk_combo_box_menu_row_changed     (GtkTreeModel     *model,
                                                    GtkTreePath      *path,
                                                    GtkTreeIter      *iter,
                                                    gpointer          data);
static gboolean gtk_combo_box_menu_key_press       (GtkWidget        *widget,
						    GdkEventKey      *event,
						    gpointer          data);
static void     gtk_combo_box_menu_popup           (GtkComboBox      *combo_box,
						    guint             button, 
						    guint32           activate_time);
static GtkWidget *gtk_cell_view_menu_item_new      (GtkComboBox      *combo_box,
						    GtkTreeModel     *model,
						    GtkTreeIter      *iter);

/* cell layout */
static void     gtk_combo_box_cell_layout_pack_start         (GtkCellLayout         *layout,
                                                              GtkCellRenderer       *cell,
                                                              gboolean               expand);
static void     gtk_combo_box_cell_layout_pack_end           (GtkCellLayout         *layout,
                                                              GtkCellRenderer       *cell,
                                                              gboolean               expand);
static void     gtk_combo_box_cell_layout_clear              (GtkCellLayout         *layout);
static void     gtk_combo_box_cell_layout_add_attribute      (GtkCellLayout         *layout,
                                                              GtkCellRenderer       *cell,
                                                              const gchar           *attribute,
                                                              gint                   column);
static void     gtk_combo_box_cell_layout_set_cell_data_func (GtkCellLayout         *layout,
                                                              GtkCellRenderer       *cell,
                                                              GtkCellLayoutDataFunc  func,
                                                              gpointer               func_data,
                                                              GDestroyNotify         destroy);
static void     gtk_combo_box_cell_layout_clear_attributes   (GtkCellLayout         *layout,
                                                              GtkCellRenderer       *cell);
static void     gtk_combo_box_cell_layout_reorder            (GtkCellLayout         *layout,
                                                              GtkCellRenderer       *cell,
                                                              gint                   position);
static gboolean gtk_combo_box_mnemonic_activate              (GtkWidget    *widget,
							      gboolean      group_cycling);

static void     gtk_combo_box_sync_cells                     (GtkComboBox   *combo_box,
					                      GtkCellLayout *cell_layout);
static void     combo_cell_data_func                         (GtkCellLayout   *cell_layout,
							      GtkCellRenderer *cell,
							      GtkTreeModel    *tree_model,
							      GtkTreeIter     *iter,
							      gpointer         data);
static void     gtk_combo_box_child_show                     (GtkWidget       *widget,
							      GtkComboBox     *combo_box);
static void     gtk_combo_box_child_hide                     (GtkWidget       *widget,
							      GtkComboBox     *combo_box);
static gboolean gtk_combo_box_child_delete_event             (GtkWidget       *widget,
							      GdkEventAny     *event,
							      GtkComboBox     *combo_box);

/* GtkCellEditable method implementations */
static void gtk_combo_box_start_editing (GtkCellEditable *cell_editable,
					 GdkEvent        *event);


G_DEFINE_TYPE_WITH_CODE (GtkComboBox, gtk_combo_box, GTK_TYPE_BIN,
			 G_IMPLEMENT_INTERFACE (GTK_TYPE_CELL_LAYOUT,
						gtk_combo_box_cell_layout_init)
			 G_IMPLEMENT_INTERFACE (GTK_TYPE_CELL_EDITABLE,
						gtk_combo_box_cell_editable_init))

/* common */
static void
gtk_combo_box_class_init (GtkComboBoxClass *klass)
{
  GObjectClass *object_class;
  GtkObjectClass *gtk_object_class;
  GtkContainerClass *container_class;
  GtkWidgetClass *widget_class;
  GtkBindingSet *binding_set;

  klass->get_active_text = gtk_combo_box_real_get_active_text;

  container_class = (GtkContainerClass *)klass;
  container_class->forall = gtk_combo_box_forall;
  container_class->add = gtk_combo_box_add;
  container_class->remove = gtk_combo_box_remove;

  widget_class = (GtkWidgetClass *)klass;
  widget_class->size_allocate = gtk_combo_box_size_allocate;
  widget_class->size_request = gtk_combo_box_size_request;
  widget_class->expose_event = gtk_combo_box_expose_event;
  widget_class->scroll_event = gtk_combo_box_scroll_event;
  widget_class->mnemonic_activate = gtk_combo_box_mnemonic_activate;
  widget_class->grab_focus = gtk_combo_box_grab_focus;
  widget_class->style_set = gtk_combo_box_style_set;
  widget_class->state_changed = gtk_combo_box_state_changed;

  gtk_object_class = (GtkObjectClass *)klass;
  gtk_object_class->destroy = gtk_combo_box_destroy;

  object_class = (GObjectClass *)klass;
  object_class->finalize = gtk_combo_box_finalize;
  object_class->set_property = gtk_combo_box_set_property;
  object_class->get_property = gtk_combo_box_get_property;

  /* signals */
  /**
   * GtkComboBox::changed:
   * @widget: the object which received the signal
   * 
   * The changed signal is emitted when the active
   * item is changed. The can be due to the user selecting
   * a different item from the list, or due to a 
   * call to gtk_combo_box_set_active_iter().
   * It will also be emitted while typing into a GtkComboBoxEntry, 
   * as well as when selecting an item from the GtkComboBoxEntry's list.
   *
   * Since: 2.4
   */
  combo_box_signals[CHANGED] =
    g_signal_new (I_("changed"),
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (GtkComboBoxClass, changed),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  combo_box_signals[MOVE_ACTIVE] =
    _gtk_binding_signal_new (I_("move-active"),
                             G_OBJECT_CLASS_TYPE (klass),
                             G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                             G_CALLBACK (gtk_combo_box_real_move_active),
                             NULL, NULL,
                             g_cclosure_marshal_VOID__ENUM,
                             G_TYPE_NONE, 1,
                             GTK_TYPE_SCROLL_TYPE);

  combo_box_signals[POPUP] =
    _gtk_binding_signal_new (I_("popup"),
                             G_OBJECT_CLASS_TYPE (klass),
                             G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                             G_CALLBACK (gtk_combo_box_real_popup),
                             NULL, NULL,
                             g_cclosure_marshal_VOID__VOID,
                             G_TYPE_NONE, 0);

  /* key bindings */
  binding_set = gtk_binding_set_by_class (widget_class);

  gtk_binding_entry_add_signal (binding_set, GDK_Down, GDK_MOD1_MASK,
				"popup", 0);

  gtk_binding_entry_add_signal (binding_set, GDK_Up, 0,
				"move-active", 1,
				GTK_TYPE_SCROLL_TYPE, GTK_SCROLL_STEP_UP);
  gtk_binding_entry_add_signal (binding_set, GDK_KP_Up, 0,
				"move-active", 1,
				GTK_TYPE_SCROLL_TYPE, GTK_SCROLL_STEP_UP);
  gtk_binding_entry_add_signal (binding_set, GDK_Page_Up, 0,
				"move-active", 1,
				GTK_TYPE_SCROLL_TYPE, GTK_SCROLL_PAGE_UP);
  gtk_binding_entry_add_signal (binding_set, GDK_KP_Page_Up, 0,
				"move-active", 1,
				GTK_TYPE_SCROLL_TYPE, GTK_SCROLL_PAGE_UP);
  gtk_binding_entry_add_signal (binding_set, GDK_Home, 0,
				"move-active", 1,
				GTK_TYPE_SCROLL_TYPE, GTK_SCROLL_START);
  gtk_binding_entry_add_signal (binding_set, GDK_KP_Home, 0,
				"move-active", 1,
				GTK_TYPE_SCROLL_TYPE, GTK_SCROLL_START);

  gtk_binding_entry_add_signal (binding_set, GDK_Down, 0,
				"move-active", 1,
				GTK_TYPE_SCROLL_TYPE, GTK_SCROLL_STEP_DOWN);
  gtk_binding_entry_add_signal (binding_set, GDK_KP_Down, 0,
				"move-active", 1,
				GTK_TYPE_SCROLL_TYPE, GTK_SCROLL_STEP_DOWN);
  gtk_binding_entry_add_signal (binding_set, GDK_Page_Down, 0,
				"move-active", 1,
				GTK_TYPE_SCROLL_TYPE, GTK_SCROLL_PAGE_DOWN);
  gtk_binding_entry_add_signal (binding_set, GDK_KP_Page_Down, 0,
				"move-active", 1,
				GTK_TYPE_SCROLL_TYPE, GTK_SCROLL_PAGE_DOWN);
  gtk_binding_entry_add_signal (binding_set, GDK_End, 0,
				"move-active", 1,
				GTK_TYPE_SCROLL_TYPE, GTK_SCROLL_END);
  gtk_binding_entry_add_signal (binding_set, GDK_KP_End, 0,
				"move-active", 1,
				GTK_TYPE_SCROLL_TYPE, GTK_SCROLL_END);

  /* properties */
  /**
   * GtkComboBox:model:
   *
   * The model from which the combo box takes the values shown
   * in the list. 
   *
   * Since: 2.4
   */
  g_object_class_install_property (object_class,
                                   PROP_MODEL,
                                   g_param_spec_object ("model",
                                                        P_("ComboBox model"),
                                                        P_("The model for the combo box"),
                                                        GTK_TYPE_TREE_MODEL,
                                                        GTK_PARAM_READWRITE));

  /**
   * GtkComboBox:wrap-width:
   *
   * If wrap-width is set to a positive value, the list will be
   * displayed in multiple columns, the number of columns is
   * determined by wrap-width.
   *
   * Since: 2.4
   */
  g_object_class_install_property (object_class,
                                   PROP_WRAP_WIDTH,
                                   g_param_spec_int ("wrap-width",
                                                     P_("Wrap width"),
                                                     P_("Wrap width for laying out the items in a grid"),
                                                     0,
                                                     G_MAXINT,
                                                     0,
                                                     GTK_PARAM_READWRITE));


  /**
   * GtkComboBox:row-span-column:
   *
   * If this is set to a non-negative value, it must be the index of a column 
   * of type %G_TYPE_INT in the model. 
   *
   * The values of that column are used to determine how many rows a value 
   * in the list will span. Therefore, the values in the model column pointed 
   * to by this property must be greater than zero and not larger than wrap-width.
   *
   * Since: 2.4
   */
  g_object_class_install_property (object_class,
                                   PROP_ROW_SPAN_COLUMN,
                                   g_param_spec_int ("row-span-column",
                                                     P_("Row span column"),
                                                     P_("TreeModel column containing the row span values"),
                                                     -1,
                                                     G_MAXINT,
                                                     -1,
                                                     GTK_PARAM_READWRITE));


  /**
   * GtkComboBox:column-span-column:
   *
   * If this is set to a non-negative value, it must be the index of a column 
   * of type %G_TYPE_INT in the model. 
   *
   * The values of that column are used to determine how many columns a value 
   * in the list will span. 
   *
   * Since: 2.4
   */
  g_object_class_install_property (object_class,
                                   PROP_COLUMN_SPAN_COLUMN,
                                   g_param_spec_int ("column-span-column",
                                                     P_("Column span column"),
                                                     P_("TreeModel column containing the column span values"),
                                                     -1,
                                                     G_MAXINT,
                                                     -1,
                                                     GTK_PARAM_READWRITE));


  /**
   * GtkComboBox:active:
   *
   * The item which is currently active. If the model is a non-flat treemodel,
   * and the active item is not an immediate child of the root of the tree,
   * this property has the value <literal>gtk_tree_path_get_indices (path)[0]</literal>,
   * where <literal>path</literal> is the #GtkTreePath of the active item.
   *
   * Since: 2.4
   */
  g_object_class_install_property (object_class,
                                   PROP_ACTIVE,
                                   g_param_spec_int ("active",
                                                     P_("Active item"),
                                                     P_("The item which is currently active"),
                                                     -1,
                                                     G_MAXINT,
                                                     -1,
                                                     GTK_PARAM_READWRITE));

  /**
   * GtkComboBox:add-tearoffs:
   *
   * The add-tearoffs property controls whether generated menus 
   * have tearoff menu items. 
   *
   * Note that this only affects menu style combo boxes.
   *
   * Since: 2.6
   */
  g_object_class_install_property (object_class,
                                   PROP_ADD_TEAROFFS,
				   g_param_spec_boolean ("add-tearoffs",
							 P_("Add tearoffs to menus"),
							 P_("Whether dropdowns should have a tearoff menu item"),
							 FALSE,
							 GTK_PARAM_READWRITE));
  
  /**
   * GtkComboBox:has-frame:
   *
   * The has-frame property controls whether a frame
   * is drawn around the entry.
   *
   * Since: 2.6
   */
  g_object_class_install_property (object_class,
                                   PROP_HAS_FRAME,
				   g_param_spec_boolean ("has-frame",
							 P_("Has Frame"),
							 P_("Whether the combo box draws a frame around the child"),
							 TRUE,
							 GTK_PARAM_READWRITE));
  
  g_object_class_install_property (object_class,
                                   PROP_FOCUS_ON_CLICK,
                                   g_param_spec_boolean ("focus-on-click",
							 P_("Focus on click"),
							 P_("Whether the combo box grabs focus when it is clicked with the mouse"),
							 TRUE,
							 GTK_PARAM_READWRITE));

  /**
   * GtkComboBox:tearoff-title:
   *
   * A title that may be displayed by the window manager 
   * when the popup is torn-off.
   *
   * Since: 2.10
   */
  g_object_class_install_property (object_class,
                                   PROP_TEAROFF_TITLE,
                                   g_param_spec_string ("tearoff-title",
                                                        P_("Tearoff Title"),
                                                        P_("A title that may be displayed by the window manager when the popup is torn-off"),
                                                        "",
                                                        GTK_PARAM_READWRITE));


  /**
   * GtkComboBox:popup-shown:
   *
   * Whether the combo boxes dropdown is popped up. 
   * Note that this property is mainly useful, because
   * it allows you to connect to notify::popup-shown.
   *
   * Since: 2.10
   */
  g_object_class_install_property (object_class,
                                   PROP_POPUP_SHOWN,
                                   g_param_spec_boolean ("popup-shown",
                                                         P_("Popup shown"),
                                                         P_("Whether the combo's dropdown is shown"),
                                                         FALSE,
                                                         GTK_PARAM_READABLE));
  
  gtk_widget_class_install_style_property (widget_class,
                                           g_param_spec_boolean ("appears-as-list",
                                                                 P_("Appears as list"),
                                                                 P_("Whether dropdowns should look like lists rather than menus"),
                                                                 FALSE,
                                                                 GTK_PARAM_READABLE));

  /**
   * GtkComboBox:arrow-size:
   *
   * Sets the minimum size of the arrow in the combo box.  Note
   * that the arrow size is coupled to the font size, so in case
   * a larger font is used, the arrow will be larger than set
   * by arrow size.
   *
   * Since: 2.12
   */
  gtk_widget_class_install_style_property (widget_class,
					   g_param_spec_int ("arrow-size",
							     P_("Arrow Size"),
							     P_("The minimum size of the arrow in the combo box"),
							     0,
							     G_MAXINT,
							     15,
							     GTK_PARAM_READABLE));
  
#ifdef MAEMO_CHANGES
  /**
   * GtkComboBox:autodimmed-button:
   *
   * When TRUE, automatically dims the button if the list is empty.
   *
   * Since: maemo 1.0
   * Stability: Unstable
   */
  gtk_widget_class_install_style_property (widget_class,
                                           g_param_spec_boolean ("autodimmed-button",
                                                                 P_("Autodimmed button"),
                                                                 P_("Automatically dims the button if the list is empty"),
                                                                 FALSE,
                                                                 GTK_PARAM_READABLE));
#endif /* MAEMO_CHANGES */
  
  g_type_class_add_private (object_class, sizeof (GtkComboBoxPrivate));
}

static void
gtk_combo_box_cell_layout_init (GtkCellLayoutIface *iface)
{
  iface->pack_start = gtk_combo_box_cell_layout_pack_start;
  iface->pack_end = gtk_combo_box_cell_layout_pack_end;
  iface->clear = gtk_combo_box_cell_layout_clear;
  iface->add_attribute = gtk_combo_box_cell_layout_add_attribute;
  iface->set_cell_data_func = gtk_combo_box_cell_layout_set_cell_data_func;
  iface->clear_attributes = gtk_combo_box_cell_layout_clear_attributes;
  iface->reorder = gtk_combo_box_cell_layout_reorder;
}

static void
gtk_combo_box_cell_editable_init (GtkCellEditableIface *iface)
{
  iface->start_editing = gtk_combo_box_start_editing;
}

static void
gtk_combo_box_init (GtkComboBox *combo_box)
{
  combo_box->priv = GTK_COMBO_BOX_GET_PRIVATE (combo_box);

  combo_box->priv->cell_view = gtk_cell_view_new ();
  gtk_widget_set_parent (combo_box->priv->cell_view, GTK_WIDGET (combo_box));
  GTK_BIN (combo_box)->child = combo_box->priv->cell_view;
  gtk_widget_show (combo_box->priv->cell_view);

  combo_box->priv->width = 0;
  combo_box->priv->height = 0;
  combo_box->priv->wrap_width = 0;

  combo_box->priv->active_row = NULL;
  combo_box->priv->col_column = -1;
  combo_box->priv->row_column = -1;

  combo_box->priv->popup_shown = FALSE;
  combo_box->priv->add_tearoffs = FALSE;
  combo_box->priv->has_frame = TRUE;
  combo_box->priv->is_cell_renderer = FALSE;
  combo_box->priv->editing_canceled = FALSE;
  combo_box->priv->auto_scroll = FALSE;
  combo_box->priv->focus_on_click = TRUE;

  gtk_combo_box_check_appearance (combo_box);
}

static void
gtk_combo_box_set_property (GObject      *object,
                            guint         prop_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  GtkComboBox *combo_box = GTK_COMBO_BOX (object);

  switch (prop_id)
    {
      case PROP_MODEL:
        gtk_combo_box_set_model (combo_box, g_value_get_object (value));
        break;

      case PROP_WRAP_WIDTH:
        gtk_combo_box_set_wrap_width (combo_box, g_value_get_int (value));
        break;

      case PROP_ROW_SPAN_COLUMN:
        gtk_combo_box_set_row_span_column (combo_box, g_value_get_int (value));
        break;

      case PROP_COLUMN_SPAN_COLUMN:
        gtk_combo_box_set_column_span_column (combo_box, g_value_get_int (value));
        break;

      case PROP_ACTIVE:
        gtk_combo_box_set_active (combo_box, g_value_get_int (value));
        break;

      case PROP_ADD_TEAROFFS:
        gtk_combo_box_set_add_tearoffs (combo_box, g_value_get_boolean (value));
        break;

      case PROP_HAS_FRAME:
        combo_box->priv->has_frame = g_value_get_boolean (value);
        break;

      case PROP_FOCUS_ON_CLICK:
	gtk_combo_box_set_focus_on_click (combo_box, 
					  g_value_get_boolean (value));
        break;

      case PROP_TEAROFF_TITLE:
	gtk_combo_box_set_title (combo_box, g_value_get_string (value));
        break;

      case PROP_POPUP_SHOWN:
        if (g_value_get_boolean (value))
          {
            gtk_combo_box_popup (combo_box);
          }
        else
          {
            gtk_combo_box_popdown (combo_box);
          }
        break;

      default:
        break;
    }
}

static void
gtk_combo_box_get_property (GObject    *object,
                            guint       prop_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  GtkComboBox *combo_box = GTK_COMBO_BOX (object);

  switch (prop_id)
    {
      case PROP_MODEL:
        g_value_set_object (value, combo_box->priv->model);
        break;

      case PROP_WRAP_WIDTH:
        g_value_set_int (value, combo_box->priv->wrap_width);
        break;

      case PROP_ROW_SPAN_COLUMN:
        g_value_set_int (value, combo_box->priv->row_column);
        break;

      case PROP_COLUMN_SPAN_COLUMN:
        g_value_set_int (value, combo_box->priv->col_column);
        break;

      case PROP_ACTIVE:
        g_value_set_int (value, gtk_combo_box_get_active (combo_box));
        break;

      case PROP_ADD_TEAROFFS:
        g_value_set_boolean (value, gtk_combo_box_get_add_tearoffs (combo_box));
        break;

      case PROP_HAS_FRAME:
        g_value_set_boolean (value, combo_box->priv->has_frame);
        break;

      case PROP_FOCUS_ON_CLICK:
        g_value_set_boolean (value, combo_box->priv->focus_on_click);
        break;

      case PROP_TEAROFF_TITLE:
        g_value_set_string (value, gtk_combo_box_get_title (combo_box));
        break;

      case PROP_POPUP_SHOWN:
        g_value_set_boolean (value, combo_box->priv->popup_shown);
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
gtk_combo_box_state_changed (GtkWidget    *widget,
			     GtkStateType  previous)
{
  GtkComboBox *combo_box = GTK_COMBO_BOX (widget);

  if (GTK_WIDGET_REALIZED (widget))
    {
      if (combo_box->priv->tree_view && combo_box->priv->cell_view)
	gtk_cell_view_set_background_color (GTK_CELL_VIEW (combo_box->priv->cell_view), 
					    &widget->style->base[GTK_WIDGET_STATE (widget)]);
    }

  gtk_widget_queue_draw (widget);
}

static void
gtk_combo_box_button_state_changed (GtkWidget    *widget,
				    GtkStateType  previous,
				    gpointer      data)
{
  GtkComboBox *combo_box = GTK_COMBO_BOX (data);

  if (GTK_WIDGET_REALIZED (widget))
    {
      if (!combo_box->priv->tree_view && combo_box->priv->cell_view)
	{
	  if ((GTK_WIDGET_STATE (widget) == GTK_STATE_INSENSITIVE) !=
	      (GTK_WIDGET_STATE (combo_box->priv->cell_view) == GTK_STATE_INSENSITIVE))
	    gtk_widget_set_sensitive (combo_box->priv->cell_view, GTK_WIDGET_SENSITIVE (widget));
	  
	  gtk_widget_set_state (combo_box->priv->cell_view, 
				GTK_WIDGET_STATE (widget));
	  
	}
    }

  gtk_widget_queue_draw (widget);
}

static void
gtk_combo_box_check_appearance (GtkComboBox *combo_box)
{
  gboolean appears_as_list;

  /* if wrap_width > 0, then we are in grid-mode and forced to use
   * unix style
   */
  if (combo_box->priv->wrap_width)
    appears_as_list = FALSE;
  else
    gtk_widget_style_get (GTK_WIDGET (combo_box),
			  "appears-as-list", &appears_as_list,
			  NULL);

  if (appears_as_list)
    {
      /* Destroy all the menu mode widgets, if they exist. */
      if (GTK_IS_MENU (combo_box->priv->popup_widget))
	gtk_combo_box_menu_destroy (combo_box);

      /* Create the list mode widgets, if they don't already exist. */
      if (!GTK_IS_TREE_VIEW (combo_box->priv->tree_view))
	gtk_combo_box_list_setup (combo_box);
    }
  else
    {
      /* Destroy all the list mode widgets, if they exist. */
      if (GTK_IS_TREE_VIEW (combo_box->priv->tree_view))
	gtk_combo_box_list_destroy (combo_box);

      /* Create the menu mode widgets, if they don't already exist. */
      if (!GTK_IS_MENU (combo_box->priv->popup_widget))
	gtk_combo_box_menu_setup (combo_box, TRUE);
    }
}

#ifdef MAEMO_CHANGES
static void
maemo_check_autodim (GtkComboBox *combo_box)
{
  if (combo_box->priv->button)
    {
      GtkTreeModel *model = combo_box->priv->model;
      GtkTreeIter iter;
      gboolean autodimmed_button = TRUE;
      gtk_widget_style_get (GTK_WIDGET (combo_box), "autodimmed-button", &autodimmed_button, NULL);
      if (autodimmed_button && (!model || !gtk_tree_model_get_iter_first (model, &iter)))
        gtk_widget_set_sensitive (combo_box->priv->button, FALSE);
      else
        gtk_widget_set_sensitive (combo_box->priv->button, TRUE);
    }
}
#endif /* MAEMO_CHANGES */

static void
gtk_combo_box_style_set (GtkWidget *widget,
                         GtkStyle  *previous)
{
  GtkComboBox *combo_box = GTK_COMBO_BOX (widget);

  gtk_combo_box_check_appearance (combo_box);

  if (combo_box->priv->tree_view && combo_box->priv->cell_view)
    gtk_cell_view_set_background_color (GTK_CELL_VIEW (combo_box->priv->cell_view), 
					&widget->style->base[GTK_WIDGET_STATE (widget)]);

#ifdef MAEMO_CHANGES
  /* Handle the dimmed state of the button in regards whether
   * the list is empty or not. This has to be done here because
   * in the callback functions of GtkTreeModel the button widget
   * may have not yet been set. However, we repeat this stuff in
   * those functions, because later the button will be set and
   * we want to update our state.
   */
  maemo_check_autodim (combo_box);
#endif /* MAEMO_CHANGES */
}

static void
gtk_combo_box_button_toggled (GtkWidget *widget,
                              gpointer   data)
{
  GtkComboBox *combo_box = GTK_COMBO_BOX (data);

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
    {
      if (!combo_box->priv->popup_in_progress)
        gtk_combo_box_popup (combo_box);
    }
  else
    gtk_combo_box_popdown (combo_box);
}

static void
gtk_combo_box_add (GtkContainer *container,
                   GtkWidget    *widget)
{
  GtkComboBox *combo_box = GTK_COMBO_BOX (container);

  if (combo_box->priv->cell_view && combo_box->priv->cell_view->parent)
    {
      gtk_widget_unparent (combo_box->priv->cell_view);
      GTK_BIN (container)->child = NULL;
      gtk_widget_queue_resize (GTK_WIDGET (container));
    }
  
  gtk_widget_set_parent (widget, GTK_WIDGET (container));
  GTK_BIN (container)->child = widget;

  if (combo_box->priv->cell_view &&
      widget != combo_box->priv->cell_view)
    {
      /* since the cell_view was unparented, it's gone now */
      combo_box->priv->cell_view = NULL;

      if (!combo_box->priv->tree_view && combo_box->priv->separator)
        {
	  gtk_container_remove (GTK_CONTAINER (combo_box->priv->separator->parent),
				combo_box->priv->separator);
	  combo_box->priv->separator = NULL;

          gtk_widget_queue_resize (GTK_WIDGET (container));
        }
      else if (combo_box->priv->cell_view_frame)
        {
          gtk_widget_unparent (combo_box->priv->cell_view_frame);
          combo_box->priv->cell_view_frame = NULL;
          combo_box->priv->box = NULL;
        }
    }

#ifdef MAEMO_CHANGES
  /* Hildon: propagate the insensitive-press */
  if (GTK_BIN (container)->child)
    g_signal_connect_swapped (GTK_BIN (container)->child, "insensitive-press",
			      G_CALLBACK (gtk_widget_insensitive_press), combo_box);
#endif /* MAEMO_CHANGES */
}

static void
gtk_combo_box_remove (GtkContainer *container,
		      GtkWidget    *widget)
{
  GtkComboBox *combo_box = GTK_COMBO_BOX (container);
  GtkTreePath *path;
  gboolean appears_as_list;

  if (widget == combo_box->priv->cell_view)
    combo_box->priv->cell_view = NULL;

#ifdef MAEMO_CHANGES
  /* Hildon: stop propagating the insensitive-press */
  g_signal_handlers_disconnect_by_func (widget, G_CALLBACK (gtk_widget_insensitive_press), combo_box);
#endif /* MAEMO_CHANGES */

  gtk_widget_unparent (widget);
  GTK_BIN (container)->child = NULL;

  if (GTK_OBJECT_FLAGS (combo_box) & GTK_IN_DESTRUCTION)
    return;

  gtk_widget_queue_resize (GTK_WIDGET (container));

  if (!combo_box->priv->tree_view)
    appears_as_list = FALSE;
  else
    appears_as_list = TRUE;
  
  if (appears_as_list)
    gtk_combo_box_list_destroy (combo_box);
  else if (GTK_IS_MENU (combo_box->priv->popup_widget))
    {
      gtk_combo_box_menu_destroy (combo_box);
      gtk_menu_detach (GTK_MENU (combo_box->priv->popup_widget));
      combo_box->priv->popup_widget = NULL;
    }

  if (!combo_box->priv->cell_view)
    {
      combo_box->priv->cell_view = gtk_cell_view_new ();
      gtk_widget_set_parent (combo_box->priv->cell_view, GTK_WIDGET (container));
      GTK_BIN (container)->child = combo_box->priv->cell_view;
      
      gtk_widget_show (combo_box->priv->cell_view);
      gtk_cell_view_set_model (GTK_CELL_VIEW (combo_box->priv->cell_view),
			       combo_box->priv->model);
      gtk_combo_box_sync_cells (combo_box, GTK_CELL_LAYOUT (combo_box->priv->cell_view));
    }


  if (appears_as_list)
    gtk_combo_box_list_setup (combo_box); 
  else
    gtk_combo_box_menu_setup (combo_box, TRUE);

  if (gtk_tree_row_reference_valid (combo_box->priv->active_row))
    {
      path = gtk_tree_row_reference_get_path (combo_box->priv->active_row);
      gtk_combo_box_set_active_internal (combo_box, path);
      gtk_tree_path_free (path);
    }
  else
    gtk_combo_box_set_active_internal (combo_box, NULL);
}

static ComboCellInfo *
gtk_combo_box_get_cell_info (GtkComboBox     *combo_box,
                             GtkCellRenderer *cell)
{
  GSList *i;

  for (i = combo_box->priv->cells; i; i = i->next)
    {
      ComboCellInfo *info = (ComboCellInfo *)i->data;

      if (info && info->cell == cell)
        return info;
    }

  return NULL;
}

static void
gtk_combo_box_menu_show (GtkWidget *menu,
                         gpointer   user_data)
{
  GtkComboBox *combo_box = GTK_COMBO_BOX (user_data);

  gtk_combo_box_child_show (menu, user_data);

  combo_box->priv->popup_in_progress = TRUE;
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (combo_box->priv->button),
                                TRUE);
  combo_box->priv->popup_in_progress = FALSE;
}

static void
gtk_combo_box_menu_hide (GtkWidget *menu,
                         gpointer   user_data)
{
  GtkComboBox *combo_box = GTK_COMBO_BOX (user_data);

  gtk_combo_box_child_hide(menu,user_data);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (combo_box->priv->button),
                                FALSE);
}

static void
gtk_combo_box_detacher (GtkWidget *widget,
			GtkMenu	  *menu)
{
  GtkComboBox *combo_box;

  g_return_if_fail (GTK_IS_COMBO_BOX (widget));

  combo_box = GTK_COMBO_BOX (widget);
  g_return_if_fail (combo_box->priv->popup_widget == (GtkWidget*) menu);

  g_signal_handlers_disconnect_by_func (menu->toplevel,
					gtk_combo_box_menu_show,
					combo_box);
  g_signal_handlers_disconnect_by_func (menu->toplevel,
					gtk_combo_box_menu_hide,
					combo_box);
  
  combo_box->priv->popup_widget = NULL;
}

static void
gtk_combo_box_set_popup_widget (GtkComboBox *combo_box,
                                GtkWidget   *popup)
{
  if (GTK_IS_MENU (combo_box->priv->popup_widget))
    {
      gtk_menu_detach (GTK_MENU (combo_box->priv->popup_widget));
      combo_box->priv->popup_widget = NULL;
    }
  else if (combo_box->priv->popup_widget)
    {
      gtk_container_remove (GTK_CONTAINER (combo_box->priv->popup_frame),
                            combo_box->priv->popup_widget);
      g_object_unref (combo_box->priv->popup_widget);
      combo_box->priv->popup_widget = NULL;
    }

  if (GTK_IS_MENU (popup))
    {
      if (combo_box->priv->popup_window)
        {
          gtk_widget_destroy (combo_box->priv->popup_window);
          combo_box->priv->popup_window = NULL;
	  combo_box->priv->popup_frame = NULL;
        }

      combo_box->priv->popup_widget = popup;

      /* 
       * Note that we connect to show/hide on the toplevel, not the
       * menu itself, since the menu is not shown/hidden when it is
       * popped up while torn-off.
       */
      g_signal_connect (GTK_MENU (popup)->toplevel, "show",
                        G_CALLBACK (gtk_combo_box_menu_show), combo_box);
      g_signal_connect (GTK_MENU (popup)->toplevel, "hide",
                        G_CALLBACK (gtk_combo_box_menu_hide), combo_box);

      gtk_menu_attach_to_widget (GTK_MENU (popup),
				 GTK_WIDGET (combo_box),
				 gtk_combo_box_detacher);
    }
  else
    {
      if (!combo_box->priv->popup_window)
        {
	  GtkWidget *toplevel;
	  
          combo_box->priv->popup_window = gtk_window_new (GTK_WINDOW_POPUP);
          gtk_widget_set_name (combo_box->priv->popup_window, "gtk-combobox-popup-window");

	  gtk_window_set_type_hint (GTK_WINDOW (combo_box->priv->popup_window),
				    GDK_WINDOW_TYPE_HINT_COMBO);

	  g_signal_connect (GTK_WINDOW(combo_box->priv->popup_window),"show",
			    G_CALLBACK (gtk_combo_box_child_show),
			    combo_box);
	  g_signal_connect (GTK_WINDOW(combo_box->priv->popup_window),"hide",
			    G_CALLBACK (gtk_combo_box_child_hide),
			    combo_box);
	  g_signal_connect (GTK_WINDOW(combo_box->priv->popup_window),"delete_event",
			    G_CALLBACK (gtk_combo_box_child_delete_event),
			    combo_box);
  	  
	  toplevel = gtk_widget_get_toplevel (GTK_WIDGET (combo_box));
	  if (GTK_IS_WINDOW (toplevel))
	    {
	      gtk_window_group_add_window (gtk_window_get_group (GTK_WINDOW (toplevel)), 
					   GTK_WINDOW (combo_box->priv->popup_window));
	      gtk_window_set_transient_for (GTK_WINDOW (combo_box->priv->popup_window),
					    GTK_WINDOW (toplevel));
	    }

	  gtk_window_set_resizable (GTK_WINDOW (combo_box->priv->popup_window), FALSE);
          gtk_window_set_screen (GTK_WINDOW (combo_box->priv->popup_window),
                                 gtk_widget_get_screen (GTK_WIDGET (combo_box)));

          combo_box->priv->popup_frame = gtk_frame_new (NULL);
          
#ifdef MAEMO_CHANGES
          /* Hildon: undo the changes made to GtkFrame defaults */
          gtk_container_set_border_width (GTK_CONTAINER (combo_box->priv->popup_frame), 0);
#endif /* MAEMO_CHANGES */

          gtk_frame_set_shadow_type (GTK_FRAME (combo_box->priv->popup_frame),
                                     GTK_SHADOW_NONE);
          gtk_container_add (GTK_CONTAINER (combo_box->priv->popup_window),
                             combo_box->priv->popup_frame);

          gtk_widget_show (combo_box->priv->popup_frame);

	  combo_box->priv->scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	  
	  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (combo_box->priv->scrolled_window),
					  GTK_POLICY_NEVER,
					  GTK_POLICY_NEVER);
	  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (combo_box->priv->scrolled_window),
					       GTK_SHADOW_NONE);

          gtk_widget_show (combo_box->priv->scrolled_window);
	  
	  gtk_container_add (GTK_CONTAINER (combo_box->priv->popup_frame),
			     combo_box->priv->scrolled_window);
        }

      gtk_container_add (GTK_CONTAINER (combo_box->priv->scrolled_window),
                         popup);

      gtk_widget_show (popup);
      g_object_ref (popup);
      combo_box->priv->popup_widget = popup;
    }
}

static void
gtk_combo_box_menu_position_below (GtkMenu  *menu,
				   gint     *x,
				   gint     *y,
				   gint     *push_in,
				   gpointer  user_data)
{
  GtkComboBox *combo_box = GTK_COMBO_BOX (user_data);
  gint sx, sy;
  GtkWidget *child;
  GtkRequisition req;
  GdkScreen *screen;
  gint monitor_num;
  GdkRectangle monitor;
  
  /* FIXME: is using the size request here broken? */
   child = GTK_BIN (combo_box)->child;
   
   gdk_window_get_origin (child->window, &sx, &sy);
   
   if (GTK_WIDGET_NO_WINDOW (child))
      {
	sx += child->allocation.x;
	sy += child->allocation.y;
      }

   gtk_widget_size_request (GTK_WIDGET (menu), &req);

   if (gtk_widget_get_direction (GTK_WIDGET (combo_box)) == GTK_TEXT_DIR_LTR)
     *x = sx;
   else
     *x = sx + child->allocation.width - req.width;
   *y = sy;

  screen = gtk_widget_get_screen (GTK_WIDGET (combo_box));
  monitor_num = gdk_screen_get_monitor_at_window (screen, 
						  GTK_WIDGET (combo_box)->window);
  gdk_screen_get_monitor_geometry (screen, monitor_num, &monitor);
  
  if (*x < monitor.x)
    *x = monitor.x;
  else if (*x + req.width > monitor.x + monitor.width)
    *x = monitor.x + monitor.width - req.width;
  
  if (monitor.y + monitor.height - *y - child->allocation.height >= req.height)
    *y += child->allocation.height;
  else if (*y - monitor.y >= req.height)
    *y -= req.height;
  else if (monitor.y + monitor.height - *y - child->allocation.height > *y - monitor.y) 
    *y += child->allocation.height;
  else
    *y -= req.height;

   *push_in = FALSE;
}

static void
gtk_combo_box_menu_position_over (GtkMenu  *menu,
				  gint     *x,
				  gint     *y,
				  gboolean *push_in,
				  gpointer  user_data)
{
  GtkComboBox *combo_box;
  GtkWidget *active;
  GtkWidget *child;
  GtkWidget *widget;
  GtkRequisition requisition;
  GList *children;
  gint screen_width;
  gint menu_xpos;
  gint menu_ypos;
  gint menu_width;

  g_return_if_fail (GTK_IS_COMBO_BOX (user_data));
  
  combo_box = GTK_COMBO_BOX (user_data);
  widget = GTK_WIDGET (combo_box);

  gtk_widget_get_child_requisition (GTK_WIDGET (menu), &requisition);
  menu_width = requisition.width;

  active = gtk_menu_get_active (GTK_MENU (combo_box->priv->popup_widget));
  gdk_window_get_origin (widget->window, &menu_xpos, &menu_ypos);

  menu_xpos += widget->allocation.x;
  menu_ypos += widget->allocation.y + widget->allocation.height / 2 - 2;

  if (active != NULL)
    {
      gtk_widget_get_child_requisition (active, &requisition);
      menu_ypos -= requisition.height / 2;
    }

  children = GTK_MENU_SHELL (combo_box->priv->popup_widget)->children;
  while (children)
    {
      child = children->data;

      if (active == child)
	break;

      if (GTK_WIDGET_VISIBLE (child))
	{
	  gtk_widget_get_child_requisition (child, &requisition);
	  menu_ypos -= requisition.height;
	}

      children = children->next;
    }

  if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL)
    menu_xpos = menu_xpos + widget->allocation.width - menu_width;

  /* Clamp the position on screen */
  screen_width = gdk_screen_get_width (gtk_widget_get_screen (widget));
  
  if (menu_xpos < 0)
    menu_xpos = 0;
  else if ((menu_xpos + menu_width) > screen_width)
    menu_xpos -= ((menu_xpos + menu_width) - screen_width);

  *x = menu_xpos;
  *y = menu_ypos;

  *push_in = TRUE;
}

static void
gtk_combo_box_menu_position (GtkMenu  *menu,
			     gint     *x,
			     gint     *y,
			     gint     *push_in,
			     gpointer  user_data)
{
  GtkComboBox *combo_box;
  GtkWidget *menu_item;

  combo_box = GTK_COMBO_BOX (user_data);

  if (combo_box->priv->wrap_width > 0 || combo_box->priv->cell_view == NULL)	
    gtk_combo_box_menu_position_below (menu, x, y, push_in, user_data);
  else
    {
      /* FIXME handle nested menus better */
      menu_item = gtk_menu_get_active (GTK_MENU (combo_box->priv->popup_widget));
      if (menu_item)
	gtk_menu_shell_select_item (GTK_MENU_SHELL (combo_box->priv->popup_widget), 
				    menu_item);

      gtk_combo_box_menu_position_over (menu, x, y, push_in, user_data);
    }

}

static void
gtk_combo_box_list_position (GtkComboBox *combo_box, 
			     gint        *x, 
			     gint        *y, 
			     gint        *width,
			     gint        *height)
{
  GtkWidget *sample;
  GdkScreen *screen;
  gint monitor_num;
  GdkRectangle monitor;
  GtkRequisition popup_req;
  GtkPolicyType hpolicy, vpolicy;
  
  /* under windows, the drop down list is as wide as the combo box itself.
     see bug #340204 */
  sample = combo_box;

  gdk_window_get_origin (sample->window, x, y);

  if (GTK_WIDGET_NO_WINDOW (sample))
    {
      *x += sample->allocation.x;
      *y += sample->allocation.y;
    }
  
  *width = sample->allocation.width;
  
  if (combo_box->priv->cell_view_frame && combo_box->priv->has_frame)
    {
       *x -= GTK_CONTAINER (combo_box->priv->cell_view_frame)->border_width +
	     GTK_WIDGET (combo_box->priv->cell_view_frame)->style->xthickness;
       *width += 2 * (GTK_CONTAINER (combo_box->priv->cell_view_frame)->border_width +
            GTK_WIDGET (combo_box->priv->cell_view_frame)->style->xthickness);
    }

  hpolicy = vpolicy = GTK_POLICY_NEVER;
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (combo_box->priv->scrolled_window),
				  hpolicy, vpolicy);
  gtk_widget_size_request (combo_box->priv->popup_frame, &popup_req);

  if (popup_req.width > *width)
    {
      hpolicy = GTK_POLICY_ALWAYS;
      gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (combo_box->priv->scrolled_window),
				      hpolicy, vpolicy);
      gtk_widget_size_request (combo_box->priv->popup_frame, &popup_req);
    }

  *height = popup_req.height;

  screen = gtk_widget_get_screen (GTK_WIDGET (combo_box));
  monitor_num = gdk_screen_get_monitor_at_window (screen, 
						  GTK_WIDGET (combo_box)->window);
  gdk_screen_get_monitor_geometry (screen, monitor_num, &monitor);

  if (*x < monitor.x)
    *x = monitor.x;
  else if (*x + *width > monitor.x + monitor.width)
    *x = monitor.x + monitor.width - *width;
  
  if (*y + sample->allocation.height + *height <= monitor.y + monitor.height)
    *y += sample->allocation.height;
  else if (*y - *height >= monitor.y)
    *y -= *height;
  else if (monitor.y + monitor.height - (*y + sample->allocation.height) > *y - monitor.y)
    {
      *y += sample->allocation.height;
      *height = monitor.y + monitor.height - *y;
    }
  else 
    {
      *height = *y - monitor.y;
      *y = monitor.y;
    }

  if (popup_req.height > *height)
    {
      vpolicy = GTK_POLICY_ALWAYS;
      
      gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (combo_box->priv->scrolled_window),
				      hpolicy, vpolicy);
    }
} 

static gboolean
cell_view_is_sensitive (GtkCellView *cell_view)
{
  GList *cells, *list;
  gboolean sensitive;
  
  cells = gtk_cell_view_get_cell_renderers (cell_view);

  sensitive = FALSE;
  list = cells;
  while (list)
    {
      g_object_get (list->data, "sensitive", &sensitive, NULL);
      
      if (sensitive)
	break;

      list = list->next;
    }
  g_list_free (cells);

  return sensitive;
}

static gboolean
tree_column_row_is_sensitive (GtkComboBox *combo_box,
			      GtkTreeIter *iter)
{
  GList *cells, *list;
  gboolean sensitive;

  if (!combo_box->priv->column)
    return TRUE;

  if (combo_box->priv->row_separator_func)
    {
      if ((*combo_box->priv->row_separator_func) (combo_box->priv->model, iter,
						  combo_box->priv->row_separator_data))
	return FALSE;
    }

  gtk_tree_view_column_cell_set_cell_data (combo_box->priv->column,
					   combo_box->priv->model,
					   iter, FALSE, FALSE);

  cells = gtk_tree_view_column_get_cell_renderers (combo_box->priv->column);

  sensitive = FALSE;
  list = cells;
  while (list)
    {
      g_object_get (list->data, "sensitive", &sensitive, NULL);
      
      if (sensitive)
	break;

      list = list->next;
    }
  g_list_free (cells);

  return sensitive;
}

static void
update_menu_sensitivity (GtkComboBox *combo_box,
			 GtkWidget   *menu)
{
  GList *children, *child;
  GtkWidget *item, *submenu, *separator;
  GtkWidget *cell_view;
  gboolean sensitive;

  if (!combo_box->priv->model)
    return;

  children = gtk_container_get_children (GTK_CONTAINER (menu));

  for (child = children; child; child = child->next)
    {
      item = GTK_WIDGET (child->data);
      cell_view = GTK_BIN (item)->child;

      if (!GTK_IS_CELL_VIEW (cell_view))
	continue;
      
      submenu = gtk_menu_item_get_submenu (GTK_MENU_ITEM (item));
      if (submenu != NULL)
	{
	  gtk_widget_set_sensitive (item, TRUE);
	  update_menu_sensitivity (combo_box, submenu);
	}
      else
	{
	  sensitive = cell_view_is_sensitive (GTK_CELL_VIEW (cell_view));

	  if (menu != combo_box->priv->popup_widget && child == children)
	    {
	      separator = GTK_WIDGET (child->next->data);
	      g_object_set (item, "visible", sensitive, NULL);
	      g_object_set (separator, "visible", sensitive, NULL);
	    }
	  else
	    gtk_widget_set_sensitive (item, sensitive);
	}
    }

  g_list_free (children);
}

static void 
gtk_combo_box_menu_popup (GtkComboBox *combo_box,
			  guint        button, 
			  guint32      activate_time)
{
  GtkTreePath *path;
  gint active_item;
  GtkRequisition requisition;
  gint width;
  
  update_menu_sensitivity (combo_box, combo_box->priv->popup_widget);

  active_item = -1;
  if (gtk_tree_row_reference_valid (combo_box->priv->active_row))
    {
      path = gtk_tree_row_reference_get_path (combo_box->priv->active_row);
      active_item = gtk_tree_path_get_indices (path)[0];
      gtk_tree_path_free (path);
      
      if (combo_box->priv->add_tearoffs)
	active_item++;
    }

  /* FIXME handle nested menus better */
  gtk_menu_set_active (GTK_MENU (combo_box->priv->popup_widget), active_item);
  
  if (combo_box->priv->wrap_width == 0)
    {
      width = GTK_WIDGET (combo_box)->allocation.width;
      gtk_widget_set_size_request (combo_box->priv->popup_widget, -1, -1);
      gtk_widget_size_request (combo_box->priv->popup_widget, &requisition);
      
      gtk_widget_set_size_request (combo_box->priv->popup_widget,
				   MAX (width, requisition.width), -1);
    }
  
  gtk_menu_popup (GTK_MENU (combo_box->priv->popup_widget),
		  NULL, NULL,
		  gtk_combo_box_menu_position, combo_box,
		  button, activate_time);
}

static gboolean
popup_grab_on_window (GdkWindow *window,
		      guint32    activate_time,
		      gboolean   grab_keyboard)
{
  if ((gdk_pointer_grab (window, TRUE,
			 GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK |
			 GDK_POINTER_MOTION_MASK,
			 NULL, NULL, activate_time) == 0))
    {
      if (!grab_keyboard ||
	  gdk_keyboard_grab (window, TRUE,
			     activate_time) == 0)
	return TRUE;
      else
	{
	  gdk_display_pointer_ungrab (gdk_drawable_get_display (window),
				      activate_time);
	  return FALSE;
	}
    }

  return FALSE;
}

/**
 * gtk_combo_box_popup:
 * @combo_box: a #GtkComboBox
 * 
 * Pops up the menu or dropdown list of @combo_box. 
 *
 * This function is mostly intended for use by accessibility technologies;
 * applications should have little use for it.
 *
 * Since: 2.4
 **/
void
gtk_combo_box_popup (GtkComboBox *combo_box)
{
  g_return_if_fail (GTK_IS_COMBO_BOX (combo_box));

  g_signal_emit (combo_box, combo_box_signals[POPUP], 0);
}

static void
gtk_combo_box_real_popup (GtkComboBox *combo_box)
{
  gint x, y, width, height;
  GtkTreePath *path = NULL, *ppath;
  GtkWidget *toplevel;

  if (!GTK_WIDGET_REALIZED (combo_box))
    return;

  if (GTK_WIDGET_MAPPED (combo_box->priv->popup_widget))
    return;

  if (GTK_IS_MENU (combo_box->priv->popup_widget))
    {
      gtk_combo_box_menu_popup (combo_box, 0, 0);
      return;
    }

  toplevel = gtk_widget_get_toplevel (GTK_WIDGET (combo_box));
  if (GTK_IS_WINDOW (toplevel))
    gtk_window_group_add_window (gtk_window_get_group (GTK_WINDOW (toplevel)), 
				 GTK_WINDOW (combo_box->priv->popup_window));

  gtk_widget_show_all (combo_box->priv->popup_frame);
  gtk_combo_box_list_position (combo_box, &x, &y, &width, &height);
  
  gtk_widget_set_size_request (combo_box->priv->popup_window, width, height);  
  gtk_window_move (GTK_WINDOW (combo_box->priv->popup_window), x, y);

  if (gtk_tree_row_reference_valid (combo_box->priv->active_row))
    {
      path = gtk_tree_row_reference_get_path (combo_box->priv->active_row);
      ppath = gtk_tree_path_copy (path);
      if (gtk_tree_path_up (ppath))
	gtk_tree_view_expand_to_path (GTK_TREE_VIEW (combo_box->priv->tree_view),
				      ppath);
      gtk_tree_path_free (ppath);
    }
  gtk_tree_view_set_hover_expand (GTK_TREE_VIEW (combo_box->priv->tree_view), 
				  TRUE);
  
  /* popup */
  gtk_widget_show (combo_box->priv->popup_window);

  if (path)
    {
      gtk_tree_view_set_cursor (GTK_TREE_VIEW (combo_box->priv->tree_view),
                                path, NULL, FALSE);
      gtk_tree_path_free (path);
    }

  gtk_widget_grab_focus (combo_box->priv->popup_window);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (combo_box->priv->button),
                                TRUE);

  if (!GTK_WIDGET_HAS_FOCUS (combo_box->priv->tree_view))
    gtk_widget_grab_focus (combo_box->priv->tree_view);

  if (!popup_grab_on_window (combo_box->priv->popup_window->window,
			     GDK_CURRENT_TIME, TRUE))
    {
      gtk_widget_hide (combo_box->priv->popup_window);
      return;
    }

  gtk_grab_add (combo_box->priv->popup_window);
}

/**
 * gtk_combo_box_popdown:
 * @combo_box: a #GtkComboBox
 * 
 * Hides the menu or dropdown list of @combo_box.
 *
 * This function is mostly intended for use by accessibility technologies;
 * applications should have little use for it.
 *
 * Since: 2.4
 **/
void
gtk_combo_box_popdown (GtkComboBox *combo_box)
{
  g_return_if_fail (GTK_IS_COMBO_BOX (combo_box));

  if (GTK_IS_MENU (combo_box->priv->popup_widget))
    {
      gtk_menu_popdown (GTK_MENU (combo_box->priv->popup_widget));
      return;
    }

  if (!GTK_WIDGET_REALIZED (GTK_WIDGET (combo_box)))
    return;

  gtk_grab_remove (combo_box->priv->popup_window);
  gtk_widget_hide_all (combo_box->priv->popup_window);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (combo_box->priv->button),
                                FALSE);
}

static gint
gtk_combo_box_calc_requested_width (GtkComboBox *combo_box,
                                    GtkTreePath *path)
{
  gint padding;
  GtkRequisition req;

  if (combo_box->priv->cell_view)
    gtk_widget_style_get (combo_box->priv->cell_view,
                          "focus-line-width", &padding,
                          NULL);
  else
    padding = 0;

  /* add some pixels for good measure */
  padding += BONUS_PADDING;

  if (combo_box->priv->cell_view)
    gtk_cell_view_get_size_of_row (GTK_CELL_VIEW (combo_box->priv->cell_view),
                                   path, &req);
  else
    req.width = 0;

  return req.width + padding;
}

static void
gtk_combo_box_remeasure (GtkComboBox *combo_box)
{
  GtkTreeIter iter;
  GtkTreePath *path;

  if (!combo_box->priv->model ||
      !gtk_tree_model_get_iter_first (combo_box->priv->model, &iter))
    return;

  combo_box->priv->width = 0;
  combo_box->priv->height = 0;

  path = gtk_tree_path_new_from_indices (0, -1);

  do
    {
      GtkRequisition req;

      if (combo_box->priv->cell_view)
	gtk_cell_view_get_size_of_row (GTK_CELL_VIEW (combo_box->priv->cell_view), 
                                       path, &req);
      else
        {
          req.width = 0;
          req.height = 0;
        }

      combo_box->priv->width = MAX (combo_box->priv->width, req.width);
      combo_box->priv->height = MAX (combo_box->priv->height, req.height);

      gtk_tree_path_next (path);
    }
  while (gtk_tree_model_iter_next (combo_box->priv->model, &iter));

  gtk_tree_path_free (path);
}

static void
gtk_combo_box_size_request (GtkWidget      *widget,
                            GtkRequisition *requisition)
{
  gint width, height;
  gint focus_width, focus_pad;
  gint font_size;
  gint arrow_size;
  GtkRequisition bin_req;
  PangoContext *context;
  PangoFontMetrics *metrics;
  PangoFontDescription *font_desc;

  GtkComboBox *combo_box = GTK_COMBO_BOX (widget);
 
  /* common */
  gtk_widget_size_request (GTK_BIN (widget)->child, &bin_req);
  gtk_combo_box_remeasure (combo_box);
  bin_req.width = MAX (bin_req.width, combo_box->priv->width);
  bin_req.height = MAX (bin_req.height, combo_box->priv->height);

  gtk_widget_style_get (GTK_WIDGET (widget),
			"focus-line-width", &focus_width,
			"focus-padding", &focus_pad,
			"arrow-size", &arrow_size,
			NULL);

  font_desc = GTK_BIN (widget)->child->style->font_desc;
  context = gtk_widget_get_pango_context (widget);
  metrics = pango_context_get_metrics (context, font_desc,
				       pango_context_get_language (context));
  font_size = PANGO_PIXELS (pango_font_metrics_get_ascent (metrics) +
			    pango_font_metrics_get_descent (metrics));
  pango_font_metrics_unref (metrics);

  arrow_size = MAX (arrow_size, font_size);

  gtk_widget_set_size_request (combo_box->priv->arrow, arrow_size, arrow_size);

  if (!combo_box->priv->tree_view)
    {
      /* menu mode */

      if (combo_box->priv->cell_view)
        {
          GtkRequisition button_req, sep_req, arrow_req;
          gint border_width, xthickness, ythickness;

          gtk_widget_size_request (combo_box->priv->button, &button_req);
	  border_width = GTK_CONTAINER (combo_box)->border_width;
          xthickness = combo_box->priv->button->style->xthickness;
          ythickness = combo_box->priv->button->style->ythickness;

          bin_req.width = MAX (bin_req.width, combo_box->priv->width);
          bin_req.height = MAX (bin_req.height, combo_box->priv->height);

          gtk_widget_size_request (combo_box->priv->separator, &sep_req);
          gtk_widget_size_request (combo_box->priv->arrow, &arrow_req);

          height = MAX (sep_req.height, arrow_req.height);
          height = MAX (height, bin_req.height);

          width = bin_req.width + sep_req.width + arrow_req.width;

          height += 2*(border_width + ythickness + focus_width + focus_pad);
          width  += 2*(border_width + xthickness + focus_width + focus_pad);

          requisition->width = width;
          requisition->height = height;
        }
      else
        {
          GtkRequisition but_req;

          gtk_widget_size_request (combo_box->priv->button, &but_req);

          requisition->width = bin_req.width + but_req.width;
          requisition->height = MAX (bin_req.height, but_req.height);
        }
    }
  else
    {
      /* list mode */
      GtkRequisition button_req, frame_req;

      /* sample + frame */
      *requisition = bin_req;

      requisition->width += 2 * focus_width;
      
      if (combo_box->priv->cell_view_frame)
        {
	  gtk_widget_size_request (combo_box->priv->cell_view_frame, &frame_req);
	  if (combo_box->priv->has_frame)
	    {
	      requisition->width += 2 *
		(GTK_CONTAINER (combo_box->priv->cell_view_frame)->border_width +
		 GTK_WIDGET (combo_box->priv->cell_view_frame)->style->xthickness);
	      requisition->height += 2 *
		(GTK_CONTAINER (combo_box->priv->cell_view_frame)->border_width +
		 GTK_WIDGET (combo_box->priv->cell_view_frame)->style->ythickness);
	    }
        }

      /* the button */
      gtk_widget_size_request (combo_box->priv->button, &button_req);

      requisition->height = MAX (requisition->height, button_req.height);
      requisition->width += button_req.width;
    }
}

static void
gtk_combo_box_size_allocate (GtkWidget     *widget,
                             GtkAllocation *allocation)
{
  GtkComboBox *combo_box = GTK_COMBO_BOX (widget);
  gint focus_width, focus_pad;
  GtkAllocation child;
  GtkRequisition req;
  gboolean is_rtl = gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL;

  widget->allocation = *allocation;

  gtk_widget_style_get (GTK_WIDGET (widget),
			"focus-line-width", &focus_width,
			"focus-padding", &focus_pad,
			NULL);

  if (!combo_box->priv->tree_view)
    {
      if (combo_box->priv->cell_view)
        {
          gint border_width, xthickness, ythickness;
          gint width;

          /* menu mode */
          gtk_widget_size_allocate (combo_box->priv->button, allocation);

          /* set some things ready */
          border_width = GTK_CONTAINER (combo_box->priv->button)->border_width;
          xthickness = combo_box->priv->button->style->xthickness;
          ythickness = combo_box->priv->button->style->ythickness;

          child.x = allocation->x;
          child.y = allocation->y;
	  width = allocation->width;
	  child.height = allocation->height;

	  if (!combo_box->priv->is_cell_renderer)
	    {
	      child.x += border_width + xthickness + focus_width + focus_pad;
	      child.y += border_width + ythickness + focus_width + focus_pad;
	      width -= 2 * (child.x - allocation->x);
	      child.height -= 2 * (child.y - allocation->y);
	    }


          /* handle the children */
          gtk_widget_size_request (combo_box->priv->arrow, &req);
          child.width = req.width;
          if (!is_rtl)
            child.x += width - req.width;
	  child.width = MAX (1, child.width);
	  child.height = MAX (1, child.height);
          gtk_widget_size_allocate (combo_box->priv->arrow, &child);
          if (is_rtl)
            child.x += req.width;
          gtk_widget_size_request (combo_box->priv->separator, &req);
          child.width = req.width;
          if (!is_rtl)
            child.x -= req.width;
	  child.width = MAX (1, child.width);
	  child.height = MAX (1, child.height);
          gtk_widget_size_allocate (combo_box->priv->separator, &child);

          if (is_rtl)
            {
              child.x += req.width;
              child.width = allocation->x + allocation->width 
                - (border_width + xthickness + focus_width + focus_pad) 
		- child.x;
            }
          else 
            {
              child.width = child.x;
              child.x = allocation->x 
		+ border_width + xthickness + focus_width + focus_pad;
              child.width -= child.x;
            }

	  child.width = MAX (1, child.width);
	  child.height = MAX (1, child.height);
          gtk_widget_size_allocate (GTK_BIN (widget)->child, &child);
        }
      else
        {
          gtk_widget_size_request (combo_box->priv->button, &req);
          if (is_rtl)
            child.x = allocation->x;
          else
            child.x = allocation->x + allocation->width - req.width;
          child.y = allocation->y;
          child.width = req.width;
          child.height = allocation->height;
	  child.width = MAX (1, child.width);
	  child.height = MAX (1, child.height);
          gtk_widget_size_allocate (combo_box->priv->button, &child);

          if (is_rtl)
            child.x = allocation->x + req.width;
          else
            child.x = allocation->x;
          child.y = allocation->y;
          child.width = allocation->width - req.width;
	  child.width = MAX (1, child.width);
	  child.height = MAX (1, child.height);
          gtk_widget_size_allocate (GTK_BIN (widget)->child, &child);
        }
    }
  else
    {
      /* list mode */

      /* button */
      gtk_widget_size_request (combo_box->priv->button, &req);
      if (is_rtl)
        child.x = allocation->x;
      else
        child.x = allocation->x + allocation->width - req.width;
      child.y = allocation->y;
      child.width = req.width;
      child.height = allocation->height;
      child.width = MAX (1, child.width);
      child.height = MAX (1, child.height);
      gtk_widget_size_allocate (combo_box->priv->button, &child);

      /* frame */
      if (is_rtl)
        child.x = allocation->x + req.width;
      else
        child.x = allocation->x;
      child.y = allocation->y;
      child.width = allocation->width - req.width;
      child.height = allocation->height;

      if (combo_box->priv->cell_view_frame)
        {
	  child.width = MAX (1, child.width);
	  child.height = MAX (1, child.height);
          gtk_widget_size_allocate (combo_box->priv->cell_view_frame, &child);

          /* the sample */
	  if (combo_box->priv->has_frame)
	    {
	      child.x +=
		GTK_CONTAINER (combo_box->priv->cell_view_frame)->border_width +
		GTK_WIDGET (combo_box->priv->cell_view_frame)->style->xthickness;
	      child.y +=
		GTK_CONTAINER (combo_box->priv->cell_view_frame)->border_width +
		GTK_WIDGET (combo_box->priv->cell_view_frame)->style->ythickness;
	      child.width -= 2 * (
				  GTK_CONTAINER (combo_box->priv->cell_view_frame)->border_width +
				  GTK_WIDGET (combo_box->priv->cell_view_frame)->style->xthickness);
	      child.height -= 2 * (
				   GTK_CONTAINER (combo_box->priv->cell_view_frame)->border_width +
				   GTK_WIDGET (combo_box->priv->cell_view_frame)->style->ythickness);
	    }
        }
      
      child.width = MAX (1, child.width);
      child.height = MAX (1, child.height);
      gtk_widget_size_allocate (GTK_BIN (combo_box)->child, &child);
    }
}

static void
gtk_combo_box_unset_model (GtkComboBox *combo_box)
{
  if (combo_box->priv->model)
    {
      g_signal_handler_disconnect (combo_box->priv->model,
				   combo_box->priv->inserted_id);
      g_signal_handler_disconnect (combo_box->priv->model,
				   combo_box->priv->deleted_id);
      g_signal_handler_disconnect (combo_box->priv->model,
				   combo_box->priv->reordered_id);
      g_signal_handler_disconnect (combo_box->priv->model,
				   combo_box->priv->changed_id);
    }

  /* menu mode */
  if (!combo_box->priv->tree_view)
    {
      if (combo_box->priv->popup_widget)
        gtk_container_foreach (GTK_CONTAINER (combo_box->priv->popup_widget),
                               (GtkCallback)gtk_widget_destroy, NULL);
    }

  if (combo_box->priv->model)
    {
      g_object_unref (combo_box->priv->model);
      combo_box->priv->model = NULL;
    }

  if (combo_box->priv->active_row)
    {
      gtk_tree_row_reference_free (combo_box->priv->active_row);
      combo_box->priv->active_row = NULL;
    }

  if (combo_box->priv->cell_view)
    gtk_cell_view_set_model (GTK_CELL_VIEW (combo_box->priv->cell_view), NULL);
}

static void
gtk_combo_box_forall (GtkContainer *container,
                      gboolean      include_internals,
                      GtkCallback   callback,
                      gpointer      callback_data)
{
  GtkComboBox *combo_box = GTK_COMBO_BOX (container);

  if (include_internals)
    {
      if (combo_box->priv->button)
	(* callback) (combo_box->priv->button, callback_data);
      if (combo_box->priv->cell_view_frame)
	(* callback) (combo_box->priv->cell_view_frame, callback_data);
    }

  if (GTK_BIN (container)->child)
    (* callback) (GTK_BIN (container)->child, callback_data);
}

static void 
gtk_combo_box_child_show (GtkWidget *widget,
                          GtkComboBox *combo_box)
{
  GtkComboBoxPrivate *priv = combo_box->priv;

  priv->popup_shown = TRUE;
  g_object_notify (G_OBJECT (combo_box), "popup-shown");
}

static void 
gtk_combo_box_child_hide (GtkWidget *widget,
                          GtkComboBox *combo_box)
{
  GtkComboBoxPrivate *priv = combo_box->priv;

  priv->popup_shown = FALSE;
  g_object_notify (G_OBJECT (combo_box), "popup-shown");
}

static gboolean
gtk_combo_box_child_delete_event (GtkWidget *widget,
				  GdkEventAny *event,
				  GtkComboBox *combo_box)
{
  gtk_combo_box_popdown (combo_box);

  return TRUE;
}

static gboolean
gtk_combo_box_expose_event (GtkWidget      *widget,
                            GdkEventExpose *event)
{
  GtkComboBox *combo_box = GTK_COMBO_BOX (widget);

  if (!combo_box->priv->tree_view)
    {
      gtk_container_propagate_expose (GTK_CONTAINER (widget),
				      combo_box->priv->button, event);
    }
  else
    {
      gtk_container_propagate_expose (GTK_CONTAINER (widget),
                                      combo_box->priv->button, event);

      if (combo_box->priv->cell_view_frame)
        gtk_container_propagate_expose (GTK_CONTAINER (widget),
                                        combo_box->priv->cell_view_frame, event);
    }

  gtk_container_propagate_expose (GTK_CONTAINER (widget),
                                  GTK_BIN (widget)->child, event);

  return FALSE;
}

typedef struct {
  GtkComboBox *combo;
  GtkTreePath *path;
  GtkTreeIter iter;
  gboolean found;
  gboolean set;
  gboolean visible;
} SearchData;

static gboolean
path_visible (GtkTreeView *view,
	      GtkTreePath *path)
{
  GtkRBTree *tree;
  GtkRBNode *node;
  
  /* Note that we rely on the fact that collapsed rows don't have nodes 
   */
  return _gtk_tree_view_find_node (view, path, &tree, &node);
}

static gboolean
tree_next_func (GtkTreeModel *model,
		GtkTreePath  *path,
		GtkTreeIter  *iter,
		gpointer      data)
{
  SearchData *search_data = (SearchData *)data;

  if (search_data->found) 
    {
      if (!tree_column_row_is_sensitive (search_data->combo, iter))
	return FALSE;
      
      if (search_data->visible &&
	  !path_visible (GTK_TREE_VIEW (search_data->combo->priv->tree_view), path))
	return FALSE;

      search_data->set = TRUE;
      search_data->iter = *iter;

      return TRUE;
    }
 
  if (gtk_tree_path_compare (path, search_data->path) == 0)
    search_data->found = TRUE;
  
  return FALSE;
}

static gboolean
tree_next (GtkComboBox  *combo,
	   GtkTreeModel *model,
	   GtkTreeIter  *iter,
	   GtkTreeIter  *next,
	   gboolean      visible)
{
  SearchData search_data;

  search_data.combo = combo;
  search_data.path = gtk_tree_model_get_path (model, iter);
  search_data.visible = visible;
  search_data.found = FALSE;
  search_data.set = FALSE;

  gtk_tree_model_foreach (model, tree_next_func, &search_data);
  
  *next = search_data.iter;

  gtk_tree_path_free (search_data.path);

  return search_data.set;
}

static gboolean
tree_prev_func (GtkTreeModel *model,
		GtkTreePath  *path,
		GtkTreeIter  *iter,
		gpointer      data)
{
  SearchData *search_data = (SearchData *)data;

  if (gtk_tree_path_compare (path, search_data->path) == 0)
    {
      search_data->found = TRUE;
      return TRUE;
    }
  
  if (!tree_column_row_is_sensitive (search_data->combo, iter))
    return FALSE;
      
  if (search_data->visible &&
      !path_visible (GTK_TREE_VIEW (search_data->combo->priv->tree_view), path))
    return FALSE; 
  
  search_data->set = TRUE;
  search_data->iter = *iter;
  
  return FALSE; 
}

static gboolean
tree_prev (GtkComboBox  *combo,
	   GtkTreeModel *model,
	   GtkTreeIter  *iter,
	   GtkTreeIter  *prev,
	   gboolean      visible)
{
  SearchData search_data;

  search_data.combo = combo;
  search_data.path = gtk_tree_model_get_path (model, iter);
  search_data.visible = visible;
  search_data.found = FALSE;
  search_data.set = FALSE;

  gtk_tree_model_foreach (model, tree_prev_func, &search_data);
  
  *prev = search_data.iter;

  gtk_tree_path_free (search_data.path);

  return search_data.set;
}

static gboolean
tree_last_func (GtkTreeModel *model,
		GtkTreePath  *path,
		GtkTreeIter  *iter,
		gpointer      data)
{
  SearchData *search_data = (SearchData *)data;

  if (!tree_column_row_is_sensitive (search_data->combo, iter))
    return FALSE;
      
  /* Note that we rely on the fact that collapsed rows don't have nodes 
   */
  if (search_data->visible &&
      !path_visible (GTK_TREE_VIEW (search_data->combo->priv->tree_view), path))
    return FALSE; 
  
  search_data->set = TRUE;
  search_data->iter = *iter;
  
  return FALSE; 
}

static gboolean
tree_last (GtkComboBox  *combo,
	   GtkTreeModel *model,
	   GtkTreeIter  *last,
	   gboolean      visible)
{
  SearchData search_data;

  search_data.combo = combo;
  search_data.visible = visible;
  search_data.set = FALSE;

  gtk_tree_model_foreach (model, tree_last_func, &search_data);
  
  *last = search_data.iter;

  return search_data.set;  
}


static gboolean
tree_first_func (GtkTreeModel *model,
		 GtkTreePath  *path,
		 GtkTreeIter  *iter,
		 gpointer      data)
{
  SearchData *search_data = (SearchData *)data;

  if (!tree_column_row_is_sensitive (search_data->combo, iter))
    return FALSE;
  
  if (search_data->visible &&
      !path_visible (GTK_TREE_VIEW (search_data->combo->priv->tree_view), path))
    return FALSE;
  
  search_data->set = TRUE;
  search_data->iter = *iter;
  
  return TRUE;
}

static gboolean
tree_first (GtkComboBox  *combo,
	    GtkTreeModel *model,
	    GtkTreeIter  *first,
	    gboolean      visible)
{
  SearchData search_data;
  
  search_data.combo = combo;
  search_data.visible = visible;
  search_data.set = FALSE;

  gtk_tree_model_foreach (model, tree_first_func, &search_data);
  
  *first = search_data.iter;

  return search_data.set;  
}

static gboolean
gtk_combo_box_scroll_event (GtkWidget          *widget,
                            GdkEventScroll     *event)
{
  GtkComboBox *combo_box = GTK_COMBO_BOX (widget);
  gboolean found;
  GtkTreeIter iter;
  GtkTreeIter new_iter;

  if (!gtk_combo_box_get_active_iter (combo_box, &iter))
    return TRUE;
  
  if (event->direction == GDK_SCROLL_UP)
    found = tree_prev (combo_box, combo_box->priv->model, 
		       &iter, &new_iter, FALSE);
  else
    found = tree_next (combo_box, combo_box->priv->model, 
		       &iter, &new_iter, FALSE);
  
  if (found)
    gtk_combo_box_set_active_iter (combo_box, &new_iter);

  return TRUE;
}

/*
 * menu style
 */

static void
gtk_combo_box_sync_cells (GtkComboBox   *combo_box,
			  GtkCellLayout *cell_layout)
{
  GSList *k;

  for (k = combo_box->priv->cells; k; k = k->next)
    {
      GSList *j;
      ComboCellInfo *info = (ComboCellInfo *)k->data;

      if (info->pack == GTK_PACK_START)
        gtk_cell_layout_pack_start (cell_layout,
                                    info->cell, info->expand);
      else if (info->pack == GTK_PACK_END)
        gtk_cell_layout_pack_end (cell_layout,
                                  info->cell, info->expand);

      gtk_cell_layout_set_cell_data_func (cell_layout,
                                          info->cell,
                                          combo_cell_data_func, info, NULL);

      for (j = info->attributes; j; j = j->next->next)
        {
          gtk_cell_layout_add_attribute (cell_layout,
                                         info->cell,
                                         j->data,
                                         GPOINTER_TO_INT (j->next->data));
        }
    }
}

static void
gtk_combo_box_menu_setup (GtkComboBox *combo_box,
                          gboolean     add_children)
{
  GtkWidget *menu;

  if (combo_box->priv->cell_view)
    {
      combo_box->priv->button = gtk_toggle_button_new ();
      gtk_button_set_focus_on_click (GTK_BUTTON (combo_box->priv->button),
				     combo_box->priv->focus_on_click);

      g_signal_connect (combo_box->priv->button, "toggled",
                        G_CALLBACK (gtk_combo_box_button_toggled), combo_box);
      gtk_widget_set_parent (combo_box->priv->button,
                             GTK_BIN (combo_box)->child->parent);

      combo_box->priv->box = gtk_hbox_new (FALSE, 0);
      gtk_container_add (GTK_CONTAINER (combo_box->priv->button), 
			 combo_box->priv->box);

      combo_box->priv->separator = gtk_vseparator_new ();
      gtk_container_add (GTK_CONTAINER (combo_box->priv->box), 
			 combo_box->priv->separator);

      combo_box->priv->arrow = gtk_arrow_new (GTK_ARROW_DOWN, GTK_SHADOW_NONE);
      gtk_container_add (GTK_CONTAINER (combo_box->priv->box), 
			 combo_box->priv->arrow);

      gtk_widget_show_all (combo_box->priv->button);
    }
  else
    {
      combo_box->priv->button = gtk_toggle_button_new ();
      gtk_button_set_focus_on_click (GTK_BUTTON (combo_box->priv->button),
				     combo_box->priv->focus_on_click);

#ifdef MAEMO_CHANGES
      if (GTK_IS_ENTRY (GTK_BIN (combo_box)->child))
        GTK_WIDGET_UNSET_FLAGS (combo_box->priv->button, GTK_CAN_FOCUS);
#endif /* MAEMO_CHANGES */

      g_signal_connect (combo_box->priv->button, "toggled",
                        G_CALLBACK (gtk_combo_box_button_toggled), combo_box);
      gtk_widget_set_parent (combo_box->priv->button,
                             GTK_BIN (combo_box)->child->parent);

      combo_box->priv->arrow = gtk_arrow_new (GTK_ARROW_DOWN, GTK_SHADOW_NONE);
      gtk_container_add (GTK_CONTAINER (combo_box->priv->button),
                         combo_box->priv->arrow);
      gtk_widget_show_all (combo_box->priv->button);
    }

#ifdef MAEMO_CHANGES
  /* Hildon: propagate the insensitive press */
  g_signal_connect_swapped (combo_box->priv->button, "insensitive-press",
			    G_CALLBACK (gtk_widget_insensitive_press), combo_box);
#endif /* MAEMO_CHANGES */

  g_signal_connect (combo_box->priv->button, "button_press_event",
                    G_CALLBACK (gtk_combo_box_menu_button_press),
                    combo_box);
  g_signal_connect (combo_box->priv->button, "state_changed",
		    G_CALLBACK (gtk_combo_box_button_state_changed), 
		    combo_box);

  /* create our funky menu */
  menu = gtk_menu_new ();
  gtk_widget_set_name (menu, "gtk-combobox-popup-menu");
  
  g_signal_connect (menu, "key_press_event",
		    G_CALLBACK (gtk_combo_box_menu_key_press), combo_box);
  gtk_combo_box_set_popup_widget (combo_box, menu);

  /* add items */
  if (add_children)
    gtk_combo_box_menu_fill (combo_box);

  /* the column is needed in tree_column_row_is_sensitive() */
  combo_box->priv->column = gtk_tree_view_column_new ();
  g_object_ref_sink (combo_box->priv->column);
  gtk_combo_box_sync_cells (combo_box, 
			    GTK_CELL_LAYOUT (combo_box->priv->column));

  gtk_combo_box_update_title (combo_box);
}

static void
gtk_combo_box_menu_fill (GtkComboBox *combo_box)
{
  GtkWidget *menu;

  if (!combo_box->priv->model)
    return;

  menu = combo_box->priv->popup_widget;

  if (combo_box->priv->add_tearoffs)
    {
      GtkWidget *tearoff = gtk_tearoff_menu_item_new ();

      gtk_widget_show (tearoff);
      
      if (combo_box->priv->wrap_width)
	gtk_menu_attach (GTK_MENU (menu), tearoff, 
			 0, combo_box->priv->wrap_width, 0, 1);
      else
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), tearoff);
    }
  
  gtk_combo_box_menu_fill_level (combo_box, menu, NULL);
}

static GtkWidget *
gtk_cell_view_menu_item_new (GtkComboBox  *combo_box,
			     GtkTreeModel *model,
			     GtkTreeIter  *iter)
{
  GtkWidget *cell_view; 
  GtkWidget *item;
  GtkTreePath *path;
  GtkRequisition req;

  cell_view = gtk_cell_view_new ();
  item = gtk_menu_item_new ();
  gtk_container_add (GTK_CONTAINER (item), cell_view);

  gtk_cell_view_set_model (GTK_CELL_VIEW (cell_view), model);	  
  path = gtk_tree_model_get_path (model, iter);
  gtk_cell_view_set_displayed_row (GTK_CELL_VIEW (cell_view), path);
  gtk_tree_path_free (path);

  gtk_combo_box_sync_cells (combo_box, GTK_CELL_LAYOUT (cell_view));
  gtk_widget_size_request (cell_view, &req);
  gtk_widget_show (cell_view);
  
  return item;
}
 
static void
gtk_combo_box_menu_fill_level (GtkComboBox *combo_box,
			       GtkWidget   *menu,
			       GtkTreeIter *parent)
{
  GtkTreeModel *model = combo_box->priv->model;
  GtkWidget *item, *submenu, *subitem, *separator;
  GtkTreeIter iter;
  gboolean is_separator;
  gint i, n_children;
  GtkWidget *last;
  GtkTreePath *path;
  
  n_children = gtk_tree_model_iter_n_children (model, parent);
  
  last = NULL;
  for (i = 0; i < n_children; i++)
    {
      gtk_tree_model_iter_nth_child (model, &iter, parent, i);

      if (combo_box->priv->row_separator_func)
	is_separator = (*combo_box->priv->row_separator_func) (combo_box->priv->model, &iter,
							       combo_box->priv->row_separator_data);
      else
	is_separator = FALSE;
      
      if (is_separator)
	{
	  item = gtk_separator_menu_item_new ();
	  path = gtk_tree_model_get_path (model, &iter);
	  g_object_set_data_full (G_OBJECT (item),
				  I_("gtk-combo-box-item-path"),
				  gtk_tree_row_reference_new (model, path),
				  (GDestroyNotify)gtk_tree_row_reference_free);
	  gtk_tree_path_free (path);
	}
      else
	{
	  item = gtk_cell_view_menu_item_new (combo_box, model, &iter);
	  if (gtk_tree_model_iter_has_child (model, &iter))
	    {
	      submenu = gtk_menu_new ();
	      gtk_widget_show (submenu);
	      gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), submenu);
	      
	      /* Ugly - since menus can only activate leafs, we have to
	       * duplicate the item inside the submenu.
	       */
	      subitem = gtk_cell_view_menu_item_new (combo_box, model, &iter);
	      separator = gtk_separator_menu_item_new ();
	      gtk_widget_show (subitem);
	      gtk_widget_show (separator);
	      g_signal_connect (subitem, "activate",
				G_CALLBACK (gtk_combo_box_menu_item_activate),
				combo_box);
	      gtk_menu_shell_append (GTK_MENU_SHELL (submenu), subitem);
	      gtk_menu_shell_append (GTK_MENU_SHELL (submenu), separator);
	      
	      gtk_combo_box_menu_fill_level (combo_box, submenu, &iter);
	    }
          g_signal_connect (item, "activate",
                            G_CALLBACK (gtk_combo_box_menu_item_activate),
                            combo_box);
	}
      
      gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
      if (combo_box->priv->wrap_width && menu == combo_box->priv->popup_widget)
        gtk_combo_box_relayout_item (combo_box, item, &iter, last);
      gtk_widget_show (item);
      
      last = item;
    }
}

static void
gtk_combo_box_menu_destroy (GtkComboBox *combo_box)
{
  g_signal_handlers_disconnect_matched (combo_box->priv->button,
                                        G_SIGNAL_MATCH_DATA,
                                        0, 0, NULL,
                                        gtk_combo_box_menu_button_press, NULL);
  g_signal_handlers_disconnect_matched (combo_box->priv->button,
                                        G_SIGNAL_MATCH_DATA,
                                        0, 0, NULL,
                                        gtk_combo_box_button_state_changed, combo_box);

  /* unparent will remove our latest ref */
  gtk_widget_unparent (combo_box->priv->button);
  
  combo_box->priv->box = NULL;
  combo_box->priv->button = NULL;
  combo_box->priv->arrow = NULL;
  combo_box->priv->separator = NULL;

  g_object_unref (combo_box->priv->column);
  combo_box->priv->column = NULL;

  /* changing the popup window will unref the menu and the children */
}

/*
 * grid
 */

static gboolean
menu_occupied (GtkMenu   *menu,
               guint      left_attach,
               guint      right_attach,
               guint      top_attach,
               guint      bottom_attach)
{
  GList *i;

  for (i = GTK_MENU_SHELL (menu)->children; i; i = i->next)
    {
      guint l, r, b, t;

      gtk_container_child_get (GTK_CONTAINER (menu), 
			       i->data,
                               "left-attach", &l,
                               "right-attach", &r,
                               "bottom-attach", &b,
                               "top-attach", &t,
                               NULL);

      /* look if this item intersects with the given coordinates */
      if (right_attach > l && left_attach < r && bottom_attach > t && top_attach < b)
	return TRUE;
    }

  return FALSE;
}

static void
gtk_combo_box_relayout_item (GtkComboBox *combo_box,
			     GtkWidget   *item,
                             GtkTreeIter *iter,
			     GtkWidget   *last)
{
  gint current_col = 0, current_row = 0;
  gint rows = 1, cols = 1;
  GtkWidget *menu = combo_box->priv->popup_widget;

  if (!GTK_IS_MENU_SHELL (menu))
    return;
  
  if (combo_box->priv->col_column == -1 &&
      combo_box->priv->row_column == -1 &&
      last)
    {
      gtk_container_child_get (GTK_CONTAINER (menu), 
			       last,
			       "right-attach", &current_col,
			       "top-attach", &current_row,
			       NULL);
      if (current_col + cols > combo_box->priv->wrap_width)
	{
	  current_col = 0;
	  current_row++;
	}
    }
  else
    {
      if (combo_box->priv->col_column != -1)
	gtk_tree_model_get (combo_box->priv->model, iter,
			    combo_box->priv->col_column, &cols,
			    -1);
      if (combo_box->priv->row_column != -1)
	gtk_tree_model_get (combo_box->priv->model, iter,
			    combo_box->priv->row_column, &rows,
			    -1);

      while (1)
	{
	  if (current_col + cols > combo_box->priv->wrap_width)
	    {
	      current_col = 0;
	      current_row++;
	    }
	  
	  if (!menu_occupied (GTK_MENU (menu), 
			      current_col, current_col + cols,
			      current_row, current_row + rows))
	    break;
	  
	  current_col++;
	}
    }

  /* set attach props */
  gtk_menu_attach (GTK_MENU (menu), item,
                   current_col, current_col + cols,
                   current_row, current_row + rows);
}

static void
gtk_combo_box_relayout (GtkComboBox *combo_box)
{
  GList *list, *j;
  GtkWidget *menu;

  menu = combo_box->priv->popup_widget;
  
  /* do nothing unless we are in menu style and realized */
  if (combo_box->priv->tree_view || !GTK_IS_MENU_SHELL (menu))
    return;
  
  list = gtk_container_get_children (GTK_CONTAINER (menu));
  
  for (j = g_list_last (list); j; j = j->prev)
    gtk_container_remove (GTK_CONTAINER (menu), j->data);
  
  gtk_combo_box_menu_fill (combo_box);

  g_list_free (list);
}

/* callbacks */
static gboolean
gtk_combo_box_menu_button_press (GtkWidget      *widget,
                                 GdkEventButton *event,
                                 gpointer        user_data)
{
  GtkComboBox *combo_box = GTK_COMBO_BOX (user_data);

  if (GTK_IS_MENU (combo_box->priv->popup_widget) &&
      event->type == GDK_BUTTON_PRESS && event->button == 1)
    {
      if (combo_box->priv->focus_on_click && 
	  !GTK_WIDGET_HAS_FOCUS (combo_box->priv->button))
	gtk_widget_grab_focus (combo_box->priv->button);

      gtk_combo_box_menu_popup (combo_box, event->button, event->time);

      return TRUE;
    }

  return FALSE;
}

static void
gtk_combo_box_menu_item_activate (GtkWidget *item,
                                  gpointer   user_data)
{
  GtkComboBox *combo_box = GTK_COMBO_BOX (user_data);
  GtkWidget *cell_view;
  GtkTreePath *path;
  GtkTreeIter iter;

  cell_view = GTK_BIN (item)->child;

  g_return_if_fail (GTK_IS_CELL_VIEW (cell_view));

  path = gtk_cell_view_get_displayed_row (GTK_CELL_VIEW (cell_view));

  if (gtk_tree_model_get_iter (combo_box->priv->model, &iter, path))
    {
      if (gtk_menu_item_get_submenu (GTK_MENU_ITEM (item)) == NULL)
        gtk_combo_box_set_active_iter (combo_box, &iter);
    }

  gtk_tree_path_free (path);

  combo_box->priv->editing_canceled = FALSE;
}

static void
gtk_combo_box_model_row_inserted (GtkTreeModel     *model,
				  GtkTreePath      *path,
				  GtkTreeIter      *iter,
				  gpointer          user_data)
{
  GtkComboBox *combo_box = GTK_COMBO_BOX (user_data);

  if (combo_box->priv->tree_view)
    gtk_combo_box_list_popup_resize (combo_box);
  else
    gtk_combo_box_menu_row_inserted (model, path, iter, user_data);

#ifdef MAEMO_CHANGES
  maemo_check_autodim (combo_box);
#endif /* MAEMO_CHANGES */
}

static void
gtk_combo_box_model_row_deleted (GtkTreeModel     *model,
				 GtkTreePath      *path,
				 gpointer          user_data)
{
  GtkComboBox *combo_box = GTK_COMBO_BOX (user_data);

  if (combo_box->priv->cell_view)
    {
      if (!gtk_tree_row_reference_valid (combo_box->priv->active_row))
	gtk_cell_view_set_displayed_row (GTK_CELL_VIEW (combo_box->priv->cell_view), NULL);
    }
  
  if (combo_box->priv->tree_view)
    gtk_combo_box_list_popup_resize (combo_box);
  else
    gtk_combo_box_menu_row_deleted (model, path, user_data);

#ifdef MAEMO_CHANGES
  maemo_check_autodim (combo_box);
#endif /* MAEMO_CHANGES */
}

static void
gtk_combo_box_model_rows_reordered (GtkTreeModel    *model,
				    GtkTreePath     *path,
				    GtkTreeIter     *iter,
				    gint            *new_order,
				    gpointer         user_data)
{
  GtkComboBox *combo_box = GTK_COMBO_BOX (user_data);

  gtk_tree_row_reference_reordered (G_OBJECT (user_data), path, iter, new_order);

  if (!combo_box->priv->tree_view)
    gtk_combo_box_menu_rows_reordered (model, path, iter, new_order, user_data);
}
						    
static void
gtk_combo_box_model_row_changed (GtkTreeModel     *model,
				 GtkTreePath      *path,
				 GtkTreeIter      *iter,
				 gpointer          user_data)
{
  GtkComboBox *combo_box = GTK_COMBO_BOX (user_data);
  GtkTreePath *active_path;

  /* FIXME this belongs to GtkCellView */
  if (gtk_tree_row_reference_valid (combo_box->priv->active_row))
    {
      active_path = gtk_tree_row_reference_get_path (combo_box->priv->active_row);
      if (gtk_tree_path_compare (path, active_path) == 0 &&
	  combo_box->priv->cell_view)
	gtk_widget_queue_resize (GTK_WIDGET (combo_box->priv->cell_view));
      gtk_tree_path_free (active_path);
    }
      
  if (combo_box->priv->tree_view)
    gtk_combo_box_list_row_changed (model, path, iter, user_data);
  else
    gtk_combo_box_menu_row_changed (model, path, iter, user_data);
}

static gboolean
list_popup_resize_idle (gpointer user_data)
{
  GtkComboBox *combo_box;
  gint x, y, width, height;

  GDK_THREADS_ENTER ();

  combo_box = GTK_COMBO_BOX (user_data);

  if (combo_box->priv->tree_view &&
      GTK_WIDGET_MAPPED (combo_box->priv->popup_window))
    {
      gtk_combo_box_list_position (combo_box, &x, &y, &width, &height);
  
      gtk_widget_set_size_request (combo_box->priv->popup_window, width, height);
      gtk_window_move (GTK_WINDOW (combo_box->priv->popup_window), x, y);
    }

  combo_box->priv->resize_idle_id = 0;

  GDK_THREADS_LEAVE ();

  return FALSE;
}

static void
gtk_combo_box_list_popup_resize (GtkComboBox *combo_box)
{
  if (!combo_box->priv->resize_idle_id)
    combo_box->priv->resize_idle_id = 
      g_idle_add (list_popup_resize_idle, combo_box);
}

static void
gtk_combo_box_model_row_expanded (GtkTreeModel     *model,
				  GtkTreePath      *path,
				  GtkTreeIter      *iter,
				  gpointer          user_data)
{
  GtkComboBox *combo_box = GTK_COMBO_BOX (user_data);
  
  gtk_combo_box_list_popup_resize (combo_box);
}


static GtkWidget *
find_menu_by_path (GtkWidget   *menu,
		   GtkTreePath *path,
		   gboolean     skip_first)
{
  GList *i, *list;
  GtkWidget *item;
  GtkWidget *submenu;    
  GtkTreeRowReference *mref;
  GtkTreePath *mpath;
  gboolean skip;

  list = gtk_container_get_children (GTK_CONTAINER (menu));
  skip = skip_first;
  item = NULL;
  for (i = list; i; i = i->next)
    {
      if (GTK_IS_SEPARATOR_MENU_ITEM (i->data))
	{
	  mref = g_object_get_data (G_OBJECT (i->data), "gtk-combo-box-item-path");
	  if (!mref)
	    continue;
	  else if (!gtk_tree_row_reference_valid (mref))
	    mpath = NULL;
	  else
	    mpath = gtk_tree_row_reference_get_path (mref);
	}
      else if (GTK_IS_CELL_VIEW (GTK_BIN (i->data)->child))
	{
	  if (skip)
	    {
	      skip = FALSE;
	      continue;
	    }

	  mpath = gtk_cell_view_get_displayed_row (GTK_CELL_VIEW (GTK_BIN (i->data)->child));
	}
      else 
	continue;

      /* this case is necessary, since the row reference of
       * the cell view may already be updated after a deletion
       */
      if (!mpath)
	{
	  item = i->data;
	  break;
	}
      if (gtk_tree_path_compare (mpath, path) == 0)
	{
	  gtk_tree_path_free (mpath);
	  item = i->data;
	  break;
	}
      if (gtk_tree_path_is_ancestor (mpath, path))
	{
	  submenu = gtk_menu_item_get_submenu (GTK_MENU_ITEM (i->data));
	  if (submenu != NULL)
	    {
	      gtk_tree_path_free (mpath);
	      item = find_menu_by_path (submenu, path, TRUE);
	      break;
	    }
	}
      gtk_tree_path_free (mpath);
    }
  
  g_list_free (list);  

  return item;
}

#if 0
static void
dump_menu_tree (GtkWidget   *menu, 
		gint         level)
{
  GList *i, *list;
  GtkWidget *submenu;    
  GtkTreePath *path;

  list = gtk_container_get_children (GTK_CONTAINER (menu));
  for (i = list; i; i = i->next)
    {
      if (GTK_IS_CELL_VIEW (GTK_BIN (i->data)->child))
	{
	  path = gtk_cell_view_get_displayed_row (GTK_CELL_VIEW (GTK_BIN (i->data)->child));
	  g_print ("%*s%s\n", 2 * level, " ", gtk_tree_path_to_string (path));
	  gtk_tree_path_free (path);

	  submenu = gtk_menu_item_get_submenu (GTK_MENU_ITEM (i->data));
	  if (submenu != NULL)
	    dump_menu_tree (submenu, level + 1);
	}
    }
  
  g_list_free (list);  
}
#endif

static void
gtk_combo_box_menu_row_inserted (GtkTreeModel *model,
                                 GtkTreePath  *path,
                                 GtkTreeIter  *iter,
                                 gpointer      user_data)
{
  GtkWidget *parent;
  GtkWidget *item, *menu, *separator;
  GtkComboBox *combo_box = GTK_COMBO_BOX (user_data);
  GtkTreePath *ppath;
  GtkTreeIter piter;
  gint depth, pos;
  gboolean is_separator;

  if (!combo_box->priv->popup_widget)
    return;

  depth = gtk_tree_path_get_depth (path);
  pos = gtk_tree_path_get_indices (path)[depth - 1];
  if (depth > 1)
    {
      ppath = gtk_tree_path_copy (path);
      gtk_tree_path_up (ppath);
      parent = find_menu_by_path (combo_box->priv->popup_widget, ppath, FALSE);
      gtk_tree_path_free (ppath);

      menu = gtk_menu_item_get_submenu (GTK_MENU_ITEM (parent));
      if (!menu)
	{
	  menu = gtk_menu_new ();
	  gtk_widget_show (menu);
	  gtk_menu_item_set_submenu (GTK_MENU_ITEM (parent), menu);
	  
	  /* Ugly - since menus can only activate leaves, we have to
	   * duplicate the item inside the submenu.
	   */
	  gtk_tree_model_iter_parent (model, &piter, iter);
	  item = gtk_cell_view_menu_item_new (combo_box, model, &piter);
	  separator = gtk_separator_menu_item_new ();
	  g_signal_connect (item, "activate",
			    G_CALLBACK (gtk_combo_box_menu_item_activate),
			    combo_box);
	  gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
	  gtk_menu_shell_append (GTK_MENU_SHELL (menu), separator);
	  if (cell_view_is_sensitive (GTK_CELL_VIEW (GTK_BIN (item)->child)))
	    {
	      gtk_widget_show (item);
	      gtk_widget_show (separator);
	    }
	}
      pos += 2;
    }
  else
    {
      menu = combo_box->priv->popup_widget;
      if (combo_box->priv->add_tearoffs)
	pos += 1;
    }
  
  if (combo_box->priv->row_separator_func)
    is_separator = (*combo_box->priv->row_separator_func) (model, iter,
							   combo_box->priv->row_separator_data);
  else
    is_separator = FALSE;

  if (is_separator)
    {
      item = gtk_separator_menu_item_new ();
      g_object_set_data_full (G_OBJECT (item),
			      I_("gtk-combo-box-item-path"),
			      gtk_tree_row_reference_new (model, path),
			      (GDestroyNotify)gtk_tree_row_reference_free);
    }
  else
    {
      item = gtk_cell_view_menu_item_new (combo_box, model, iter);
      
      g_signal_connect (item, "activate",
			G_CALLBACK (gtk_combo_box_menu_item_activate),
			combo_box);
    }

  gtk_widget_show (item);
  gtk_menu_shell_insert (GTK_MENU_SHELL (menu), item, pos);
}

static void
gtk_combo_box_menu_row_deleted (GtkTreeModel *model,
                                GtkTreePath  *path,
                                gpointer      user_data)
{
  GtkComboBox *combo_box = GTK_COMBO_BOX (user_data);
  GtkWidget *menu;
  GtkWidget *item;

  if (!combo_box->priv->popup_widget)
    return;

  item = find_menu_by_path (combo_box->priv->popup_widget, path, FALSE);
  menu = gtk_widget_get_parent (item);
  gtk_container_remove (GTK_CONTAINER (menu), item);

  if (gtk_tree_path_get_depth (path) > 1)
    {
      GtkTreePath *parent_path;
      GtkTreeIter iter;
      GtkWidget *parent;

      parent_path = gtk_tree_path_copy (path);
      gtk_tree_path_up (parent_path);
      gtk_tree_model_get_iter (model, &iter, parent_path);

      if (!gtk_tree_model_iter_has_child (model, &iter))
        {
          parent = find_menu_by_path (combo_box->priv->popup_widget, 
                                      parent_path, FALSE);
          gtk_menu_item_remove_submenu (GTK_MENU_ITEM (parent));
        }
    }
}

static void
gtk_combo_box_menu_rows_reordered  (GtkTreeModel     *model,
				    GtkTreePath      *path,
				    GtkTreeIter      *iter,
	      			    gint             *new_order,
				    gpointer          user_data)
{
  GtkComboBox *combo_box = GTK_COMBO_BOX (user_data);

  gtk_combo_box_relayout (combo_box);
}
				    
static void
gtk_combo_box_menu_row_changed (GtkTreeModel *model,
                                GtkTreePath  *path,
                                GtkTreeIter  *iter,
                                gpointer      user_data)
{
  GtkComboBox *combo_box = GTK_COMBO_BOX (user_data);
  GtkWidget *item;
  gint width;
  gboolean is_separator;

  if (!combo_box->priv->popup_widget)
    return;

  item = find_menu_by_path (combo_box->priv->popup_widget, path, FALSE);

  if (combo_box->priv->row_separator_func)
    is_separator = (*combo_box->priv->row_separator_func) (model, iter,
							   combo_box->priv->row_separator_data);
  else
    is_separator = FALSE;

  if (is_separator != GTK_IS_SEPARATOR_MENU_ITEM (item))
    {
      gtk_combo_box_menu_row_deleted (model, path, combo_box);
      gtk_combo_box_menu_row_inserted (model, path, iter, combo_box);
    }

  if (combo_box->priv->wrap_width
      && item->parent == combo_box->priv->popup_widget)
    {
      GtkWidget *pitem = NULL;
      GtkTreePath *prev;

      prev = gtk_tree_path_copy (path);

      if (gtk_tree_path_prev (prev))
        pitem = find_menu_by_path (combo_box->priv->popup_widget, prev, FALSE);

      gtk_tree_path_free (prev);

      /* unattach item so gtk_combo_box_relayout_item() won't spuriously
         move it */
      gtk_container_child_set (GTK_CONTAINER (combo_box->priv->popup_widget),
                               item, 
			       "left-attach", -1, 
			       "right-attach", -1,
                               "top-attach", -1, 
			       "bottom-attach", -1, 
			       NULL);

      gtk_combo_box_relayout_item (combo_box, item, iter, pitem);
    }

  width = gtk_combo_box_calc_requested_width (combo_box, path);

  if (width > combo_box->priv->width)
    {
      if (combo_box->priv->cell_view)
	{
	  gtk_widget_set_size_request (combo_box->priv->cell_view, width, -1);
	  gtk_widget_queue_resize (combo_box->priv->cell_view);
	}
      combo_box->priv->width = width;
    }
}

/*
 * list style
 */

static void
gtk_combo_box_list_setup (GtkComboBox *combo_box)
{
  GtkTreeSelection *sel;

  combo_box->priv->button = gtk_toggle_button_new ();
  gtk_widget_set_parent (combo_box->priv->button,
                         GTK_BIN (combo_box)->child->parent);

#ifdef MAEMO_CHANGES
      if (GTK_IS_ENTRY (GTK_BIN (combo_box)->child))
        GTK_WIDGET_UNSET_FLAGS (combo_box->priv->button, GTK_CAN_FOCUS);
#endif /* MAEMO_CHANGES */

  g_signal_connect (combo_box->priv->button, "button_press_event",
                    G_CALLBACK (gtk_combo_box_list_button_pressed), combo_box);
  g_signal_connect (combo_box->priv->button, "toggled",
                    G_CALLBACK (gtk_combo_box_button_toggled), combo_box);

#ifdef MAEMO_CHANGES
  /* Hildon: propagate the insensitive press */
  g_signal_connect_swapped (combo_box->priv->button, "insensitive-press",
                            G_CALLBACK (gtk_widget_insensitive_press), combo_box);
#endif /* MAEMO_CHANGES */

  combo_box->priv->arrow = gtk_arrow_new (GTK_ARROW_DOWN, GTK_SHADOW_NONE);
  gtk_container_add (GTK_CONTAINER (combo_box->priv->button),
                     combo_box->priv->arrow);
  combo_box->priv->separator = NULL;
  gtk_widget_show_all (combo_box->priv->button);

  if (combo_box->priv->cell_view)
    {
      gtk_cell_view_set_background_color (GTK_CELL_VIEW (combo_box->priv->cell_view), 
					  &GTK_WIDGET (combo_box)->style->base[GTK_WIDGET_STATE (combo_box)]);

      combo_box->priv->box = gtk_event_box_new ();
      gtk_event_box_set_visible_window (GTK_EVENT_BOX (combo_box->priv->box), 
					FALSE);

#ifdef MAEMO_CHANGES
      /* Hildon: propagate the insensitive press */
      g_signal_connect_swapped (combo_box->priv->box, "insensitive-press",
                                G_CALLBACK (gtk_widget_insensitive_press), combo_box);
#endif /* MAEMO_CHANGES */

      if (combo_box->priv->has_frame)
	{
	  combo_box->priv->cell_view_frame = gtk_frame_new (NULL);
	  gtk_frame_set_shadow_type (GTK_FRAME (combo_box->priv->cell_view_frame),
				     GTK_SHADOW_IN);
	}
      else 
	{
	  combo_box->priv->cell_view_frame = gtk_event_box_new ();
	  gtk_event_box_set_visible_window (GTK_EVENT_BOX (combo_box->priv->cell_view_frame), 
					    FALSE);
	}
      
      gtk_widget_set_parent (combo_box->priv->cell_view_frame,
			     GTK_BIN (combo_box)->child->parent);
      gtk_container_add (GTK_CONTAINER (combo_box->priv->cell_view_frame),
			 combo_box->priv->box);
      gtk_widget_show_all (combo_box->priv->cell_view_frame);

      g_signal_connect (combo_box->priv->box, "button_press_event",
			G_CALLBACK (gtk_combo_box_list_button_pressed), 
			combo_box);
    }

  combo_box->priv->tree_view = gtk_tree_view_new ();
  sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (combo_box->priv->tree_view));
  gtk_tree_selection_set_mode (sel, GTK_SELECTION_BROWSE);
  gtk_tree_selection_set_select_function (sel,
					  gtk_combo_box_list_select_func,
					  NULL, NULL);
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (combo_box->priv->tree_view),
                                     FALSE);
  gtk_tree_view_set_hover_selection (GTK_TREE_VIEW (combo_box->priv->tree_view),
				     TRUE);
  if (combo_box->priv->row_separator_func)
    gtk_tree_view_set_row_separator_func (GTK_TREE_VIEW (combo_box->priv->tree_view), 
					  combo_box->priv->row_separator_func, 
					  combo_box->priv->row_separator_data, 
					  NULL);
  if (combo_box->priv->model)
    gtk_tree_view_set_model (GTK_TREE_VIEW (combo_box->priv->tree_view),
			     combo_box->priv->model);
    
  combo_box->priv->column = gtk_tree_view_column_new ();
  gtk_tree_view_append_column (GTK_TREE_VIEW (combo_box->priv->tree_view),
                               combo_box->priv->column);

  /* sync up */
  gtk_combo_box_sync_cells (combo_box, 
			    GTK_CELL_LAYOUT (combo_box->priv->column));

  if (gtk_tree_row_reference_valid (combo_box->priv->active_row))
    {
      GtkTreePath *path;

      path = gtk_tree_row_reference_get_path (combo_box->priv->active_row);
      gtk_tree_view_set_cursor (GTK_TREE_VIEW (combo_box->priv->tree_view),
				path, NULL, FALSE);
      gtk_tree_path_free (path);
    }

  /* set sample/popup widgets */
  gtk_combo_box_set_popup_widget (combo_box, combo_box->priv->tree_view);

  g_signal_connect (combo_box->priv->tree_view, "key_press_event",
                    G_CALLBACK (gtk_combo_box_list_key_press),
                    combo_box);
  g_signal_connect (combo_box->priv->tree_view, "enter_notify_event",
                    G_CALLBACK (gtk_combo_box_list_enter_notify),
                    combo_box);
  g_signal_connect (combo_box->priv->tree_view, "row_expanded",
		    G_CALLBACK (gtk_combo_box_model_row_expanded),
		    combo_box);
  g_signal_connect (combo_box->priv->tree_view, "row_collapsed",
		    G_CALLBACK (gtk_combo_box_model_row_expanded),
		    combo_box);
  g_signal_connect (combo_box->priv->popup_window, "button_press_event",
                    G_CALLBACK (gtk_combo_box_list_button_pressed),
                    combo_box);
  g_signal_connect (combo_box->priv->popup_window, "button_release_event",
                    G_CALLBACK (gtk_combo_box_list_button_released),
                    combo_box);

  gtk_widget_show (combo_box->priv->tree_view);
}

static void
gtk_combo_box_list_destroy (GtkComboBox *combo_box)
{
  /* disconnect signals */
  g_signal_handlers_disconnect_matched (combo_box->priv->tree_view,
                                        G_SIGNAL_MATCH_DATA,
                                        0, 0, NULL, NULL, combo_box);
  g_signal_handlers_disconnect_matched (combo_box->priv->button,
                                        G_SIGNAL_MATCH_DATA,
                                        0, 0, NULL,
                                        gtk_combo_box_list_button_pressed,
                                        NULL);
  g_signal_handlers_disconnect_matched (combo_box->priv->popup_window,
                                        G_SIGNAL_MATCH_DATA,
                                        0, 0, NULL,
                                        gtk_combo_box_list_button_pressed,
                                        NULL);
  g_signal_handlers_disconnect_matched (combo_box->priv->popup_window,
                                        G_SIGNAL_MATCH_DATA,
                                        0, 0, NULL,
                                        gtk_combo_box_list_button_released,
                                        NULL);

  g_signal_handlers_disconnect_matched (combo_box->priv->popup_window,
					G_SIGNAL_MATCH_DATA,
					0, 0, NULL, 
					gtk_combo_box_child_show,
					NULL);

  g_signal_handlers_disconnect_matched (combo_box->priv->popup_window,
					G_SIGNAL_MATCH_DATA,
					0, 0, NULL, 
					gtk_combo_box_child_hide,
					NULL);

  g_signal_handlers_disconnect_matched (combo_box->priv->popup_window,
					G_SIGNAL_MATCH_DATA,
					0, 0, NULL, 
					gtk_combo_box_child_delete_event,
					NULL);
  
  if (combo_box->priv->box)
    g_signal_handlers_disconnect_matched (combo_box->priv->box,
					  G_SIGNAL_MATCH_DATA,
					  0, 0, NULL,
					  gtk_combo_box_list_button_pressed,
					  NULL);

  /* destroy things (unparent will kill the latest ref from us)
   * last unref on button will destroy the arrow
   */
  gtk_widget_unparent (combo_box->priv->button);
  combo_box->priv->button = NULL;
  combo_box->priv->arrow = NULL;

  if (combo_box->priv->cell_view)
    {
      g_object_set (combo_box->priv->cell_view,
                    "background-set", FALSE,
                    NULL);
    }

  if (combo_box->priv->cell_view_frame)
    {
      gtk_widget_unparent (combo_box->priv->cell_view_frame);
      combo_box->priv->cell_view_frame = NULL;
      combo_box->priv->box = NULL;
    }

  if (combo_box->priv->scroll_timer)
    {
      g_source_remove (combo_box->priv->scroll_timer);
      combo_box->priv->scroll_timer = 0;
    }

  if (combo_box->priv->resize_idle_id)
    {
      g_source_remove (combo_box->priv->resize_idle_id);
      combo_box->priv->resize_idle_id = 0;
    }

  gtk_widget_destroy (combo_box->priv->tree_view);

  combo_box->priv->tree_view = NULL;
  if (combo_box->priv->popup_widget)
    {
      g_object_unref (combo_box->priv->popup_widget);
      combo_box->priv->popup_widget = NULL;
    }
}

/* callbacks */

static gboolean
gtk_combo_box_list_button_pressed (GtkWidget      *widget,
                                   GdkEventButton *event,
                                   gpointer        data)
{
  GtkComboBox *combo_box = GTK_COMBO_BOX (data);

  GtkWidget *ewidget = gtk_get_event_widget ((GdkEvent *)event);

  if (ewidget == combo_box->priv->popup_window)
    return TRUE;

  if ((ewidget != combo_box->priv->button && ewidget != combo_box->priv->box) ||
      gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (combo_box->priv->button)))
    return FALSE;

#ifdef MAEMO_CHANGES
  gtk_widget_grab_focus (GTK_WIDGET (combo_box));
#else
  if (combo_box->priv->focus_on_click && 
      !GTK_WIDGET_HAS_FOCUS (combo_box->priv->button))
    gtk_widget_grab_focus (combo_box->priv->button);
#endif /* MAEMO_CHANGES */

  gtk_combo_box_popup (combo_box);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (combo_box->priv->button),
                                TRUE);

  combo_box->priv->auto_scroll = FALSE;
  if (combo_box->priv->scroll_timer == 0)
    combo_box->priv->scroll_timer = g_timeout_add (SCROLL_TIME, 
						   (GSourceFunc) gtk_combo_box_list_scroll_timeout, 
						   combo_box);

  combo_box->priv->popup_in_progress = TRUE;

  return TRUE;
}

static gboolean
gtk_combo_box_list_button_released (GtkWidget      *widget,
                                    GdkEventButton *event,
                                    gpointer        data)
{
  gboolean ret;
  GtkTreePath *path = NULL;
  GtkTreeIter iter;

  GtkComboBox *combo_box = GTK_COMBO_BOX (data);

  gboolean popup_in_progress = FALSE;

  GtkWidget *ewidget = gtk_get_event_widget ((GdkEvent *)event);

  if (combo_box->priv->popup_in_progress)
    {
      popup_in_progress = TRUE;
      combo_box->priv->popup_in_progress = FALSE;
    }

  gtk_tree_view_set_hover_expand (GTK_TREE_VIEW (combo_box->priv->tree_view), 
				  FALSE);
  if (combo_box->priv->scroll_timer)
    {
      g_source_remove (combo_box->priv->scroll_timer);
      combo_box->priv->scroll_timer = 0;
    }

  if (ewidget != combo_box->priv->tree_view)
    {
      if ((ewidget == combo_box->priv->button || 
	   ewidget == combo_box->priv->box) &&
          !popup_in_progress &&
          gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (combo_box->priv->button)))
        {
          gtk_combo_box_popdown (combo_box);
          return TRUE;
        }

      /* released outside treeview */
      if (ewidget != combo_box->priv->button && 
	  ewidget != combo_box->priv->box)
        {
          gtk_combo_box_popdown (combo_box);

          return TRUE;
        }

      return FALSE;
    }

  /* select something cool */
  ret = gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (combo_box->priv->tree_view),
                                       event->x, event->y,
                                       &path,
                                       NULL, NULL, NULL);

  if (!ret)
    return TRUE; /* clicked outside window? */

  gtk_tree_model_get_iter (combo_box->priv->model, &iter, path);
  gtk_tree_path_free (path);

  gtk_combo_box_popdown (combo_box);

  if (tree_column_row_is_sensitive (combo_box, &iter))
    gtk_combo_box_set_active_iter (combo_box, &iter);

  return TRUE;
}

static gboolean
gtk_combo_box_menu_key_press (GtkWidget   *widget,
			      GdkEventKey *event,
			      gpointer     data)
{
  GtkComboBox *combo_box = GTK_COMBO_BOX (data);
  guint state = event->state & gtk_accelerator_get_default_mod_mask ();

  if ((event->keyval == GDK_Up || event->keyval == GDK_KP_Up) && 
      state == GDK_MOD1_MASK)
    {
      gtk_combo_box_popdown (combo_box);

      return TRUE;
    }
  
  return FALSE;
}

static gboolean
gtk_combo_box_list_key_press (GtkWidget   *widget,
                              GdkEventKey *event,
                              gpointer     data)
{
  GtkComboBox *combo_box = GTK_COMBO_BOX (data);
  GtkTreeIter iter;
  guint state = event->state & gtk_accelerator_get_default_mod_mask ();

  if (event->keyval == GDK_Escape ||
#ifdef MAEMO_CHANGES
      event->keyval == GDK_F4 || /* hildon menu key */
      event->keyval == GDK_F5 || /* hildon home key */
#endif /* MAEMO_CHANGES */
      ((event->keyval == GDK_Up || event->keyval == GDK_KP_Up) && 
       state == GDK_MOD1_MASK))
    {
      gtk_combo_box_popdown (combo_box);
      
      /* reset active item -- this is incredibly lame and ugly */
      if (gtk_combo_box_get_active_iter (combo_box, &iter))
	gtk_combo_box_set_active_iter (combo_box, &iter);
      
      return TRUE;
    }
    
  if (event->keyval == GDK_Return || event->keyval == GDK_KP_Enter ||
      event->keyval == GDK_space || event->keyval == GDK_KP_Space) 
  {
    GtkTreeIter iter;
    GtkTreeModel *model = NULL;
    
    gtk_combo_box_popdown (combo_box);
    
    if (combo_box->priv->model)
      {
	GtkTreeSelection *sel;

	sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (combo_box->priv->tree_view));

	if (gtk_tree_selection_get_selected (sel, &model, &iter))
	  gtk_combo_box_set_active_iter (combo_box, &iter);
      }
    
    return TRUE;
  }

  return FALSE;
}

static void
gtk_combo_box_list_auto_scroll (GtkComboBox *combo_box,
				gint         x, 
				gint         y)
{
  GtkWidget *tree_view = combo_box->priv->tree_view;
  GtkAdjustment *adj;
  gdouble value;

  adj = gtk_scrolled_window_get_hadjustment (GTK_SCROLLED_WINDOW (combo_box->priv->scrolled_window));
  if (adj && adj->upper - adj->lower > adj->page_size)
    {
      if (x <= tree_view->allocation.x && 
	  adj->lower < adj->value)
	{
	  value = adj->value - (tree_view->allocation.x - x + 1);
	  gtk_adjustment_set_value (adj, CLAMP (value, adj->lower, adj->upper - adj->page_size));
	}
      else if (x >= tree_view->allocation.x + tree_view->allocation.width &&
	       adj->upper - adj->page_size > adj->value)
	{
	  value = adj->value + (x - tree_view->allocation.x - tree_view->allocation.width + 1);
	  gtk_adjustment_set_value (adj, CLAMP (value, 0.0, adj->upper - adj->page_size));
	}
    }

  adj = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (combo_box->priv->scrolled_window));
  if (adj && adj->upper - adj->lower > adj->page_size)
    {
      if (y <= tree_view->allocation.y && 
	  adj->lower < adj->value)
	{
	  value = adj->value - (tree_view->allocation.y - y + 1);
	  gtk_adjustment_set_value (adj, CLAMP (value, adj->lower, adj->upper - adj->page_size));
	}
      else if (y >= tree_view->allocation.height &&
	       adj->upper - adj->page_size > adj->value)
	{
	  value = adj->value + (y - tree_view->allocation.height + 1);
	  gtk_adjustment_set_value (adj, CLAMP (value, 0.0, adj->upper - adj->page_size));
	}
    }
}

static gboolean
gtk_combo_box_list_scroll_timeout (GtkComboBox *combo_box)
{
  gint x, y;

  GDK_THREADS_ENTER ();

  if (combo_box->priv->auto_scroll)
    {
      gdk_window_get_pointer (combo_box->priv->tree_view->window, 
			      &x, &y, NULL);
      gtk_combo_box_list_auto_scroll (combo_box, x, y);
    }

  GDK_THREADS_LEAVE ();

  return TRUE;
}

static gboolean 
gtk_combo_box_list_enter_notify (GtkWidget        *widget,
				 GdkEventCrossing *event,
				 gpointer          data)
{
  GtkComboBox *combo_box = GTK_COMBO_BOX (data);

  combo_box->priv->auto_scroll = TRUE;

  return TRUE;
}

static gboolean
gtk_combo_box_list_select_func (GtkTreeSelection *selection,
				GtkTreeModel     *model,
				GtkTreePath      *path,
				gboolean          path_currently_selected,
				gpointer          data)
{
  GList *list;
  gboolean sensitive = FALSE;

  for (list = selection->tree_view->priv->columns; list && !sensitive; list = list->next)
    {
      GList *cells, *cell;
      gboolean cell_sensitive, cell_visible;
      GtkTreeIter iter;
      GtkTreeViewColumn *column = GTK_TREE_VIEW_COLUMN (list->data);

      if (!column->visible)
	continue;

      gtk_tree_model_get_iter (model, &iter, path);
      gtk_tree_view_column_cell_set_cell_data (column, model, &iter,
					       FALSE, FALSE);

      cell = cells = gtk_tree_view_column_get_cell_renderers (column);
      while (cell)
        {
	  g_object_get (cell->data,
			"sensitive", &cell_sensitive,
			"visible", &cell_visible,
			NULL);

	  if (cell_visible && cell_sensitive)
	    break;

	  cell = cell->next;
	}
      g_list_free (cells);

      sensitive = cell_sensitive;
    }

  return sensitive;
}

static void
gtk_combo_box_list_row_changed (GtkTreeModel *model,
                                GtkTreePath  *path,
                                GtkTreeIter  *iter,
                                gpointer      data)
{
  GtkComboBox *combo_box = GTK_COMBO_BOX (data);
  gint width;

  width = gtk_combo_box_calc_requested_width (combo_box, path);

  if (width > combo_box->priv->width)
    {
      if (combo_box->priv->cell_view) 
	{
	  gtk_widget_set_size_request (combo_box->priv->cell_view, width, -1);
	  gtk_widget_queue_resize (combo_box->priv->cell_view);
	}
      combo_box->priv->width = width;
    }
}

/*
 * GtkCellLayout implementation
 */

static void
pack_start_recurse (GtkWidget       *menu,
		    GtkCellRenderer *cell,
		    gboolean         expand)
{
  GList *i, *list;
  GtkWidget *submenu;    
  
  list = gtk_container_get_children (GTK_CONTAINER (menu));
  for (i = list; i; i = i->next)
    {
      if (GTK_IS_CELL_LAYOUT (GTK_BIN (i->data)->child))
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (GTK_BIN (i->data)->child), 
				    cell, expand);

      submenu = gtk_menu_item_get_submenu (GTK_MENU_ITEM (i->data));
      if (submenu != NULL)
	pack_start_recurse (submenu, cell, expand);
    }

  g_list_free (list);
}

static void
gtk_combo_box_cell_layout_pack_start (GtkCellLayout   *layout,
                                      GtkCellRenderer *cell,
                                      gboolean         expand)
{
  ComboCellInfo *info;
  GtkComboBox *combo_box;

  g_return_if_fail (GTK_IS_COMBO_BOX (layout));
  g_return_if_fail (GTK_IS_CELL_RENDERER (cell));

  combo_box = GTK_COMBO_BOX (layout);

  g_object_ref_sink (cell);

  info = g_new0 (ComboCellInfo, 1);
  info->cell = cell;
  info->expand = expand;
  info->pack = GTK_PACK_START;

  combo_box->priv->cells = g_slist_append (combo_box->priv->cells, info);

  if (combo_box->priv->cell_view)
    gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo_box->priv->cell_view),
                                cell, expand);

  if (combo_box->priv->column)
    gtk_tree_view_column_pack_start (combo_box->priv->column, cell, expand);

  if (GTK_IS_MENU (combo_box->priv->popup_widget))
    pack_start_recurse (combo_box->priv->popup_widget, cell, expand);
}

static void
pack_end_recurse (GtkWidget       *menu,
		  GtkCellRenderer *cell,
		  gboolean         expand)
{
  GList *i, *list;
  GtkWidget *submenu;    
  
  list = gtk_container_get_children (GTK_CONTAINER (menu));
  for (i = list; i; i = i->next)
    {
      if (GTK_IS_CELL_LAYOUT (GTK_BIN (i->data)->child))
	gtk_cell_layout_pack_end (GTK_CELL_LAYOUT (GTK_BIN (i->data)->child), 
				  cell, expand);

      submenu = gtk_menu_item_get_submenu (GTK_MENU_ITEM (i->data));
      if (submenu != NULL)
	pack_end_recurse (submenu, cell, expand);
    }

  g_list_free (list);
}

static void
gtk_combo_box_cell_layout_pack_end (GtkCellLayout   *layout,
                                    GtkCellRenderer *cell,
                                    gboolean         expand)
{
  ComboCellInfo *info;
  GtkComboBox *combo_box;

  g_return_if_fail (GTK_IS_COMBO_BOX (layout));
  g_return_if_fail (GTK_IS_CELL_RENDERER (cell));

  combo_box = GTK_COMBO_BOX (layout);

  g_object_ref_sink (cell);

  info = g_new0 (ComboCellInfo, 1);
  info->cell = cell;
  info->expand = expand;
  info->pack = GTK_PACK_END;

  combo_box->priv->cells = g_slist_append (combo_box->priv->cells, info);

  if (combo_box->priv->cell_view)
    gtk_cell_layout_pack_end (GTK_CELL_LAYOUT (combo_box->priv->cell_view),
                              cell, expand);

  if (combo_box->priv->column)
    gtk_tree_view_column_pack_end (combo_box->priv->column, cell, expand);

  if (GTK_IS_MENU (combo_box->priv->popup_widget))
    pack_end_recurse (combo_box->priv->popup_widget, cell, expand);
}

static void
clear_recurse (GtkWidget *menu)
{
  GList *i, *list;
  GtkWidget *submenu;    
  
  list = gtk_container_get_children (GTK_CONTAINER (menu));
  for (i = list; i; i = i->next)
    {
      if (GTK_IS_CELL_LAYOUT (GTK_BIN (i->data)->child))
	gtk_cell_layout_clear (GTK_CELL_LAYOUT (GTK_BIN (i->data)->child)); 

      submenu = gtk_menu_item_get_submenu (GTK_MENU_ITEM (i->data));
      if (submenu != NULL)
	clear_recurse (submenu);
    }

  g_list_free (list);
}

static void
gtk_combo_box_cell_layout_clear (GtkCellLayout *layout)
{
  GtkComboBox *combo_box;
  GSList *i;
  
  g_return_if_fail (GTK_IS_COMBO_BOX (layout));

  combo_box = GTK_COMBO_BOX (layout);
 
  if (combo_box->priv->cell_view)
    gtk_cell_layout_clear (GTK_CELL_LAYOUT (combo_box->priv->cell_view));

  if (combo_box->priv->column)
    gtk_tree_view_column_clear (combo_box->priv->column);

  for (i = combo_box->priv->cells; i; i = i->next)
    {
     ComboCellInfo *info = (ComboCellInfo *)i->data;

      gtk_combo_box_cell_layout_clear_attributes (layout, info->cell);
      g_object_unref (info->cell);
      g_free (info);
      i->data = NULL;
    }
  g_slist_free (combo_box->priv->cells);
  combo_box->priv->cells = NULL;

  if (GTK_IS_MENU (combo_box->priv->popup_widget))
    clear_recurse (combo_box->priv->popup_widget);
}

static void
add_attribute_recurse (GtkWidget       *menu,
		       GtkCellRenderer *cell,
		       const gchar     *attribute,
		       gint             column)
{
  GList *i, *list;
  GtkWidget *submenu;    
  
  list = gtk_container_get_children (GTK_CONTAINER (menu));
  for (i = list; i; i = i->next)
    {
      if (GTK_IS_CELL_LAYOUT (GTK_BIN (i->data)->child))
	gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (GTK_BIN (i->data)->child),
				       cell, attribute, column); 

      submenu = gtk_menu_item_get_submenu (GTK_MENU_ITEM (i->data));
      if (submenu != NULL)
	add_attribute_recurse (submenu, cell, attribute, column);
    }

  g_list_free (list);
}
		       
static void
gtk_combo_box_cell_layout_add_attribute (GtkCellLayout   *layout,
                                         GtkCellRenderer *cell,
                                         const gchar     *attribute,
                                         gint             column)
{
  ComboCellInfo *info;
  GtkComboBox *combo_box;

  g_return_if_fail (GTK_IS_COMBO_BOX (layout));
  g_return_if_fail (GTK_IS_CELL_RENDERER (cell));

  combo_box = GTK_COMBO_BOX (layout);

  info = gtk_combo_box_get_cell_info (combo_box, cell);

  info->attributes = g_slist_prepend (info->attributes,
                                      GINT_TO_POINTER (column));
  info->attributes = g_slist_prepend (info->attributes,
                                      g_strdup (attribute));

  if (combo_box->priv->cell_view)
    gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (combo_box->priv->cell_view),
                                   cell, attribute, column);

  if (combo_box->priv->column)
    gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (combo_box->priv->column),
                                   cell, attribute, column);

  if (GTK_IS_MENU (combo_box->priv->popup_widget))
    add_attribute_recurse (combo_box->priv->popup_widget, cell, attribute, column);
  gtk_widget_queue_resize (GTK_WIDGET (combo_box));
}

static void
combo_cell_data_func (GtkCellLayout   *cell_layout,
		      GtkCellRenderer *cell,
		      GtkTreeModel    *tree_model,
		      GtkTreeIter     *iter,
		      gpointer         data)
{
  ComboCellInfo *info = (ComboCellInfo *)data;
  GtkWidget *parent = NULL;
  
  if (!info->func)
    return;

  (*info->func) (cell_layout, cell, tree_model, iter, info->func_data);
  
  if (GTK_IS_WIDGET (cell_layout))
    parent = gtk_widget_get_parent (GTK_WIDGET (cell_layout));
  
  if (GTK_IS_MENU_ITEM (parent) && 
      gtk_menu_item_get_submenu (GTK_MENU_ITEM (parent)))
    g_object_set (cell, "sensitive", TRUE, NULL);
}


static void 
set_cell_data_func_recurse (GtkWidget             *menu,
			    GtkCellRenderer       *cell,
			    ComboCellInfo         *info)
{
  GList *i, *list;
  GtkWidget *submenu;    
  GtkWidget *cell_view;
  
 list = gtk_container_get_children (GTK_CONTAINER (menu));
  for (i = list; i; i = i->next)
    {
      cell_view = GTK_BIN (i->data)->child;
      if (GTK_IS_CELL_LAYOUT (cell_view))
	{
	  /* Override sensitivity for inner nodes; we don't
	   * want menuitems with submenus to appear insensitive 
	   */ 
	  gtk_cell_layout_set_cell_data_func (GTK_CELL_LAYOUT (cell_view), 
					      cell, 
					      combo_cell_data_func, 
					      info, NULL); 
	  submenu = gtk_menu_item_get_submenu (GTK_MENU_ITEM (i->data));
	  if (submenu != NULL)
	    set_cell_data_func_recurse (submenu, cell, info);
	}
    }

  g_list_free (list);
}

static void
gtk_combo_box_cell_layout_set_cell_data_func (GtkCellLayout         *layout,
                                              GtkCellRenderer       *cell,
                                              GtkCellLayoutDataFunc  func,
                                              gpointer               func_data,
                                              GDestroyNotify         destroy)
{
  ComboCellInfo *info;
  GtkComboBox *combo_box;

  g_return_if_fail (GTK_IS_COMBO_BOX (layout));

  combo_box = GTK_COMBO_BOX (layout);

  info = gtk_combo_box_get_cell_info (combo_box, cell);
  g_return_if_fail (info != NULL);
  
  if (info->destroy)
    {
      GDestroyNotify d = info->destroy;

      info->destroy = NULL;
      d (info->func_data);
    }

  info->func = func;
  info->func_data = func_data;
  info->destroy = destroy;

  if (combo_box->priv->cell_view)
    gtk_cell_layout_set_cell_data_func (GTK_CELL_LAYOUT (combo_box->priv->cell_view), cell, func, func_data, NULL);

  if (combo_box->priv->column)
    gtk_cell_layout_set_cell_data_func (GTK_CELL_LAYOUT (combo_box->priv->column), cell, func, func_data, NULL);

  if (GTK_IS_MENU (combo_box->priv->popup_widget))
    set_cell_data_func_recurse (combo_box->priv->popup_widget, cell, info);

  gtk_widget_queue_resize (GTK_WIDGET (combo_box));
}

static void 
clear_attributes_recurse (GtkWidget             *menu,
			  GtkCellRenderer       *cell)
{
  GList *i, *list;
  GtkWidget *submenu;    
  
  list = gtk_container_get_children (GTK_CONTAINER (menu));
  for (i = list; i; i = i->next)
    {
      if (GTK_IS_CELL_LAYOUT (GTK_BIN (i->data)->child))
	gtk_cell_layout_clear_attributes (GTK_CELL_LAYOUT (GTK_BIN (i->data)->child),
					    cell); 
      
      submenu = gtk_menu_item_get_submenu (GTK_MENU_ITEM (i->data));
      if (submenu != NULL)
	clear_attributes_recurse (submenu, cell);
    }

  g_list_free (list);
}

static void
gtk_combo_box_cell_layout_clear_attributes (GtkCellLayout   *layout,
                                            GtkCellRenderer *cell)
{
  ComboCellInfo *info;
  GtkComboBox *combo_box;
  GSList *list;

  g_return_if_fail (GTK_IS_COMBO_BOX (layout));
  g_return_if_fail (GTK_IS_CELL_RENDERER (cell));

  combo_box = GTK_COMBO_BOX (layout);

  info = gtk_combo_box_get_cell_info (combo_box, cell);
  g_return_if_fail (info != NULL);

  list = info->attributes;
  while (list && list->next)
    {
      g_free (list->data);
      list = list->next->next;
    }
  g_slist_free (info->attributes);
  info->attributes = NULL;

  if (combo_box->priv->cell_view)
    gtk_cell_layout_clear_attributes (GTK_CELL_LAYOUT (combo_box->priv->cell_view), cell);

  if (combo_box->priv->column)
    gtk_cell_layout_clear_attributes (GTK_CELL_LAYOUT (combo_box->priv->column), cell);

  if (GTK_IS_MENU (combo_box->priv->popup_widget))
    clear_attributes_recurse (combo_box->priv->popup_widget, cell);

  gtk_widget_queue_resize (GTK_WIDGET (combo_box));
}

static void 
reorder_recurse (GtkWidget             *menu,
		 GtkCellRenderer       *cell,
		 gint                   position)
{
  GList *i, *list;
  GtkWidget *submenu;    
  
  list = gtk_container_get_children (GTK_CONTAINER (menu));
  for (i = list; i; i = i->next)
    {
      if (GTK_IS_CELL_LAYOUT (GTK_BIN (i->data)->child))
	gtk_cell_layout_reorder (GTK_CELL_LAYOUT (GTK_BIN (i->data)->child),
				 cell, position); 
      
      submenu = gtk_menu_item_get_submenu (GTK_MENU_ITEM (i->data));
      if (submenu != NULL)
	reorder_recurse (submenu, cell, position);
    }

  g_list_free (list);
}

static void
gtk_combo_box_cell_layout_reorder (GtkCellLayout   *layout,
                                   GtkCellRenderer *cell,
                                   gint             position)
{
  ComboCellInfo *info;
  GtkComboBox *combo_box;
  GSList *link;

  g_return_if_fail (GTK_IS_COMBO_BOX (layout));
  g_return_if_fail (GTK_IS_CELL_RENDERER (cell));

  combo_box = GTK_COMBO_BOX (layout);

  info = gtk_combo_box_get_cell_info (combo_box, cell);

  g_return_if_fail (info != NULL);
  g_return_if_fail (position >= 0);

  link = g_slist_find (combo_box->priv->cells, info);

  g_return_if_fail (link != NULL);

  combo_box->priv->cells = g_slist_delete_link (combo_box->priv->cells, link);
  combo_box->priv->cells = g_slist_insert (combo_box->priv->cells, info,
                                           position);

  if (combo_box->priv->cell_view)
    gtk_cell_layout_reorder (GTK_CELL_LAYOUT (combo_box->priv->cell_view),
                             cell, position);

  if (combo_box->priv->column)
    gtk_cell_layout_reorder (GTK_CELL_LAYOUT (combo_box->priv->column),
                             cell, position);

  if (GTK_IS_MENU (combo_box->priv->popup_widget))
    reorder_recurse (combo_box->priv->popup_widget, cell, position);

  gtk_widget_queue_draw (GTK_WIDGET (combo_box));
}

/*
 * public API
 */

/**
 * gtk_combo_box_new:
 *
 * Creates a new empty #GtkComboBox.
 *
 * Return value: A new #GtkComboBox.
 *
 * Since: 2.4
 */
GtkWidget *
gtk_combo_box_new (void)
{
  return g_object_new (GTK_TYPE_COMBO_BOX, NULL);
}

/**
 * gtk_combo_box_new_with_model:
 * @model: A #GtkTreeModel.
 *
 * Creates a new #GtkComboBox with the model initialized to @model.
 *
 * Return value: A new #GtkComboBox.
 *
 * Since: 2.4
 */
GtkWidget *
gtk_combo_box_new_with_model (GtkTreeModel *model)
{
  GtkComboBox *combo_box;

  g_return_val_if_fail (GTK_IS_TREE_MODEL (model), NULL);

  combo_box = g_object_new (GTK_TYPE_COMBO_BOX, "model", model, NULL);

  return GTK_WIDGET (combo_box);
}

/**
 * gtk_combo_box_get_wrap_width:
 * @combo_box: A #GtkComboBox.
 *
 * Returns the wrap width which is used to determine the number
 * of columns for the popup menu. If the wrap width is larger than
 * 1, the combo box is in table mode.
 *
 * Returns: the wrap width.
 *
 * Since: 2.6
 */
gint
gtk_combo_box_get_wrap_width (GtkComboBox *combo_box)
{
  g_return_val_if_fail (GTK_IS_COMBO_BOX (combo_box), -1);

  return combo_box->priv->wrap_width;
}

/**
 * gtk_combo_box_set_wrap_width:
 * @combo_box: A #GtkComboBox.
 * @width: Preferred number of columns.
 *
 * Sets the wrap width of @combo_box to be @width. The wrap width is basically
 * the preferred number of columns when you want the popup to be layed out
 * in a table.
 *
 * Since: 2.4
 */
void
gtk_combo_box_set_wrap_width (GtkComboBox *combo_box,
                              gint         width)
{
  g_return_if_fail (GTK_IS_COMBO_BOX (combo_box));
  g_return_if_fail (width >= 0);

  if (width != combo_box->priv->wrap_width)
    {
      combo_box->priv->wrap_width = width;

      gtk_combo_box_check_appearance (combo_box);
      gtk_combo_box_relayout (combo_box);
      
      g_object_notify (G_OBJECT (combo_box), "wrap-width");
    }
}

/**
 * gtk_combo_box_get_row_span_column:
 * @combo_box: A #GtkComboBox.
 *
 * Returns the column with row span information for @combo_box.
 *
 * Returns: the row span column.
 *
 * Since: 2.6
 */
gint
gtk_combo_box_get_row_span_column (GtkComboBox *combo_box)
{
  g_return_val_if_fail (GTK_IS_COMBO_BOX (combo_box), -1);

  return combo_box->priv->row_column;
}

/**
 * gtk_combo_box_set_row_span_column:
 * @combo_box: A #GtkComboBox.
 * @row_span: A column in the model passed during construction.
 *
 * Sets the column with row span information for @combo_box to be @row_span.
 * The row span column contains integers which indicate how many rows
 * an item should span.
 *
 * Since: 2.4
 */
void
gtk_combo_box_set_row_span_column (GtkComboBox *combo_box,
                                   gint         row_span)
{
  gint col;

  g_return_if_fail (GTK_IS_COMBO_BOX (combo_box));

  col = gtk_tree_model_get_n_columns (combo_box->priv->model);
  g_return_if_fail (row_span >= -1 && row_span < col);

  if (row_span != combo_box->priv->row_column)
    {
      combo_box->priv->row_column = row_span;
      
      gtk_combo_box_relayout (combo_box);
 
      g_object_notify (G_OBJECT (combo_box), "row-span-column");
    }
}

/**
 * gtk_combo_box_get_column_span_column:
 * @combo_box: A #GtkComboBox.
 *
 * Returns the column with column span information for @combo_box.
 *
 * Returns: the column span column.
 *
 * Since: 2.6
 */
gint
gtk_combo_box_get_column_span_column (GtkComboBox *combo_box)
{
  g_return_val_if_fail (GTK_IS_COMBO_BOX (combo_box), -1);

  return combo_box->priv->col_column;
}

/**
 * gtk_combo_box_set_column_span_column:
 * @combo_box: A #GtkComboBox.
 * @column_span: A column in the model passed during construction.
 *
 * Sets the column with column span information for @combo_box to be
 * @column_span. The column span column contains integers which indicate
 * how many columns an item should span.
 *
 * Since: 2.4
 */
void
gtk_combo_box_set_column_span_column (GtkComboBox *combo_box,
                                      gint         column_span)
{
  gint col;

  g_return_if_fail (GTK_IS_COMBO_BOX (combo_box));

  col = gtk_tree_model_get_n_columns (combo_box->priv->model);
  g_return_if_fail (column_span >= -1 && column_span < col);

  if (column_span != combo_box->priv->col_column)
    {
      combo_box->priv->col_column = column_span;
      
      gtk_combo_box_relayout (combo_box);

      g_object_notify (G_OBJECT (combo_box), "column-span-column");
    }
}

/**
 * gtk_combo_box_get_active:
 * @combo_box: A #GtkComboBox.
 *
 * Returns the index of the currently active item, or -1 if there's no
 * active item. If the model is a non-flat treemodel, and the active item 
 * is not an immediate child of the root of the tree, this function returns 
 * <literal>gtk_tree_path_get_indices (path)[0]</literal>, where 
 * <literal>path</literal> is the #GtkTreePath of the active item.
 *
 * Return value: An integer which is the index of the currently active item, or
 * -1 if there's no active item.
 *
 * Since: 2.4
 */
gint
gtk_combo_box_get_active (GtkComboBox *combo_box)
{
  gint result;
  g_return_val_if_fail (GTK_IS_COMBO_BOX (combo_box), 0);

  if (gtk_tree_row_reference_valid (combo_box->priv->active_row))
    {
      GtkTreePath *path;

      path = gtk_tree_row_reference_get_path (combo_box->priv->active_row);      
      result = gtk_tree_path_get_indices (path)[0];
      gtk_tree_path_free (path);
    }
  else
    result = -1;

  return result;
}

/**
 * gtk_combo_box_set_active:
 * @combo_box: A #GtkComboBox.
 * @index_: An index in the model passed during construction, or -1 to have
 * no active item.
 *
 * Sets the active item of @combo_box to be the item at @index.
 *
 * Since: 2.4
 */
void
gtk_combo_box_set_active (GtkComboBox *combo_box,
                          gint         index_)
{
  GtkTreePath *path = NULL;
  g_return_if_fail (GTK_IS_COMBO_BOX (combo_box));
  g_return_if_fail (index_ >= -1);

  if (index_ != -1)
    path = gtk_tree_path_new_from_indices (index_, -1);
   
  gtk_combo_box_set_active_internal (combo_box, path);

  if (path)
    gtk_tree_path_free (path);
}

static void
gtk_combo_box_set_active_internal (GtkComboBox *combo_box,
				   GtkTreePath *path)
{
  GtkTreePath *active_path;
  gint path_cmp;

  if (path && gtk_tree_row_reference_valid (combo_box->priv->active_row))
    {
      active_path = gtk_tree_row_reference_get_path (combo_box->priv->active_row);
      path_cmp = gtk_tree_path_compare (path, active_path);
      gtk_tree_path_free (active_path);
      if (path_cmp == 0)
	return;
    }

  if (combo_box->priv->active_row)
    {
      gtk_tree_row_reference_free (combo_box->priv->active_row);
      combo_box->priv->active_row = NULL;
    }
  
  if (!path)
    {
      if (combo_box->priv->tree_view)
        gtk_tree_selection_unselect_all (gtk_tree_view_get_selection (GTK_TREE_VIEW (combo_box->priv->tree_view)));
      else
        {
          GtkMenu *menu = GTK_MENU (combo_box->priv->popup_widget);

          if (GTK_IS_MENU (menu))
            gtk_menu_set_active (menu, -1);
        }

      if (combo_box->priv->cell_view)
        gtk_cell_view_set_displayed_row (GTK_CELL_VIEW (combo_box->priv->cell_view), NULL);
    }
  else
    {
      combo_box->priv->active_row = 
	gtk_tree_row_reference_new (combo_box->priv->model, path);

      if (combo_box->priv->tree_view)
	{
	  gtk_tree_view_set_cursor (GTK_TREE_VIEW (combo_box->priv->tree_view), 
				    path, NULL, FALSE);
	}
      else if (GTK_IS_MENU (combo_box->priv->popup_widget))
        {
	  /* FIXME handle nested menus better */
	  gtk_menu_set_active (GTK_MENU (combo_box->priv->popup_widget), 
			       gtk_tree_path_get_indices (path)[0]);
        }

      if (combo_box->priv->cell_view)
	gtk_cell_view_set_displayed_row (GTK_CELL_VIEW (combo_box->priv->cell_view), 
					 path);
    }

  g_signal_emit (combo_box, combo_box_signals[CHANGED], 0);
  g_object_notify (G_OBJECT (combo_box), "active");
}


/**
 * gtk_combo_box_get_active_iter:
 * @combo_box: A #GtkComboBox
 * @iter: The uninitialized #GtkTreeIter.
 * 
 * Sets @iter to point to the current active item, if it exists.
 * 
 * Return value: %TRUE, if @iter was set
 *
 * Since: 2.4
 **/
gboolean
gtk_combo_box_get_active_iter (GtkComboBox     *combo_box,
                               GtkTreeIter     *iter)
{
  GtkTreePath *path;
  gboolean result;

  g_return_val_if_fail (GTK_IS_COMBO_BOX (combo_box), FALSE);

  if (!gtk_tree_row_reference_valid (combo_box->priv->active_row))
    return FALSE;

  path = gtk_tree_row_reference_get_path (combo_box->priv->active_row);
  result = gtk_tree_model_get_iter (combo_box->priv->model, iter, path);
  gtk_tree_path_free (path);

  return result;
}

/**
 * gtk_combo_box_set_active_iter:
 * @combo_box: A #GtkComboBox
 * @iter: The #GtkTreeIter.
 * 
 * Sets the current active item to be the one referenced by @iter. 
 * @iter must correspond to a path of depth one.
 * 
 * Since: 2.4
 **/
void
gtk_combo_box_set_active_iter (GtkComboBox     *combo_box,
                               GtkTreeIter     *iter)
{
  GtkTreePath *path;

  g_return_if_fail (GTK_IS_COMBO_BOX (combo_box));

  path = gtk_tree_model_get_path (gtk_combo_box_get_model (combo_box), iter);
  gtk_combo_box_set_active_internal (combo_box, path);
  gtk_tree_path_free (path);
}

/**
 * gtk_combo_box_set_model:
 * @combo_box: A #GtkComboBox.
 * @model: A #GtkTreeModel.
 *
 * Sets the model used by @combo_box to be @model. Will unset a previously set 
 * model (if applicable). If model is %NULL, then it will unset the model.
 *
 * Note that this function does not clear the cell renderers, you have to 
 * call gtk_combo_box_cell_layout_clear() yourself if you need to set up 
 * different cell renderers for the new model.
 *
 * Since: 2.4
 */
void
gtk_combo_box_set_model (GtkComboBox  *combo_box,
                         GtkTreeModel *model)
{
  g_return_if_fail (GTK_IS_COMBO_BOX (combo_box));
  g_return_if_fail (model == NULL || GTK_IS_TREE_MODEL (model));

  if (model == combo_box->priv->model)
    return;

  gtk_combo_box_unset_model (combo_box);
#ifdef MAEMO_CHANGES
  maemo_check_autodim (combo_box);
#endif /* MAEMO_CHANGES */

  if (model == NULL)
    return;

  combo_box->priv->model = model;
  g_object_ref (combo_box->priv->model);

  combo_box->priv->inserted_id =
    g_signal_connect (combo_box->priv->model, "row_inserted",
		      G_CALLBACK (gtk_combo_box_model_row_inserted),
		      combo_box);
  combo_box->priv->deleted_id =
    g_signal_connect (combo_box->priv->model, "row_deleted",
		      G_CALLBACK (gtk_combo_box_model_row_deleted),
		      combo_box);
  combo_box->priv->reordered_id =
    g_signal_connect (combo_box->priv->model, "rows_reordered",
		      G_CALLBACK (gtk_combo_box_model_rows_reordered),
		      combo_box);
  combo_box->priv->changed_id =
    g_signal_connect (combo_box->priv->model, "row_changed",
		      G_CALLBACK (gtk_combo_box_model_row_changed),
		      combo_box);
      
  if (combo_box->priv->tree_view)
    {
      /* list mode */
      gtk_tree_view_set_model (GTK_TREE_VIEW (combo_box->priv->tree_view),
                               combo_box->priv->model);
      gtk_combo_box_list_popup_resize (combo_box);
    }
  else
    {
      /* menu mode */
      if (combo_box->priv->popup_widget)
	gtk_combo_box_menu_fill (combo_box);

    }

  if (combo_box->priv->cell_view)
    gtk_cell_view_set_model (GTK_CELL_VIEW (combo_box->priv->cell_view),
                             combo_box->priv->model);
#ifdef MAEMO_CHANGES
  maemo_check_autodim (combo_box);
#endif /* MAEMO_CHANGES */
}

/**
 * gtk_combo_box_get_model
 * @combo_box: A #GtkComboBox.
 *
 * Returns the #GtkTreeModel which is acting as data source for @combo_box.
 *
 * Return value: A #GtkTreeModel which was passed during construction.
 *
 * Since: 2.4
 */
GtkTreeModel *
gtk_combo_box_get_model (GtkComboBox *combo_box)
{
  g_return_val_if_fail (GTK_IS_COMBO_BOX (combo_box), NULL);

  return combo_box->priv->model;
}


/* convenience API for simple text combos */

/**
 * gtk_combo_box_new_text:
 *
 * Convenience function which constructs a new text combo box, which is a
 * #GtkComboBox just displaying strings. If you use this function to create
 * a text combo box, you should only manipulate its data source with the
 * following convenience functions: gtk_combo_box_append_text(),
 * gtk_combo_box_insert_text(), gtk_combo_box_prepend_text() and
 * gtk_combo_box_remove_text().
 *
 * Return value: A new text combo box.
 *
 * Since: 2.4
 */
GtkWidget *
gtk_combo_box_new_text (void)
{
  GtkWidget *combo_box;
  GtkCellRenderer *cell;
  GtkListStore *store;

  store = gtk_list_store_new (1, G_TYPE_STRING);
  combo_box = gtk_combo_box_new_with_model (GTK_TREE_MODEL (store));
  g_object_unref (store);

  cell = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo_box), cell, TRUE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo_box), cell,
                                  "text", 0,
                                  NULL);

  return combo_box;
}

/**
 * gtk_combo_box_append_text:
 * @combo_box: A #GtkComboBox constructed using gtk_combo_box_new_text().
 * @text: A string.
 *
 * Appends @string to the list of strings stored in @combo_box. Note that
 * you can only use this function with combo boxes constructed with
 * gtk_combo_box_new_text().
 *
 * Since: 2.4
 */
void
gtk_combo_box_append_text (GtkComboBox *combo_box,
                           const gchar *text)
{
  GtkTreeIter iter;
  GtkListStore *store;

  g_return_if_fail (GTK_IS_COMBO_BOX (combo_box));
  g_return_if_fail (GTK_IS_LIST_STORE (combo_box->priv->model));
  g_return_if_fail (text != NULL);

  store = GTK_LIST_STORE (combo_box->priv->model);

  gtk_list_store_append (store, &iter);
  gtk_list_store_set (store, &iter, 0, text, -1);
}

/**
 * gtk_combo_box_insert_text:
 * @combo_box: A #GtkComboBox constructed using gtk_combo_box_new_text().
 * @position: An index to insert @text.
 * @text: A string.
 *
 * Inserts @string at @position in the list of strings stored in @combo_box.
 * Note that you can only use this function with combo boxes constructed
 * with gtk_combo_box_new_text().
 *
 * Since: 2.4
 */
void
gtk_combo_box_insert_text (GtkComboBox *combo_box,
                           gint         position,
                           const gchar *text)
{
  GtkTreeIter iter;
  GtkListStore *store;

  g_return_if_fail (GTK_IS_COMBO_BOX (combo_box));
  g_return_if_fail (GTK_IS_LIST_STORE (combo_box->priv->model));
  g_return_if_fail (position >= 0);
  g_return_if_fail (text != NULL);

  store = GTK_LIST_STORE (combo_box->priv->model);

  gtk_list_store_insert (store, &iter, position);
  gtk_list_store_set (store, &iter, 0, text, -1);
}

/**
 * gtk_combo_box_prepend_text:
 * @combo_box: A #GtkComboBox constructed with gtk_combo_box_new_text().
 * @text: A string.
 *
 * Prepends @string to the list of strings stored in @combo_box. Note that
 * you can only use this function with combo boxes constructed with
 * gtk_combo_box_new_text().
 *
 * Since: 2.4
 */
void
gtk_combo_box_prepend_text (GtkComboBox *combo_box,
                            const gchar *text)
{
  GtkTreeIter iter;
  GtkListStore *store;

  g_return_if_fail (GTK_IS_COMBO_BOX (combo_box));
  g_return_if_fail (GTK_IS_LIST_STORE (combo_box->priv->model));
  g_return_if_fail (text != NULL);

  store = GTK_LIST_STORE (combo_box->priv->model);

  gtk_list_store_prepend (store, &iter);
  gtk_list_store_set (store, &iter, 0, text, -1);
}

/**
 * gtk_combo_box_remove_text:
 * @combo_box: A #GtkComboBox constructed with gtk_combo_box_new_text().
 * @position: Index of the item to remove.
 *
 * Removes the string at @position from @combo_box. Note that you can only use
 * this function with combo boxes constructed with gtk_combo_box_new_text().
 *
 * Since: 2.4
 */
void
gtk_combo_box_remove_text (GtkComboBox *combo_box,
                           gint         position)
{
  GtkTreeIter iter;
  GtkListStore *store;

  g_return_if_fail (GTK_IS_COMBO_BOX (combo_box));
  g_return_if_fail (GTK_IS_LIST_STORE (combo_box->priv->model));
  g_return_if_fail (position >= 0);

  store = GTK_LIST_STORE (combo_box->priv->model);

  if (gtk_tree_model_iter_nth_child (combo_box->priv->model, &iter,
                                     NULL, position))
    gtk_list_store_remove (store, &iter);
}

/**
 * gtk_combo_box_get_active_text:
 * @combo_box: A #GtkComboBox constructed with gtk_combo_box_new_text().
 *
 * Returns the currently active string in @combo_box or %NULL if none
 * is selected.  Note that you can only use this function with combo
 * boxes constructed with gtk_combo_box_new_text() and with 
 * #GtkComboBoxEntry<!-- -->s.
 *
 * Returns: a newly allocated string containing the currently active text.
 *
 * Since: 2.6
 */
gchar *
gtk_combo_box_get_active_text (GtkComboBox *combo_box)
{
  GtkComboBoxClass *class;

  g_return_val_if_fail (GTK_IS_COMBO_BOX (combo_box), NULL);

  class = GTK_COMBO_BOX_GET_CLASS (combo_box);

  if (class->get_active_text)
    return (* class->get_active_text) (combo_box);

  return NULL;
}

static gchar *
gtk_combo_box_real_get_active_text (GtkComboBox *combo_box)
{
  GtkTreeIter iter;
  gchar *text = NULL;

  g_return_val_if_fail (GTK_IS_LIST_STORE (combo_box->priv->model), NULL);

  if (gtk_combo_box_get_active_iter (combo_box, &iter))
    gtk_tree_model_get (combo_box->priv->model, &iter, 
			0, &text, -1);

  return text;
}

static void
gtk_combo_box_real_move_active (GtkComboBox   *combo_box,
                                GtkScrollType  scroll)
{
  GtkTreeIter iter;
  GtkTreeIter new_iter;
  gboolean    active_iter;
  gboolean    found;

  if (!combo_box->priv->model)
    {
      gtk_widget_error_bell (GTK_WIDGET (combo_box));
      return;
    }

  active_iter = gtk_combo_box_get_active_iter (combo_box, &iter);

  switch (scroll)
    {
    case GTK_SCROLL_STEP_BACKWARD:
    case GTK_SCROLL_STEP_UP:
    case GTK_SCROLL_STEP_LEFT:
      if (active_iter)
        {
	  found = tree_prev (combo_box, combo_box->priv->model,
			     &iter, &new_iter, FALSE);
	  break;
        }
      /* else fall through */

    case GTK_SCROLL_PAGE_FORWARD:
    case GTK_SCROLL_PAGE_DOWN:
    case GTK_SCROLL_PAGE_RIGHT:
    case GTK_SCROLL_END:
      found = tree_last (combo_box, combo_box->priv->model, &new_iter, FALSE);
      break;

    case GTK_SCROLL_STEP_FORWARD:
    case GTK_SCROLL_STEP_DOWN:
    case GTK_SCROLL_STEP_RIGHT:
      if (active_iter)
        {
	  found = tree_next (combo_box, combo_box->priv->model,
			     &iter, &new_iter, FALSE);
          break;
        }
      /* else fall through */

    case GTK_SCROLL_PAGE_BACKWARD:
    case GTK_SCROLL_PAGE_UP:
    case GTK_SCROLL_PAGE_LEFT:
    case GTK_SCROLL_START:
      found = tree_first (combo_box, combo_box->priv->model, &new_iter, FALSE);
      break;

    default:
      return;
    }

  if (found && active_iter)
    {
      GtkTreePath *old_path;
      GtkTreePath *new_path;

      old_path = gtk_tree_model_get_path (combo_box->priv->model, &iter);
      new_path = gtk_tree_model_get_path (combo_box->priv->model, &new_iter);

      if (gtk_tree_path_compare (old_path, new_path) == 0)
        found = FALSE;

      gtk_tree_path_free (old_path);
      gtk_tree_path_free (new_path);
    }

  if (found)
    {
      gtk_combo_box_set_active_iter (combo_box, &new_iter);
    }
  else
    {
      gtk_widget_error_bell (GTK_WIDGET (combo_box));
    }
}

static gboolean
gtk_combo_box_mnemonic_activate (GtkWidget *widget,
				 gboolean   group_cycling)
{
  GtkComboBox *combo_box = GTK_COMBO_BOX (widget);

  gtk_widget_grab_focus (combo_box->priv->button);

  return TRUE;
}

static void
gtk_combo_box_grab_focus (GtkWidget *widget)
{
  GtkComboBox *combo_box = GTK_COMBO_BOX (widget);

  gtk_widget_grab_focus (combo_box->priv->button);
}

static void
gtk_combo_box_destroy (GtkObject *object)
{
  GtkComboBox *combo_box = GTK_COMBO_BOX (object);

  if (combo_box->priv->popup_idle_id > 0)
    {
      g_source_remove (combo_box->priv->popup_idle_id);
      combo_box->priv->popup_idle_id = 0;
    }

  gtk_combo_box_popdown (combo_box);

  if (combo_box->priv->row_separator_destroy)
    (* combo_box->priv->row_separator_destroy) (combo_box->priv->row_separator_data);

  combo_box->priv->row_separator_func = NULL;
  combo_box->priv->row_separator_data = NULL;
  combo_box->priv->row_separator_destroy = NULL;

  GTK_OBJECT_CLASS (gtk_combo_box_parent_class)->destroy (object);
  combo_box->priv->cell_view = NULL;
}

static void
gtk_combo_box_finalize (GObject *object)
{
  GtkComboBox *combo_box = GTK_COMBO_BOX (object);
  GSList *i;
  
  if (GTK_IS_MENU (combo_box->priv->popup_widget))
    {
      gtk_combo_box_menu_destroy (combo_box);
      gtk_menu_detach (GTK_MENU (combo_box->priv->popup_widget));
      combo_box->priv->popup_widget = NULL;
    }
  
  if (GTK_IS_TREE_VIEW (combo_box->priv->tree_view))
    gtk_combo_box_list_destroy (combo_box);

  if (combo_box->priv->popup_window)
    gtk_widget_destroy (combo_box->priv->popup_window);

  gtk_combo_box_unset_model (combo_box);

  for (i = combo_box->priv->cells; i; i = i->next)
    {
      ComboCellInfo *info = (ComboCellInfo *)i->data;
      GSList *list = info->attributes;

      if (info->destroy)
	info->destroy (info->func_data);

      while (list && list->next)
	{
	  g_free (list->data);
	  list = list->next->next;
	}
      g_slist_free (info->attributes);

      g_object_unref (info->cell);
      g_free (info);
    }
   g_slist_free (combo_box->priv->cells);

   g_free (combo_box->priv->tearoff_title);

   G_OBJECT_CLASS (gtk_combo_box_parent_class)->finalize (object);
}

static gboolean
gtk_cell_editable_key_press (GtkWidget   *widget,
			     GdkEventKey *event,
			     gpointer     data)
{
  GtkComboBox *combo_box = GTK_COMBO_BOX (data);

  if (event->keyval == GDK_Escape)
    {
      combo_box->priv->editing_canceled = TRUE;

      gtk_cell_editable_editing_done (GTK_CELL_EDITABLE (combo_box));
      gtk_cell_editable_remove_widget (GTK_CELL_EDITABLE (combo_box));
      
      return TRUE;
    }
  else if (event->keyval == GDK_Return)
    {
      gtk_cell_editable_editing_done (GTK_CELL_EDITABLE (combo_box));
      gtk_cell_editable_remove_widget (GTK_CELL_EDITABLE (combo_box));
      
      return TRUE;
    }

  return FALSE;
}

static gboolean
popdown_idle (gpointer data)
{
  GtkComboBox *combo_box;

  GDK_THREADS_ENTER ();

  combo_box = GTK_COMBO_BOX (data);
  
  gtk_cell_editable_editing_done (GTK_CELL_EDITABLE (combo_box));
  gtk_cell_editable_remove_widget (GTK_CELL_EDITABLE (combo_box));

  g_object_unref (combo_box);

  GDK_THREADS_LEAVE ();

  return FALSE;
}

static void
popdown_handler (GtkWidget *widget,
		 gpointer   data)
{
  g_idle_add (popdown_idle, g_object_ref (data));
}

static gboolean
popup_idle (gpointer data)
{
  GtkComboBox *combo_box;

  GDK_THREADS_ENTER ();

  combo_box = GTK_COMBO_BOX (data);

  if (GTK_IS_MENU (combo_box->priv->popup_widget) &&
      combo_box->priv->cell_view)
    g_signal_connect_object (combo_box->priv->popup_widget,
			     "unmap", G_CALLBACK (popdown_handler),
			     combo_box, 0);
  
  /* we unset this if a menu item is activated */
  combo_box->priv->editing_canceled = TRUE;
  gtk_combo_box_popup (combo_box);

  combo_box->priv->popup_idle_id = 0;

  GDK_THREADS_LEAVE ();

  return FALSE;
}

static void
gtk_combo_box_start_editing (GtkCellEditable *cell_editable,
			     GdkEvent        *event)
{
  GtkComboBox *combo_box = GTK_COMBO_BOX (cell_editable);

  combo_box->priv->is_cell_renderer = TRUE;

  if (combo_box->priv->cell_view)
    {
      g_signal_connect_object (combo_box->priv->button, "key_press_event",
			       G_CALLBACK (gtk_cell_editable_key_press), 
			       cell_editable, 0);  

      gtk_widget_grab_focus (combo_box->priv->button);
    }
  else
    {
      g_signal_connect_object (GTK_BIN (combo_box)->child, "key_press_event",
			       G_CALLBACK (gtk_cell_editable_key_press), 
			       cell_editable, 0);  

      gtk_widget_grab_focus (GTK_WIDGET (GTK_BIN (combo_box)->child));
      GTK_WIDGET_UNSET_FLAGS (combo_box->priv->button, GTK_CAN_FOCUS);
    }

  /* we do the immediate popup only for the optionmenu-like 
   * appearance 
   */  
  if (combo_box->priv->is_cell_renderer && 
      combo_box->priv->cell_view && !combo_box->priv->tree_view)
    combo_box->priv->popup_idle_id = g_idle_add (popup_idle, combo_box);
}


/**
 * gtk_combo_box_get_add_tearoffs:
 * @combo_box: a #GtkComboBox
 * 
 * Gets the current value of the :add-tearoffs property.
 * 
 * Return value: the current value of the :add-tearoffs property.
 **/
gboolean
gtk_combo_box_get_add_tearoffs (GtkComboBox *combo_box)
{
  g_return_val_if_fail (GTK_IS_COMBO_BOX (combo_box), FALSE);

  return combo_box->priv->add_tearoffs;
}

/**
 * gtk_combo_box_set_add_tearoffs:
 * @combo_box: a #GtkComboBox 
 * @add_tearoffs: %TRUE to add tearoff menu items
 *  
 * Sets whether the popup menu should have a tearoff 
 * menu item.
 *
 * Since: 2.6
 **/
void
gtk_combo_box_set_add_tearoffs (GtkComboBox *combo_box,
				gboolean     add_tearoffs)
{
  g_return_if_fail (GTK_IS_COMBO_BOX (combo_box));

  add_tearoffs = add_tearoffs != FALSE;

  if (combo_box->priv->add_tearoffs != add_tearoffs)
    {
      combo_box->priv->add_tearoffs = add_tearoffs;
      gtk_combo_box_check_appearance (combo_box);
      gtk_combo_box_relayout (combo_box);
      g_object_notify (G_OBJECT (combo_box), "add-tearoffs");
    }
}

/**
 * gtk_combo_box_get_title:
 * @combo_box: a #GtkComboBox
 *
 * Gets the current title of the menu in tearoff mode. See
 * gtk_combo_box_set_add_tearoffs().
 *
 * Returns: the menu's title in tearoff mode. This is an internal copy of the
 * string which must not be freed.
 *
 * Since: 2.10
 */
G_CONST_RETURN gchar*
gtk_combo_box_get_title (GtkComboBox *combo_box)
{
  g_return_val_if_fail (GTK_IS_COMBO_BOX (combo_box), NULL);
  
  return combo_box->priv->tearoff_title;
}

static void
gtk_combo_box_update_title (GtkComboBox *combo_box)
{
  gtk_combo_box_check_appearance (combo_box);
  
  if (combo_box->priv->popup_widget && 
      GTK_IS_MENU (combo_box->priv->popup_widget))
    gtk_menu_set_title (GTK_MENU (combo_box->priv->popup_widget), 
			combo_box->priv->tearoff_title);
}

/**
 * gtk_combo_box_set_title:
 * @combo_box: a #GtkComboBox 
 * @title: a title for the menu in tearoff mode.
 *  
 * Sets the menu's title in tearoff mode.
 *
 * Since: 2.10
 */
void
gtk_combo_box_set_title (GtkComboBox *combo_box,
			 const gchar *title)
{
  g_return_if_fail (GTK_IS_COMBO_BOX (combo_box));

  if (strcmp (title ? title : "", 
	      combo_box->priv->tearoff_title ? combo_box->priv->tearoff_title : "") != 0)
    {
      g_free (combo_box->priv->tearoff_title);
      combo_box->priv->tearoff_title = g_strdup (title);

      gtk_combo_box_update_title (combo_box);

      g_object_notify (G_OBJECT (combo_box), "tearoff-title");
    }
}

gboolean
_gtk_combo_box_editing_canceled (GtkComboBox *combo_box)
{
  g_return_val_if_fail (GTK_IS_COMBO_BOX (combo_box), TRUE);

  return combo_box->priv->editing_canceled;
}

/**
 * gtk_combo_box_get_popup_accessible:
 * @combo_box: a #GtkComboBox
 *
 * Gets the accessible object corresponding to the combo box's popup.
 *
 * This function is mostly intended for use by accessibility technologies;
 * applications should have little use for it.
 *
 * Returns: the accessible object corresponding to the combo box's popup.
 *
 * Since: 2.6
 **/
AtkObject*
gtk_combo_box_get_popup_accessible (GtkComboBox *combo_box)
{
  AtkObject *atk_obj;

  g_return_val_if_fail (GTK_IS_COMBO_BOX (combo_box), NULL);

  if (combo_box->priv->popup_widget)
    {
      atk_obj = gtk_widget_get_accessible (combo_box->priv->popup_widget);
      return atk_obj;
    }

  return NULL;
}

/**
 * gtk_combo_box_get_row_separator_func:
 * @combo_box: a #GtkComboBox
 * 
 * Returns the current row separator function.
 * 
 * Return value: the current row separator function.
 *
 * Since: 2.6
 **/
GtkTreeViewRowSeparatorFunc 
gtk_combo_box_get_row_separator_func (GtkComboBox *combo_box)
{
  g_return_val_if_fail (GTK_IS_COMBO_BOX (combo_box), NULL);

  return combo_box->priv->row_separator_func;
}

/**
 * gtk_combo_box_set_row_separator_func:
 * @combo_box: a #GtkComboBox
 * @func: a #GtkTreeViewRowSeparatorFunc
 * @data: user data to pass to @func, or %NULL
 * @destroy: destroy notifier for @data, or %NULL
 * 
 * Sets the row separator function, which is used to determine
 * whether a row should be drawn as a separator. If the row separator
 * function is %NULL, no separators are drawn. This is the default value.
 *
 * Since: 2.6
 **/
void
gtk_combo_box_set_row_separator_func (GtkComboBox                 *combo_box,
				      GtkTreeViewRowSeparatorFunc  func,
				      gpointer                     data,
				      GtkDestroyNotify             destroy)
{
  g_return_if_fail (GTK_IS_COMBO_BOX (combo_box));

  if (combo_box->priv->row_separator_destroy)
    (* combo_box->priv->row_separator_destroy) (combo_box->priv->row_separator_data);

  combo_box->priv->row_separator_func = func;
  combo_box->priv->row_separator_data = data;
  combo_box->priv->row_separator_destroy = destroy;

  if (combo_box->priv->tree_view)
    gtk_tree_view_set_row_separator_func (GTK_TREE_VIEW (combo_box->priv->tree_view), 
					  func, data, NULL);

  gtk_combo_box_relayout (combo_box);

  gtk_widget_queue_draw (GTK_WIDGET (combo_box));
}


/**
 * gtk_combo_box_set_focus_on_click:
 * @combo: a #GtkComboBox
 * @focus_on_click: whether the combo box grabs focus when clicked 
 *    with the mouse
 * 
 * Sets whether the combo box will grab focus when it is clicked with 
 * the mouse. Making mouse clicks not grab focus is useful in places 
 * like toolbars where you don't want the keyboard focus removed from 
 * the main area of the application.
 *
 * Since: 2.6
 **/
void
gtk_combo_box_set_focus_on_click (GtkComboBox *combo_box,
				  gboolean     focus_on_click)
{
  g_return_if_fail (GTK_IS_COMBO_BOX (combo_box));
  
  focus_on_click = focus_on_click != FALSE;

  if (combo_box->priv->focus_on_click != focus_on_click)
    {
      combo_box->priv->focus_on_click = focus_on_click;

      if (combo_box->priv->button)
	gtk_button_set_focus_on_click (GTK_BUTTON (combo_box->priv->button),
				       focus_on_click);
      
      g_object_notify (G_OBJECT (combo_box), "focus-on-click");
    }
}

/**
 * gtk_combo_box_get_focus_on_click:
 * @combo: a #GtkComboBox
 * 
 * Returns whether the combo box grabs focus when it is clicked 
 * with the mouse. See gtk_combo_box_set_focus_on_click().
 *
 * Return value: %TRUE if the combo box grabs focus when it is 
 *     clicked with the mouse.
 *
 * Since: 2.6
 **/
gboolean
gtk_combo_box_get_focus_on_click (GtkComboBox *combo_box)
{
  g_return_val_if_fail (GTK_IS_COMBO_BOX (combo_box), FALSE);
  
  return combo_box->priv->focus_on_click;
}


#define __GTK_COMBO_BOX_C__
#include "gtkaliasdef.c"
