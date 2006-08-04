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

/* Note: This program only uses/needs 'libxml' and is in no way Gnome dependent.
 */
#include <libxml/parser.h>     /* libXML */

#include <unistd.h>     /* write() */
#include <string.h>     /* strcmp() */
#include <sys/param.h>  /* PATH_MAX */

#include <ctype.h>      /* isspace() */

#include <libintl.h>    /* gettext() */
#define _(str) gettext(str)

#include "osso-helplib-private.h"
#include "internal.h"

/* Runtime tag configuration: */
#define OSSO_HELP_CFG "htmltags.cfg"

/* Big enough for any HTML output from a single XML tag (100 is too little for image tags) */
#define HTML_TAG_BUFLEN 350

/* Max. length for .cfg file lines */
#define MAX_CFG_LINELENGTH 200

/* Variables: */
static gboolean verbose= FALSE;

static unsigned unknown_depth;  /* 0= not within unknown block
                                   1..N= depth within an unknown block */
#ifndef USE_CSS
static const char *emphasis_stop;
#endif

FILE* filter_outf;
gboolean dialog_mode;   /* no title, no links, no... */

/*--*/
static const char* TOPICTITLE_START;
static const char* TOPICTITLE_STOP;
static const char* DISPLAY_TEXT_START;
static const char* DISPLAY_TEXT_STOP;
static const char* TIP_START;
static const char* TIP_STOP;
static const char* NOTE_START;
static const char* NOTE_STOP;
static const char* EXAMPLE_START;
static const char* EXAMPLE_STOP;
static const char* WARNING_START;
static const char* WARNING_STOP;
static const char* IMPORTANT_START;
static const char* IMPORTANT_STOP;
static const char* PARA_START;
static const char* PARA_STOP;
static const char* EMPHASIS_BOLD_START;
static const char* EMPHASIS_BOLD_STOP;
static const char* EMPHASIS_ITALIC_START;
static const char* EMPHASIS_ITALIC_STOP;
static const char* MENU_SEQ_START;
static const char* MENU_SEQ_STOP;
static const char* TASK_SEQ_START;
static const char* TASK_SEQ_STEP_START;
static const char* TASK_SEQ_STEP_STOP;
static const char* TASK_SEQ_STOP;
static const char* GRAPHIC_S;
static const char* LIST_START;
static const char* LIST_STYLE_BULLET;
static const char* LIST_STOP;
static const char* LISTITEM_START;
static const char* LISTITEM_STOP;
static const char* HEADING_START;
static const char* HEADING_STOP;
static const char* TAG_BREADCRUMB_TRAIL_START;
static const char* TAG_BREADCRUMB_TRAIL_STOP;
static const char* TAG_TOPIC_TITLE_START;
static const char* TAG_TOPIC_TITLE_STOP;
static const char* TAG_SUBHEADING_START;
static const char* TAG_SUBHEADING_STOP;
static const char* TAG_BODY_TEXT_START;
static const char* TAG_BODY_TEXT_STOP;
static const char* TAG_IMAGE_START;
static const char* TAG_IMAGE_STOP;
static const char* TAG_CAPTION_START;
static const char* TAG_CAPTION_STOP;
static const char* TAG_NOTE_START;
static const char* TAG_NOTE_STOP;
static const char* TAG_TIP_START;
static const char* TAG_TIP_STOP;
static const char* TAG_EXAMPLE_START;
static const char* TAG_EXAMPLE_STOP;
static const char* TAG_WARNING_START;
static const char* TAG_WARNING_STOP;
static const char* TAG_IMPORTANT_START;
static const char* TAG_IMPORTANT_STOP;
static const char* TAG_NUMBERED_LIST_START;
static const char* TAG_NUMBERED_LIST_ITEM_START;
static const char* TAG_NUMBERED_LIST_ITEM_STOP;
static const char* TAG_NUMBERED_LIST_STOP;
static const char* TAG_BULLET_LIST_START;
static const char* TAG_BULLET_LIST_ITEM_START;
static const char* TAG_BULLET_LIST_ITEM_STOP;
static const char* TAG_BULLET_LIST_STOP;
static const char* TAG_TASK_SEQUENCE_START;
static const char* TAG_TASK_SEQUENCE_STEP_START;
static const char* TAG_TASK_SEQUENCE_STEP_STOP;
static const char* TAG_TASK_SEQUENCE_STOP;
static const char* TAG_DEFINITION_LIST_START;
static const char* TAG_DEFINITION_LIST_STOP;
static const char* TAG_EMPHASIS_START;
static const char* TAG_EMPHASIS_STOP;
static const char* TAG_DISPLAY_TEXT_START;
static const char* TAG_DISPLAY_TEXT_STOP;
static const char* TAG_WEB_LINK_START;
static const char* TAG_WEB_LINK_STOP;
static const char* TAG_MENU_SEQUENCE_START;
static const char* TAG_MENU_SEQUENCE_STOP;
/*...*/

/*--*/
struct s_my{ const char* key;   /* i.e. "TIP_START" */
             const char** ref;  /* i.e. &TIP_START */
           };
#define _ITEM(name) { #name, &(name) }

static struct s_my html_keys[]= {
    _ITEM(TOPICTITLE_START),
    _ITEM(TOPICTITLE_STOP),
    _ITEM(DISPLAY_TEXT_START),
    _ITEM(DISPLAY_TEXT_STOP),
    _ITEM(TIP_START),
    _ITEM(TIP_STOP),
    _ITEM(NOTE_START),
    _ITEM(NOTE_STOP),
    _ITEM(EXAMPLE_START),
    _ITEM(EXAMPLE_STOP),
    _ITEM(WARNING_START),
    _ITEM(WARNING_STOP),
    _ITEM(IMPORTANT_START),
    _ITEM(IMPORTANT_STOP),
    _ITEM(PARA_START),
    _ITEM(PARA_STOP),
    _ITEM(EMPHASIS_BOLD_START),
    _ITEM(EMPHASIS_BOLD_STOP),
    _ITEM(EMPHASIS_ITALIC_START),
    _ITEM(EMPHASIS_ITALIC_STOP),
    _ITEM(MENU_SEQ_START),
    _ITEM(MENU_SEQ_STOP),
    _ITEM(GRAPHIC_S),
    _ITEM(LIST_START),
    _ITEM(LIST_STYLE_BULLET),
    _ITEM(LIST_STOP),
    _ITEM(LISTITEM_START),
    _ITEM(LISTITEM_STOP),
    _ITEM(HEADING_START),
    _ITEM(HEADING_STOP),
    _ITEM(TAG_BREADCRUMB_TRAIL_START),
    _ITEM(TAG_BREADCRUMB_TRAIL_STOP),
    _ITEM(TAG_TOPIC_TITLE_START),
    _ITEM(TAG_TOPIC_TITLE_STOP),
    _ITEM(TAG_SUBHEADING_START),
    _ITEM(TAG_SUBHEADING_STOP),
    _ITEM(TAG_BODY_TEXT_START),
    _ITEM(TAG_BODY_TEXT_STOP),
    _ITEM(TAG_IMAGE_START),
    _ITEM(TAG_IMAGE_STOP),
    _ITEM(TAG_CAPTION_START),
    _ITEM(TAG_CAPTION_STOP),
    _ITEM(TAG_NOTE_START),
    _ITEM(TAG_NOTE_STOP),
    _ITEM(TAG_TIP_START),
    _ITEM(TAG_TIP_STOP),
    _ITEM(TAG_EXAMPLE_START),
    _ITEM(TAG_EXAMPLE_STOP),
    _ITEM(TAG_WARNING_START),
    _ITEM(TAG_WARNING_STOP),
    _ITEM(TAG_IMPORTANT_START),
    _ITEM(TAG_IMPORTANT_STOP),
    _ITEM(TAG_NUMBERED_LIST_START),
    _ITEM(TAG_NUMBERED_LIST_ITEM_START),
    _ITEM(TAG_NUMBERED_LIST_ITEM_STOP),
    _ITEM(TAG_NUMBERED_LIST_STOP),
    _ITEM(TAG_BULLET_LIST_START),
    _ITEM(TAG_BULLET_LIST_ITEM_START),
    _ITEM(TAG_BULLET_LIST_ITEM_STOP),
    _ITEM(TAG_BULLET_LIST_STOP),
    _ITEM(TAG_TASK_SEQUENCE_START),
    _ITEM(TAG_TASK_SEQUENCE_STEP_START),
    _ITEM(TAG_TASK_SEQUENCE_STEP_STOP),
    _ITEM(TAG_TASK_SEQUENCE_STOP),
    _ITEM(TAG_DEFINITION_LIST_START),
    _ITEM(TAG_DEFINITION_LIST_STOP),
    _ITEM(TAG_EMPHASIS_START),
    _ITEM(TAG_EMPHASIS_STOP),
    _ITEM(TAG_DISPLAY_TEXT_START),
    _ITEM(TAG_DISPLAY_TEXT_STOP),
    _ITEM(TAG_WEB_LINK_START),
    _ITEM(TAG_WEB_LINK_STOP),
    _ITEM(TAG_MENU_SEQUENCE_START),
    _ITEM(TAG_MENU_SEQUENCE_STOP),
    /*...*/
    {NULL} };

/*...*/


/*--- Filter help routines ---*/

/**
  Return number of XML attributes
  
  @param attrs array of name/value -pairs, NULL terminated
  @return number of name/value pairs
*/
unsigned loc_getattr_count( const xmlChar **attrs )
{
    unsigned n=0;

    if (attrs) {
        while( attrs[n*2] ) n++;
    }
    return n;
}

/**
  Provide a particular attribute's value (by name)
  
  @param attrs array of XML attribute name/value pairs, 
               NULL if no arguments
  @param name attribute name to find
  @return value for the attribute, or NULL if none
  
  @note This function also converts 'xmlChar' pointers
        to ASCII 'char' pointers. 
*/
const char *loc_getattr( const xmlChar **attrs, const char *name )
{
    unsigned n;

    if (attrs && name) {
        for( n=0; attrs[n*2]; n++ ) {
            if ( strcmp(attrs[n*2],name)==0 )
                return (const char *)attrs[n*2+1];
        }
    }
    return NULL;    /* not found */
}


/*--- Filter callbacks ---*/

/**
  'characters' XML callback (for filter mode)
 
  @param data ignored
  @param str text (not NULL terminated!)
  @param len length of text
*/
void filter_characters_cb( void *data, const xmlChar *str, int len )
{
    int n;
    (void)data;

    if (!filter_outf || !str) return;

    if (unknown_depth)     /* ignore if within unknown tag */
        return;

    for( n=0; n<len; n++ ) {
        fputc( str[n], filter_outf );
    }
}


/**
  Tag start handling
 
  @param name name of the tag
  @param attrs array of tag attributes (NULL for none)
  @return output for the tag (NULL = none, give debug comments,
          "" = truly give none). '%s' in the output is to be
          replaced by the tag name.
*/
static
const char *loc_startelem( const xmlChar *name, const xmlChar **attrs )
{
    static char buf[HTML_TAG_BUFLEN];

    /* Anything under an unknown tag will be ignored: */
    if (unknown_depth) {
        unknown_depth++;
        return "";
    }

    /* Tags to be ignored: */
    if ( (strcmp(name,"comment")==0) ) {
      StartUnknown:
        unknown_depth++;    /* this will shut up about the contents */
        return "";     /* just ignore :) */
    }

    /*
    TENTATIVE
    This works only if .xml files have <seealso> </seealso> tags.
    Please note that The dot in </seealso>.</para> (if seealso is added
    should be removed too. (Refer to how </seealso> is handled)

    At the current stage, where <seealso is not used yet,
    this wouldn't harm anyway.
    */

    if (strcmp(name,"seealso")==0) {
        if (dialog_mode)
            goto StartUnknown;
        else /*normal*/
        {
            snprintf( buf, sizeof(buf), "%s%s%s",
              EMPHASIS_BOLD_START, _("help_ia_see_also"), EMPHASIS_BOLD_STOP);    
            return buf;
        }  
    }

    /* State independent tags (text style etc.): */
    if ( (strcmp(name,"b")==0) || 
         (strcmp(name,"i")==0) || 
         (strcmp(name,"sup")==0) ||
         (strcmp(name,"sub")==0) ) {
        return "<%s>";  /* pass-through to HTML */
    }

#ifdef USE_CSS /*  CSS2 implementation */

    if ( strcmp(name,"para")==0 )
        return TAG_BODY_TEXT_START;

    if ( strcmp(name,"heading")==0 )
        return TAG_SUBHEADING_START;

    if ( strcmp(name,"ldquote")==0 )    return "&8220;";
    if ( strcmp(name,"rdquote")==0 )    return "&8221;";
    if ( strcmp(name,"lquote")==0 )     return "&8216;";
    if ( strcmp(name,"rquote")==0 )     return "&8217;";
    if ( strcmp(name,"mdash")==0 )      return "&8212;";
    if ( strcmp(name,"ndash")==0 )      return "&8211;";
    if ( strcmp(name,"break")==0 )      return "&8232;";
    if ( strcmp(name,"nbhyphen")==0 )   return "&8208;";
    if ( strcmp(name,"nbspace")==0 )    return "&160;";
    if ( strcmp(name,"tab")==0 )        return "&160;&160;&160;";

    if ( strcmp(name,"display_text")==0 ) {
        return TAG_DISPLAY_TEXT_START;
    }

    if ( strcmp(name,"tip")==0 ) {
        sprintf(buf, TAG_TIP_START, helplib_ui_str(HELP_UI_TAG_TIP));
        return buf;
    }

    if ( strcmp(name,"note")==0 ) {
        sprintf(buf, TAG_NOTE_START, helplib_ui_str(HELP_UI_TAG_NOTE));
        return buf;
    }

    if ( strcmp(name,"example")==0 ) {
        sprintf(buf, TAG_EXAMPLE_START, helplib_ui_str(HELP_UI_TAG_EXAMPLE));
        return buf;
    }

    if ( strcmp(name,"warning")==0 ) {
        sprintf(buf, TAG_WARNING_START, helplib_ui_str(HELP_UI_TAG_WARNING));
        return buf;
    }

    if ( strcmp(name,"important")==0 ) {
        sprintf(buf, TAG_IMPORTANT_START, helplib_ui_str(HELP_UI_TAG_IMPORTANT));
        return buf;
    }

    if ( strcmp(name,"menu_seq")==0 )
        return TAG_MENU_SEQUENCE_START;

    if ( strcmp(name,"task_seq")==0 )
        return TAG_TASK_SEQUENCE_START;

    if ( strcmp(name,"step")==0 )
        return TAG_TASK_SEQUENCE_STEP_START;

    if ( strcmp(name,"main_method")==0 )
        return "";

    if ( strcmp(name,"emphasis")==0 ) {

        const char *role= loc_getattr( attrs, "role" );
        if (!role) role= "";

        /* Handle emphasis tag's role in CSS */
        sprintf(buf,TAG_EMPHASIS_START, role);
        return buf;
    }

    if ( strcmp(name,"topic")==0 )
        return "";

    if ( strcmp(name,"topictitle")==0 ) {
        if (dialog_mode)
            goto StartUnknown;  /* ignore title text completely */
            
        return TAG_TOPIC_TITLE_START;
    }

    if ( strcmp(name,"list")==0 ) {
        const char *s= loc_getattr( attrs, "style" );
        if (!s) s="";
        
        if ( strcmp(s,"bullet")!=0 )
            ULOG_WARN( "Unknown <list> style: '%s'", s );

        return TAG_BULLET_LIST_START;         
    }

    if ( strcmp(name,"listitem")==0 )
        return TAG_BULLET_LIST_ITEM_START;

#else /* Old implementation without CSS */

    if ( strcmp(name,"para")==0 )
        return PARA_START;  /*"<p>";*/

    if ( strcmp(name,"heading")==0 )
        return HEADING_START;   /*"<hX>";*/

    if ( strcmp(name,"ldquote")==0 )    return "&8220;";
    if ( strcmp(name,"rdquote")==0 )    return "&8221;";
    if ( strcmp(name,"lquote")==0 )     return "&8216;";
    if ( strcmp(name,"rquote")==0 )     return "&8217;";
    if ( strcmp(name,"mdash")==0 )      return "&8212;";
    if ( strcmp(name,"ndash")==0 )      return "&8211;";
    if ( strcmp(name,"break")==0 )      return "&8232;";
    if ( strcmp(name,"nbhyphen")==0 )   return "&8208;";
    if ( strcmp(name,"nbspace")==0 )    return "&160;";
    if ( strcmp(name,"tab")==0 )        return "&160;&160;&160;";

    if ( strcmp(name,"display_text")==0 ) {
        const char *logical_name= loc_getattr( attrs, "logical_name" );
        (void)logical_name;     /* what to do with it?? (nothing) */
        return DISPLAY_TEXT_START;
    }

    if ( strcmp(name,"tip")==0 ) {
        snprintf(buf, sizeof(buf), TIP_START, helplib_ui_str(HELP_UI_TAG_TIP));
        return buf;
    }

    if ( strcmp(name,"note")==0 ) {
        snprintf(buf, sizeof(buf), NOTE_START, helplib_ui_str(HELP_UI_TAG_NOTE));
        return buf;
    }

    if ( strcmp(name,"example")==0 ) {
        snprintf(buf, sizeof(buf), EXAMPLE_START, helplib_ui_str(HELP_UI_TAG_EXAMPLE));
        return buf;
    }

    if ( strcmp(name,"warning")==0 ) {
        snprintf(buf, sizeof(buf), WARNING_START, helplib_ui_str(HELP_UI_TAG_WARNING));
        return buf;
    }

    if ( strcmp(name,"important")==0 ) {
        snprintf(buf, sizeof(buf), IMPORTANT_START, helplib_ui_str(HELP_UI_TAG_IMPORTANT));
        return buf;
    }

    if ( strcmp(name,"menu_seq")==0 )
        return MENU_SEQ_START;

    if ( strcmp(name,"task_seq")==0 )
        return TASK_SEQ_START;

    if ( strcmp(name,"step")==0 )
        return TASK_SEQ_STEP_START;

    if ( strcmp(name,"main_method")==0 )
        return "";

    if ( strcmp(name,"emphasis")==0 ) {

        const char *role= loc_getattr( attrs, "role" );
        if (!role) role= "";
        
        if ( strcmp(role,"bold")==0 ) {
            emphasis_stop= EMPHASIS_BOLD_STOP;
            return EMPHASIS_BOLD_START;
        } else if ( strcmp(role,"italics")==0 ) {
            emphasis_stop= EMPHASIS_ITALIC_STOP;
            return EMPHASIS_ITALIC_START;
        } else {
            ULOG_WARN( "Unknown <emphasis> role: '%s'", role );
            return NULL;
        }
    }

#endif

    if ( strcmp(name,"graphic")==0 ) {
        gboolean exists= FALSE;
        
        /* Paths to try to find picture files (& suffix mask) */
        static const char* paths[]= { HELPLIB_PICTURE_PATHS, 
                                      NULL };
        static const char* picture_extensions[] = { HELPLIB_PICTURE_EXTENSIONS,
                                                    NULL };

        const char *fname= loc_getattr( attrs, "filename" );
        char buf2[FILENAME_MAX];

        if (!fname) {
            ULOG_WARN( "<graphic> without filename!" );
            return NULL;
        }       

        /* Special handling for "2102KEY_" filenames (ignore path)
        */
        {
        const char *p0, *p1;
        char buf3[FILENAME_MAX];
        
        p0= strstr( fname, "/2102KEY_" );
        if (p0) {
            p1= strchr( p0, '.' );
            if (!p1) p1= strchr( p0, '\0' );
            
            strcpy_safe_auto_max( buf3, p0+1, p1-p0-1 );
            ULOG_DEBUG("Stripped to: %s\n", buf3 );
            
            fname= buf3;
            exists= TRUE;
        }
        }
        
        /* Search path for pictures.
         *
         * Note: Application specific pictures may be given with an absolute
         *       path (if the material is provided by the application package).
         *
         *       Generic pictures should be given without path, nor suffix.
         */
        if (*fname!='/') {  /* no path -> activate search */
            int i;
            for( i=0; paths[i]; i++ ) {
                snprintf( buf2, sizeof(buf2), paths[i], fname );
                exists= fexists( buf2 );
                if (exists) {
                    fname= buf2; break;
                }
            }
        }

        else {
            /* absolute path without suffix
             * example : /usr/share/pixmaps/osso-chess/Undo_Redo
             * Action taken: Add a ".png" suffix
             */
    
            if (NULL == strstr(fname, ".") ){
                ULOG_DEBUG("absolute path: fname = %s", fname );
                int i;
                for( i=0; picture_extensions[i]; i++ ) {
                    snprintf( buf2, sizeof(buf2), picture_extensions[i],
                              fname );
                    exists= fexists( buf2 );
                    if (exists) {
                        fname= buf2; break;
                    }
                }
            }
        }

        if ((!exists) && (!fexists(fname))) {
            ULOG_WARN( "No picture file: %s", fname );
            }
            
        if (!graphic_tag( buf, sizeof(buf), fname)) {
            ULOG_DEBUG("Failed to create tag for: %s\n", fname );
            return NULL;    /* should not happen? */
        }
        
        return buf;     /* valid until next tag (enough, but nasty!) */
        }

#ifndef USE_CSS /* Old implementation without CSS */
    if ( strcmp(name,"list")==0 ) {
        const char *s= loc_getattr( attrs, "style" );
        if (!s) s="";
        
        if ( strcmp(s,"bullet")==0 )
            s= LIST_STYLE_BULLET;
        else
            ULOG_WARN( "Unknown <list> style: '%s'", s );

        snprintf( buf, sizeof(buf), LIST_START, s );
        return buf;         
    }

    if ( strcmp(name,"listitem")==0 )
        return LISTITEM_START;

    if ( strcmp(name,"topic")==0 )
        return "";  /*TOPIC_START;*/

    if ( strcmp(name,"topictitle")==0 ) {
        if (dialog_mode)
            goto StartUnknown;  /* ignore title text completely */
            
        return TOPICTITLE_START;
    }
#endif

    if ( strcmp(name,"ref")==0 ) {
        const char *refid= loc_getattr( attrs, "refid" );
        const char *refdoc= loc_getattr( attrs, "refdoc" );

        if (!refid) refid= "";  /* should always have one */
        if (!refdoc) refdoc= "";    /* can do without (picture only) */

        /* Note: Tried triplet links without the "//" but it didn't
                 work. So, do it like "help://applications_clock_support" */
        /*  Specs say dialogs shouldn't have links  
        Nothing is printed if so */
        if (!dialog_mode)
            snprintf( buf, sizeof(buf),
                       "<a href=\"help://%s\">"
                         "%s"
                       "</a>",
                       refid,
                       refdoc );

        return buf;         
    }

    /*Only display if dialog_mode == FALSE. Else print nothing*/

    if (( strcmp(name,"web_link")==0 ) && (!dialog_mode) ){
        const char *url= loc_getattr( attrs, "url" );
        if (url) {
            /* TBD: Move this to htmltags.cfg once stabilized? */
            snprintf( buf, sizeof(buf),
#ifndef USE_CSS
                           "<a href=\"%s\">"
                             "%s"
                           "</a>",
#else
                           TAG_WEB_LINK_START,
#endif
                           url,
                           "web link" );      /* TBD: where the real shown text? */
    
            return buf;         
        }
    }
  
    /*...*/

    /* Some tags knowingly ignored (metadata etc.) */
    if ( (strcmp(name,"context")==0) || 
         (strcmp(name,"synonyms")==0)) {
         unknown_depth++;
         return "";     /* ssh.. */
    } else {
        ULOG_DEBUG("Unknown tag '%s'", name );

        /* Unknown tags (anything within it will be ignored): */
        unknown_depth++;    /* 0->1 */
        return NULL;    /* debug output */
    }
}

/** 
  'startElement' XML callback
  
  @param data ignored
  @param name XML tag
  @param attrs array of tag attributes
*/
void filter_startElement_cb( void *data, const xmlChar *name, const xmlChar **attrs )
{
    (void)data;
    unsigned n=0;
	const char* str;

    if (!filter_outf || !name) return;

    /*---*/
    str= loc_startelem( name, attrs );

    if (!str) {    /* NULL = debug comment */
        fprintf( filter_outf, "<!-- %s", (const char*)name );
        if (attrs) {
            for ( n=0; attrs[n*2]; n+=2 ) {
                fprintf( filter_outf, " %s=\"%s\"",
                         (const char*)attrs[n*2], (const char*)attrs[n*2+1] );
            }
        }
        fprintf( filter_outf, " -->\n" );
        
    } else if (!str[0]) {
        /* "" = keep quiet */
    } else {
        fprintf( filter_outf, str, (const char*)name );
    }
}


/**
  Tag end handling
  
  @param name tag name
  @return output for the tag end (if contains '%s', that is
          to be replaced by tag name)
*/
static
const char *loc_endelem( const xmlChar *name )
{
    /* Unknown tags: */
    if (unknown_depth) {
        unknown_depth--;    /* state machine activates when we reach 0 */
        return "";
    }

    if (strcmp(name,"seealso")==0) {
        /*if (dialog_mode)*/
        return "";
    }
    /* State independent: */
    if ( (strcmp(name,"b")==0) || 
         (strcmp(name,"i")==0) || 
         (strcmp(name,"sup")==0) ||
         (strcmp(name,"sub")==0) ) {
        return "</%s>";     /* HTML pass-through */
    }

#ifdef USE_CSS /* CSS2 implementation */

    if ( strcmp(name,"para")==0 )
        return TAG_BODY_TEXT_STOP;

    if ( strcmp(name,"heading")==0 )
        return TAG_SUBHEADING_STOP;

    if ( (strcmp(name,"ldquote")==0) ||
         (strcmp(name,"rdquote")==0) ||
         (strcmp(name,"lquote")==0) ||
         (strcmp(name,"rquote")==0) ||
         (strcmp(name,"mdash")==0) ||
         (strcmp(name,"ndash")==0) ||
         (strcmp(name,"break")==0) ||
         (strcmp(name,"nbhyphen")==0) ||
         (strcmp(name,"nbspace")==0) ||
         (strcmp(name,"tab")==0) )
        return "";

    if ( strcmp(name,"display_text")==0 )
        return TAG_DISPLAY_TEXT_STOP;

    if ( strcmp(name,"tip")==0 )
        return TAG_TIP_STOP;

    if ( strcmp(name,"note")==0 )
        return TAG_NOTE_STOP;

    if ( strcmp(name,"example")==0 )
        return TAG_EXAMPLE_STOP;

    if ( strcmp(name,"warning")==0 )
        return TAG_WARNING_STOP;

    if ( strcmp(name,"important")==0 )
        return TAG_IMPORTANT_STOP;

    if ( strcmp(name,"menu_seq")==0 )
        return TAG_MENU_SEQUENCE_STOP;

    if ( strcmp(name,"task_seq")==0 )
        return TAG_TASK_SEQUENCE_STOP;

    if ( strcmp(name,"step")==0 )
        return TAG_TASK_SEQUENCE_STEP_STOP;

    if ( strcmp(name,"main_method")==0 )
        return "";

    if ( strcmp(name,"graphic")==0 )
        return "";

    if ( strcmp(name,"ref")==0 )
        return "";

    if ( strcmp(name,"list")==0 )
        return TAG_BULLET_LIST_STOP;

    if ( strcmp(name,"listitem")==0 )
        return TAG_BULLET_LIST_ITEM_STOP;

    if ( strcmp(name,"topic")==0 )
        return "";

    if ( strcmp(name,"topictitle")==0 )
        return TAG_TOPIC_TITLE_STOP;   /* only comes here in non-dialog mode */

    if ( strcmp(name,"emphasis")==0 )
        return TAG_EMPHASIS_STOP;

    /*Only prints to file if !dialog_mode*/
    if ( (strcmp(name,"web_link")==0 ) && (!dialog_mode) )
        return TAG_WEB_LINK_STOP;

#else /* Old implementation without CSS */

    if ( strcmp(name,"para")==0 )
        return PARA_STOP;   /*"</p>\n"; // looks smarter :) */

    if ( strcmp(name,"heading")==0 )
        return HEADING_STOP;    /*"</hX>";*/

    if ( (strcmp(name,"ldquote")==0) ||
         (strcmp(name,"rdquote")==0) ||
         (strcmp(name,"lquote")==0) ||
         (strcmp(name,"rquote")==0) ||
         (strcmp(name,"mdash")==0) ||
         (strcmp(name,"ndash")==0) ||
         (strcmp(name,"break")==0) ||
         (strcmp(name,"nbhyphen")==0) ||
         (strcmp(name,"nbspace")==0) ||
         (strcmp(name,"tab")==0) )
        return "";

    if ( strcmp(name,"display_text")==0 )
        return DISPLAY_TEXT_STOP;

    if ( strcmp(name,"tip")==0 )
        return TIP_STOP;

    if ( strcmp(name,"note")==0 )
        return NOTE_STOP;

    if ( strcmp(name,"example")==0 )
        return EXAMPLE_STOP;

    if ( strcmp(name,"warning")==0 )
        return WARNING_STOP;

    if ( strcmp(name,"important")==0 )
        return IMPORTANT_STOP;

    if ( strcmp(name,"emphasis")==0 )
        return emphasis_stop;   /* may vary, based on '<emphasis role>' */

    if ( strcmp(name,"menu_seq")==0 )
        return MENU_SEQ_STOP;

    if ( strcmp(name,"task_seq")==0 )
        return TASK_SEQ_STOP;

    if ( strcmp(name,"step")==0 )
        return TASK_SEQ_STEP_STOP;

    if ( strcmp(name,"main_method")==0 )
        return "";

    if ( strcmp(name,"graphic")==0 )
        return "";

    if ( strcmp(name,"ref")==0 )
        return "";

    if ( strcmp(name,"list")==0 )
        return LIST_STOP;

    if ( strcmp(name,"listitem")==0 )
        return LISTITEM_STOP;

    if ( strcmp(name,"topic")==0 )
        return "";  /*TOPIC_STOP;*/

    if ( strcmp(name,"topictitle")==0 )
        return TOPICTITLE_STOP;   /* only comes here in non-dialog mode */

#endif
    /*...*/

    return NULL;    /* ignore any other endings */
}

/**
  'endElement' XML callback (for filter mode)
  
  @param data ignored
  @param name tag name
*/
void filter_endElement_cb( void *data, const xmlChar *name )
{
    (void)data;
    const char *str;

    if (verbose) {
        ULOG_DEBUG("(endElement: %s)\n", name );
    }

    if (!name || !filter_outf) return;

	str= loc_endelem( name );

    if (!str) {
        fprintf( filter_outf, "<!-- /%s -->\n", name );
        
    } else if (!str[0]) {
        /* keep quiet */
    } else {
        fprintf( filter_outf, str, name );
    }
}

/**
  Initialize the HTML output table, either from external file
  (if exists) or from the built-in tables (same file, statically
  #included at compilation).
  
  @param i index to fill
  @param val string for it
  
  @note This code is only used if compiled-in tag values are
        not good. Mainly intended to help ad-hoc tests with
        different HTML output, not for production use.
        (in production use, the compiled-in tags should be
        'right')
*/
static
void loc_set_html_value( int i, const char* val )
{
    g_assert(val);

    /* malloc'ed copy of the string */
    char* str= g_strdup( val );
    
    /* Do C syntax conversions ('\n' etc.) */
    char* p= str;
    while ( (p=strstr(p,"\\n")) != NULL ) {
        /* replace by " \n" (the space won't show up) */
        *p++= ' ';
        *p++= '\n';
    }

    ULOG_DEBUG( "HTML config: %s=%s", html_keys[i].key, str );
    
    /* Note: we don't ever intend to 'g_free' these strings. This
     *       is intentional, since a config file will be read in
     *       only once, and be anyways valid until the program is
     *       terminated.
     */
    *(html_keys[i].ref)= str;
}

/**
  Initialize the filter system
  <p>
  Checks if there is a 'htmltags.cfg' override of the compiled-in
  tag strings.  At production run, this shouldn't be needed.
*/
void init_filter_html( void )
{
    static gboolean been_here= FALSE;
    int i=0;

    /* Use the filename, line by line.. */
    char fn_buf[FILENAME_MAX];
    FILE *f;
    char buf[MAX_CFG_LINELENGTH];
    int line= 0;

    if (been_here) return;      /* done it */
    been_here= TRUE;

    ULOG_INFO( "Initializing HTML tags." );
    
    /* Static init (defaults) */
    #include "htmltags.cfg"

    /* Do a sanity check to see every item has been given a default. */
    for (i=0; html_keys[i].key; i++) {
        if (!(*html_keys[i].ref)) {
            ULOG_WARN( "HTML config: no default for '%s'.", html_keys[i].key );
            /* ..but carry on */
        }
    }

    strcpy_safe_auto( fn_buf, ossohelp_getpath(NULL) );
    strcat_safe_auto( fn_buf, OSSO_HELP_CFG );  /* dynamic config */
        
    f= fopen( fn_buf, "r" );
    if (!f) {
        ULOG_INFO( "HTML config file: %s not found", fn_buf );
        return; /* no such file */
    }
        
    while ( fgets( buf, sizeof(buf), f ) != NULL ) {
        char *p1, *p2, *p3, *p4;
        
        line++;
        p1= buf;    /* skip white space */
        while( *p1 && isspace(*p1) ) p1++;
        
        if (!*p1) continue;         /* empty line */
        if (*p1=='/') continue;     /* comment */
        
        p2= strchr( p1, '=' );      /* 'tagname= "...";' */
        if (p2) {
            p3= strchr( p2+1, '"' );
            if (p3) {
                p4= strchr( p3+1, '\0' );   /* end mark */
                while( p4>p3 && *p4!='"' )  /* looking for last '"' */
                    p4--;
                
                if (p4>p3) {
                    /* p1: start of tag name
                     * p2: end of tag name (points to '=')
                     * p3: start of value (points to left '"')
                     * p4: end of value (points to right '"')
                     */
                    int i;
                    const char *tag= p1;
                    const char *val= p3+1;
                    *p2= '\0';
                    *p4= '\0';
                    
                    for ( i=0; html_keys[i].key; i++ ) {
                        if (strcmp(html_keys[i].key,tag)==0) {
                            loc_set_html_value( i, val );
                            break;  /* next line, please */
                        }
                    }
                    if (html_keys[i].key)
                        continue;   /* next line */
                }
            }
        }
        ULOG_ERR( "syntax error in %s line %d (ignoring):\n%s",
                  fn_buf, line, buf );
        continue;
    } /*while(fgets)*/
        
    ULOG_INFO( "HTML tags initialized" );
}
