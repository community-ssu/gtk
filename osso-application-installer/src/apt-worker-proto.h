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

#ifndef APT_WORKER_PROTO_H
#define APT_WORKER_PROTO_H

#include <stdlib.h>

/* No, this is not DBUS. */

enum apt_command {
  APTCMD_NOOP,
  APTCMD_STATUS,
  APTCMD_GET_PACKAGE_LIST,
  APTCMD_GET_PACKAGE_INFO,
  APTCMD_GET_PACKAGE_DETAILS,
  APTCMD_UPDATE_PACKAGE_CACHE,

  APTCMD_MAX
};

struct apt_request_header {
  int cmd;
  int seq;
  int len;
};

struct apt_response_header {
  int cmd;
  int seq;
  int len;
};

// Encoding and decoding of data types

struct apt_proto_encoder {

  apt_proto_encoder ();
  ~apt_proto_encoder ();
  
  void reset ();

  void encode_mem (const void *, int);
  void encode_int (int);
  void encode_string (const char *);

  char *get_buf ();
  int get_len ();

private:
  char *buf;
  int buf_len;
  int len;
  void grow (int delta);
};

struct apt_proto_decoder {

  apt_proto_decoder ();
  apt_proto_decoder (const char *data, int len);
  ~apt_proto_decoder ();
  
  void reset (const char *data, int len);

  void decode_mem (void *, int);
  int decode_int ();
  const char *decode_string_in_place ();
  char *decode_string_dup ();

  bool at_end ();
  bool corrupted ();

private:
  const char *buf, *ptr;
  int len;
  bool corrupted_flag, at_end_flag;
};

// NOOP - do nothing, no paramters, no results

// STATUS - status report
//
// This command is special: you never send a request for it but
// instead you will receive STATUS responses whenever apt-worker has
// some progress to announce.
//
// The response always has seq == -1.  It contains:
//
// - label or null (string).  If null, use last one.
// - percent (int)

// GET_PACKAGE_LIST - get a list of packages with their names,
//                    versions, and partial status
//
// No parameters
//
// For each interesting package, the response contains:
//
// - name (string) 
// - installed version or null (string) 
// - installed_size (int)
// - available version or null (string) 
// - section of available version or null (string)

// UPDATE_PACKAGE_CACHE - recreate package cache
//
// No parameters.
//
// Response contains:
//
// - success (int)
//
// Error messages appear on stdout/stderr of the apt-worker process.

// GET_PACKAGE_INFO - get some more information about a specific
//                    package.  This information is used to augment
//                    the displayed list of packages but it is
//                    sufficiently expensive to calculate so that we
//                    dont want to include it in the GET_PACKAGE_LIST
//                    response.
//
// Parameters:
//
// - name (string).  Name of the package.
//
// Response:
//
// - info (apt_proto_package_info).
// - installed_short_description or null (string).
// - available_short_description or null (string).
//
// When the available_short_description would be identical to the
// installed_short_description, it is set to null.

struct apt_proto_package_info {
  int installable;
  int download_size;
  int install_user_size_delta;

  int removable;
  int remove_user_size_delta;
};

// GET_PACKAGE_DETAILS - get a lot of details about a specific
//                       package.  This is intended for the "Details"
//                       dialog, of course.
//
// Parameters:
//
// - name (string).
// - version (string).
// - summary_kind (int).  0 == none, 1 == install, 2 == remove
//
// Response:
//
// - maintainer (string).
// - description (string).
// - dependencies (deptype,string)*,(deptype_end).
// - summary (sumtype,string)*,(sumtype_end).

enum apt_proto_deptype { 
  deptype_end,
  deptype_depends,
  deptype_conflicts
};

enum apt_proto_sumtype {
  sumtype_end,
  sumtype_installing,
  sumtype_upgrading,
  sumtype_removing,
  sumtype_needed_by,
  sumtype_missing,
  sumtype_conflicting,
};

#endif /* !APT_WORKER_PROTO_H */
