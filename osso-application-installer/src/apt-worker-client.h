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

#ifndef APT_WORKER_CLIENTH
#define APT_WORKER_CLIENTH

extern int apt_worker_in_fd, apt_worker_out_fd;

void start_apt_worker ();
void stop_apt_worker ();

typedef void apt_worker_callback (int cmd,
				  char *response_data, int response_len,
				  void *callback_data);

/* There can be only one non-completed request per command.  If you
   try tp queue another request before the pending one has completed,
   you get an error and the DONE callback will be called with a NULL
   response data.
*/
void call_apt_worker (int cmd, char *data, int len,
		      apt_worker_callback *done,
		      void *done_data);

bool apt_worker_is_running ();
void send_apt_request (int cmd, int seq, char *data, int len);
void handle_one_apt_worker_response ();

/* Specific commands.
 */

void apt_worker_set_status_callback (apt_worker_callback *callback,
				     void *data);

void apt_worker_get_package_list (apt_worker_callback *callback,
				  void *data);

void apt_worker_update_cache (apt_worker_callback *callback,
			      void *data);

void apt_worker_get_package_info (const char *package,
				  apt_worker_callback *callback,
				  void *data);

void apt_worker_get_package_details (const char *package,
				     const char *version,
				     int summary_kind,
				     apt_worker_callback *callback,
				     void *data);

#endif /* !APT_WORKER_CLIENT_H */
