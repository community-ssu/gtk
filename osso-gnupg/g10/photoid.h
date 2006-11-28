/* photoid.h
 * Copyright (C) 2001, 2002 Free Software Foundation, Inc.
 *
 * This file is part of GnuPG.
 *
 * GnuPG is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * GnuPG is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
 * USA.
 */

/* Photo ID functions */

#ifndef _PHOTOID_H_
#define _PHOTOID_H_

#include "packet.h"

PKT_user_id *generate_photo_id(PKT_public_key *pk);
int parse_image_header(const struct user_attribute *attr,byte *type,u32 *len);
char *image_type_to_string(byte type,int style);
void show_photos(const struct user_attribute *attrs,
		 int count,PKT_public_key *pk,PKT_secret_key *sk);

#endif /* !_PHOTOID_H_ */