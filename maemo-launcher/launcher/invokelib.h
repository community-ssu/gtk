/*
 * $Id$
 *
 * Copyright (C) 2005, 2006 Nokia
 *
 * Author: Guillem Jover <guillem.jover@nokia.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 *
 */

#ifndef INVOKELIB_H
#define INVOKELIB_H

#include <stdbool.h>
#include <stdint.h>

bool invoke_send_msg(int fd, uint32_t msg);
bool invoke_recv_msg(int fd, uint32_t *msg);

bool invoke_send_str(int fd, char *str);
char *invoke_recv_str(int fd);

/* FIXME: Should be '/var/run/'. */
#define INVOKER_SOCK	"/tmp/."PACKAGE

/* Protocol messages and masks. */
#define INVOKER_MSG_MASK		0xffff0000
#define INVOKER_MSG_MAGIC		0xb0070000
#define INVOKER_MSG_MAGIC_VERSION_MASK		0x0000ff00
#define INVOKER_MSG_MAGIC_VERSION		0x00000100
#define INVOKER_MSG_MAGIC_OPTION_MASK		0x000000ff
#define INVOKER_MSG_MAGIC_OPTION_WAIT		0x00000001
#define INVOKER_MSG_EXEC		0xe8ec0000
#define INVOKER_MSG_ARGS		0xa4650000
#define INVOKER_MSG_END			0xdead0000
#define INVOKER_MSG_ACK			0x600d0000

#endif

