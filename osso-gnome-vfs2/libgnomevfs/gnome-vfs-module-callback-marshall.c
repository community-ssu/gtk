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
#include <libbonobo.h>
#include "gnome-vfs-module-callback.h"

#include "gnome-vfs-module-callback-module-api.h"
#include "gnome-vfs-module-callback-private.h"
#include "gnome-vfs-backend.h"
#include "gnome-vfs-private.h"
#include "gnome-vfs-client-call.h"
#include "gnome-vfs-standard-callbacks.h"
#include <GNOME_VFS_Daemon.h>


static CORBA_char *
corba_string_or_null_dup (char *str)
{
	if (str != NULL) {
		return CORBA_string_dup (str);
	} else {
		return CORBA_string_dup ("");
	}
}

/* empty string interpreted as NULL */
static char *
decode_corba_string_or_null (CORBA_char *str, gboolean empty_is_null)
{
	if (empty_is_null && *str == 0) {
		return NULL;
	} else {
		return g_strdup (str);
	}
}



static CORBA_any *
auth_marshal_in (gconstpointer in, gsize in_size)
{
	CORBA_any *retval;
	GNOME_VFS_ModuleCallbackAuthenticationIn *ret_in;
	const GnomeVFSModuleCallbackAuthenticationIn *auth_in;
	
	if (in_size != sizeof (GnomeVFSModuleCallbackAuthenticationIn)) {
		return NULL;
	}
	auth_in = in;

	retval = CORBA_any_alloc ();
	retval->_type = TC_GNOME_VFS_ModuleCallbackAuthenticationIn;
	retval->_value = GNOME_VFS_ModuleCallbackAuthenticationIn__alloc ();
	ret_in = retval->_value;

	ret_in->uri = corba_string_or_null_dup (auth_in->uri);
	ret_in->realm = corba_string_or_null_dup (auth_in->realm);
	ret_in->previous_attempt_failed = auth_in->previous_attempt_failed;
	ret_in->auth_type = auth_in->auth_type;

	return retval;
}

static gboolean
auth_demarshal_in (const CORBA_any *any_in,
		   gpointer *in, gsize *in_size,
		   gpointer *out, gsize *out_size)
{
	GNOME_VFS_ModuleCallbackAuthenticationIn *corba_in;
	GnomeVFSModuleCallbackAuthenticationIn *auth_in;
	GnomeVFSModuleCallbackAuthenticationOut *auth_out;
	
	if (!CORBA_TypeCode_equal (any_in->_type, TC_GNOME_VFS_ModuleCallbackAuthenticationIn, NULL)) {
		return FALSE;
	}
	
	auth_in = *in = g_new0 (GnomeVFSModuleCallbackAuthenticationIn, 1);
	*in_size = sizeof (GnomeVFSModuleCallbackAuthenticationIn);
	auth_out = *out = g_new0 (GnomeVFSModuleCallbackAuthenticationOut, 1);
	*out_size = sizeof (GnomeVFSModuleCallbackAuthenticationOut);

	corba_in = (GNOME_VFS_ModuleCallbackAuthenticationIn *)any_in->_value;

	auth_in->uri = decode_corba_string_or_null (corba_in->uri, TRUE);
	auth_in->realm = decode_corba_string_or_null (corba_in->realm, TRUE);
	auth_in->previous_attempt_failed = corba_in->previous_attempt_failed;
	auth_in->auth_type = corba_in->auth_type;
	
	return TRUE;
}

static CORBA_any *
auth_marshal_out (gconstpointer out, gsize out_size)
{
	CORBA_any *retval;
	GNOME_VFS_ModuleCallbackAuthenticationOut *ret_out;
	const GnomeVFSModuleCallbackAuthenticationOut *auth_out;

	if (out_size != sizeof (GnomeVFSModuleCallbackAuthenticationOut)) {
		return NULL;
	}
	auth_out = out;

	retval = CORBA_any_alloc ();
	retval->_type = TC_GNOME_VFS_ModuleCallbackAuthenticationOut;
	retval->_value = GNOME_VFS_ModuleCallbackAuthenticationOut__alloc ();
	ret_out = retval->_value;

	ret_out->username = corba_string_or_null_dup (auth_out->username);
	ret_out->no_username = auth_out->username == NULL;
	ret_out->password = corba_string_or_null_dup (auth_out->password);

	return retval;
}

static gboolean
auth_demarshal_out (CORBA_any *any_out, gpointer out, gsize out_size)
{
	GNOME_VFS_ModuleCallbackAuthenticationOut *corba_out;
	GnomeVFSModuleCallbackAuthenticationOut *auth_out;

	if (!CORBA_TypeCode_equal (any_out->_type, TC_GNOME_VFS_ModuleCallbackAuthenticationOut, NULL) ||
	    out_size != sizeof (GnomeVFSModuleCallbackAuthenticationOut)) {
		return FALSE;
	}
	auth_out = out;

	corba_out = (GNOME_VFS_ModuleCallbackAuthenticationOut *)any_out->_value;

	auth_out->username = decode_corba_string_or_null (corba_out->username,
							  corba_out->no_username);
	auth_out->password = decode_corba_string_or_null (corba_out->password, TRUE);
	
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

static CORBA_any *
full_auth_marshal_in (gconstpointer in, gsize in_size)
{
	CORBA_any *retval;
	GNOME_VFS_ModuleCallbackFullAuthenticationIn *ret_in;
	const GnomeVFSModuleCallbackFullAuthenticationIn *auth_in;
	
	if (in_size != sizeof (GnomeVFSModuleCallbackFullAuthenticationIn)) {
		return NULL;
	}
	auth_in = in;

	retval = CORBA_any_alloc ();
	retval->_type = TC_GNOME_VFS_ModuleCallbackFullAuthenticationIn;
	retval->_value = GNOME_VFS_ModuleCallbackFullAuthenticationIn__alloc ();
	ret_in = retval->_value;

	ret_in->flags = auth_in->flags;
	ret_in->uri = corba_string_or_null_dup (auth_in->uri);
	ret_in->protocol = corba_string_or_null_dup (auth_in->protocol);
	ret_in->server = corba_string_or_null_dup (auth_in->server);
	ret_in->object = corba_string_or_null_dup (auth_in->object);
	ret_in->port = auth_in->port;
	ret_in->username = corba_string_or_null_dup (auth_in->username);
	ret_in->authtype = corba_string_or_null_dup (auth_in->authtype);
	ret_in->domain = corba_string_or_null_dup (auth_in->domain);
	ret_in->default_user = corba_string_or_null_dup (auth_in->default_user);
	ret_in->default_domain = corba_string_or_null_dup (auth_in->default_domain);

	return retval;
}

static gboolean
full_auth_demarshal_in (const CORBA_any *any_in,
			gpointer *in, gsize *in_size,
			gpointer *out, gsize *out_size)
{
	GNOME_VFS_ModuleCallbackFullAuthenticationIn *corba_in;
	GnomeVFSModuleCallbackFullAuthenticationIn *auth_in;
	GnomeVFSModuleCallbackFullAuthenticationOut *auth_out;
	
	if (!CORBA_TypeCode_equal (any_in->_type, TC_GNOME_VFS_ModuleCallbackFullAuthenticationIn, NULL)) {
		return FALSE;
	}
	
	auth_in = *in = g_new0 (GnomeVFSModuleCallbackFullAuthenticationIn, 1);
	*in_size = sizeof (GnomeVFSModuleCallbackFullAuthenticationIn);
	auth_out = *out = g_new0 (GnomeVFSModuleCallbackFullAuthenticationOut, 1);
	*out_size = sizeof (GnomeVFSModuleCallbackFullAuthenticationOut);

	corba_in = (GNOME_VFS_ModuleCallbackFullAuthenticationIn *)any_in->_value;

	auth_in->flags = corba_in->flags;
	auth_in->uri = decode_corba_string_or_null (corba_in->uri, TRUE);
	auth_in->protocol = decode_corba_string_or_null (corba_in->protocol, TRUE);
	auth_in->server = decode_corba_string_or_null (corba_in->server, TRUE);
	auth_in->object = decode_corba_string_or_null (corba_in->object, TRUE);
	auth_in->port = corba_in->port;
	auth_in->username = decode_corba_string_or_null (corba_in->username, TRUE);
	auth_in->authtype = decode_corba_string_or_null (corba_in->authtype, TRUE);
	auth_in->domain = decode_corba_string_or_null (corba_in->domain, TRUE);
	auth_in->default_user = decode_corba_string_or_null (corba_in->default_user, TRUE);
	auth_in->default_domain = decode_corba_string_or_null (corba_in->default_domain, TRUE);
	
	return TRUE;
}

static CORBA_any *
full_auth_marshal_out (gconstpointer out, gsize out_size)
{
	CORBA_any *retval;
	GNOME_VFS_ModuleCallbackFullAuthenticationOut *ret_out;
	const GnomeVFSModuleCallbackFullAuthenticationOut *auth_out;

	if (out_size != sizeof (GnomeVFSModuleCallbackFullAuthenticationOut)) {
		return NULL;
	}
	auth_out = out;

	retval = CORBA_any_alloc ();
	retval->_type = TC_GNOME_VFS_ModuleCallbackFullAuthenticationOut;
	retval->_value = GNOME_VFS_ModuleCallbackFullAuthenticationOut__alloc ();
	ret_out = retval->_value;

	ret_out->abort_auth = auth_out->abort_auth;
	ret_out->username = corba_string_or_null_dup (auth_out->username);
	ret_out->domain = corba_string_or_null_dup (auth_out->domain);
	ret_out->password = corba_string_or_null_dup (auth_out->password);

	ret_out->save_password = auth_out->save_password;
	ret_out->keyring = corba_string_or_null_dup (auth_out->keyring);
	
	return retval;
}

static gboolean
full_auth_demarshal_out (CORBA_any *any_out, gpointer out, gsize out_size)
{
	GNOME_VFS_ModuleCallbackFullAuthenticationOut *corba_out;
	GnomeVFSModuleCallbackFullAuthenticationOut *auth_out;

	if (!CORBA_TypeCode_equal (any_out->_type, TC_GNOME_VFS_ModuleCallbackFullAuthenticationOut, NULL) ||
	    out_size != sizeof (GnomeVFSModuleCallbackFullAuthenticationOut)) {
		return FALSE;
	}
	auth_out = out;

	corba_out = (GNOME_VFS_ModuleCallbackFullAuthenticationOut *)any_out->_value;

	auth_out->abort_auth = corba_out->abort_auth;
	auth_out->username = decode_corba_string_or_null (corba_out->username, TRUE);
	auth_out->domain = decode_corba_string_or_null (corba_out->domain, TRUE);
	auth_out->password = decode_corba_string_or_null (corba_out->password, TRUE);
	
	auth_out->save_password = corba_out->save_password;
	auth_out->keyring = decode_corba_string_or_null (corba_out->keyring, TRUE);
	
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

static CORBA_any *
fill_auth_marshal_in (gconstpointer in, gsize in_size)
{
	CORBA_any *retval;
	GNOME_VFS_ModuleCallbackFillAuthenticationIn *ret_in;
	const GnomeVFSModuleCallbackFillAuthenticationIn *auth_in;
	
	if (in_size != sizeof (GnomeVFSModuleCallbackFillAuthenticationIn)) {
		return NULL;
	}
	auth_in = in;

	retval = CORBA_any_alloc ();
	retval->_type = TC_GNOME_VFS_ModuleCallbackFillAuthenticationIn;
	retval->_value = GNOME_VFS_ModuleCallbackFillAuthenticationIn__alloc ();
	ret_in = retval->_value;

	ret_in->uri = corba_string_or_null_dup (auth_in->uri);
	ret_in->protocol = corba_string_or_null_dup (auth_in->protocol);
	ret_in->server = corba_string_or_null_dup (auth_in->server);
	ret_in->object = corba_string_or_null_dup (auth_in->object);
	ret_in->port = auth_in->port;
	ret_in->username = corba_string_or_null_dup (auth_in->username);
	ret_in->authtype = corba_string_or_null_dup (auth_in->authtype);
	ret_in->domain = corba_string_or_null_dup (auth_in->domain);

	return retval;
}

static gboolean
fill_auth_demarshal_in (const CORBA_any *any_in,
			gpointer *in, gsize *in_size,
			gpointer *out, gsize *out_size)
{
	GNOME_VFS_ModuleCallbackFillAuthenticationIn *corba_in;
	GnomeVFSModuleCallbackFillAuthenticationIn *auth_in;
	GnomeVFSModuleCallbackFillAuthenticationOut *auth_out;
	
	if (!CORBA_TypeCode_equal (any_in->_type, TC_GNOME_VFS_ModuleCallbackFillAuthenticationIn, NULL)) {
		return FALSE;
	}
	
	auth_in = *in = g_new0 (GnomeVFSModuleCallbackFillAuthenticationIn, 1);
	*in_size = sizeof (GnomeVFSModuleCallbackFillAuthenticationIn);
	auth_out = *out = g_new0 (GnomeVFSModuleCallbackFillAuthenticationOut, 1);
	*out_size = sizeof (GnomeVFSModuleCallbackFillAuthenticationOut);

	corba_in = (GNOME_VFS_ModuleCallbackFillAuthenticationIn *)any_in->_value;

	auth_in->uri = decode_corba_string_or_null (corba_in->uri, TRUE);
	auth_in->protocol = decode_corba_string_or_null (corba_in->protocol, TRUE);
	auth_in->server = decode_corba_string_or_null (corba_in->server, TRUE);
	auth_in->object = decode_corba_string_or_null (corba_in->object, TRUE);
	auth_in->port = corba_in->port;
	auth_in->username = decode_corba_string_or_null (corba_in->username, TRUE);
	auth_in->authtype = decode_corba_string_or_null (corba_in->authtype, TRUE);
	auth_in->domain = decode_corba_string_or_null (corba_in->domain, TRUE);
	
	return TRUE;
}

static CORBA_any *
fill_auth_marshal_out (gconstpointer out, gsize out_size)
{
	CORBA_any *retval;
	GNOME_VFS_ModuleCallbackFillAuthenticationOut *ret_out;
	const GnomeVFSModuleCallbackFillAuthenticationOut *auth_out;

	if (out_size != sizeof (GnomeVFSModuleCallbackFillAuthenticationOut)) {
		return NULL;
	}
	auth_out = out;

	retval = CORBA_any_alloc ();
	retval->_type = TC_GNOME_VFS_ModuleCallbackFillAuthenticationOut;
	retval->_value = GNOME_VFS_ModuleCallbackFillAuthenticationOut__alloc ();
	ret_out = retval->_value;

	ret_out->valid = auth_out->valid;
	ret_out->username = corba_string_or_null_dup (auth_out->username);
	ret_out->domain = corba_string_or_null_dup (auth_out->domain);
	ret_out->password = corba_string_or_null_dup (auth_out->password);
	
	return retval;
}

static gboolean
fill_auth_demarshal_out (CORBA_any *any_out, gpointer out, gsize out_size)
{
	GNOME_VFS_ModuleCallbackFillAuthenticationOut *corba_out;
	GnomeVFSModuleCallbackFillAuthenticationOut *auth_out;

	if (!CORBA_TypeCode_equal (any_out->_type, TC_GNOME_VFS_ModuleCallbackFillAuthenticationOut, NULL) ||
	    out_size != sizeof (GnomeVFSModuleCallbackFillAuthenticationOut)) {
		return FALSE;
	}
	auth_out = out;

	corba_out = (GNOME_VFS_ModuleCallbackFillAuthenticationOut *)any_out->_value;

	auth_out->valid = corba_out->valid;
	auth_out->username = decode_corba_string_or_null (corba_out->username, TRUE);
	auth_out->domain = decode_corba_string_or_null (corba_out->domain, TRUE);
	auth_out->password = decode_corba_string_or_null (corba_out->password, TRUE);
	
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

static CORBA_any *
save_auth_marshal_in (gconstpointer in, gsize in_size)
{
	CORBA_any *retval;
	GNOME_VFS_ModuleCallbackSaveAuthenticationIn *ret_in;
	const GnomeVFSModuleCallbackSaveAuthenticationIn *auth_in;
	
	if (in_size != sizeof (GnomeVFSModuleCallbackSaveAuthenticationIn)) {
		return NULL;
	}
	auth_in = in;

	retval = CORBA_any_alloc ();
	retval->_type = TC_GNOME_VFS_ModuleCallbackSaveAuthenticationIn;
	retval->_value = GNOME_VFS_ModuleCallbackSaveAuthenticationIn__alloc ();
	ret_in = retval->_value;

	ret_in->keyring = corba_string_or_null_dup (auth_in->keyring);
	ret_in->uri = corba_string_or_null_dup (auth_in->uri);
	ret_in->protocol = corba_string_or_null_dup (auth_in->protocol);
	ret_in->server = corba_string_or_null_dup (auth_in->server);
	ret_in->object = corba_string_or_null_dup (auth_in->object);
	ret_in->port = auth_in->port;
	ret_in->username = corba_string_or_null_dup (auth_in->username);
	ret_in->authtype = corba_string_or_null_dup (auth_in->authtype);
	ret_in->domain = corba_string_or_null_dup (auth_in->domain);
	ret_in->password = corba_string_or_null_dup (auth_in->password);

	return retval;
}

static gboolean
save_auth_demarshal_in (const CORBA_any *any_in,
			gpointer *in, gsize *in_size,
			gpointer *out, gsize *out_size)
{
	GNOME_VFS_ModuleCallbackSaveAuthenticationIn *corba_in;
	GnomeVFSModuleCallbackSaveAuthenticationIn *auth_in;
	GnomeVFSModuleCallbackSaveAuthenticationOut *auth_out;
	
	if (!CORBA_TypeCode_equal (any_in->_type, TC_GNOME_VFS_ModuleCallbackSaveAuthenticationIn, NULL)) {
		return FALSE;
	}
	
	auth_in = *in = g_new0 (GnomeVFSModuleCallbackSaveAuthenticationIn, 1);
	*in_size = sizeof (GnomeVFSModuleCallbackSaveAuthenticationIn);
	auth_out = *out = g_new0 (GnomeVFSModuleCallbackSaveAuthenticationOut, 1);
	*out_size = sizeof (GnomeVFSModuleCallbackSaveAuthenticationOut);

	corba_in = (GNOME_VFS_ModuleCallbackSaveAuthenticationIn *)any_in->_value;

	auth_in->keyring = decode_corba_string_or_null (corba_in->keyring, TRUE);
	auth_in->uri = decode_corba_string_or_null (corba_in->uri, TRUE);
	auth_in->protocol = decode_corba_string_or_null (corba_in->protocol, TRUE);
	auth_in->server = decode_corba_string_or_null (corba_in->server, TRUE);
	auth_in->object = decode_corba_string_or_null (corba_in->object, TRUE);
	auth_in->port = corba_in->port;
	auth_in->username = decode_corba_string_or_null (corba_in->username, TRUE);
	auth_in->authtype = decode_corba_string_or_null (corba_in->authtype, TRUE);
	auth_in->domain = decode_corba_string_or_null (corba_in->domain, TRUE);
	auth_in->password = decode_corba_string_or_null (corba_in->password, FALSE);
	
	return TRUE;
}

static CORBA_any *
save_auth_marshal_out (gconstpointer out, gsize out_size)
{
	CORBA_any *retval;
	GNOME_VFS_ModuleCallbackSaveAuthenticationOut *ret_out;
	const GnomeVFSModuleCallbackSaveAuthenticationOut *auth_out;

	if (out_size != sizeof (GnomeVFSModuleCallbackSaveAuthenticationOut)) {
		return NULL;
	}
	auth_out = out;

	retval = CORBA_any_alloc ();
	retval->_type = TC_GNOME_VFS_ModuleCallbackSaveAuthenticationOut;
	retval->_value = GNOME_VFS_ModuleCallbackSaveAuthenticationOut__alloc ();
	ret_out = retval->_value;

	return retval;
}

static gboolean
save_auth_demarshal_out (CORBA_any *any_out, gpointer out, gsize out_size)
{
	GNOME_VFS_ModuleCallbackSaveAuthenticationOut *corba_out;
	GnomeVFSModuleCallbackSaveAuthenticationOut *auth_out;

	if (!CORBA_TypeCode_equal (any_out->_type, TC_GNOME_VFS_ModuleCallbackSaveAuthenticationOut, NULL) ||
	    out_size != sizeof (GnomeVFSModuleCallbackSaveAuthenticationOut)) {
		return FALSE;
	}
	auth_out = out;

	corba_out = (GNOME_VFS_ModuleCallbackSaveAuthenticationOut *)any_out->_value;

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

static CORBA_any *
question_marshal_in (gconstpointer in, gsize in_size)
{
	CORBA_any *retval;
	GNOME_VFS_ModuleCallbackQuestionIn *ret_in;
	const GnomeVFSModuleCallbackQuestionIn *question_in;
	int cnt;

	if (in_size != sizeof (GnomeVFSModuleCallbackQuestionIn)) {
		return NULL;
	}
	question_in = in;

	retval = CORBA_any_alloc ();
	retval->_type = TC_GNOME_VFS_ModuleCallbackQuestionIn;
	retval->_value = GNOME_VFS_ModuleCallbackQuestionIn__alloc ();
	ret_in = retval->_value;

	ret_in->primary_message = corba_string_or_null_dup (question_in->primary_message);
	ret_in->secondary_message = corba_string_or_null_dup (question_in->secondary_message);
	if (question_in->choices) {
		
		/* Just count the number of elements and allocate the memory */
		for (cnt=0; question_in->choices[cnt] != NULL; cnt++);
		ret_in->choices._maximum = cnt; 
		ret_in->choices._length = cnt;
		ret_in->choices._buffer = CORBA_sequence_CORBA_string_allocbuf (cnt);
		CORBA_sequence_set_release (&ret_in->choices, TRUE);
		
		/* Insert the strings into the sequence */
		for (cnt=0; question_in->choices[cnt] != NULL; cnt++) {
			ret_in->choices._buffer[cnt] = corba_string_or_null_dup (question_in->choices[cnt]);
		}
	}
	return retval;
}

static gboolean
question_demarshal_in (const CORBA_any *any_in,
			gpointer *in, gsize *in_size,
			gpointer *out, gsize *out_size)
{
	GNOME_VFS_ModuleCallbackQuestionIn *corba_in;
	GnomeVFSModuleCallbackQuestionIn *question_in;
	GnomeVFSModuleCallbackQuestionOut *question_out;
	int cnt;

	if (!CORBA_TypeCode_equal (any_in->_type, TC_GNOME_VFS_ModuleCallbackQuestionIn, NULL)) {
		return FALSE;
	}
	
	question_in = *in = g_new0 (GnomeVFSModuleCallbackQuestionIn, 1);
	*in_size = sizeof (GnomeVFSModuleCallbackQuestionIn);
	question_out = *out = g_new0 (GnomeVFSModuleCallbackQuestionOut, 1);
	*out_size = sizeof (GnomeVFSModuleCallbackQuestionOut);

	corba_in = (GNOME_VFS_ModuleCallbackQuestionIn *)any_in->_value;

	if (corba_in) {
		question_in->primary_message = decode_corba_string_or_null (corba_in->primary_message, FALSE);	
		question_in->secondary_message = decode_corba_string_or_null (corba_in->secondary_message, FALSE);	
		question_in->choices = g_new (char *, corba_in->choices._length+1);
		for (cnt=0; cnt<corba_in->choices._length; cnt++){
			question_in->choices[cnt] = g_strdup (corba_in->choices._buffer[cnt]);
		}
		question_in->choices[corba_in->choices._length] = NULL;
		
		return TRUE;
	}
	return FALSE;
}

static CORBA_any *
question_marshal_out (gconstpointer out, gsize out_size)
{
	CORBA_any *retval;
	GNOME_VFS_ModuleCallbackQuestionOut *ret_out;
	const GnomeVFSModuleCallbackQuestionOut *question_out;

	if (out_size != sizeof (GnomeVFSModuleCallbackQuestionOut)) {
		return NULL;
	}
	question_out = out;

	retval = CORBA_any_alloc ();
	retval->_type = TC_GNOME_VFS_ModuleCallbackQuestionOut;
	retval->_value = GNOME_VFS_ModuleCallbackQuestionOut__alloc ();
	ret_out = retval->_value;

	ret_out->answer = question_out->answer;

	return retval;
}

static gboolean
question_demarshal_out (CORBA_any *any_out, gpointer out, gsize out_size)
{
	GNOME_VFS_ModuleCallbackQuestionOut *corba_out;
	GnomeVFSModuleCallbackQuestionOut *question_out;

	if (!CORBA_TypeCode_equal (any_out->_type, TC_GNOME_VFS_ModuleCallbackQuestionOut, NULL) ||
	    out_size != sizeof (GnomeVFSModuleCallbackQuestionOut)) {
		return FALSE;
	}
	question_out = out;

	corba_out = (GNOME_VFS_ModuleCallbackQuestionOut *)any_out->_value;

	question_out->answer = corba_out->answer;

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
	CORBA_any *(*marshal_in)(gconstpointer in, gsize in_size);
	gboolean (*demarshal_in)(const CORBA_any *any_in, gpointer *in, gsize *in_size, gpointer *out, gsize *out_size);
	CORBA_any *(*marshal_out)(gconstpointer out, gsize out_size);
	gboolean (*demarshal_out)(CORBA_any *any_out, gpointer out, gsize out_size);
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
	CORBA_Environment ev;
	CORBA_any *any_in;
	CORBA_any *any_out;
	gboolean res;
	struct ModuleCallbackMarshaller *marshaller;

	marshaller = lookup_marshaller (callback_name);
	if (marshaller == NULL) {
		return FALSE;
	}

	any_in = (marshaller->marshal_in)(in, in_size);
	if (any_in == NULL) {
		return FALSE;
	}

	CORBA_exception_init (&ev);
	any_out = NULL;
	res = GNOME_VFS_ClientCall_ModuleCallbackInvoke (_gnome_vfs_daemon_get_current_daemon_client_call (),
							 callback_name,
							 any_in,
							 &any_out,
							 &ev);
	CORBA_free (any_in);
	if (BONOBO_EX (&ev)) {
		CORBA_exception_free (&ev);
		return FALSE;
	}

	if (!res) {
		CORBA_free (any_out);
		return FALSE;
	}
	
	if (!(marshaller->demarshal_out)(any_out, out, out_size)) {
		CORBA_free (any_out);
		return FALSE;
	}
	CORBA_free (any_out);
	return TRUE;
}



gboolean
_gnome_vfs_module_callback_demarshal_invoke (const char    *callback_name,
					     const CORBA_any * module_in,
					     CORBA_any ** module_out)
{
	gboolean res;
	gpointer in, out;
	gsize in_size, out_size;
	struct ModuleCallbackMarshaller *marshaller;
	CORBA_any *empty_any;

	marshaller = lookup_marshaller (callback_name);
	if (marshaller == NULL) {
		return FALSE;
	}
	
	if (!(marshaller->demarshal_in)(module_in,
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
		empty_any = CORBA_any_alloc ();
		empty_any->_type = TC_null;
		empty_any->_value = NULL;
		*module_out = empty_any;

		return FALSE;
	}
	(marshaller->free_in) (in);

	*module_out = (marshaller->marshal_out)(out, out_size);
	(marshaller->free_out) (out);

	if (*module_out == NULL) {
		empty_any = CORBA_any_alloc ();
		empty_any->_type = TC_null;
		empty_any->_value = NULL;
		*module_out = empty_any;
		return FALSE;
	}
	
	return TRUE;
}
