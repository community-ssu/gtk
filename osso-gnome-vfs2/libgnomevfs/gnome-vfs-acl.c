/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* gnome-vfs-acl.h - ACL Handling for the GNOME Virtual File System.
   Access Control List Object

   Copyright (C) 2005 Christian Kellner

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

   Author: Christian Kellner <gicmo@gnome.org>
*/

#include "gnome-vfs-acl.h"

/* ************************************************************************** */
/* GObject Stuff */

static GObjectClass *parent_class = NULL;

#define GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), \
			  GNOME_VFS_TYPE_ACL, GnomeVFSACLPrivate))

G_DEFINE_TYPE (GnomeVFSACL, gnome_vfs_acl, G_TYPE_OBJECT);

struct _GnomeVFSACLPrivate {
	GList    *entries;
	gboolean  modified;
};

static void
gnome_vfs_acl_finalize (GObject *object)
{
	GnomeVFSACL        *acl;
	GnomeVFSACLPrivate *priv;
	GList              *iter;
	
	acl = GNOME_VFS_ACL (object);
	priv = acl->priv;
	
	for (iter = priv->entries; iter; iter = iter->next) {
		GnomeVFSACE *ace;
		
		ace = GNOME_VFS_ACE (iter->data);
		
		g_object_unref (ace);
	}
	
	g_list_free (priv->entries);
	
	if (G_OBJECT_CLASS (parent_class)->finalize)
		(*G_OBJECT_CLASS (parent_class)->finalize) (object);
}

static void
gnome_vfs_acl_class_init (GnomeVFSACLClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	g_type_class_add_private (klass, sizeof (GnomeVFSACLPrivate));

	gobject_class->finalize = gnome_vfs_acl_finalize;
}

static void
gnome_vfs_acl_init (GnomeVFSACL *acl)
{
	acl->priv = GET_PRIVATE (acl);
	
}

/* ************************************************************************** */
/* Public Interface */

/**
 * gnome_vfs_acl_new:
 *
 * Creates a new #GnomeVFSACL object.
 *
 * Return value: The new #GnomeVFSACL 
 *
 * Since: 2.16
 **/

GnomeVFSACL *
gnome_vfs_acl_new (void)
{
	GnomeVFSACL *acl;	

	acl = g_object_new (GNOME_VFS_TYPE_ACL, NULL);

	return acl;
}

/**
 * gnome_vfs_acl_clear:
 * @acl: A valid #GnomeVFSACL object
 *
 * This will clear the #GnomeVFSACL object resetting it to a
 * state exaclty as if it was newly created.
 * 
 * Since: 2.16
 **/
void
gnome_vfs_acl_clear (GnomeVFSACL *acl)
{
	GnomeVFSACLPrivate *priv;
	GnomeVFSACE        *entry;
	GList              *iter;	
	
	priv = acl->priv;
	
	entry = NULL;
	
	for (iter = priv->entries; iter; iter = iter->next) {
		entry = GNOME_VFS_ACE (iter->data);
		
		g_object_unref (entry);		
	}

	g_list_free (priv->entries);

	priv->entries = NULL;
}

/**
 * gnome_vfs_acl_set:
 * @acl: A valid #GnomeVFSACL object
 * @ace: A #GnomeVFSACE to set in @acl
 *
 * This will search the #GnomeVFSACL object specified at @acl for an 
 * existing #GnomeVFSACE that machtes @ace. If found it will update the
 * permission on the object otherwise it will ref @ace and insert it
 * into it's list. 
 *
 * Since: 2.16
 **/
void
gnome_vfs_acl_set (GnomeVFSACL *acl,
                   GnomeVFSACE *ace)
{
	GnomeVFSACLPrivate *priv;
	GnomeVFSACE        *entry;
	GList              *iter;	

	g_return_if_fail (GNOME_VFS_IS_ACE (ace));

	priv = acl->priv;
	entry = NULL;

	/* that could be made faster with some HashTable foo,
	 * not sure it is worth the effort, though */
	for (iter = priv->entries; iter; iter = iter->next) {
		entry = GNOME_VFS_ACE (iter->data);

		if (gnome_vfs_ace_equal (entry, ace)) {
			break;	
		}		
	}

	if (iter == NULL) {
		priv->entries = g_list_prepend (priv->entries, g_object_ref (ace));
	} else {
		gnome_vfs_ace_copy_perms (ace, entry);
	}
}

/**
 * gnome_vfs_acl_unset:
 * @acl: A valid #GnomeVFSACL object
 * @ace: A #GnomeVFSACE to remove from @acl
 *
 * This will search the #GnomeVFSACL object specified at @acl for an 
 * existing #GnomeVFSACE that machtes @ace. If found it will remove it
 * from its internal list.
 *
 * Since: 2.16
 **/
void
gnome_vfs_acl_unset (GnomeVFSACL *acl,
                     GnomeVFSACE *ace)
{
	GnomeVFSACLPrivate *priv;
	GnomeVFSACE        *entry;
	GList              *iter;	
	
	priv = acl->priv;
	
	entry = NULL;
	
	/* that could be made faster with some HashTable foo,
	 * not sure it is worth the effort, though */
	for (iter = priv->entries; iter; iter = iter->next) {
		entry = GNOME_VFS_ACE (iter->data);
		
		if (gnome_vfs_ace_equal (entry, ace)) {
			break;	
		}		
	}
	
	if (iter != NULL) {
		g_object_unref (entry);
		priv->entries = g_list_remove_link (priv->entries, iter);
	}
}

/**
 * gnome_vfs_acl_get_ace_list:
 * @acl: A valid #GnomeVFSACL object
 *
 * This will create you a list of all #GnomeVFSACE objects that are
 * contained in @acl. 
 *
 * Return value: A newly created list you have to free with 
 * gnome_vfs_acl_get_ace_list.
 * 
 * Since: 2.16
 **/
GList*
gnome_vfs_acl_get_ace_list (GnomeVFSACL *acl)
{
	GnomeVFSACLPrivate *priv;
	GList              *list;
	
	priv = acl->priv;
	
	list = g_list_copy (priv->entries);
	
	g_list_foreach (list, (GFunc) g_object_ref, NULL);
	
	return list;
}

/**
 * gnome_vfs_ace_free_ace_list:
 * @ace_list: A GList return by gnome_vfs_acl_get_ace_list
 *
 * This will unref all the #GnomeVFSACE in the list and free it
 * afterwards
 * 
 * Since: 2.16
 **/
void
gnome_vfs_acl_free_ace_list (GList *ace_list)
{
	g_list_foreach (ace_list, (GFunc) g_object_unref, NULL);
	g_list_free (ace_list);
}

/* ************************************************************************** */
/* ************************************************************************** */
const char *
gnome_vfs_acl_kind_to_string (GnomeVFSACLKind kind)
{
	const char *value;
	
	if (kind < GNOME_VFS_ACL_KIND_SYS_LAST) {
		
		switch (kind) {
		
		case GNOME_VFS_ACL_USER:
			value = "user";
			break;
			
		case GNOME_VFS_ACL_GROUP:
			value = "group";
			break;
				
		case GNOME_VFS_ACL_OTHER:
			value = "other";
			break;
			
		default:
			value = "unknown";
			break;
		}
		
	} else {
		value = "unknown";
	}
	return value;		
}

const char *
gnome_vfs_acl_perm_to_string (GnomeVFSACLPerm perm)
{
	const char *value;
	
	if (perm < GNOME_VFS_ACL_PERM_SYS_LAST) {
		
		switch (perm) {
	
		case GNOME_VFS_ACL_READ:
			value = "read";
			break;
			
		case GNOME_VFS_ACL_WRITE:
			value = "write";
			break;
		
		case GNOME_VFS_ACL_EXECUTE:
			value = "execute";
			break;
				
		default:
			value = "unknown";
			break;	
		}
	} else {
		value = "unknown";	
	}
	
	return value;
}

