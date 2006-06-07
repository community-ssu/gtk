/* 
 * @file log-functions.h
 * This file is the API for Libosso-internal logging functions.
 *
 * Copyright (C) 2005 Nokia Corporation.
 *
 * Contact: Kimmo Hämäläinen <kimmo.hamalainen@nokia.com> 
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; version 2.1 of
 * the License.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

#ifndef LOG_FUNCTIONS_H_
# define LOG_FUNCTIONS_H_ 

G_BEGIN_DECLS

# define LOG_D  __FILE__,__LINE__,LOG_DEBUG

/** Funcion to log debug info to the syslog.
 *      Will be empty with DEBUG not defined hoping that
 *      compiler will eliminate the call.
 *
 *   The first four arguments can be replaced with 
 *      LOG_D
 *
 * @param file the file this came from
 *
 * @param line the linenumber this came from
 *
 * @param level Log level. Set to CRIT , ERR, WARN or INFO.
 *                      Usage in similar manner as for syscall(2)
 *
 * @param format Printf styled format string, see printf(2) for details
 *
 * @param ... arguments for the format string.
 *
 */
inline void d_log(const char *file,int line, int level,
		  const char *format, ...);
		  
/** Funcion to log to the syslog.
 *
 * @param level Log level. Set to LOG_CRIT , LOG_ERR, LOG_WARNING or LOG_INFO.
 *                      Usage in similar manner as for syscall(2)
 *
 * @param format Printf styled format string, see printf(2) for details
 *
 * @param ... arguments for the format string.
 *
 */
void osso_log(int level, const char *format, ...);



G_END_DECLS

#endif /* LOG_FUNCTIONS_H_*/

