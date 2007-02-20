/*
 * This file is part of maemo-af-desktop
 *
 * Copyright (C) 2005, 2006 Nokia Corporation.
 *
 * Contact: Karoliina Salminen <karoliina.t.salminen@nokia.com>
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "statusbar-item-wrapper.h"

#include <string.h>  /* for strcmp */
#include <dlfcn.h>   /* Dynamic library include */

/* Locale includes */
#include <libintl.h>
#include <locale.h>

typedef struct _StatusbarItemWrapperPrivate StatusbarItemWrapperPrivate;

struct _StatusbarItemWrapperPrivate
{
  gint 	      priority; /* deprecated years ago */
  gchar      *library;
  void       *dlhandle;
  void       *data;
  StatusbarItemWrapperAPI_st *fn;
  StatusbarItemWrapperEntryFn  entryfn;
};

enum
{
  SB_SIGNAL_LOG_START,
  SB_SIGNAL_LOG_END,
  SB_SIGNALS_LOG
};

enum 
{
  SB_WRAPPER_PROP_0,
  SB_WRAPPER_PROP_LIB
};

/* item conditional signal */
/*
static guint statusbar_signal = 0;
static guint statusbar_log_signals[SB_SIGNALS_LOG];
*/
/* parent class pointer */
static StatusbarItemClass *parent_class;

/* helper functions */
static gboolean statusbar_item_wrapper_load_symbols( 
		StatusbarItemWrapper *item );
static void statusbar_item_wrapper_warning_function( void );

/* internal widget functions */
static void statusbar_item_wrapper_init (StatusbarItemWrapper *item );
static GObject *statusbar_item_wrapper_constructor (GType gtype, guint n_params, GObjectConstructParam *params);
static void statusbar_item_wrapper_finalize (GObject *obj);
static void statusbar_item_wrapper_class_init (StatusbarItemWrapperClass *item_class );

static void statusbar_item_wrapper_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec);
static void statusbar_item_wrapper_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);
/*
static void statusbar_item_wrapper_forall(GtkContainer *container,
                                          gboolean      include_internals,
                                          GtkCallback   callback,
                                          gpointer      callback_data);
*/

GType statusbar_item_wrapper_get_type( void )
{
    static GType item_type = 0;

    if ( !item_type )
    {
        static const GTypeInfo item_info =
        {
            sizeof (StatusbarItemWrapperClass),
            NULL, /* base_init */
            NULL, /* base_finalize */
            (GClassInitFunc) statusbar_item_wrapper_class_init,
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof (StatusbarItemWrapper),
            0,    /* n_preallocs */
            ( GInstanceInitFunc ) statusbar_item_wrapper_init,
        };

        item_type = g_type_register_static (STATUSBAR_TYPE_ITEM,
                                            "StatusbarItemWrapper",
                                            &item_info,
                                            0);
    }
    
    return item_type;
}

static void 
statusbar_item_wrapper_class_init (StatusbarItemWrapperClass *item_class)
{
  /* set convenience variables */
  /*GtkContainerClass  *container_class = GTK_CONTAINER_CLASS (item_class);*/
  StatusbarItemClass *statusbar_class = STATUSBAR_ITEM_CLASS (item_class);
  GObjectClass *object_class          = G_OBJECT_CLASS (item_class);

  /* set the private structure */
  g_type_class_add_private(item_class, sizeof (StatusbarItemWrapperPrivate));

  /* set the global parent_class here */
  parent_class = g_type_class_peek_parent (item_class);

  /* now the object stuff */
  object_class->constructor       = statusbar_item_wrapper_constructor;
  object_class->finalize          = statusbar_item_wrapper_finalize;

  object_class->get_property      = statusbar_item_wrapper_get_property;
  object_class->set_property      = statusbar_item_wrapper_set_property;

  statusbar_class->condition_update = statusbar_item_wrapper_update_conditional_cb; 
/* TODO: Implement watchdog
  statusbar_log_signals[SB_SIGNAL_LOG_START] = 
	g_signal_new("hildon_status_bar_log_start",
		     G_OBJECT_CLASS_TYPE(object_class),
		     G_SIGNAL_RUN_FIRST,
		     G_STRUCT_OFFSET
		     (StatusbarItemWrapperClass, 
		      hildon_status_bar_log_start),
		     NULL, NULL,
		     g_cclosure_marshal_VOID__VOID, 
		     G_TYPE_NONE, 0);

  statusbar_log_signals[SB_SIGNAL_LOG_END] = 
	g_signal_new("hildon_status_bar_log_end",
		     G_OBJECT_CLASS_TYPE(object_class),
		     G_SIGNAL_RUN_FIRST,
		     G_STRUCT_OFFSET
		     (StatusbarItemWrapperClass, 
		      hildon_status_bar_log_end),
		     NULL, NULL,
		     g_cclosure_marshal_VOID__VOID, 
		     G_TYPE_NONE, 0);
*/

  g_object_class_install_property (object_class,
                                   SB_WRAPPER_PROP_LIB,
                                   g_param_spec_string("library",
                                                       "library with",
                                                       "Name of the file to dlopen",
                                                       NULL,
                                                       G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE));
}


static void statusbar_item_wrapper_init (StatusbarItemWrapper *item)
{
    StatusbarItemWrapperPrivate *priv;

    g_return_if_fail (item); 

    priv = STATUSBAR_ITEM_WRAPPER_GET_PRIVATE( item );

    g_return_if_fail (priv);

    priv->dlhandle = NULL;
    priv->data     = NULL;
    priv->fn       = g_new0 (StatusbarItemWrapperAPI_st, 1);

    priv->entryfn = 
        (StatusbarItemWrapperEntryFn) 
	statusbar_item_wrapper_warning_function;

    priv->fn->initialize = 
        (StatusbarItemWrapperInitializeFn) 
        statusbar_item_wrapper_warning_function;

    priv->fn->destroy = 
        (StatusbarItemWrapperDestroyFn) 
        statusbar_item_wrapper_warning_function;

    priv->fn->update =
        (StatusbarItemWrapperUpdateFn) 
	statusbar_item_wrapper_warning_function;

    priv->fn->get_priority = 
        (StatusbarItemWrapperGetPriorityFn) 
        statusbar_item_wrapper_warning_function;

    priv->fn->set_conditional = 
	(StatusbarItemWrapperSetConditionalFn)
	statusbar_item_wrapper_warning_function;
    
    GTK_WIDGET_SET_FLAGS (item, GTK_NO_WINDOW);
}

static GObject *  
statusbar_item_wrapper_constructor (GType gtype, 
				    guint n_params, 
				    GObjectConstructParam *params)
{
  GObject *self;
  StatusbarItemWrapper *wrapper;
  StatusbarItemWrapperPrivate *priv;
  GtkWidget *button;
  gchar *entryAPI_name, *error_str;

  self    = G_OBJECT_CLASS (parent_class)->constructor (gtype, n_params, params);
  wrapper = STATUSBAR_ITEM_WRAPPER (self);
  priv    = STATUSBAR_ITEM_WRAPPER_GET_PRIVATE (wrapper);

  priv->dlhandle  = dlopen (priv->library, RTLD_NOW);

  if (!priv->dlhandle)
  {
    g_debug ("SBW: I can't open %s %s", priv->library, dlerror());
    gtk_object_sink (GTK_OBJECT (self)); /* FIXME: This or unref? */
    return self; /* FIXME: NULL or self? */
  }
  
  entryAPI_name = g_strdup_printf( "%s_entry", HILDON_DESKTOP_ITEM (self)->name);
  priv->entryfn = (StatusbarItemWrapperEntryFn) dlsym (priv->dlhandle, entryAPI_name);

  g_free (entryAPI_name);

  if ((error_str = dlerror()) != NULL)
  {
    g_debug ("SBW: Unable to load entry symbol for plugin %s: %s",
	     HILDON_DESKTOP_ITEM (self)->name, error_str);

      gtk_object_sink (GTK_OBJECT (self));
      return self; /* FIXME: NULL or self? */
  }

  if (!statusbar_item_wrapper_load_symbols (wrapper))
  {
      gtk_object_sink (GTK_OBJECT (self));
      return self; /* FIXME: NULL or self? */
  }

  /* FIXME: Move this out!!!! 
  g_signal_emit (G_OBJECT(item), statusbar_log_signals[SB_SIGNAL_LOG_START], 0, NULL);	
  */

  /* Initialize the plugin */
  priv->data = priv->fn->initialize (wrapper, &button);

  /* Conditional plugins don't need to implement button on initialization */
  if (button)
  { 
    g_object_set (G_OBJECT (button), "can-focus", FALSE, NULL);
    gtk_container_add (GTK_CONTAINER (self), button);
  }

  /* Get the priority for this item */
  priv->priority = priv->fn->get_priority( priv->data );
/*
  g_signal_emit ( G_OBJECT(item), statusbar_log_signals[SB_SIGNAL_LOG_END], 0, NULL);
*/
  return self;
}

static void 
statusbar_item_wrapper_set_property (GObject *object,
                           guint prop_id,
                           const GValue *value,
                           GParamSpec *pspec)
{
  StatusbarItemWrapper *item;
  StatusbarItemWrapperPrivate *priv;

  g_assert (object && STATUSBAR_IS_ITEM_WRAPPER (object));

  item = STATUSBAR_ITEM_WRAPPER (object);
  priv = STATUSBAR_ITEM_WRAPPER_GET_PRIVATE (item);

  switch (prop_id)
  {
    case SB_WRAPPER_PROP_LIB:
      g_free (priv->library);
      priv->library = g_strdup (g_value_get_string (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  } 
}

static void 
statusbar_item_wrapper_get_property (GObject *object,
                                     guint prop_id,
                                     GValue *value,
                                     GParamSpec *pspec)
{
  StatusbarItemWrapper *item;
  StatusbarItemWrapperPrivate *priv;

  g_assert (object && STATUSBAR_IS_ITEM_WRAPPER (object));

  item = STATUSBAR_ITEM_WRAPPER (object);
  priv = STATUSBAR_ITEM_WRAPPER_GET_PRIVATE (item);

  switch (prop_id)
  {
    case SB_WRAPPER_PROP_LIB:
      g_value_set_string (value,priv->library);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;						
  }
}

static void 
statusbar_item_wrapper_finalize (GObject *obj)
{
  StatusbarItemWrapperPrivate *priv;

  g_return_if_fail (obj);

  priv = STATUSBAR_ITEM_WRAPPER_GET_PRIVATE (obj);

  /* allow the lib to clean up itself */
  if (priv->fn && priv->fn->destroy)
    priv->fn->destroy (priv->data);

  if (priv->dlhandle)
    dlclose (priv->dlhandle); 

  g_free (priv->fn);
  g_free (priv->library);

  if (G_OBJECT_CLASS (parent_class)->finalize)
    G_OBJECT_CLASS( parent_class )->finalize( obj );
}

static void 
statusbar_item_wrapper_warning_function( void )
{
  g_debug ("Item not properly loaded.\n"  
           "\tThis function is only called, if a item couldn't initialise "
           "itself properly.\n"
           "\tSee statusbar_item_wrapper_init/new for more information\n" );
}

static gboolean 
statusbar_item_wrapper_load_symbols( StatusbarItemWrapper *item ) 
{
  StatusbarItemWrapperPrivate *priv;

  g_return_val_if_fail( item, 0 );

  priv = STATUSBAR_ITEM_WRAPPER_GET_PRIVATE (item);

  /* NULL the function pointers, so we detect easily if 
   * the plugin don't set them */
  priv->fn->initialize = NULL;
  priv->fn->destroy = NULL;
  priv->fn->update = NULL;
  priv->fn->get_priority = NULL;
  priv->fn->set_conditional = NULL;

  /* Ask plugin to set the function pointers */
  priv->entryfn (priv->fn);

  /* check for mandatory functions */
  if (!priv->fn->initialize)
  {
    g_debug ("StatusbarItemWrapper: "
             "Plugin '%s' did not set initialize()", HILDON_DESKTOP_ITEM (item)->name);
    return FALSE;
  }

  if (!priv->fn->get_priority)
  {
    g_debug ("StatusbarItemWrapper: "
             "Plugin '%s' did not set get_priority()", HILDON_DESKTOP_ITEM (item)->name);
        return FALSE;
  }

  if (!priv->fn->destroy)
  {
    g_debug ("StatusbarItemWrapper: "
             "Plugin '%s' did not set destroy()", HILDON_DESKTOP_ITEM (item)->name);
    return FALSE;
  }

  /* if plugin is conditional */   
  if (priv->fn->set_conditional)
    STATUSBAR_ITEM (item)->conditional = FALSE;

  return TRUE;
}

StatusbarItemWrapper *
statusbar_item_wrapper_new (const gchar *name, 
			    const gchar *library, 
			    gboolean mandatory)
{
  StatusbarItemWrapper *item;
  
  g_return_val_if_fail (name && library, NULL);

  item = g_object_new (STATUSBAR_TYPE_ITEM_WRAPPER, 
		       "name", name,
		       "library", library,
		       "mandatory", mandatory,
		       NULL);
  
  return item;
}

void 
statusbar_item_wrapper_update (StatusbarItemWrapper *item,
                               gint value1,
                               gint value2,
                               const gchar *str )
{
  StatusbarItemWrapperPrivate *priv;

  priv = STATUSBAR_ITEM_WRAPPER_GET_PRIVATE (item);

  if (priv->fn->update)
  {
    /* Must ref, because the plugin can call gtk_widget_destroy */
    g_object_ref (item);
    priv->fn->update (priv->data, value1, value2, str);
    g_object_unref (item);
  } 
}

/* Function for updating item conditional state */
void 
statusbar_item_wrapper_update_conditional_cb (StatusbarItem *sbitem, 
		                              gboolean user_data)
{ 
  StatusbarItemWrapper *item = STATUSBAR_ITEM_WRAPPER (sbitem);
  StatusbarItemWrapperPrivate *priv;

  priv = STATUSBAR_ITEM_WRAPPER_GET_PRIVATE (item);

  gboolean is_conditional = (gboolean)user_data;
  /* Must ref, because the plugin can call gtk_widget_destroy */
  g_object_ref (item);
  priv->fn->set_conditional (priv->data, is_conditional);
  g_object_unref (item);

  STATUSBAR_ITEM (item)->conditional = is_conditional;
}
