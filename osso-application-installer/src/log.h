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

#ifndef LOG_H
#define LOG_H

#include "apt-worker-proto.h"

void add_log (const char *fmt, ...);
void log_perror (const char *msg);
void log_from_fd (int fd);

/* Log scanning.  This is used to guess at some causes for errors.

   SET_LOG_START remembers the current end of the log.
   SCAN_LOG scans from the last remembered position to the end.
 */

void set_log_start ();
bool scan_log (const char *str);

apt_proto_result_code scan_log_for_result_code (apt_proto_result_code init);

void show_log ();

#endif /* !LOG_H */
