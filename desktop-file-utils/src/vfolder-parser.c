/* Vfolder parsing */

/*
 * Copyright (C) 2002 Red Hat, Inc.
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#include <config.h>
#include "vfolder-parser.h"
#include <string.h>
#include <stdlib.h>

#include <libintl.h>
#define _(x) gettext ((x))
#define N_(x) x


struct _Vfolder
{
  char *name;
  char *desktop_file;
  
  GSList *subfolders;

  VfolderQuery *query;

  GSList *merge_dirs;
  GSList *desktop_dirs;
  GSList *excludes;
  GSList *includes;
  
  guint only_unallocated : 1;
  guint show_if_empty : 1;
};

struct _VfolderQuery
{
  VfolderQueryType type;
  gboolean negated;
};

typedef struct
{
  VfolderQuery query;
  VfolderQuery *child;
} VfolderRootQuery;

typedef struct
{
  VfolderQuery query;
  GSList *sub_queries;
} VfolderLogicalQuery;

typedef struct
{
  VfolderQuery query;
  char *category;
} VfolderCategoryQuery;

#define VFOLDER_ROOT_QUERY(q)     ((VfolderRootQuery*)q)
#define VFOLDER_LOGICAL_QUERY(q)  ((VfolderLogicalQuery*)q)
#define VFOLDER_CATEGORY_QUERY(q) ((VfolderCategoryQuery*)q)

typedef enum
{
  STATE_START,
  STATE_VFOLDER_INFO,
  STATE_MERGE_DIR,
  STATE_DESKTOP_DIR,
  STATE_FOLDER,
  STATE_FOLDER_NAME,
  STATE_FOLDER_DESKTOP_FILE,
  STATE_EXCLUDE,
  STATE_INCLUDE,
  STATE_QUERY,
  STATE_DONT_SHOW_IF_EMPTY,
  STATE_AND,
  STATE_OR,
  STATE_NOT,
  STATE_KEYWORD,
  STATE_ONLY_UNALLOCATED
} ParseState;

/* When adding fields add them to parse_info_init */
typedef struct
{
  GSList *states;

  GSList *folder_stack;
  GSList *query_stack;
  
  Vfolder *root_folder;

  GSList *merge_dirs;
  GSList *desktop_dirs;
} ParseInfo;

static Vfolder* vfolder_new  (void);

static VfolderQuery* vfolder_query_new  (VfolderQueryType  type);
static void          vfolder_query_free (VfolderQuery     *query);

static void set_error (GError             **err,
                       GMarkupParseContext *context,
                       int                  error_domain,
                       int                  error_code,
                       const char          *format,
                       ...) G_GNUC_PRINTF (5, 6);

static void          parse_info_init        (ParseInfo    *info);
static void          parse_info_free        (ParseInfo    *info);
static Vfolder*      parse_info_push_folder (ParseInfo    *info);
static Vfolder*      parse_info_peek_folder (ParseInfo    *info);
static void          parse_info_pop_folder  (ParseInfo    *info);
static VfolderQuery* parse_info_push_query  (ParseInfo    *info,
                                             VfolderQuery *query);
static VfolderQuery* parse_info_peek_query  (ParseInfo    *info);
static void          parse_info_pop_query   (ParseInfo    *info);

static void       push_state (ParseInfo  *info,
                              ParseState  state);
static void       pop_state  (ParseInfo  *info);
static ParseState peek_state (ParseInfo  *info);

static void parse_toplevel_element     (GMarkupParseContext  *context,
                                        const gchar          *element_name,
                                        const gchar         **attribute_names,
                                        const gchar         **attribute_values,
                                        ParseInfo            *info,
                                        GError              **error);
static void parse_folder_element       (GMarkupParseContext  *context,
                                        const gchar          *element_name,
                                        const gchar         **attribute_names,
                                        const gchar         **attribute_values,
                                        ParseInfo            *info,
                                        GError              **error);
static void parse_folder_child_element (GMarkupParseContext  *context,
                                        const gchar          *element_name,
                                        const gchar         **attribute_names,
                                        const gchar         **attribute_values,
                                        ParseInfo            *info,
                                        GError              **error);
static void parse_query_child_element  (GMarkupParseContext  *context,
                                        const gchar          *element_name,
                                        const gchar         **attribute_names,
                                        const gchar         **attribute_values,
                                        ParseInfo            *info,
                                        GError              **error);

static void start_element_handler (GMarkupParseContext  *context,
                                   const gchar          *element_name,
                                   const gchar         **attribute_names,
                                   const gchar         **attribute_values,
                                   gpointer              user_data,
                                   GError              **error);
static void end_element_handler   (GMarkupParseContext  *context,
                                   const gchar          *element_name,
                                   gpointer              user_data,
                                   GError              **error);
static void text_handler          (GMarkupParseContext  *context,
                                   const gchar          *text,
                                   gsize                 text_len,
                                   gpointer              user_data,
                                   GError              **error);

static GMarkupParser vfolder_parser = {
  start_element_handler,
  end_element_handler,
  text_handler,
  NULL,
  NULL
};

static void
set_error (GError             **err,
           GMarkupParseContext *context,
           int                  error_domain,
           int                  error_code,
           const char          *format,
           ...)
{
  int line, ch;
  va_list args;
  char *str;
  
  g_markup_parse_context_get_position (context, &line, &ch);

  va_start (args, format);
  str = g_strdup_vprintf (format, args);
  va_end (args);

  g_set_error (err, error_domain, error_code,
               _("Line %d character %d: %s"),
               line, ch, str);

  g_free (str);
}

static void
parse_info_init (ParseInfo *info)
{
  info->states = g_slist_prepend (NULL, GINT_TO_POINTER (STATE_START));
  info->folder_stack = NULL;
  info->root_folder = NULL;
  info->merge_dirs = NULL;
  info->desktop_dirs = NULL;
}

static void
parse_info_free (ParseInfo *info)
{
  if (info->root_folder)
    {
      g_assert (info->folder_stack == NULL);
      vfolder_free (info->root_folder);
    }      
  
  g_slist_free (info->states);

  g_slist_foreach (info->folder_stack, (GFunc) vfolder_free, NULL);
  g_slist_free (info->folder_stack);

  g_slist_foreach (info->merge_dirs, (GFunc) g_free, NULL);
  g_slist_free (info->merge_dirs);
  g_slist_foreach (info->desktop_dirs, (GFunc) g_free, NULL);
  g_slist_free (info->desktop_dirs);
}

static Vfolder*
parse_info_push_folder (ParseInfo *info)
{
  info->folder_stack = g_slist_prepend (info->folder_stack,
                                        vfolder_new ());

  return info->folder_stack->data;
}

static Vfolder*
parse_info_peek_folder (ParseInfo *info)
{
  return info->folder_stack ? info->folder_stack->data : NULL;
}

static void
parse_info_pop_folder (ParseInfo *info)
{
  info->folder_stack = g_slist_remove (info->folder_stack, info->folder_stack->data);
}

static VfolderQuery*
parse_info_push_query (ParseInfo    *info,
                       VfolderQuery *query)
{
  query->negated = (peek_state (info) == STATE_NOT);
  
  info->query_stack = g_slist_prepend (info->query_stack, query);

  return info->folder_stack->data;
}

static VfolderQuery*
parse_info_peek_query (ParseInfo *info)
{
  return info->query_stack ? info->query_stack->data : NULL;
}

static void
parse_info_pop_query (ParseInfo *info)
{
  info->query_stack = g_slist_remove (info->query_stack, info->query_stack->data);
}

static void
push_state (ParseInfo  *info,
            ParseState  state)
{
  info->states = g_slist_prepend (info->states, GINT_TO_POINTER (state));
}

static void
pop_state (ParseInfo *info)
{
  g_return_if_fail (info->states != NULL);
  
  info->states = g_slist_remove (info->states, info->states->data);
}

static ParseState
peek_state (ParseInfo *info)
{
  g_return_val_if_fail (info->states != NULL, STATE_START);

  return GPOINTER_TO_INT (info->states->data);
}

#define ELEMENT_IS(name) (strcmp (element_name, (name)) == 0)

static gboolean
check_no_attributes (GMarkupParseContext *context,
                     const char  *element_name,
                     const char **attribute_names,
                     const char **attribute_values,
                     GError     **error)
{
  if (attribute_names[0] != NULL)
    {
      set_error (error, context,
                 G_MARKUP_ERROR,
                 G_MARKUP_ERROR_PARSE,
                 _("Attribute \"%s\" is invalid on <%s> element in this context"),
                 attribute_names[0], element_name);
      return FALSE;
    }

  return TRUE;
}

static void
parse_toplevel_element (GMarkupParseContext  *context,
                        const gchar          *element_name,
                        const gchar         **attribute_names,
                        const gchar         **attribute_values,
                        ParseInfo            *info,
                        GError              **error)
{
  g_return_if_fail (peek_state (info) == STATE_VFOLDER_INFO);

  if (ELEMENT_IS ("MergeDir"))
    {
      if (!check_no_attributes (context, element_name,
                                attribute_names, attribute_values,
                                error))
        return;

      push_state (info, STATE_MERGE_DIR);
    }
  else if (ELEMENT_IS ("DesktopDir"))
    {
      if (!check_no_attributes (context, element_name,
                                attribute_names, attribute_values,
                                error))
        return;
      
      push_state (info, STATE_DESKTOP_DIR);
    }
  else if (ELEMENT_IS ("Folder"))
    {
      parse_folder_element (context, element_name,
                            attribute_names, attribute_values,
                            info, error);
    }
  else
    {
      set_error (error, context,
                 G_MARKUP_ERROR, G_MARKUP_ERROR_PARSE,
                 _("Element <%s> is not allowed below <%s>"),
                 element_name, "VFolderInfo");
    }
}

static void
parse_folder_element (GMarkupParseContext  *context,
                      const gchar          *element_name,
                      const gchar         **attribute_names,
                      const gchar         **attribute_values,
                      ParseInfo            *info,
                      GError              **error)
{
  Vfolder *folder;
  
  if (!check_no_attributes (context, element_name,
                            attribute_names, attribute_values,
                            error))
    return;
      
  push_state (info, STATE_FOLDER);

  folder = parse_info_push_folder (info);
}

static void
parse_folder_child_element (GMarkupParseContext  *context,
                            const gchar          *element_name,
                            const gchar         **attribute_names,
                            const gchar         **attribute_values,
                            ParseInfo            *info,
                            GError              **error)
{
  if (ELEMENT_IS ("Folder"))
    {
      parse_folder_element (context, element_name,
                            attribute_names, attribute_values,
                            info, error);

      return;
    }
  
  if (!check_no_attributes (context, element_name,
                            attribute_names, attribute_values,
                            error))
    return;
  
  if (ELEMENT_IS ("Name"))
    {
      push_state (info, STATE_FOLDER_NAME);
    }
  else if (ELEMENT_IS ("Desktop"))
    {
      push_state (info, STATE_FOLDER_DESKTOP_FILE);
    }
  else if (ELEMENT_IS ("Exclude"))
    {      
      push_state (info, STATE_EXCLUDE);
    }
  else if (ELEMENT_IS ("Query"))
    {
      VfolderQuery *query;

      query = vfolder_query_new (VFOLDER_QUERY_ROOT);
      parse_info_push_query (info, query);

      push_state (info, STATE_QUERY);
    }
  else if (ELEMENT_IS ("DontShowIfEmpty"))
    {
      push_state (info, STATE_DONT_SHOW_IF_EMPTY);
    }
  else if (ELEMENT_IS ("Include"))
    {
      push_state (info, STATE_INCLUDE);
    }
  else if (ELEMENT_IS ("OnlyUnallocated"))
    {
      Vfolder *folder;

      folder = parse_info_peek_folder (info);

      folder->only_unallocated = TRUE;
      
      push_state (info, STATE_ONLY_UNALLOCATED);
    }
  else
    {
      set_error (error, context,
                 G_MARKUP_ERROR, G_MARKUP_ERROR_PARSE,
                 _("Element <%s> is not allowed below <%s>"),
                 element_name, "Folder");
    }
}

static void
parse_query_child_element  (GMarkupParseContext  *context,
                            const gchar          *element_name,
                            const gchar         **attribute_names,
                            const gchar         **attribute_values,
                            ParseInfo            *info,
                            GError              **error)
{
  if (!check_no_attributes (context, element_name,
                            attribute_names, attribute_values,
                            error))
    return;

  if (ELEMENT_IS ("Keyword"))
    {
      VfolderQuery *query;

      query = vfolder_query_new (VFOLDER_QUERY_CATEGORY);
      parse_info_push_query (info, query);

      push_state (info, STATE_KEYWORD);
    }
  else if (ELEMENT_IS ("And"))
    {
      VfolderQuery *query;

      query = vfolder_query_new (VFOLDER_QUERY_AND);
      parse_info_push_query (info, query);
      
      push_state (info, STATE_AND);
    }
  else if (ELEMENT_IS ("Or"))
    {
      VfolderQuery *query;

      query = vfolder_query_new (VFOLDER_QUERY_OR);
      parse_info_push_query (info, query);
      
      push_state (info, STATE_OR);
    }
  else if (ELEMENT_IS ("Not"))
    {
      push_state (info, STATE_NOT);
    }
  else
    {
      set_error (error, context,
                 G_MARKUP_ERROR, G_MARKUP_ERROR_PARSE,
                 _("Element <%s> is not allowed below <%s>"),
                 element_name, "Folder");
    }
}

static void
start_element_handler (GMarkupParseContext *context,
                       const gchar         *element_name,
                       const gchar        **attribute_names,
                       const gchar        **attribute_values,
                       gpointer             user_data,
                       GError             **error)
{
  ParseInfo *info = user_data;

  switch (peek_state (info))
    {
    case STATE_START:
      if (ELEMENT_IS ("VFolderInfo"))
        {
          if (!check_no_attributes (context, element_name,
                                    attribute_names, attribute_values,
                                    error))
            return;
          
          push_state (info, STATE_VFOLDER_INFO);
        }
      else
        set_error (error, context, G_MARKUP_ERROR, G_MARKUP_ERROR_PARSE,
                   _("Outermost element in menu file must be <VFolderInfo> not <%s>"),
                   element_name);
      break;

    case STATE_VFOLDER_INFO:
      parse_toplevel_element (context, element_name,
                              attribute_names, attribute_values,
                              info, error);
      break;
      
    case STATE_MERGE_DIR:
      set_error (error, context, G_MARKUP_ERROR, G_MARKUP_ERROR_PARSE,
                 _("Element <%s> is not allowed inside a <%s> element"),
                 element_name, "MergeDir");
      break;

    case STATE_DESKTOP_DIR:
      set_error (error, context, G_MARKUP_ERROR, G_MARKUP_ERROR_PARSE,
                 _("Element <%s> is not allowed inside a <%s> element"),
                 element_name, "DesktopDir");
      break;

    case STATE_FOLDER:
      parse_folder_child_element (context, element_name,
                                  attribute_names, attribute_values,
                                  info, error);
      break;

    case STATE_FOLDER_NAME:
      set_error (error, context, G_MARKUP_ERROR, G_MARKUP_ERROR_PARSE,
                 _("Element <%s> is not allowed inside a <%s> element"),
                 element_name, "Name");
      break;

    case STATE_FOLDER_DESKTOP_FILE:
      set_error (error, context, G_MARKUP_ERROR, G_MARKUP_ERROR_PARSE,
                 _("Element <%s> is not allowed inside a <%s> element"),
                 element_name, "Desktop");
      break;      

    case STATE_EXCLUDE:
      set_error (error, context, G_MARKUP_ERROR, G_MARKUP_ERROR_PARSE,
                 _("Element <%s> is not allowed inside a <%s> element"),
                 element_name, "Exclude");
      break;

    case STATE_INCLUDE:
      set_error (error, context, G_MARKUP_ERROR, G_MARKUP_ERROR_PARSE,
                 _("Element <%s> is not allowed inside a <%s> element"),
                 element_name, "Include");
      break;

    case STATE_QUERY:
    case STATE_AND:
    case STATE_OR:
    case STATE_NOT:
      parse_query_child_element (context, element_name,
                                 attribute_names, attribute_values,
                                 info, error);      
      break;

    case STATE_DONT_SHOW_IF_EMPTY:
      set_error (error, context, G_MARKUP_ERROR, G_MARKUP_ERROR_PARSE,
                 _("Element <%s> is not allowed inside a <%s> element"),
                 element_name, "DontShowIfEmpty");
      break;

    case STATE_KEYWORD:
      set_error (error, context, G_MARKUP_ERROR, G_MARKUP_ERROR_PARSE,
                 _("Element <%s> is not allowed inside a <%s> element"),
                 element_name, "Keyword");
      break;

    case STATE_ONLY_UNALLOCATED:
      set_error (error, context, G_MARKUP_ERROR, G_MARKUP_ERROR_PARSE,
                 _("Element <%s> is not allowed inside a <%s> element"),
                 element_name, "OnlyUnallocated");
      break;

    }
}

static void
end_element_handler (GMarkupParseContext *context,
                     const gchar         *element_name,
                     gpointer             user_data,
                     GError             **error)
{
  ParseInfo *info = user_data;

  switch (peek_state (info))
    {
    case STATE_START:
      break;
    case STATE_VFOLDER_INFO:      
      pop_state (info);
      g_assert (peek_state (info) == STATE_START);
      break;
    case STATE_FOLDER:
      {
        Vfolder *folder;

        folder = parse_info_peek_folder (info);

        if (folder->desktop_file == NULL)
          {
            set_error (error, context, G_MARKUP_ERROR, G_MARKUP_ERROR_PARSE,
                       _("<Folder> does not specify a desktop file with <Desktop>"));
            return;
          }

        if (folder->name == NULL)
          {
            set_error (error, context, G_MARKUP_ERROR, G_MARKUP_ERROR_PARSE,
                       _("<Folder> does not specify a name with <Name>"));
            return;
          }
        
        parse_info_pop_folder (info);        
        pop_state (info);

        if (info->folder_stack == NULL)
          {
            if (info->root_folder)
              {
                vfolder_free (folder);
                set_error (error, context, G_MARKUP_ERROR, G_MARKUP_ERROR_PARSE,
                           _("Two root-level <Folder> elements given"));
                return;
              }
            else
              {
                info->root_folder = folder;
                info->root_folder->merge_dirs = info->merge_dirs;
                info->root_folder->desktop_dirs = info->desktop_dirs;
                info->merge_dirs = NULL;
                info->desktop_dirs = NULL;
              }
          }
        else
          {
            /* Add as a subfolder to parent */
            Vfolder *parent;

            parent = parse_info_peek_folder (info);

            parent->subfolders = g_slist_append (parent->subfolders,
                                                 folder);
          }
      }
      break;
    case STATE_NOT:
      pop_state (info);
      break;
    case STATE_QUERY:
      {
        VfolderQuery *query;
        Vfolder *folder;
        
        query = parse_info_peek_query (info);
        parse_info_pop_query (info);
        pop_state (info);
        
        folder = parse_info_peek_folder (info);

        if (folder->query != NULL)
          {
            vfolder_query_free (query);
            set_error (error, context, G_MARKUP_ERROR, G_MARKUP_ERROR_PARSE,
                       _("Two <Query> elements given for a single <Folder>"));
            return;
          }

        folder->query = query;
      }
      break;

    case STATE_AND:
    case STATE_OR:
    case STATE_KEYWORD:
      {
        VfolderQuery *query;
        VfolderQuery *parent;
        
        query = parse_info_peek_query (info);
        parse_info_pop_query (info);
        pop_state (info);
        
        parent = parse_info_peek_query (info);

        if (parent->type == VFOLDER_QUERY_ROOT)
          {
            if (VFOLDER_ROOT_QUERY (parent)->child != NULL)
              {
                vfolder_query_free (query);
                set_error (error, context, G_MARKUP_ERROR, G_MARKUP_ERROR_PARSE,
                           _("Two child elements given for a single <Query>"));
                return;
              }
            else
              {
                VFOLDER_ROOT_QUERY (parent)->child = query;
              }
          }
        else
          {
            VFOLDER_LOGICAL_QUERY (parent)->sub_queries =
              g_slist_prepend (VFOLDER_LOGICAL_QUERY (parent)->sub_queries,
                               query);
          }
      }
      break;
      
    case STATE_MERGE_DIR:
    case STATE_DESKTOP_DIR:
    case STATE_FOLDER_NAME:
    case STATE_FOLDER_DESKTOP_FILE:
    case STATE_EXCLUDE:
    case STATE_INCLUDE:
    case STATE_DONT_SHOW_IF_EMPTY:
    case STATE_ONLY_UNALLOCATED:
      pop_state (info);
      break;
    }
}

#define NO_TEXT(element_name) set_error (error, context, G_MARKUP_ERROR, G_MARKUP_ERROR_PARSE, _("No text is allowed inside element <%s>"), element_name)

static gboolean
all_whitespace (const char *text,
                int         text_len)
{
  const char *p;
  const char *end;
  
  p = text;
  end = text + text_len;
  
  while (p != end)
    {
      if (!g_ascii_isspace (*p))
        return FALSE;

      p = g_utf8_next_char (p);
    }

  return TRUE;
}

static void
text_handler (GMarkupParseContext *context,
              const gchar         *text,
              gsize                text_len,
              gpointer             user_data,
              GError             **error)
{
  ParseInfo *info = user_data;
  Vfolder *folder;
  
  if (all_whitespace (text, text_len))
    return;
  
  /* FIXME http://bugzilla.gnome.org/show_bug.cgi?id=70448 would
   * allow a nice cleanup here.
   */

  switch (peek_state (info))
    {
    case STATE_START:
      g_assert_not_reached (); /* gmarkup shouldn't do this */
      break;
    case STATE_VFOLDER_INFO:
      NO_TEXT ("VFolderInfo");
      break;
    case STATE_DESKTOP_DIR:
      info->desktop_dirs =
        g_slist_append (info->desktop_dirs,
                        g_strndup (text, text_len));
      break;
    case STATE_MERGE_DIR:
      info->merge_dirs =
        g_slist_append (info->merge_dirs,
                        g_strndup (text, text_len));
      break;
    case STATE_FOLDER:
      NO_TEXT ("Folder");
      break;
    case STATE_FOLDER_NAME:
      folder = parse_info_peek_folder (info);
      g_assert (folder != NULL);
      if (folder->name)
        set_error (error, context, G_MARKUP_ERROR, G_MARKUP_ERROR_PARSE, _("Two names given for folder"));
      else
        folder->name = g_strndup (text, text_len);
      break;
    case STATE_FOLDER_DESKTOP_FILE:
      folder = parse_info_peek_folder (info);
      g_assert (folder != NULL);
      if (folder->desktop_file)
        set_error (error, context, G_MARKUP_ERROR, G_MARKUP_ERROR_PARSE, _("Two desktop files given for folder"));
      else
        folder->desktop_file = g_strndup (text, text_len);
      break;
    case STATE_EXCLUDE:
      folder = parse_info_peek_folder (info);
      g_assert (folder != NULL);
      folder->excludes =
        g_slist_append (folder->excludes,
                        g_strndup (text, text_len));
      break;
    case STATE_INCLUDE:
      folder = parse_info_peek_folder (info);
      g_assert (folder != NULL);
      folder->includes =
        g_slist_append (folder->includes,
                        g_strndup (text, text_len));
      break;
    case STATE_DONT_SHOW_IF_EMPTY:
      folder = parse_info_peek_folder (info);
      g_assert (folder != NULL);
      folder->show_if_empty = FALSE;
      break;
    case STATE_QUERY:
      NO_TEXT ("Query");
      break;
    case STATE_AND:
      NO_TEXT ("And");
      break;
    case STATE_OR:
      NO_TEXT ("Or");
      break;
    case STATE_NOT:
      NO_TEXT ("Not");
      break;
    case STATE_ONLY_UNALLOCATED:
      NO_TEXT ("OnlyUnallocated");
      break;
    case STATE_KEYWORD:
      {
        VfolderQuery *query;

        query = parse_info_peek_query (info);

        g_assert (query->type == VFOLDER_QUERY_CATEGORY);
        g_assert (VFOLDER_CATEGORY_QUERY (query)->category == NULL);
        
        VFOLDER_CATEGORY_QUERY (query)->category = g_strndup (text, text_len);
      }
      break;
    }
}

Vfolder*
vfolder_load (const char *filename,
              GError    **err)
{
  GMarkupParseContext *context;
  GError *error;
  ParseInfo info;
  char *text;
  gsize length;
  Vfolder *retval;

  text = NULL;
  length = 0;
  retval = NULL;
  context = NULL;
  
  if (!g_file_get_contents (filename,
                            &text,
                            &length,
                            err))
    return NULL;

  g_assert (text);

  parse_info_init (&info);
  
  context = g_markup_parse_context_new (&vfolder_parser,
                                        0, &info, NULL);

  error = NULL;
  if (!g_markup_parse_context_parse (context,
                                     text,
                                     length,
                                     &error))
    goto out;

  error = NULL;
  if (!g_markup_parse_context_end_parse (context, &error))
    goto out;

  goto out;

 out:

  if (context)
    g_markup_parse_context_free (context);
  g_free (text);
  
  if (error)
    {
      g_propagate_error (err, error);
    }
  else if (info.root_folder)
    {
      retval = info.root_folder;
      info.root_folder = NULL;
    }
  else
    {
      g_set_error (err, G_MARKUP_ERROR, G_MARKUP_ERROR_PARSE,
                   _("Menu file %s did not contain a root <VFolderInfo> element"),
                   filename);
    }

  parse_info_free (&info);
  
  return retval;
}

GSList*
vfolder_get_subfolders (Vfolder *folder)
{
  return folder->subfolders;
}

GSList*
vfolder_get_excludes (Vfolder *folder)
{
  return folder->excludes;
}

GSList*
vfolder_get_includes (Vfolder *folder)
{
  return folder->includes;
}

GSList*
vfolder_get_merge_dirs (Vfolder *folder)
{
  return folder->merge_dirs;
}

GSList*
vfolder_get_desktop_dirs (Vfolder *folder)
{
  return folder->desktop_dirs;
}

const char*
vfolder_get_name (Vfolder *folder)
{
  return folder->name;
}

const char*
vfolder_get_desktop_file (Vfolder *folder)
{
  return folder->desktop_file;
}

gboolean
vfolder_get_show_if_empty (Vfolder *folder)
{
  return folder->show_if_empty;
}

gboolean
vfolder_get_only_unallocated (Vfolder *folder)
{
  return folder->only_unallocated;
}

VfolderQuery*
vfolder_get_query (Vfolder *folder)
{
  if (folder->query)
    {
      g_assert (folder->query->type == VFOLDER_QUERY_ROOT);
      return VFOLDER_ROOT_QUERY (folder->query)->child;
    }
  else
    return NULL;
}

VfolderQueryType
vfolder_query_get_type (VfolderQuery *query)
{
  return query->type;
}

GSList*
vfolder_query_get_subqueries (VfolderQuery *query)
{
  g_return_val_if_fail (query->type == VFOLDER_QUERY_OR ||
                        query->type == VFOLDER_QUERY_AND,
                        NULL);

  return VFOLDER_LOGICAL_QUERY (query)->sub_queries;
}

const char*
vfolder_query_get_category (VfolderQuery *query)
{
  g_return_val_if_fail (query->type == VFOLDER_QUERY_CATEGORY, NULL);
  
  return VFOLDER_CATEGORY_QUERY (query)->category;
}

gboolean
vfolder_query_get_negated (VfolderQuery *query)
{
  return query->negated;
}

static Vfolder*
vfolder_new  (void)
{
  Vfolder *folder;

  folder = g_new0 (Vfolder, 1);

  folder->show_if_empty = TRUE;
  
  return folder;
}

void
vfolder_free (Vfolder *folder)
{
  g_return_if_fail (folder != NULL);

  g_free (folder->name);
  g_free (folder->desktop_file);

  g_slist_foreach (folder->subfolders,
                   (GFunc) vfolder_free, NULL);

  /* FIXME free the query */  

  g_slist_foreach (folder->merge_dirs,
                   (GFunc) g_free, NULL);

  g_slist_foreach (folder->desktop_dirs,
                   (GFunc) g_free, NULL);

  g_slist_foreach (folder->excludes,
                   (GFunc) g_free, NULL);

  g_slist_foreach (folder->includes,
                   (GFunc) g_free, NULL);
  
  g_free (folder);
}

static VfolderQuery*
vfolder_query_new (VfolderQueryType type)
{
  VfolderQuery *query;

  query = NULL;
  switch (type)
    {
    case VFOLDER_QUERY_ROOT:
      query = (VfolderQuery*) g_new0 (VfolderRootQuery, 1);
      break;

    case VFOLDER_QUERY_OR:
    case VFOLDER_QUERY_AND:
      query = (VfolderQuery*) g_new0 (VfolderLogicalQuery, 1);
      break;

    case VFOLDER_QUERY_CATEGORY:
      query = (VfolderQuery*) g_new0 (VfolderCategoryQuery, 1);
      break;
    }

  query->type = type;

  return query;
}

static void
vfolder_query_free (VfolderQuery *query)
{
  switch (query->type)
    {
    case VFOLDER_QUERY_ROOT:
      if (VFOLDER_ROOT_QUERY (query)->child)
        vfolder_query_free (VFOLDER_ROOT_QUERY (query)->child);
      break;
      
    case VFOLDER_QUERY_OR:
    case VFOLDER_QUERY_AND:
      g_slist_foreach (VFOLDER_LOGICAL_QUERY (query)->sub_queries,
                       (GFunc) vfolder_query_free,
                       NULL);
      break;

    case VFOLDER_QUERY_CATEGORY:
      g_free (VFOLDER_CATEGORY_QUERY (query)->category);
      break;
    }

  g_free (query);
}
