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

static const gboolean verbose= FALSE;

enum e_mode {
    MODE_CONTENTS,  /* list contents at given branch */
    MODE_FILTER,    /* find & output a certain topic */
    MODE_TITLEHUNT, /* find & output a certain topic/filter title text */
};


/*---=== XML system ===---*/

/**
  Advance 'sub_branch' to next level, and return the 'f_count'
  for this one.
  
  @param sub_branch_ref Reference to a 'sub_branch' pointer.
  @return int
*/
static
int loc_fcount( const char** sub_branch_ref )
{
    const char* ptr;
    int n;

    g_assert( sub_branch_ref && *sub_branch_ref );  /* "a/[.../]" */

    ptr= *sub_branch_ref;
    
    if (!*ptr) return 0;    /* end of string */
    
    n= *ptr-'a'+1;  /* 'a'..'z' */
    ptr++;
        
    g_assert(*ptr=='/');
    g_assert(n>0);
    
    *sub_branch_ref= ptr+1;
    return n;
}

/*--*/
struct s_mydata {
    enum e_mode mode;   /* MODE_CONTENTS / MODE_FILTER / MODE_TITLEHUNT */

    const char *sub_branch;
    int folders_met;
    int topics_met;

    /* MODE_FILTER (& MODE_TITLEHUNT): */
    int topic_number;   /* 1..N (number of topic to find)
                         * 0: looking for folder title (TITLEHUNT)
                         * -1: using topic_triplet (see below)
                         */
    const char *topic_triplet;  /* "xxx_yyy_zzz" if searching a file by topic id */
    
    gboolean filter_active;
    
    /* MODE_TITLEHUNT: */
    gboolean titlehunt_active;  /* collect characters to 'result_buf' */
    
    char result_buf[ HELP_TITLE_MAXLEN ]; /* used for both titles or helpid (context hunt) */
        
    /*--*/
    gboolean inside;
    int folders_until_mom;  /* number of <folder> tags (at this level) to skip */
    int folder_depth;       /* >0: doing subfolders (ignore them) */
    int active_depth;
    
    int listing_depth;      /* >0: list folders & topics at this depth */
};
static struct s_mydata d;

/**
  'startElement' XML callback
  <p>  
  We're only curious about the <folder> and <topic> tags,
  nothing else matters (well, <ossohelpsource> does).

  @param data ignored
  @param name name of the tag being processed
  @param attrs array of attributes that tag has
*/
static
void startElement_cb( void *data, const xmlChar *name, const xmlChar **attrs )
{
    (void)data;
    unsigned n=0;

    g_assert(d.sub_branch);
    
    if (verbose) {

        if (attrs) {
            for ( n=0; attrs[n]; n+=2 )
                ULOG_DEBUG(" %s=%s", attrs[n], attrs[n+1] );
        }
    }
    
    if (d.filter_active) {  /* pass any elements on to 'filter.c' */
        filter_startElement_cb( data, name, attrs );
        return;
    }

    if (strcmp(name,"ossohelpsource")==0) {
        g_assert( !d.inside );
        d.inside= TRUE;
        
        d.folders_until_mom=
            loc_fcount( &d.sub_branch );    /* 1==mom is next */

        g_assert( d.folders_until_mom > 0);
        
        d.folder_depth= 0;   /* currently not within a folder */
        d.active_depth= 1;   /* consider folders at this depth (ignore deeper) */
        d.listing_depth= -1;
        return;
    }

    if (!d.inside) return;      /* not met 'ossohelpsource' yet */

    if (d.listing_depth == d.folder_depth) {
        if (strcmp(name,"topic")==0) {
            d.topics_met++;

            
            if ((d.mode==MODE_FILTER) && (d.topic_number==d.topics_met)) {
                d.filter_active= TRUE;  /* filter to HTML until </topic> */
                filter_startElement_cb( data, name, attrs );  /* pass on <topic> */
                return;
            }
        } else if (strcmp(name,"folder")==0) {
            d.folders_met++;

            
        } else if (d.mode==MODE_TITLEHUNT) {
            if ((strcmp(name,"title")==0) && (d.topic_number==0)) {
                /* hunt current folder's title? (hey, we're there!) */
                d.titlehunt_active= TRUE;   /* catch any characters until '</title>' */
            } else if ((strcmp(name,"topictitle")==0) &&
                       (d.topic_number==d.topics_met)) {
                d.titlehunt_active= TRUE;   /* catch any characters until '</topictitle>' */
            }
            return;
        }
    }

    if (strcmp(name,"folder")==0) {
        d.folder_depth++;
        if ( d.folder_depth == d.active_depth) {
            d.folders_until_mom--;
            if (d.folders_until_mom == 0) {     /* almost there! */
                d.folders_until_mom=
                    loc_fcount( &(d.sub_branch) );  /* 1==mom is next */

                if ( d.folders_until_mom == 0 )
                    d.listing_depth= d.folder_depth;      /* we're ON! */
                else                        
                    d.active_depth++;
            }
        }
        return;
    }
}

/**
  'endElement' XML callback

  @param data ignored
  @param name name of the tag being processed
*/
static
void endElement_cb( void *data, const xmlChar *name )
{
    (void)data;

    g_assert(d.sub_branch); /* always "a/.../" */
        
    if (verbose) {
        ULOG_DEBUG ("(endElement: %s)\n", name );
    }
    
    if (!d.inside) return;
    
    if (d.filter_active) {      /* pass any elements on to 'filter.c' */
        /* let also </topic> pass on */
        filter_endElement_cb( data, name );

        if (strcmp(name,"topic")==0) {
            d.filter_active= FALSE;
            d.inside= FALSE;    /* ignore all the rest (could we jump out?) */
        }

        return;
    }

    if (strcmp(name,"ossohelpsource")==0) {
        /* If still active, did not find the folder. */
        d.inside= FALSE;
        return;
    }

    if (!d.inside) return;

    if (strcmp(name,"folder")==0) {
        if (d.listing_depth == d.folder_depth)
            d.inside= FALSE;    /* end of our work (could we just jump off?) */
        else
            d.folder_depth--;

        return;
    }
            
    if (d.titlehunt_active &&
        ((strcmp(name,"title")==0) || (strcmp(name,"topictitle")==0))) {
        d.inside= FALSE;    /* ignore all the rest (could we jump out?) */
        return;
    }
}

/**
  'characters' XML callback

  @param data ignored
  @param str text between the tags (need not be zero terminated!)
  @param len length of that text
*/
static
void characters_cb( void *data, const xmlChar *str, int len )
{
    (void)data;

    if (!d.inside) return;
    
    if (d.mode==MODE_CONTENTS) 
        return; /* nothing of interest */
        
    if (d.mode==MODE_TITLEHUNT && d.titlehunt_active) {
        strcat_safe_auto_max( d.result_buf, (const char*)str, len );    /* collect title */
        return;
    }

    if (d.filter_active) {
        filter_characters_cb( data, str, len );
        return;
    }
}

/**
  'entity' XML callback
  <p>
  Handles '&lt;', '&gt;', '&apos;', '&quot;' and '&amp;'
  
  @param data ignored
  @param name entity name
  @return interpretation of the entity
*/
static
xmlEntityPtr getEntity_cb( void *data, const xmlChar *name )
{
    (void)data;
    return xmlGetPredefinedEntity(name);
}

static xmlSAXHandler funcs_tbl= {   /* callbacks for libXML */
    NULL,   /*internalSubset,*/
    NULL,   /*isStandalone,*/
    NULL,   /*hasInternalSubset,*/
    NULL,   /*hasExternalSubset,*/
    NULL,   /*resolveEntity,*/
    getEntity_cb,
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
    characters_cb,
    NULL,   /*ignorableWhitespace,*/
    NULL,   /*processingInstruction,*/
    NULL,   /*comment,*/
    NULL,   /*warning_cb,*/
    NULL,   /*error_cb,*/
    NULL,   /*fatalError_cb*/
};

/**
  Check through a certain XML file's contents, listing how many 
  folders and/or topics exist on a certain sublevel.

  @note This function is a major candidate for caching, once
        performance optimizations are done (if the xml file
        isn't changed, we could=should store the info somewhere
        faster). A memory list that's only read once, then used
        automatically (much like how HelpGS output is cached).
        
  @param fn filename    XML filename to search through
  @param sub_branch     sublevel within that file
  @param folders_ref    Count of subfolders at this level (output)
  @param topics_ref     Count of topics at this level (output)
  @return always #TRUE
*/
static
gboolean loc_level_contents( const char *fn,
                             const char *sub_branch,
                             int *folders_ref,
                             int *topics_ref )
{
    g_assert(fn);
    g_assert(sub_branch);

    memset( &d, 0, sizeof(d) );
    d.sub_branch= sub_branch;
    d.mode= MODE_CONTENTS;

    xmlSAXParseFile( &funcs_tbl, fn, 0 /*recovery*/ );

    if (folders_ref) *folders_ref= d.folders_met;
    if (topics_ref) *topics_ref= d.topics_met;
    
    return TRUE;
}


/*---=== Big functions ===---*/

/**
  Try if a similar filename (with different suffix) exists.
  
  @param path path
  @param fn filename
  @param replace string to be replaced (old suffix)
  @param with replacement (new suffix)
  @return TRUE if file existed, #FALSE if not
*/
static
gboolean loc_exists( const char *path,
                     const char *fn, 
                     const char *replace, 
                     const char *with )
{
    char buf[FILENAME_MAX];
    char *ptr;

    strcpy_safe_auto( buf, path );
    strcat_safe_auto( buf, fn );
    ptr= strstr( buf, replace );
    
    if (!ptr) return FALSE;   /* no such suffix! */
    
    *ptr= '\0';
    strcat_safe_auto( buf, with );
    
    return g_file_test( buf, G_FILE_TEST_EXISTS );
}

/**
  Find the next topic/folder item in a list of contents
  for a particular level.
  
  @param s iteration state structure
  @param branch the branch we are iterating
  @return Next Help ID in the contents (NULL for no more)

  @see osso-helplib.h: ossohelp_next()
*/
const gchar *contents( struct s_OssoHelpWalker *s, 
                       const gchar *branch )
{
    if (!s) return NULL;

    if (branch && strcmp(branch,"//")==0)      /* both NULL and "//" work for the root */
        branch= NULL;
        
    /* Root level:
     *
     * list xml/gz files available (unique base names)
     * i.e. "sketch.xml" -> "//sketch/" and so on.
     */
    if (!branch) {
        GError* gerr= NULL;
        const gchar* fn;
        const gchar* path= ossohelp_getpath(NULL);
        
        if (!s->gdir) {     /* first call, no dir iterator */
            s->gdir= g_dir_open( path, 
                                 0, /*flags ("must be set to 0") */
                                 &gerr );
            if (!s->gdir) {
                /* Gets here if a language is used, which does not
                   have help material installed (= no such dir) */
                ULOG_ERR( "HelpLib: %s", gerr->message );
                g_error_free (gerr);
                return NULL;
            }
        }

        /* Files will be listed in random order, they need to be sorted
           _by titles_ anyways, so this is done at the UI level. */
        while ( (fn= g_dir_read_name(s->gdir)) != NULL ) {

            /* skip ".svn" (development phase) and ".index" files: */
            if (fn[0]=='.') continue;

            /* Filter out based on the suffix (pass "*.xml", "*.xml.gz", "*.gz"). */
            const char *last_ex= strrchr(fn,'.');   /* tail of 'fn' (or NULL) */

            if (!last_ex) continue; /* no extension! */

            /*---           
             * Only return one entry for: ".xml", ".xml.gz", ".gz"
             *
             *  ".xml" passes through always
             *  ".xml.gz" passes through if no ".xml" of same name
             *  ".gz" passes through if no ".xml" or ".xml.gz"
             *
             * Note: It really does not matter here, which precedence
             *      the filenames are given, as long as only one entry
             *      is returned. Only the base name is given further.
             */
            if (strcmp(last_ex,".xml")==0) {
                /* always pass through */
                
            } else if (strcmp(last_ex,".gz")==0) {
                if (strstr(fn,".xml.gz")) {
                    if (loc_exists( path, fn, ".xml.gz", ".xml" )) {
                        continue;   /* ".xml" was there (and will be reported) */
                    }
                } else {  /* ".gz" */
                    /*  Mystery: why does this code (the 'continue' line)
                     *  give "warning: will never be executed"?
                     *
                     * SOLUTION: DISABLING -Werror for now.. :(
                     */
                    if (loc_exists( path, fn, ".gz", ".xml" ) ||
                        loc_exists( path, fn, ".gz", ".xml.gz" )) {
                        continue;   /* either ".xml.gz" or ".xml" existed */
                    }
                }
            } else {
                continue;   /* not ".xml", "[.xml].gz" -> skip */
            }
                
            /* we have a contact. */
            /*ULOG_DEBUG( "Listing: %s", fn );*/
            
            strcpy_safe_auto( s->key, "//" );
            strcat_safe_auto_max( s->key, fn, strchr(fn,'.')-fn );  /* just the base part */
            strcat_safe_auto( s->key, "/a/" );  /* root folder (always just one) */
            
            return s->key;  /* we're done! (this time) */
        }
            
        g_dir_close(s->gdir);
        s->gdir= NULL;
        return NULL;    /* no more */
        
    } else {
        /* Folder/topic level (within a certain file) */
        if (last_char(branch)!='/')   /* what, asking contents of a topic?! */
            return NULL;
            
        if ((s->folders==0) && (s->topics==0)) {    /* first time here */
            char fn[FILENAME_MAX];

            const char *sub_branch= ossohelp_file( branch, fn, sizeof(fn) );
            g_assert(sub_branch);   /* must have found the file! */
            
            if (!sub_branch[0])
                s->folders= 1;  /* root, always just one folder (right?) */
            else {
                if (!loc_level_contents( fn, sub_branch, &s->folders, &s->topics )) {
                    ULOG_CRIT( "Error accessing help: %s", branch );
                    return NULL;
                }
            
                /* If _still_ no contents, it was an empty folder! */
                if ((s->folders==0) && (s->topics==0))
                    return NULL;
            }
        }
        
        /* Just refresh the key string.. (no more hard work this level) */
        {
        char minibuf[10];   /* should fit "x/" or "nnn" */

        /* folders first */
        if (s->listed_folder < s->folders)  /* "a/".."z/" */
            snprintf( minibuf, sizeof(minibuf), "%c/", ('a'-1)+ (++(s->listed_folder)) );
        else
        if (s->listed_topic < s->topics)    /* "nnn" */
            snprintf( minibuf, sizeof(minibuf), "%d", ++(s->listed_topic) );
        else
            return NULL;    /* went through them all! */
        
        strcpy_safe_auto( s->key, branch );   /* ends with '/' */
        strcat_safe_auto( s->key, minibuf );
        }
        
        return s->key;
    }
}


/**
  HTML output of topic 'key' to 'outf'.
  
  @param outf file to write to
  @param key Help ID ("//fname/a/[../]NN")
  @param dialog #TRUE for producing dialog contents (no navigational
                links etc.), #FALSE for regular (full) output
  @return #TRUE if all well, things output,
          #FALSE on error
*/  
gboolean filter( FILE *outf, const char *key, gboolean dialog )
{
    char fn[FILENAME_MAX];
    char keybuf[HELP_KEY_MAXLEN];
    char *subkey;       /* after filename part */
    char *ptr;

    if (!key) return FALSE;

    init_filter_html();   /* prepares html output tables (only once) */
        
    strcpy_safe_auto( keybuf, key );
    
    subkey= (char*)ossohelp_file( keybuf, fn, sizeof(fn) );
    if (!subkey) {  
        ULOG_ERR( "No such file: %s", fn );
        return FALSE;
    }

    memset( &d, 0, sizeof(d) );

    ptr= strrchr( subkey, '/' );    /* ptr to last '/' */
    if (!ptr[1]) {
        ULOG_ERR( "Is a folder (no HTML output): %s", key );
        return FALSE;
    }

    d.topic_number= atoi( ptr+1 );  
    g_assert( d.topic_number > 0 ); /* 1..NN */

    ptr[1]= '\0';   /* parent */
    d.sub_branch= subkey;

    d.mode= MODE_FILTER;
    filter_outf= outf;
    dialog_mode= dialog;
    
    xmlSAXParseFile( &funcs_tbl, fn, 0 /*recovery*/ );

    return TRUE;    
}

/**
  Find plain text title for folder or topic 'key_or_triplet'.

  @param key_or_triplet Either contextUID triplet ("xxx_fname_zzz")
                        or direct Help ID ("//fname/a/[../][NN]")
  @param buf buffer for the title (may be NULL just to check a
             topic/folder exists)
  @param bufsize size of 'buf' (should be HELP_TITLE_MAXLEN)
  @return #TRUE if a title was found, #FALSE if not
*/
gboolean title_from_key( const char *key_or_triplet, 
                         char *buf, size_t bufsize )
{
    char fn[FILENAME_MAX];
    char keybuf[HELP_KEY_MAXLEN];
    char *subkey;       /* after filename part */
    char *ptr;
    char *p1, *p2;     /* used to skip any whitespace at beginning & end */

    if (!key_or_triplet) return FALSE;

    if (strchr( key_or_triplet, '/' ))
        strcpy_safe_auto( keybuf, key_or_triplet ); /* "//..." (help id) */
    else {
        if (!key_from_triplet( key_or_triplet,
                               keybuf, sizeof(keybuf) ))
            return FALSE;   /* triplet not found! */
            
        if (!buf)
            return TRUE;    /* caller only checking, triplet existed */
    }
    
    subkey= (char*)ossohelp_file( keybuf, fn, sizeof(fn) );
    if (!subkey) {
        ULOG_CRIT( "Missing help file: %s (%s)", fn, key_or_triplet );
        return FALSE;
    }

    memset( &d, 0, sizeof(d) );

/*ULOG_DEBUG( "subkey: %s", subkey );
  ULOG_DEBUG( "keybuf: %s", keybuf );*/

    ptr= strrchr( subkey, '/' );    /* ptr to last '/' (in buffer) */
    g_assert(ptr);
    
    if (ptr[1]) {    /* topic */
        d.topic_number= atoi( ptr+1 );  
        g_assert( d.topic_number > 0 ); /* 1..NN */
        
        ptr[1]= '\0';   /* cut topic part off */
    }
    d.sub_branch= subkey;
    d.mode= MODE_TITLEHUNT;
    
    xmlSAXParseFile( &funcs_tbl, fn, 0 /*recovery*/ );
    
    p1= d.result_buf;
    while (*p1 && isspace(*p1)) p1++;
    
    p2= strchr(p1,'\0')-1;
    while ( (p2>p1) && isspace(*p2) ) {
        *p2='\0'; p2--;
    }
    
    if (!p1[0])
        return FALSE;   /* no such folder or topic! */
    else {
        if (buf) strcpy_safe( buf, p1, bufsize );
        return TRUE;
    }
}

