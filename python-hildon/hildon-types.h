/* -*- Mode: C; c-basic-offset: 4 -*-
 * python-hildon - Python bindings for the Hildon toolkit.
 *
 * hildon-types: definitions that should be in hildon
 *               itself but are not yet.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 */


#ifndef HILDON_TYPES_H
#define HILDON_TYPES_H

#include <glib.h>
#include <hildon-base-lib/hildon-base-dnotify.h>
#include <hildon-base-lib/hildon-base-types.h>
#include <hildon-widgets/hildon-add-home-dialog.h>
#include <hildon-widgets/hildon-app.h>
#include <hildon-widgets/hildon-banner.h>
#include <hildon-widgets/hildon-calendar-popup.h>
#include <hildon-widgets/hildon-caption.h>
#include <hildon-widgets/hildon-color-button.h>
#include <hildon-widgets/hildon-color-selector.h>
#include <hildon-widgets/hildon-controlbar.h>
#include <hildon-widgets/hildon-date-editor.h>
#include <hildon-widgets/hildon-dialoghelp.h>
#include <hildon-widgets/hildon-file-chooser-dialog.h>
#include <hildon-widgets/hildon-file-details-dialog.h>
#include <hildon-widgets/hildon-file-handling-note.h>
#include <hildon-widgets/hildon-file-system-model.h>
#include <hildon-widgets/hildon-find-toolbar.h>
#include <hildon-widgets/hildon-font-selection-dialog.h>
#include <hildon-widgets/hildon-get-password-dialog.h>
#include <hildon-widgets/hildon-grid.h>
#include <hildon-widgets/hildon-grid-item.h>
#include <hildon-widgets/hildon-hvolumebar.h>
#include <hildon-widgets/hildon-insert-object-dialog.h>
#include <hildon-widgets/hildon-name-password-dialog.h>
#include <hildon-widgets/hildon-note.h>
#include <hildon-widgets/hildon-number-editor.h>
#include <hildon-widgets/hildon-program.h>
#include <hildon-widgets/hildon-range-editor.h>
#include <hildon-widgets/hildon-scroll-area.h>
#include <hildon-widgets/hildon-seekbar.h>
#include <hildon-widgets/hildon-set-password-dialog.h>
#include <hildon-widgets/hildon-sort-dialog.h>
#include <hildon-widgets/hildon-telephone-editor.h>
#include <hildon-widgets/hildon-time-editor.h>
#include <hildon-widgets/hildon-time-picker.h>
#include <hildon-widgets/hildon-volumebar.h>
#include <hildon-widgets/hildon-vvolumebar.h>
#include <hildon-widgets/hildon-weekday-picker.h>
#include <hildon-widgets/hildon-window.h>
#include <hildon-widgets/hildon-wizard-dialog.h>

#ifndef HILDON_DISABLE_DEPRECATED
#include <hildon-widgets/hildon-search.h>
#include <hildon-widgets/hildon-find-object-dialog.h>
#include <hildon-widgets/hildon-find-replace-dialog.h>
#endif

#include "hildon-types.h.in"

#endif /* !HILDON_TYPES_H */
