/*
 * This file is part of libosso-help
 *
 * Copyright (C) 2006 Nokia Corporation. All rights reserved.
 *
 * Contact: Jakub Pavelek <jakub.pavelek@nokia.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
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
#ifndef OSSO_HELPLIB_H
#define OSSO_HELPLIB_H

#include <libosso.h>
#include <gtk/gtkdialog.h>

#ifdef __cplusplus
  extern "C" {
#endif

/*--- Public interface (for any applications) ---*/

enum {
    OSSO_HELP_SHOW_DIALOG= 0x01,
    OSSO_HELP_SHOW_JUSTASK= 0x02,
    };
    
/**
  Show Help topic or folder.  This same function can be used either for
  Help dialogs (displaying a single topic _without links_ in the
  calling application's dialog box), or for launching Help Application,
  providing browsing, search and history functions, too.
  <p>  
  Normally, applications would only use this to launch help topics, but
  it can be used for opening a folder view as well.
  
  @param osso OSSO context handle of the calling application, required 
              for RPC/DBUS calls to HelpApp or Browser. Can be NULL for
              #OSSO_HELP_SHOW_JUSTASK.

  @param topic_id Help topic ID ("xxx_yyy_zzz")

  @param flags 0 for default (open Help Application)
            <br>+1 (#OSSOHELP_SHOW_DIALOG) for opening as system dialog,
               without navigation possibilities
            <br>+2 (#OSSOHELP_SHOW_JUSTASK) to check, whether a certain 
               topic or folder is available on the system.
                    
  @return   #OSSO_OK            All well, help found (and being displayed)
            <br>#OSSO_ERROR     No help for such topic ID
            <br>#OSSO_RPC_ERROR Unable to contact HelpApp or Browser
            <br>#OSSO_INVALID   Param not formally right (NULL, or bad ID
                                format)
*/
osso_return_t ossohelp_show( osso_context_t *osso,
                             const gchar *topic_id,
                             guint flags );

/**
  Enable context specific help for a dialog.
  <p>
  Calling this function adds a '?' icon to the dialog's titlebar,
  allowing user to access the context specific help topic. All
  UI binding is done for you, one line is all it takes.
  
  @todo If the given help topic is not installed on the system, 
        the '?' icon should be dimmed (insensitive); is there a 
        way to do that?
  
  @note Do this call before 'gtk_widget_show_all()'; otherwise, the
        '?' icon won't show.

  @param dialog The GTK+ dialog to attach help to
  @param topic Help topic ID ("xxx_yyy_zzz") to show if the user
                presses the '?' icon
  @param osso OSSO context pointer of the calling application
 
  @return TRUE if the help topic existed, and '?' enabled
          <br>FALSE if the topic ID is not found in this system.
*/
gboolean ossohelp_dialog_help_enable( GtkDialog *dialog, 
                                      const gchar *topic,
                                      osso_context_t *osso );

#ifdef __cplusplus
  }
#endif

#endif  /* OSSO_HELPLIB_H */

