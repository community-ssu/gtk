/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * This is file is part of libhildonmime
 *
 * Copyright (C) 2004-2007 Nokia Corporation.
 *
 * Contact: Erik Karlsson <erik.b.karlsson@nokia.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; version 2.1 of the
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
#ifndef HILDON_URI_H
#define HILDON_URI_H

#include <glib.h>

G_BEGIN_DECLS

/*
 * New Additions to the API include:
 *   - HildonURIActionType
 *   - hildon_uri_action_get_service()
 *   - hildon_uri_action_get_method()
 *   - hildon_uri_get_actions_by_uri()
 *   - hildon_uri_get_default_action_by_uri()
 *   - hildon_uri_set_default_action_by_uri()
 *   - hildon_uri_is_default_action_by_uri()
 * 
 * Considerations:
 *   - Finding out if we have the default action by uri may be ok but
 *     really we should do it by mime type and scheme, that makes more
 *     sense to me.
 */

#define HILDON_URI_ERROR hildon_uri_error_quark()

/*@}*/

/**
 * \defgroup MIMEICON MIME actions
 */
/*@{*/

typedef struct _HildonURIAction HildonURIAction;

/**
 * HildonURIActionType:
 * @HILDON_URI_ACTION_NORMAL: This is the default and it has no special
 * meaning or value.
 * @HILDON_URI_ACTION_NEUTRAL: This type of action applies to ALL mime
 * types, even if the mime type is unknown.
 * @HILDON_URI_ACTION_FALLBACK: This type of action is used exclusively
 * when the mime type is unknown.
 **/
typedef enum {
	HILDON_URI_ACTION_NORMAL,
	HILDON_URI_ACTION_NEUTRAL,
	HILDON_URI_ACTION_FALLBACK,
} HildonURIActionType;

typedef enum { 
	HILDON_URI_INVALID_URI,
	HILDON_URI_INVALID_ACTION,
	HILDON_URI_INVALID_SCHEME,
	HILDON_URI_NO_DEFAULT_ACTION,
	HILDON_URI_OPEN_FAILED,
	HILDON_URI_SAVE_FAILED,
	HILDON_URI_DBUS_FAILED,
	HILDON_URI_NO_ACTIONS
} HildonURIError;

GQuark              hildon_uri_error_quark                   (void) G_GNUC_CONST;

HildonURIAction *   hildon_uri_action_ref                    (HildonURIAction      *action);
void                hildon_uri_action_unref                  (HildonURIAction      *action);
HildonURIActionType hildon_uri_action_get_type               (HildonURIAction      *action);
const gchar *       hildon_uri_action_get_name               (HildonURIAction      *action);
const gchar *       hildon_uri_action_get_service            (HildonURIAction      *action);
const gchar *       hildon_uri_action_get_method             (HildonURIAction      *action);
const gchar *       hildon_uri_action_get_translation_domain (HildonURIAction      *action);

GSList *            hildon_uri_get_actions                   (const gchar          *scheme,
							      GError              **error);
GSList *            hildon_uri_get_actions_by_uri            (const gchar          *uri_str,
							      HildonURIActionType   type,
							      GError              **error);
void                hildon_uri_free_actions                  (GSList               *list);

gchar *             hildon_uri_get_scheme_from_uri           (const gchar          *uri,
							      GError              **error);
gboolean            hildon_uri_is_default_action             (HildonURIAction      *action,
							      GError              **error);
gboolean            hildon_uri_is_default_action_by_uri      (const gchar          *uri,
							      HildonURIAction      *action,
							      GError              **error);
HildonURIAction *   hildon_uri_get_default_action            (const gchar          *scheme,
							      GError              **error);
HildonURIAction *   hildon_uri_get_default_action_by_uri     (const gchar          *uri,
							      GError              **error);
gboolean            hildon_uri_set_default_action            (const gchar          *scheme,
							      HildonURIAction      *action,
							      GError              **error);
gboolean            hildon_uri_set_default_action_by_uri     (const gchar          *uri_str,
							      HildonURIAction      *action,
							      GError              **error);
gboolean            hildon_uri_open                          (const gchar          *uri,
							      HildonURIAction      *action,
							      GError              **error);
 /*@}*/

G_END_DECLS

#endif /* HILDON_URI_H */
