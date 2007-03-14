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

#include <libosso.h>
#include <hildon/hildon-help.h>
#include "osso-helplib.h"

#define link_warning(symbol, msg) \
  static const char __evoke_link_warning_##symbol[]     \
    __attribute__ ((unused, section (".gnu.warning." #symbol ))) \
    = msg;

/*---=== Public API ===---*/

/*---( doc in header )---*/
extern
osso_return_t ossohelp_show( osso_context_t* osso,
                             const char* help_id,
                             guint flags )
{
  return hildon_help_show (osso, help_id, flags) ;
}
link_warning(ossohelp_show, "warning: ossohelp_show has been deprecated in favour of hildon_help_show from pkg-config: hildon-help and Debian libhildonhelp0.");


/*---=== Dialog help enabling ===---*/

/* NOTE: code in this section should serve as a sample, or rather,
 *       be moved to the HildonDialogHelp codebase (that's where
 *       it belongs, so applications don't need to duplicate this)
 *
 *       If there's a way to pass the 'osso' pointer without giving
 *       it as a parameter, that would be fine.
 */

/*
    * This is signal ID for the on_help g_signal_connect to the ? icon
    * For multiple ContextUIDs using the same ? icon in a dialog 
    
    * A good use of this is: A sample_dialog has 2 (even more) TABs tab1 and tab2 
    * Required behaviour:
    *  + when clicking on tab1, clicking ? icon will show help_id_1
    *  + when clicking on tab2, clicking ? icon will show help_id_2

    * Now when handling the event of clicking tab1, this should be called:
        ossohelp_dialog_help_enable( sample_dialog, help_id_1,osso );
    * Similarly, clicking tab2 should invoke this call somewhere
        ossohelp_dialog_help_enable( sample_dialog, help_id_2,osso );

    * Doing this will solve the problem.    

*/

/*---( docs in header )---*/
gboolean ossohelp_dialog_help_enable( GtkDialog *dialog, 
                                      const gchar *topic,
                                      osso_context_t *osso )
{
  return hildon_help_dialog_help_enable (dialog, topic, osso) ;
}
link_warning(ossohelp_dialog_help_enable, "warning: ossohelp_dialog_help_enable has been deprecated in favour of hildon_help_dialog_help_enable from pkg-config: hildon-help and Debian libhildonhelp0.");
