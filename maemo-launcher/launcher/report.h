/*
 * report.h
 *
 * Copyright (C) 2005 Nokia
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

#ifndef REPORT_H
#define REPORT_H

enum report_output {
  report_console,
  report_syslog,
  report_none
};

extern void report_set_output(enum report_output new_output);

#ifdef DEBUG
extern void debug(char *msg, ...);
#else
#define debug(...)
#endif

extern void info(char *msg, ...);
extern void error(char *msg, ...);
extern void __attribute__((noreturn)) die(int status, char *msg, ...);

#endif

