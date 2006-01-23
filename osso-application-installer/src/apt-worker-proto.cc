/*
 * This file is part of osso-application-installer
 *
 * Copyright (C) 2005 Nokia Corporation.
 *
 * Contact: Marius Vollmer <marius.vollmer@nokia.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "apt-worker-proto.h"

apt_proto_encoder::apt_proto_encoder ()
{
  buf = NULL;
  buf_len = len = 0;
}

apt_proto_encoder::~apt_proto_encoder ()
{
  if (buf)
    free (buf);
}

void
apt_proto_encoder::reset ()
{
  len = 0;
}

char *
apt_proto_encoder::get_buf ()
{
  return buf;
}

int
apt_proto_encoder::get_len ()
{
  return len;
}

static int
roundup (int val, int factor)
{
  return ((val + factor - 1) / factor) * factor;
}

void
apt_proto_encoder::grow (int delta)
{
  if (len + delta > buf_len)    {
      buf_len = roundup (len + delta, 4096);
      buf = (char *)realloc (buf, buf_len);
      if (buf == NULL)
	{
	  perror ("realloc");
	  exit (1);
	}
    }
}

void
apt_proto_encoder::encode_mem (const void *val, int n)
{
  int r = roundup (n, sizeof (int));
  grow (r);
  memcpy (buf+len, (char *)val, n);
  len += r;
}

void
apt_proto_encoder::encode_int (int val)
{
  encode_mem (&val, sizeof (int));
}

void
apt_proto_encoder::encode_string (const char *val)
{
  if (val == NULL)
    encode_int (-1);
  else
    {
      int len = strlen (val);
      encode_int (len);
      encode_mem (val, len+1);
    }
}

apt_proto_decoder::apt_proto_decoder ()
{
  reset (NULL, 0);
}

apt_proto_decoder::apt_proto_decoder (const char *buf, int len)
{
  reset (buf, len);
}

apt_proto_decoder::~apt_proto_decoder ()
{
}

void
apt_proto_decoder::reset (const char *buf, int len)
{
  this->buf = this->ptr = buf;
  this->len = len;
  corrupted_flag = false;
  at_end_flag = (len == 0);
}  

bool
apt_proto_decoder::at_end ()
{
  return at_end_flag;
}

bool
apt_proto_decoder::corrupted ()
{
  return corrupted_flag;
}

void
apt_proto_decoder::decode_mem (void *mem, int n)
{
  if (corrupted ())
    return;

  int r = roundup (n, sizeof (int));
  if (ptr + r > buf + len)
    {
      corrupted_flag = true;
      at_end_flag = true;
    }
  else
    {
      if (mem)
	memcpy ((char *)mem, ptr, n);
      ptr += r;
      if (ptr == buf + len)
	at_end_flag = true;
    }
}

int
apt_proto_decoder::decode_int ()
{
  int val = 0;
  decode_mem (&val, sizeof (int));
  return val;
}

const char *
apt_proto_decoder::decode_string_in_place ()
{
  int len = decode_int ();
  const char *str;

  if (len == -1 || corrupted ())
    return NULL;

  str = ptr;
  decode_mem (NULL, len+1);
  return str;
}

char *
apt_proto_decoder::decode_string_dup ()
{
  const char *ptr = decode_string_in_place ();
  if (ptr == NULL)
    return NULL;
  char *str = strdup (ptr);
  if (str == NULL)
    {
      perror ("strdup");
      exit (1);
    }
  return str;
}
