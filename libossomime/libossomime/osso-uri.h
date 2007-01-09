/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2006 Nokia Corporation.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/* Contact: Andrey Kochanov <andrey.kochanov@nokia.com> */

#ifndef OSSO_URI_H_
#define OSSO_URI_H_

#include <glib.h>

G_BEGIN_DECLS

/*
 * New Additions to the API include:
 *   - OssoURIActionType
 *   - osso_uri_action_get_service()
 *   - osso_uri_action_get_method()
 *   - osso_uri_get_actions_by_uri()
 *   - osso_uri_get_default_action_by_uri()
 *   - osso_uri_set_default_action_by_uri()
 *   - osso_uri_is_default_action_by_uri()
 * 
 * Considerations:
 *   - Finding out if we have the default action by uri may be ok but
 *     really we should do it by mime type and scheme, that makes more
 *     sense to me.
 */

#define OSSO_URI_ERROR osso_uri_error_quark()

/*@}*/

/**
 * \defgroup MIMEICON MIME actions
 */
/*@{*/

typedef struct _OssoURIAction OssoURIAction;

/**
 * OssoURIActionType:
 * @OSSO_URI_ACTION_NORMAL: This is the default and it has no special
 * meaning or value.
 * @OSSO_URI_ACTION_NEUTRAL: This type of action applies to ALL mime
 * types, even if the mime type is unknown.
 * @OSSO_URI_ACTION_FALLBACK: This type of action is used exclusively
 * when the mime type is unknown.
 **/
typedef enum {
	OSSO_URI_ACTION_NORMAL,
	OSSO_URI_ACTION_NEUTRAL,
	OSSO_URI_ACTION_FALLBACK,
} OssoURIActionType;

typedef enum { 
	OSSO_URI_INVALID_URI,
	OSSO_URI_INVALID_ACTION,
	OSSO_URI_INVALID_SCHEME,
	OSSO_URI_NO_DEFAULT_ACTION,
	OSSO_URI_OPEN_FAILED,
	OSSO_URI_SAVE_FAILED,
	OSSO_URI_DBUS_FAILED,
	OSSO_URI_NO_ACTIONS
} OssoURIError;


GQuark             osso_uri_error_quark                    (void) G_GNUC_CONST;


/**
 * osso_uri_action_ref:
 * @action: A @OssoURIAction pointer.
 *
 * Increments the object's reference count.
 *
 * Return: The original pointer @action.
 **/
OssoURIAction *    osso_uri_action_ref                    (OssoURIAction      *action);


/**
 * osso_uri_action_unref:
 * @action: A @OssoURIAction pointer.
 *
 * Decrements the object's reference count. If the count happens to be
 * < 1 after the decrement, @action is freed.
 **/
void               osso_uri_action_unref                  (OssoURIAction      *action);


/**
 * osso_uri_action_get_type:
 * @action: A @OssoURIAction pointer.
 *
 * This returns the type of the @action.
 *
 * Return: A @OssoURIActionType associated with the @action.
 **/
OssoURIActionType  osso_uri_action_get_type               (OssoURIAction      *action);


/**
 * osso_uri_action_get_name:
 * @action: A @OssoURIAction pointer.
 *
 * This returns the name associated with an @action. The name is the
 * detailed description of the action. For example, if you are
 * presenting a list of actions for the user to choose what to do with
 * a URI, the name is what represents this @action.
 *
 * Return: A %const @gchar pointer to the name associated with the
 * @action.
 **/
const gchar *      osso_uri_action_get_name               (OssoURIAction      *action);


/**
 * osso_uri_action_get_service:
 * @action: A @OssoURIAction pointer.
 *
 * This returns the service associated with an @action. The service is
 * the D-Bus service that will be used when opening a URI.
 *
 * Return: A %const @gchar pointer to the service associated with the
 * @action.
 **/
const gchar *      osso_uri_action_get_service            (OssoURIAction       *action);


/**
 * osso_uri_action_get_method:
 * @action: A @OssoURIAction pointer.
 *
 * This returns the method associated with an @action. The method is
 * the D-Bus method that will be used when opening a URI.
 *
 * Return: A %const @gchar pointer to the method associated with the
 * @action.
 **/
const gchar *      osso_uri_action_get_method             (OssoURIAction       *action);

/**
 * osso_uri_action_get_translation_domain:
 * @action: A @OssoURIAction pointer.
 *
 * This returns the translation domain associated with an @action. 
 *
 * Return: A %const @gchar pointer to the translation domain
 * associated with the @action.
 **/
const gchar *      osso_uri_action_get_translation_domain (OssoURIAction      *action);


/**
 * osso_uri_action_get_actions:
 * @scheme: A %const @gchar pointer to a scheme.
 * @error: The address of a pointer to a @GError structure. This is
 * optional and can be %NULL.
 *
 * @Deprecated: Use @osso_uri_get_actions_by_uri() instead.
 *
 * This returns a @GSList pointer to all actions associated with the
 * @scheme. The @scheme can be obtained from a URI by calling
 * osso_uri_get_scheme_from_uri().
 *
 * If %NULL is returned and @error is %NULL then there is no default
 * @OssoURIAction for @scheme. If %NULL is returned and @error is
 * non-%NULL, it will hold the error that occurred while trying to
 * obtain a list of actions. 
 *
 *
 * Return: A @GSList of actions associated with the @scheme. This list
 * is freed using @osso_uri_free_actions(). 
 **/
GSList *           osso_uri_get_actions                   (const gchar        *scheme,
							   GError            **error);


/**
 * osso_uri_action_get_actions_by_uri:
 * @uri: A %const @gchar pointer to a URI.
 * @type: The type of actions you want returned, can be -1 to all.
 * @error: The address of a pointer to a @GError structure. This is
 * optional and can be %NULL.
 *
 * This function operates the same way @osso_uri_get_actions() does
 * but instead uses the mime type of the uri to know which actions it
 * can return and has the added ability to specify the types you want
 * returned. For most cases, this will be -1 to return all actions
 * available for a given URI.
 *
 * Return: A @GSList of actions. This list is freed using
 * @osso_uri_free_actions().
 **/
GSList *           osso_uri_get_actions_by_uri            (const gchar        *uri_str,
							   OssoURIActionType   type,
							   GError            **error);


/**
 * osso_uri_free_actions:
 * @list: A @GSList pointer.
 *
 * The @GSlist is freed and all data members in the list are freed
 * too. 
 **/
void               osso_uri_free_actions                  (GSList             *list);


/**
 * osso_uri_get_scheme_from_uri:
 * @uri: A %const @gchar pointer to a URI.
 * @error: The address of a pointer to a @GError structure. This is
 * optional and can be %NULL.
 *
 * This returns the scheme part of @uri. An example of a scheme would
 * be "http", "callto", "mailto", etc. 
 * 
 * If %NULL is returned and @error is non-%NULL, it will hold the
 * error that occurred while trying to obtain the scheme. 
 *
 * Return: A newly allocated @gchar pointer which must be freed with
 * g_free().
 **/
gchar *            osso_uri_get_scheme_from_uri           (const gchar        *uri,
							   GError            **error);


/**
 * osso_uri_action_is_default_action:
 * @action: A @OssoURIAction pointer. 
 * @error: The address of a pointer to a @GError structure. This is
 * optional and can be %NULL. 
 *
 * Return: The %TRUE if it is the default action or %FALSE.
 **/
gboolean           osso_uri_is_default_action             (OssoURIAction      *action,
							   GError            **error);


/**
 * osso_uri_action_is_default_action_by_uri:
 * @uri: A %const @gchar pointer to a URI.
 * @action: A @OssoURIAction pointer. 
 * @error: The address of a pointer to a @GError structure. This is
 * optional and can be %NULL. 
 *
 * Return: The %TRUE if it is the default action or %FALSE.
 **/
gboolean           osso_uri_is_default_action_by_uri      (const gchar        *uri,
							   OssoURIAction      *action,
							   GError            **error);


/**
 * osso_uri_action_get_default_action:
 * @scheme: A string which represents a scheme. 
 * @error: The address of a pointer to a @GError structure. This is
 * optional and can be %NULL. 
 *
 * This returns the @OssoURIAction which is the default for a
 * @scheme. The @scheme can be obtained from a URI by calling
 * osso_uri_get_scheme_from_uri().  
 * 
 * If %NULL is returned and @error is %NULL then there is no default
 * @OssoURIAction for @scheme. If %NULL is returned and @error is
 * non-%NULL, it will hold the error that occurred while trying to
 * obtain the default @action. 
 *
 * Return: The default @OssoURIAction for @scheme.
 **/
OssoURIAction *    osso_uri_get_default_action            (const gchar        *scheme,
							   GError            **error);


/**
 * osso_uri_action_get_default_action_by_uri
 * @scheme: A string which represents a uri
 * @error: The address of a pointer to a @GError structure. This is
 * optional and can be %NULL. 
 *
 * @Deprecated: Use @osso_uri_get_default_action_by_uri() instead.
 *
 * This returns the @OssoURIAction which is the default for a
 * @uri. This function works similarly to
 * @osso_uri_get_default_action() with the difference being that the
 * mime type of the uri is used to look up the action. The reason for
 * this being that default actions now are effective with scheme and
 * mime type instead of JUST the scheme as before.
 * 
 * If %NULL is returned and @error is %NULL then there is no default
 * @OssoURIAction for @scheme. If %NULL is returned and @error is
 * non-%NULL, it will hold the error that occurred while trying to
 * obtain the default @action. 
 *
 * Return: The default @OssoURIAction for @scheme.
 **/
OssoURIAction *    osso_uri_get_default_action_by_uri     (const gchar        *uri,
							   GError            **error);

/**
 * osso_uri_set_default_action:
 * @scheme: A string which represents a scheme. 
 * @action: A @OssoURIAction pointer.
 * @error: The address of a pointer to a @GError structure. This is
 * optional and can be %NULL.
 *
 * @Deprecated: Use @osso_uri_set_default_action_by_uri() instead.
 *
 * Sets the default @action which should be used with a @scheme when
 * osso_uri_open() is called. The @scheme can be obtained from a URI
 * by calling osso_uri_get_scheme_from_uri().
 * 
 * If @action is NULL, the default action is unset. It is important to
 * note that ONLY the user's default actions are unset NOT the system
 * default actions. The user's default actions are in
 * $home/.local/share/applications/defaults.list (the system
 * default actions are in
 * $prefix/share/applications/defaults.list). This means that if you
 * remove a user's default action, the system default will be used
 * instead if there is one. 
 * 
 * If %FALSE is returned and @error is non-%NULL, it will hold the error
 * that occurred while trying to set the default @action.  
 *
 * Return: %TRUE if it was successfully set or %FALSE.
 **/
gboolean           osso_uri_set_default_action            (const gchar        *scheme,
							   OssoURIAction      *action,
							   GError            **error);

/**
 * osso_uri_set_default_action_by_uri:
 * @scheme: A string which represents a uri. 
 * @action: A @OssoURIAction pointer.
 * @error: The address of a pointer to a @GError structure. This is
 * optional and can be %NULL.
 *
 * Sets the default @action which should be used with a @uri when
 * osso_uri_open() is called. The @action is saved by the @uri scheme
 * and mime type.
 * 
 * If @action is NULL, the default action is unset. It is important to
 * note that ONLY the user's default actions are unset NOT the system
 * default actions. The user's default actions are in
 * $home/.local/share/applications/defaults.list (the system
 * default actions are in
 * $prefix/share/applications/defaults.list). This means that if you
 * remove a user's default action, the system default will be used
 * instead if there is one. 
 * 
 * If %FALSE is returned and @error is non-%NULL, it will hold the error
 * that occurred while trying to set the default @action.  
 *
 * Return: %TRUE if it was successfully set or %FALSE.
 **/
gboolean           osso_uri_set_default_action_by_uri     (const gchar    *uri_str,
							   OssoURIAction  *action,
							   GError        **error);

/**
 * osso_uri_open:
 * @uri: A string which represents a URI. 
 * @action: A @OssoURIAction pointer. This is optional and %NULL can
 * be specified to use the default action instead.
 * @error: The address of a pointer to a @GError structure. This is
 * optional and can be %NULL.
 *
 * This will open the @uri with the application associated with
 * @action. Using the details in @action, a DBus signal is sent to the
 * application to open @uri.
 * 
 * If @action is %NULL then the default action will be tried. If the
 * default action is %NULL, the first available action in the desktop
 * file is used instead. 
 * 
 * If %FALSE is returned and @error is non-%NULL, it will hold the error
 * that occurred while trying to open @uri.
 *
 * Return: %TRUE if successfull or %FALSE.
 **/
gboolean           osso_uri_open                          (const gchar        *uri,
							   OssoURIAction      *action,
							   GError            **error);


 /*@}*/

G_END_DECLS

#endif /* OSSO_URI_H_ */
