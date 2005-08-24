/**
   @file applicationinstaller-i18n.h

   All localization macros
   <p>
*/

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

#ifndef __AI_INTL_H__
#define __AI_INTL_H__

#ifdef ENABLE_NLS
#include <libintl.h>
#define _(String) gettext(String)
#ifdef gettext_noop
#define N_(String) gettext_noop(String)
#else
#define N_(String) (String)
#endif
#else /* NLS is disabled */
#define _(String) (String)
#define N_(String) (String)
#define textdomain(String) (String)
#define gettext(String) (String)
#define dgettext(Domain,String) (String)
#define dcgettext(Domain,String,Type) (String)
#define bindtextdomain(Domain,Directory) (Domain) 
#define bind_textdomain_codeset(Domain,Codeset) (Codeset) 
#endif /* ENABLE_NLS */

/* Logical ids do not contain format specifiers and GCC is smart
   enough to warn about that.  In order to suppress these warnings in
   selected places, you can use this macro around the format argument.
*/

#define SUPPRESS_FORMAT_WARNING(x) ((char *)(long)(x))

/* Sometimes there is a temporary disruption in the space-time
   continuum and the localization files do not match the logical
   strings that the source code wants to use.  In order to be
   backwards compatible with old localization files but still
   magically use the newer, better logical ids when they become
   available in the l10n files, you can use this variant of gettext
   that tries a NULL-terminated sequence of strings until it can get a
   translation.  If no translation is available for any of the
   strings, it returns the last string in the sequence.
*/

char *gettext_try_many (const char *logical_id, ...);

#endif /* __AI_INTL_H__ */
