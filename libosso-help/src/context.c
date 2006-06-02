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

#include "osso-helplib-private.h"
#include "internal.h"

#include <libxml/parser.h>     /* libXML */

#include <assert.h>
#include <string.h>
#include <ctype.h>      /* isspace() */


static const char *triplet_to_look_for;

static char trace[20];      /* "abcdefg".. */
static int current_depth = 0;    /* pointer to level in 'trace' we're working on */
static int topic_count = 0;

static gboolean matched = FALSE;    /* when TRUE, data is available */


/*---=== Helper functions ===---*/

/*---=== XML system ===---*/

static
void startElement_cb( void *data, const xmlChar *name, const xmlChar **attrs )
{
    (void)data;

    if (matched) return;
    
    if (strcmp(name,"context")==0) {
        const char *uid= (const char*)loc_getattr( attrs, "contextUID" );
        
        if (!uid) {
            ULOG_ERR( "<context> tag without 'contextUID'!" );
            return;   /* skip it */
        }

        if (g_ascii_strcasecmp(triplet_to_look_for,uid)==0) {
            matched= TRUE;  /* This freezes the data, but there's no way */
            return;         /* we could stop the iteration, is there? */
        }
    }

    if (strcmp(name,"folder")==0) {
        /* First folder, 'trace'=="", 'current_depth'==0 */
        g_assert( current_depth < sizeof(trace)-2 );

        if (!trace[current_depth])
            trace[current_depth]= 'a';
        else
            trace[current_depth]++;  /* 'a'->'b'->... */

        current_depth++;    /* further items will be subfolders */
        trace[current_depth]= '\0';
    }

    if (strcmp(name,"topic")==0)
        topic_count++;
}

static
void endElement_cb( void *data, const xmlChar *name )
{
    (void)data;

    if (matched) return;
    
    if (strcmp(name,"folder")==0) {
        g_assert( current_depth >= 1 );
        current_depth--;
        
        topic_count= 0;
    }
}

/*---*/
static xmlSAXHandler funcs_tbl= {   /* callbacks for libXML */
    NULL,   /*internalSubset,*/
    NULL,   /*isStandalone,*/
    NULL,   /*hasInternalSubset,*/
    NULL,   /*hasExternalSubset,*/
    NULL,   /*resolveEntity,*/
    NULL,   /*getEntity_cb,*/
    NULL,   /*entityDecl,*/
    NULL,   /*notationDecl,*/
    NULL,   /*attributeDecl,*/
    NULL,   /*elementDecl,*/
    NULL,   /*unparsedEntityDecl,*/
    NULL,   /*setDocumentLocator,*/
    NULL,   /*startDocument_cb,*/
    NULL,   /*endDocument_cb,*/
    startElement_cb,
    endElement_cb,
    NULL,   /*reference,*/
    NULL,   /*characters_cb,*/
    NULL,   /*ignorableWhitespace,*/
    NULL,   /*processingInstruction,*/
    NULL,   /*comment,*/
    NULL,   /*warning_cb,*/
    NULL,   /*error_cb,*/
    NULL,   /*fatalError_cb*/
};


/**
  Find the help key ("//fname/[../]nnn") for a certain triplet
  ("xxx_fname_zzz").

  @note Applications use triplets (same as topic "contextUID")
        for referring to their help material. This is the _only_
        use for them.   Currently, using triplets needs two runs
        through the xml file (triplet->key + actual filtering).
        These could be combined, but it is not quite trivial
        (since the contextUID is not within the title tag itself,
        a linear search through the material alone wouldn't be
        enough).
        <p>
        Anyways, this can be optimized later, but the benefit is
        arguable since it only affects the _first page_ (or launch
        speed) of the Help page. And not by much. Further links
        will be using the help native id's ("//...") which are fast.

  @param triplet topic contextUID ("xxx_fname_zzz") to search for.
         Note that regular "//fname/a[../]NN" keys can also be used,
         in which case they're simply copied to the given buffer.
  @param buf buffer to place the direct help id ("//fname/a/[../]NN")
         within. Allowed to be NULL (just checking if 'triplet' is
         found).
  @param length of 'buf' (should be #HELP_KEY_MAXLEN long)
  @return #TRUE if such a contextUID existed ('buf' has path to it)
          #FALSE if not
*/
gboolean key_from_triplet( const char *triplet, 
                           char *buf, size_t bufsize )
{
    
    char fn[FILENAME_MAX]= "";
    char *p1, *p2= NULL;

    if (!triplet) return FALSE;


    if (triplet[0] == '/') {    /* already a key ("//fname/a/[../][NN]") */
        if (buf) strcpy_safe( buf, triplet, bufsize );
        return TRUE;    /* assume it exists */
    }

    /* Mid part ("xxx_yyy_zzz") of triplet should be the help source
       filename part (s.a. "sketch"). */
    p1= strchr( triplet, '_' );
    if (p1) {
        p2= strchr( p1+1, '_' );
        if (p2) {
            if (!ossohelp_file2( p1+1, p2-p1-1, fn, sizeof(fn) )) {
                return FALSE;   /* no such help file! (obviously, no topic) */
                /*
                 * Note: this could also be due to the mid part _not_
                 *       matching the filename base - if so, we need caching
                 *       and going through _all_ the help material..
                 */
            }
        }
    }
        
    if (!fn[0])
        return FALSE;   /* not a valid triplet?! */

    /* Set up searching pattern */
    triplet_to_look_for= triplet;

    trace[0]= '\0';
    current_depth= 0;
    topic_count= 0;
    matched= FALSE;

    /* Search through the xml */
    xmlSAXParseFile( &funcs_tbl, fn, 0 /*recovery*/ );

    if (!matched) 
        return FALSE;     /* no such triplet! */

    /* Analyze results */
    if (buf) {
        char buf2[10];
        int n;

        /* "abgd", 7 -> "//fname/a/b/g/d/7" */
        strcpy_safe( buf, "//", bufsize );
        strcat_safe_max( buf, p1+1, bufsize, p2-p1-1 );  /* fn part */
        strcat_safe( buf, "/", bufsize );
        
        for ( n=0; n<current_depth; n++ ) {
            strcat_safe_max( buf, &trace[n], bufsize, 1 );  /* one char */
            strcat_safe( buf, "/", bufsize );
        }
        snprintf( buf2, sizeof(buf2), "%d", topic_count );
        strcat_safe( buf, buf2, bufsize );
    }
        
    return TRUE;    /* found! */
}

