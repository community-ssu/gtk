/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*

   Copyright (C) 2003 Red Hat, Inc

   The Gnome Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Gnome Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Gnome Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   Author: Alexander Larsson <alexl@redhat.com>
*/


#include <config.h>
#include <string.h>
#include "gnome-vfs-module-callback.h"

#include "gnome-vfs-module-callback-module-api.h"
#include "gnome-vfs-module-callback-private.h"
#include "gnome-vfs-backend.h"
#include "gnome-vfs-private.h"
#include "gnome-vfs-standard-callbacks.h"
#include "gnome-vfs-dbus-utils.h"


static void
utils_append_string_or_null (DBusMessageIter *iter,
			     const gchar     *str)
{
	if (str == NULL)
		str = "";
	
	dbus_message_iter_append_basic (iter, DBUS_TYPE_STRING, &str);
}

static gchar *
utils_get_string_or_null (DBusMessageIter *iter, gboolean empty_is_null)
{
	const gchar *str;

	dbus_message_iter_get_basic (iter, &str);

	if (empty_is_null && *str == 0) {
		return NULL;
	} else {
		return g_strdup (str);
	}
}


static gboolean
fill_auth_marshal_in (gconstpointer in, gsize in_size, DBusMessageIter *iter)
{
	const GnomeVFSModuleCallbackFillAuthenticationIn *auth_in;
	gint32 i;
	
	if (in_size != sizeof (GnomeVFSModuleCallbackFillAuthenticationIn)) {
		return FALSE;
	}
	auth_in = in;

	utils_append_string_or_null (iter, auth_in->uri);
	utils_append_string_or_null (iter, auth_in->protocol);
	utils_append_string_or_null (iter, auth_in->server);
	utils_append_string_or_null (iter, auth_in->object);
	i = auth_in->port;
	dbus_message_iter_append_basic (iter, DBUS_TYPE_INT32, &i);
	utils_append_string_or_null (iter, auth_in->username);
	utils_append_string_or_null (iter, auth_in->authtype);
	utils_append_string_or_null (iter, auth_in->domain);
	
	return TRUE;
}

static gboolean
fill_auth_demarshal_in (DBusMessageIter *iter,
			gpointer *in, gsize *in_size,
			gpointer *out, gsize *out_size)
{
	GnomeVFSModuleCallbackFillAuthenticationIn *auth_in;
	gint32 i;
	
	auth_in = *in = g_new0 (GnomeVFSModuleCallbackFillAuthenticationIn, 1);
	*in_size = sizeof (GnomeVFSModuleCallbackFillAuthenticationIn);
	*out = g_new0 (GnomeVFSModuleCallbackFillAuthenticationOut, 1);
	*out_size = sizeof (GnomeVFSModuleCallbackFillAuthenticationOut);

	auth_in->uri = utils_get_string_or_null (iter, TRUE);
	dbus_message_iter_next (iter);
	auth_in->protocol = utils_get_string_or_null (iter, TRUE);
	dbus_message_iter_next (iter);
	auth_in->server = utils_get_string_or_null (iter, TRUE);
	dbus_message_iter_next (iter);
	auth_in->object = utils_get_string_or_null (iter, TRUE);
	dbus_message_iter_next (iter);
	dbus_message_iter_get_basic (iter, &i);
	auth_in->port = i;
	dbus_message_iter_next (iter);
	auth_in->username = utils_get_string_or_null (iter, TRUE);
	dbus_message_iter_next (iter);
	auth_in->authtype = utils_get_string_or_null (iter, TRUE);
	dbus_message_iter_next (iter);
	auth_in->domain = utils_get_string_or_null (iter, TRUE);
	dbus_message_iter_next (iter);
	
	return TRUE;
}

static gboolean
fill_auth_marshal_out (gconstpointer out, gsize out_size, DBusMessageIter *iter)
{
	const GnomeVFSModuleCallbackFillAuthenticationOut *auth_out;
	gboolean b;

	if (out_size != sizeof (GnomeVFSModuleCallbackFillAuthenticationOut)) {
		return FALSE;
	}

	auth_out = out;
	
	b = auth_out->valid;
	dbus_message_iter_append_basic (iter, DBUS_TYPE_BOOLEAN, &b);
	utils_append_string_or_null (iter, auth_out->username);
	utils_append_string_or_null (iter, auth_out->domain);
	utils_append_string_or_null (iter, auth_out->password);

	return TRUE;
}

static gboolean
fill_auth_demarshal_out (DBusMessageIter *iter, gpointer out, gsize out_size)
{
	GnomeVFSModuleCallbackFillAuthenticationOut *auth_out;
	gboolean b;

	if (out_size != sizeof (GnomeVFSModuleCallbackFillAuthenticationOut)) {
		return FALSE;
	}
	auth_out = out;

	dbus_message_iter_get_basic (iter, &b);
	auth_out->valid = b;
	dbus_message_iter_next (iter);
	auth_out->username = utils_get_string_or_null (iter, TRUE);
	dbus_message_iter_next (iter);
	auth_out->domain = utils_get_string_or_null (iter, TRUE);
	dbus_message_iter_next (iter);
	auth_out->password = utils_get_string_or_null (iter, TRUE);
	
	return TRUE;
}

static void
fill_auth_free_in (gpointer in)
{
	GnomeVFSModuleCallbackFillAuthenticationIn *auth_in;

	auth_in = in;

	g_free (auth_in->uri);
	g_free (auth_in->protocol);
	g_free (auth_in->server);
	g_free (auth_in->object);
	g_free (auth_in->authtype);
	g_free (auth_in->username);
	g_free (auth_in->domain);

	g_free (auth_in);
}

static void
fill_auth_free_out (gpointer out)
{
	GnomeVFSModuleCallbackFillAuthenticationOut *auth_out;

	auth_out = out;
	g_free (auth_out->username);
	g_free (auth_out->domain);
	g_free (auth_out->password);
	g_free (auth_out);
}


static gboolean
auth_marshal_in (gconstpointer in, gsize in_size, DBusMessageIter *iter)
{
	const GnomeVFSModuleCallbackAuthenticationIn *auth_in;
	gboolean b;
	gint32 i;
	
	if (in_size != sizeof (GnomeVFSModuleCallbackAuthenticationIn)) {
		return FALSE;
	}
	auth_in = in;

	utils_append_string_or_null (iter, auth_in->uri);
	utils_append_string_or_null (iter, auth_in->realm);
	b = auth_in->previous_attempt_failed;
	dbus_message_iter_append_basic (iter, DBUS_TYPE_BOOLEAN, &b);
	i = auth_in->auth_type;
	dbus_message_iter_append_basic (iter, DBUS_TYPE_INT32, &i);
	
	return TRUE;
}

static gboolean
auth_demarshal_in (DBusMessageIter *iter,
		   gpointer *in, gsize *in_size,
		   gpointer *out, gsize *out_size)
{
	GnomeVFSModuleCallbackAuthenticationIn *auth_in;
	gboolean b;
	gint32 i;
	
	auth_in = *in = g_new0 (GnomeVFSModuleCallbackAuthenticationIn, 1);
	*in_size = sizeof (GnomeVFSModuleCallbackAuthenticationIn);
	*out = g_new0 (GnomeVFSModuleCallbackAuthenticationOut, 1);
	*out_size = sizeof (GnomeVFSModuleCallbackAuthenticationOut);

	auth_in->uri = utils_get_string_or_null (iter, TRUE);
	dbus_message_iter_next (iter);
	auth_in->realm = utils_get_string_or_null (iter, TRUE);
	dbus_message_iter_next (iter);
	dbus_message_iter_get_basic (iter, &b);
	auth_in->previous_attempt_failed = b;
	dbus_message_iter_next (iter);
	dbus_message_iter_get_basic (iter, &i);
	auth_in->auth_type = i;
	dbus_message_iter_next (iter);
	
	return TRUE;
}

static gboolean
auth_marshal_out (gconstpointer out, gsize out_size, DBusMessageIter *iter)
{
	const GnomeVFSModuleCallbackAuthenticationOut *auth_out;
	gboolean b;

	if (out_size != sizeof (GnomeVFSModuleCallbackAuthenticationOut)) {
		return FALSE;
	}
	auth_out = out;

	b = auth_out->username == NULL;
	dbus_message_iter_append_basic (iter, DBUS_TYPE_BOOLEAN, &b);
	utils_append_string_or_null (iter, auth_out->username);
	utils_append_string_or_null (iter, auth_out->password);

	return TRUE;
}

static gboolean
auth_demarshal_out (DBusMessageIter *iter, gpointer out, gsize out_size)
{
	GnomeVFSModuleCallbackAuthenticationOut *auth_out;
	gboolean b;

	if (out_size != sizeof (GnomeVFSModuleCallbackAuthenticationOut)) {
		return FALSE;
	}
	auth_out = out;


	dbus_message_iter_get_basic (iter, &b);
	dbus_message_iter_next (iter);
	
	auth_out->username = utils_get_string_or_null (iter, b);
	dbus_message_iter_next (iter);

	auth_out->password = utils_get_string_or_null (iter, b);
	dbus_message_iter_next (iter);

	return TRUE;
}

static void
auth_free_in (gpointer in)
{
	GnomeVFSModuleCallbackAuthenticationIn *auth_in;

	auth_in = in;

	g_free (auth_in->uri);
	g_free (auth_in->realm);

	g_free (auth_in);
}

static void
auth_free_out (gpointer out)
{
	GnomeVFSModuleCallbackAuthenticationOut *auth_out;

	auth_out = out;
	g_free (auth_out->username);
	g_free (auth_out->password);
	g_free (auth_out);
}

static gboolean
full_auth_marshal_in (gconstpointer in, gsize in_size, DBusMessageIter *iter)
{
	const GnomeVFSModuleCallbackFullAuthenticationIn *auth_in;
	gint32 i;
	
	if (in_size != sizeof (GnomeVFSModuleCallbackFullAuthenticationIn)) {
		return FALSE;
	}
	auth_in = in;

	i = auth_in->flags;
	dbus_message_iter_append_basic (iter, DBUS_TYPE_INT32, &i);
	utils_append_string_or_null (iter, auth_in->uri);
	utils_append_string_or_null (iter, auth_in->protocol);
	utils_append_string_or_null (iter, auth_in->server);
	utils_append_string_or_null (iter, auth_in->object);
	i = auth_in->port;
	dbus_message_iter_append_basic (iter, DBUS_TYPE_INT32, &i);
	utils_append_string_or_null (iter, auth_in->username);
	utils_append_string_or_null (iter, auth_in->authtype);
	utils_append_string_or_null (iter, auth_in->domain);
	utils_append_string_or_null (iter, auth_in->default_user);
	utils_append_string_or_null (iter, auth_in->default_domain);

	return TRUE;
}

static gboolean
full_auth_demarshal_in (DBusMessageIter *iter,
			gpointer *in, gsize *in_size,
			gpointer *out, gsize *out_size)
{
	GnomeVFSModuleCallbackFullAuthenticationIn *auth_in;
	gint32 i;
	
	auth_in = *in = g_new0 (GnomeVFSModuleCallbackFullAuthenticationIn, 1);
	*in_size = sizeof (GnomeVFSModuleCallbackFullAuthenticationIn);
	*out = g_new0 (GnomeVFSModuleCallbackFullAuthenticationOut, 1);
	*out_size = sizeof (GnomeVFSModuleCallbackFullAuthenticationOut);


	dbus_message_iter_get_basic (iter, &i);
	auth_in->flags = i;
	dbus_message_iter_next (iter);
	auth_in->uri = utils_get_string_or_null (iter, TRUE);
	dbus_message_iter_next (iter);
	auth_in->protocol = utils_get_string_or_null (iter, TRUE);
	dbus_message_iter_next (iter);
	auth_in->server = utils_get_string_or_null (iter, TRUE);
	dbus_message_iter_next (iter);
	auth_in->object = utils_get_string_or_null (iter, TRUE);
	dbus_message_iter_next (iter);
	dbus_message_iter_get_basic (iter, &i);
	auth_in->port = i;
	dbus_message_iter_next (iter);
	auth_in->username = utils_get_string_or_null (iter, TRUE);
	dbus_message_iter_next (iter);
	auth_in->authtype = utils_get_string_or_null (iter, TRUE);
	dbus_message_iter_next (iter);
	auth_in->domain = utils_get_string_or_null (iter, TRUE);
	dbus_message_iter_next (iter);
	auth_in->default_user = utils_get_string_or_null (iter, TRUE);
	dbus_message_iter_next (iter);
	auth_in->default_domain = utils_get_string_or_null (iter, TRUE);
	dbus_message_iter_next (iter);
	
	return TRUE;
}

static gboolean
full_auth_marshal_out (gconstpointer out, gsize out_size, DBusMessageIter *iter)
{
	const GnomeVFSModuleCallbackFullAuthenticationOut *auth_out;
	gboolean b;

	if (out_size != sizeof (GnomeVFSModuleCallbackFullAuthenticationOut)) {
		return FALSE;
	}
	auth_out = out;

	b = auth_out->abort_auth;
	dbus_message_iter_append_basic (iter, DBUS_TYPE_BOOLEAN, &b);
	utils_append_string_or_null (iter, auth_out->username);
	utils_append_string_or_null (iter, auth_out->domain);
	utils_append_string_or_null (iter, auth_out->password);
	b = auth_out->save_password;
	dbus_message_iter_append_basic (iter, DBUS_TYPE_BOOLEAN, &b);
	utils_append_string_or_null (iter, auth_out->keyring);
	
	return TRUE;
}

static gboolean
full_auth_demarshal_out (DBusMessageIter *iter, gpointer out, gsize out_size)
{
	GnomeVFSModuleCallbackFullAuthenticationOut *auth_out;
	gboolean b;
	
	if (out_size != sizeof (GnomeVFSModuleCallbackFullAuthenticationOut)) {
		return FALSE;
	}
	auth_out = out;

	dbus_message_iter_get_basic (iter, &b);
	auth_out->abort_auth = b;
	dbus_message_iter_next (iter);
	auth_out->username = utils_get_string_or_null (iter, TRUE);
	dbus_message_iter_next (iter);
	auth_out->domain = utils_get_string_or_null (iter, TRUE);
	dbus_message_iter_next (iter);
	auth_out->password = utils_get_string_or_null (iter, TRUE);

	dbus_message_iter_get_basic (iter, &b);
	auth_out->save_password = b;
	auth_out->keyring = utils_get_string_or_null (iter, TRUE);
	
	return TRUE;
}

static void
full_auth_free_in (gpointer in)
{
	GnomeVFSModuleCallbackFullAuthenticationIn *auth_in;

	auth_in = in;

	g_free (auth_in->uri);
	g_free (auth_in->protocol);
	g_free (auth_in->server);
	g_free (auth_in->object);
	g_free (auth_in->authtype);
	g_free (auth_in->username);
	g_free (auth_in->domain);
	g_free (auth_in->default_user);
	g_free (auth_in->default_domain);

	g_free (auth_in);
}

static void
full_auth_free_out (gpointer out)
{
	GnomeVFSModuleCallbackFullAuthenticationOut *auth_out;

	auth_out = out;
	g_free (auth_out->username);
	g_free (auth_out->domain);
	g_free (auth_out->password);
	g_free (auth_out->keyring);
	g_free (auth_out);
}

static gboolean
save_auth_marshal_in (gconstpointer in, gsize in_size, DBusMessageIter *iter)
{
	const GnomeVFSModuleCallbackSaveAuthenticationIn *auth_in;
	gint32 i;
	
	if (in_size != sizeof (GnomeVFSModuleCallbackSaveAuthenticationIn)) {
		return FALSE;
	}
	auth_in = in;

	
	utils_append_string_or_null (iter, auth_in->keyring);
	utils_append_string_or_null (iter, auth_in->uri);
	utils_append_string_or_null (iter, auth_in->protocol);
	utils_append_string_or_null (iter, auth_in->server);
	utils_append_string_or_null (iter, auth_in->object);
	i = auth_in->port;
	dbus_message_iter_append_basic (iter, DBUS_TYPE_INT32, &i);
	utils_append_string_or_null (iter, auth_in->username);
	utils_append_string_or_null (iter, auth_in->authtype);
	utils_append_string_or_null (iter, auth_in->domain);
	utils_append_string_or_null (iter, auth_in->password);

	return TRUE;
}

static gboolean
save_auth_demarshal_in (DBusMessageIter *iter,
			gpointer *in, gsize *in_size,
			gpointer *out, gsize *out_size)
{
	GnomeVFSModuleCallbackSaveAuthenticationIn *auth_in;
	gint32 i;
	
	auth_in = *in = g_new0 (GnomeVFSModuleCallbackSaveAuthenticationIn, 1);
	*in_size = sizeof (GnomeVFSModuleCallbackSaveAuthenticationIn);
	*out = g_new0 (GnomeVFSModuleCallbackSaveAuthenticationOut, 1);
	*out_size = sizeof (GnomeVFSModuleCallbackSaveAuthenticationOut);

	auth_in->keyring = utils_get_string_or_null (iter, TRUE);
	dbus_message_iter_next (iter);
	auth_in->uri = utils_get_string_or_null (iter, TRUE);
	dbus_message_iter_next (iter);
	auth_in->protocol = utils_get_string_or_null (iter, TRUE);
	dbus_message_iter_next (iter);
	auth_in->server = utils_get_string_or_null (iter, TRUE);
	dbus_message_iter_next (iter);
	auth_in->object = utils_get_string_or_null (iter, TRUE);
	dbus_message_iter_next (iter);
	dbus_message_iter_get_basic (iter, &i);
	auth_in->port = i;
	dbus_message_iter_next (iter);
	auth_in->username = utils_get_string_or_null (iter, TRUE);
	dbus_message_iter_next (iter);
	auth_in->authtype = utils_get_string_or_null (iter, TRUE);
	dbus_message_iter_next (iter);
	auth_in->domain = utils_get_string_or_null (iter, TRUE);
	dbus_message_iter_next (iter);
	auth_in->password = utils_get_string_or_null (iter, FALSE);
	dbus_message_iter_next (iter);
	
	return TRUE;
}

static gboolean
save_auth_marshal_out (gconstpointer out, gsize out_size, DBusMessageIter *iter)
{
	gboolean b;

	if (out_size != sizeof (GnomeVFSModuleCallbackSaveAuthenticationOut)) {
		return FALSE;
	}

	/* Must marshal something */
	b = TRUE;
	dbus_message_iter_append_basic (iter, DBUS_TYPE_BOOLEAN, &b);

	return TRUE;
}

static gboolean
save_auth_demarshal_out (DBusMessageIter *iter, gpointer out, gsize out_size)
{
	gboolean b;
	
	if (out_size != sizeof (GnomeVFSModuleCallbackSaveAuthenticationOut)) {
		return FALSE;
	}

	dbus_message_iter_get_basic (iter, &b);
	
	return TRUE;
}

static void
save_auth_free_in (gpointer in)
{
	GnomeVFSModuleCallbackSaveAuthenticationIn *auth_in;

	auth_in = in;

	g_free (auth_in->keyring);
	g_free (auth_in->uri);
	g_free (auth_in->protocol);
	g_free (auth_in->server);
	g_free (auth_in->object);
	g_free (auth_in->authtype);
	g_free (auth_in->username);
	g_free (auth_in->domain);
	g_free (auth_in->password);

	g_free (auth_in);
}

static void
save_auth_free_out (gpointer out)
{
	GnomeVFSModuleCallbackSaveAuthenticationOut *auth_out;

	auth_out = out;
	g_free (auth_out);
}

static gboolean
question_marshal_in (gconstpointer in, gsize in_size, DBusMessageIter *iter)
{
	const GnomeVFSModuleCallbackQuestionIn *question_in;
	DBusMessageIter array_iter;
	int cnt;

	if (in_size != sizeof (GnomeVFSModuleCallbackQuestionIn)) {
		return FALSE;
	}
	question_in = in;

	utils_append_string_or_null (iter, question_in->primary_message);
	utils_append_string_or_null (iter, question_in->secondary_message);
	
	if (!dbus_message_iter_open_container (iter,
					       DBUS_TYPE_ARRAY,
					       DBUS_TYPE_STRING_AS_STRING,
					       &array_iter)) {
		return FALSE;
	}

	if (question_in->choices) {
		for (cnt=0; question_in->choices[cnt] != NULL; cnt++) {
			utils_append_string_or_null (&array_iter,
						     question_in->choices[cnt]);
		}
	}

	
	if (!dbus_message_iter_close_container (iter, &array_iter)) {
		return FALSE;
	}
	
	return TRUE;
}

static gboolean
question_demarshal_in (DBusMessageIter *iter,
		       gpointer *in, gsize *in_size,
		       gpointer *out, gsize *out_size)
{
	GnomeVFSModuleCallbackQuestionIn *question_in;
	DBusMessageIter array_iter;
	int cnt;

	question_in = *in = g_new0 (GnomeVFSModuleCallbackQuestionIn, 1);
	*in_size = sizeof (GnomeVFSModuleCallbackQuestionIn);
	*out = g_new0 (GnomeVFSModuleCallbackQuestionOut, 1);
	*out_size = sizeof (GnomeVFSModuleCallbackQuestionOut);

	question_in->primary_message = utils_get_string_or_null (iter, FALSE);
        dbus_message_iter_next (iter);
	question_in->secondary_message = utils_get_string_or_null (iter, FALSE);
        dbus_message_iter_next (iter);
	
        if (dbus_message_iter_get_arg_type (iter) == DBUS_TYPE_ARRAY) {
                dbus_message_iter_recurse (iter, &array_iter);

		cnt = 0;
                while (dbus_message_iter_get_arg_type (&array_iter) == DBUS_TYPE_STRING) {
			cnt++;
                        if (!dbus_message_iter_has_next (&array_iter)) {
                                break;
                        }
                        dbus_message_iter_next (&array_iter);
		}

		
		question_in->choices = g_new (char *, cnt + 1);

                dbus_message_iter_recurse (iter, &array_iter);
		
		cnt = 0;
                while (dbus_message_iter_get_arg_type (&array_iter) == DBUS_TYPE_STRING) {
			question_in->choices[cnt++] = utils_get_string_or_null (&array_iter, FALSE);
                        if (!dbus_message_iter_has_next (&array_iter)) {
                                break;
                        }
                        dbus_message_iter_next (&array_iter);
                }
		question_in->choices[cnt++] = NULL;
        }
        dbus_message_iter_next (iter);
		
	return TRUE;
}

static gboolean
question_marshal_out (gconstpointer out, gsize out_size, DBusMessageIter *iter)
{
	const GnomeVFSModuleCallbackQuestionOut *question_out;
	gint32 i;
	
	if (out_size != sizeof (GnomeVFSModuleCallbackQuestionOut)) {
		return FALSE;
	}
	question_out = out;

	i = question_out->answer;
	dbus_message_iter_append_basic (iter, DBUS_TYPE_INT32, &i);

	return TRUE;
}

static gboolean
question_demarshal_out (DBusMessageIter *iter, gpointer out, gsize out_size)
{
	GnomeVFSModuleCallbackQuestionOut *question_out;
	gint32 i;

	if (out_size != sizeof (GnomeVFSModuleCallbackQuestionOut)) {
		return FALSE;
	}
	question_out = out;

	dbus_message_iter_get_basic (iter, &i);
	question_out->answer = i;
	dbus_message_iter_next (iter);

	return TRUE;
}

static void
question_free_in (gpointer in)
{
	GnomeVFSModuleCallbackQuestionIn *question_in;

	question_in = in;

	g_free (question_in->primary_message);
	g_free (question_in->secondary_message);
	g_strfreev (question_in->choices);
	g_free (question_in);
}

static void
question_free_out (gpointer out)
{
	GnomeVFSModuleCallbackQuestionOut *question_out;

	question_out = out;
	g_free (question_out);
}

struct ModuleCallbackMarshaller {
	char *name;
	gboolean (*marshal_in)(gconstpointer in, gsize in_size, DBusMessageIter *message);
	gboolean (*demarshal_in)(DBusMessageIter *iter, gpointer *in, gsize *in_size, gpointer *out, gsize *out_size);
	gboolean (*marshal_out)(gconstpointer out, gsize out_size, DBusMessageIter *iter);
	gboolean (*demarshal_out)(DBusMessageIter *iter, gpointer out, gsize out_size);
	void (*free_in)(gpointer in);
	void (*free_out)(gpointer out);
};

static struct ModuleCallbackMarshaller module_callback_marshallers[] = {
	{ GNOME_VFS_MODULE_CALLBACK_AUTHENTICATION,
	  auth_marshal_in,
	  auth_demarshal_in,
	  auth_marshal_out,
	  auth_demarshal_out,
	  auth_free_in,
	  auth_free_out
	},
	{ GNOME_VFS_MODULE_CALLBACK_HTTP_PROXY_AUTHENTICATION,
	  auth_marshal_in,
	  auth_demarshal_in,
	  auth_marshal_out,
	  auth_demarshal_out,
	  auth_free_in,
	  auth_free_out
	},
	{ GNOME_VFS_MODULE_CALLBACK_FULL_AUTHENTICATION,
	  full_auth_marshal_in,
	  full_auth_demarshal_in,
	  full_auth_marshal_out,
	  full_auth_demarshal_out,
	  full_auth_free_in,
	  full_auth_free_out
	},
	{ GNOME_VFS_MODULE_CALLBACK_FILL_AUTHENTICATION,
	  fill_auth_marshal_in,
	  fill_auth_demarshal_in,
	  fill_auth_marshal_out,
	  fill_auth_demarshal_out,
	  fill_auth_free_in,
	  fill_auth_free_out
	},
	{ GNOME_VFS_MODULE_CALLBACK_SAVE_AUTHENTICATION,
	  save_auth_marshal_in,
	  save_auth_demarshal_in,
	  save_auth_marshal_out,
	  save_auth_demarshal_out,
	  save_auth_free_in,
	  save_auth_free_out
	},
	{ GNOME_VFS_MODULE_CALLBACK_QUESTION,
	  question_marshal_in,
	  question_demarshal_in,
	  question_marshal_out,
	  question_demarshal_out,
	  question_free_in,
	  question_free_out
	},
};


static struct ModuleCallbackMarshaller *
lookup_marshaller (const char *callback_name)
{
	int i;

	for (i = 0; i < G_N_ELEMENTS (module_callback_marshallers); i++) {
		if (strcmp (module_callback_marshallers[i].name, callback_name) == 0) {
			return &module_callback_marshallers[i];
		}
	}
	return NULL;
}



gboolean
_gnome_vfs_module_callback_marshal_invoke (const char    *callback_name,
					   gconstpointer  in,
					   gsize          in_size,
					   gpointer       out,
					   gsize          out_size)
{
	DBusMessage *message;
	DBusMessage *reply;
	DBusConnection *conn;
	struct ModuleCallbackMarshaller *marshaller;
	DBusError error;
	DBusMessageIter iter;

	conn = _gnome_vfs_daemon_get_current_connection ();
	if (conn == NULL) {
		return FALSE;
	}
	
	marshaller = lookup_marshaller (callback_name);
	if (marshaller == NULL) {
		return FALSE;
	}

	message = dbus_message_new_method_call (NULL,
						DVD_CLIENT_OBJECT,
						DVD_CLIENT_INTERFACE,
						DVD_CLIENT_METHOD_CALLBACK);
	dbus_message_append_args (message,
				  DBUS_TYPE_STRING, &callback_name,
				  DBUS_TYPE_INVALID);
        dbus_message_iter_init_append (message, &iter);
	
	if (!(marshaller->marshal_in)(in, in_size, &iter)) {
		dbus_message_unref (message);
		return FALSE;
	}
	
	dbus_error_init (&error);
	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);
	dbus_message_unref (message);


	if (reply == NULL) {
		return FALSE;
	}

	if (!dbus_message_iter_init (reply, &iter)) {
		dbus_message_unref (reply);
		return FALSE;
	}

	if (dbus_message_iter_get_arg_type (&iter) == DBUS_TYPE_INVALID) {
		dbus_message_unref (reply);
		return FALSE;
	}
	
	if (!(marshaller->demarshal_out)(&iter, out, out_size)) {
		dbus_message_unref (reply);
		return FALSE;
	}
	
	dbus_message_unref (reply);
	return TRUE;
}



gboolean
_gnome_vfs_module_callback_demarshal_invoke (const char  *callback_name,
					     DBusMessageIter *iter_in,
					     DBusMessage *reply)
{
	gboolean res;
	gpointer in, out;
	gsize in_size, out_size;
	struct ModuleCallbackMarshaller *marshaller;
	DBusMessageIter iter_out;

	marshaller = lookup_marshaller (callback_name);
	if (marshaller == NULL) {
		return FALSE;
	}

	if (!(marshaller->demarshal_in) (iter_in,
					 &in, &in_size,
					 &out, &out_size)) {
		return FALSE;
	}

	res = gnome_vfs_module_callback_invoke (callback_name,
						in, in_size,
						out, out_size);
	if (!res) {
		(marshaller->free_in) (in);
		g_free (out); /* only shallow free necessary */
		return FALSE;
	}
	(marshaller->free_in) (in);

        dbus_message_iter_init_append (reply, &iter_out);
	
	res = (marshaller->marshal_out)(out, out_size, &iter_out);
	(marshaller->free_out) (out);

	return res;
}
