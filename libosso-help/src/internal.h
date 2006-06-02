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

#ifndef OSSO_HELPLIB_INTERNAL_H
#define OSSO_HELPLIB_INTERNAL_H

#include <libosso.h>
#include <osso-log.h>       /* ULOG_...() */

#include <libxml/parser.h>  /* xmlChar */

#include "osso-helplib-private.h"   /* HELP_KEY_MAXLEN */

#define fexists(fn)  g_file_test( fn, G_FILE_TEST_EXISTS )

/*#define USE_CSS*/  /* do we use CSS decorations */

/* osso-helplib.c: */
const gchar *helplib_str_close(void);
const gchar *helplib_str_title(void);

gboolean ossohelp_file2( const char *basename, int basename_len,
                         char *fn_buf, size_t buflen );


/* contents.c: */
struct s_OssoHelpWalker   /* contents secret, not part of the API */
{
    char key[ HELP_KEY_MAXLEN ];    /* i.e. "//file/a/z/a" */

    /* walking state (root dir): */
    GDir* gdir;     /* if non-NULL, walking the filenames */

    /* walking state (within a file): */
    int folders;    /* folders within that level (0..N) */
    int topics;     /* topics within that level (0..N) */
    
    int listed_folder;
    int listed_topic;
};

const char *contents( struct s_OssoHelpWalker *s, 
                      const char *branch );

gboolean filter( FILE *outf, const char *key, gboolean dialog );

gboolean title_from_key( const char *key, 
                         char *buf, size_t bufsize );

void context( const char *fn );


/* graphic.c */
gboolean graphic_tag( char *buf, size_t bufsize, const char *fname);


/* filter.c */
extern FILE* filter_outf;
extern gboolean dialog_mode;

unsigned loc_getattr_count( const xmlChar **attrs );
const char *loc_getattr( const xmlChar **attrs, const char *key );

void filter_characters_cb( void *data, const xmlChar *str, int len );
void filter_startElement_cb( void *data, const xmlChar *name, const xmlChar **attrs );
void filter_endElement_cb( void *data, const xmlChar *name );

void init_filter_html( void );

/* context.c */
gboolean key_from_triplet( const char *triplet, 
                           char *buf, size_t bufsize );

/* dialog.c */
osso_return_t system_dialog( osso_context_t* osso,
                             const char* topic_key );

#endif  /* OSSO_HELPLIB_INTERNAL_H */

