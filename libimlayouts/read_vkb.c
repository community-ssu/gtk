/*
 * This file is part of libimlayouts
 *
 * Copyright (C) 2005 Nokia Corporation.
 *
 * Contact: Mohammad Anwari <mdamt@maemo.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
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

#include <glib.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <expat.h>

#include "imlayout_vkb.h"

#ifdef TEST
#define g_warning g_print
const gchar *langs[] = { "de_DE", "fi_FI", "en_US", "en_GB", "fr_FR", "fr_CA", "it_IT", "ru_RU", "es_ES", "es_MX", "pt_BR", "latin.special"};
#define MAX_LANG 12
#endif

#define BUFFER_SIZE 512
typedef struct
{
  gboolean eof;
  ssize_t index;
  ssize_t size;
  int fd;
  char buffer [BUFFER_SIZE];
} binary_reader;

typedef struct _xml_data
{
  gchar *name, *lang, *wc;
} xml_data;

#define XML_BUFFER_SIZE 100

binary_reader *br_open (const gchar *filename)
{
  binary_reader *b = NULL;

  b = malloc (sizeof (binary_reader));
  if (b) {
    b->eof = FALSE;
    b->index = -1;
    b->fd = open (filename, O_RDONLY);
  }
  return b;
}

ssize_t br_read (binary_reader *b, void *buf, size_t count)
{
  ssize_t copy_size = -1;
  if (b && b->fd >= 0) {
    if (b->index >= BUFFER_SIZE || b->index < 0) {
      int r = read (b->fd, (void *) b->buffer, BUFFER_SIZE);
      if (r < 0)
        b->eof = TRUE;
      b->size = r;
      b->index = 0;
    }

    if (count > (b->size - b->index)) {
      int copy_size_now = b->size - b->index;
      copy_size = copy_size_now;
  
      memcpy (buf, (void*) (b->buffer + b->index), copy_size_now);
      b->index = -1;
      copy_size += br_read (b, buf + copy_size_now, count - copy_size_now);
    } else {
      copy_size = count;

      memcpy (buf, (void*) (b->buffer + b->index), copy_size);
      b->index += copy_size;
    }
  } 
  return copy_size;
}

void br_close (binary_reader *b)
{
  if (b) {
    if (b->fd >= 0)
      close (b->fd);
    free (b);
  }
  b = NULL;
}

void free_layout (vkb_layout *layout, gint num_layout)
{
  vkb_layout *l = layout;
  int i = 0;

  if (num_layout <= 0)
    return;

  if (layout == NULL)
    return;

  while (i < num_layout) {
    if (l) {
      if (l->labels != NULL) {
        int k;
        IM_LABEL *lbl = l->labels;
        for (k = 0; k < layout->TOTAL_chars; k ++, lbl ++) {
          if (lbl->text)
            g_free (lbl->text);
          lbl->text = NULL;
        }
        g_free (l->labels);
        l->labels = NULL;
      }

      if (l->keys != NULL) {
        if (l->keys->special.scancodes != NULL) {
          g_free (l->keys->special.scancodes);
          l->keys->special.scancodes = NULL;
        }
        g_free (l->keys);
        l->keys = NULL;
      }

      if (l->num_keys_in_rows != NULL) {
        g_free (l->num_keys_in_rows);
        l->num_keys_in_rows = NULL;
      }

      if (l->tabs != NULL) {
        vkb_scv_tab *t;
        
        t = l->tabs;
        for (i = 0; i < l->num_tabs; i ++, t ++) {
          if (t->label != NULL) {
            g_free (t->label);
            t->label = NULL;
          }

          if (t->labels != NULL) {
            int k;
            IM_LABEL *lbl = t->labels;
            for (k = 0; k < t->num_keys; k ++, lbl ++) {
              if (lbl->text)
                g_free (lbl->text);
              lbl->text = NULL;
            }
            g_free (t->labels);
            t->labels = NULL;
          }
          
          if (t->keys != NULL) {
            g_free (t->keys);
            t->keys = NULL;
          }
        }
        g_free (l->tabs);
        l->tabs = NULL;
      }
    }
    l ++;
    i ++;
  }
  g_free (layout);
  layout = NULL;
}

vkb_layout_collection *read_header (binary_reader *fd)
{
  gboolean success = FALSE;
  vkb_layout_collection *retval = NULL;
  gint8 version, byte, num_layout;
  vkb_layout *layout = NULL;
  gchar *name = NULL, *lang = NULL, *wc = NULL;

  if (!fd || fd->fd < 0)
    return NULL;

  retval = (vkb_layout_collection *) g_malloc0 (
    sizeof (vkb_layout_collection));
  if (retval == NULL) {
    g_warning ("insufficient memory to allocate vkb_layout_collection");
    return NULL;
  }

  /*reading version*/
  if (br_read (fd, &version, sizeof (gint8)) == sizeof (gint8)) {
    if (version != VKB_VERSION) {
      g_warning ("invalid version");
      goto end_read_header;
    }
  } else {
    g_warning ("premature_read when reading version");
    goto end_read_header;
  }

  /* total layout */
  if (br_read (fd, &num_layout, sizeof (gint8)) == sizeof (gint8)) {
    if (num_layout <= 0) {
      g_warning ("number of layout <= 0");
      goto end_read_header;
    }
  } else {
    g_warning ("premature_read reading number of layout");
    goto end_read_header;
  }

  layout = g_malloc0 (num_layout * sizeof (vkb_layout));
  retval->collection = layout;
  if (!layout) {
    g_warning ("insufficient memory");
    goto end_read_header;
  }
  
  retval->num_layouts = num_layout;

  /* numeric layout */
  if (br_read (fd, &retval->numeric_layout, sizeof (gint8)) == sizeof (gint8)) {
  } else {
    g_warning ("premature_read reading numeric layout");
    goto end_read_header;
  }

  /* length of name */
  if (br_read (fd, &byte, sizeof (gint8)) == sizeof (gint8)) {
  } else {
    g_warning ("premature_read reading numeric layout");
    goto end_read_header;
  }
  name = (gchar *) g_malloc0 (byte + 1);
  if (!name) {
    g_warning ("insufficient memory allocating name");
    goto end_read_header;
  }

  retval->name = name;

  /* name */
  if (br_read (fd, name, sizeof (gint8) * byte) == sizeof (gint8) * byte) {
  } else {
    g_warning ("premature_read reading name");
    goto end_read_header;
  }

  /* length of lang */
  if (br_read (fd, &byte, sizeof (gint8)) == sizeof (gint8)) {
  } else {
    g_warning ("premature_read reading length of lang");
    goto end_read_header;
  }
  
  lang = (gchar *) g_malloc0 (byte + 1);
  if (!lang) {
    g_warning ("insufficient memory allocating language");
    goto end_read_header;
  }

  retval->language = lang;

  /* lang */
  if (br_read (fd, lang, sizeof (gint8) * byte) == sizeof (gint8) * byte) {
  } else {
    g_warning ("premature_read reading language");
    goto end_read_header;
  }

  /* length of wc */
  if (br_read (fd, &byte, sizeof (gint8)) != sizeof (gint8)) {
    g_warning ("premature_read reading length of wc");
    goto end_read_header;
  }
  
  if (byte > 0) {
    wc = (gchar *) g_malloc0 (byte + 1);
    retval->word_completion = wc;
    if (!wc) {
      g_warning ("insufficient memory allocating wc");
      goto end_read_header;
    }
  }

  /* wc */
  if (br_read (fd, wc, sizeof (gint8) * byte) != sizeof (gint8) * byte) {
    goto end_read_header;
  }

  success = TRUE;
end_read_header:

  if (success == FALSE) {
    imlayout_vkb_free_layout_collection (retval);
    retval = NULL;
  }

  return retval;
}

vkb_layout *read_keyboard_layout (binary_reader *fd, vkb_layout *p)
{
  gboolean success = FALSE;
  gint8  byte, *r;
  int j;
  IM_LABEL *l;
  IM_BUTTON *b;

  if (!fd || fd->fd < 0)
    return NULL;

  if (p == NULL)
    return NULL;

  p->num_tabs = 0;
  p->tabs = NULL;

  /* total keys */
  if (br_read (fd, &byte, sizeof (gint8)) == sizeof (gint8)) {
    if (byte <= 0)
      goto end_rkl;
  } else {
    g_warning ("premature_read reading total keys");
      goto end_rkl;
  }

  p->TOTAL_chars = byte;

  /* number of rows */
  if (br_read (fd, &byte, sizeof (gint8)) == sizeof (gint8)) {
    if (byte <= 0)
      goto end_rkl;
  } else {
    g_warning ("premature_read reading number of rows");
    goto end_rkl;
  }

  p->num_rows = byte;
  p->num_keys_in_rows = (gint8*) g_malloc0 (byte * sizeof (gint8));
  r = p->num_keys_in_rows;

  for (j = 0; j < p->num_rows; j ++, r++) {
    if (br_read (fd, &byte, sizeof (gint8)) == sizeof (gint8)) {
      *r = byte;
    } else {
      g_warning ("premature_read reading number of keys in rows");
      goto end_rkl;
    }
  }

  p->keys = (IM_BUTTON *) g_malloc0 (sizeof (IM_BUTTON) *  p->TOTAL_chars);
  if (p->keys == NULL) {
    g_warning ("insufficient memory allocating keys");
    goto end_rkl;
  }
  p->labels = (IM_LABEL *) g_malloc0 (sizeof (IM_LABEL) *  p->TOTAL_chars);
  if (p->labels == NULL) {
    g_warning ("insufficient memory allocating labels");
    goto end_rkl;
  }

  b = p->keys;
  l = p->labels;
  for (j = 0; j < p->TOTAL_chars; j ++, l ++, b ++) {
    gchar *text;

    /* type */
    if (br_read (fd, &byte, sizeof (gint8)) == sizeof (gint8)) {
      b->type = byte;
    } else {
      g_warning ("premature_read reading key type");
      goto end_rkl;
    }

    /* length */
    if (br_read (fd, &byte, sizeof (gint8)) == sizeof (gint8)) {
      l->length = byte;
    } else {
      g_warning ("premature_read reading label length");
      goto end_rkl;
    }

    if (byte == 0) {
      l->text = NULL;
      continue;
    }

    /* the label */
    text = (gchar*) g_malloc0 (byte + 1);
    l->text = text;
    if (text == NULL) {
      g_warning ("insufficient memory allocation label");
      goto end_rkl;
    }

    if (br_read (fd, text, sizeof (gchar) * byte) != sizeof (gchar) * byte) {
      g_warning ("premature_read reading label");
      goto end_rkl;
    }

    if (b->type & KEY_TYPE_RAW) {
      gint8 *scancodes;     
      /* size */
      if (br_read (fd, &byte, sizeof (gint8)) == sizeof (gint8)) {
        b->special.length = byte;
      } else {
        g_warning ("premature_read reading special key size");
        goto end_rkl;
      }

      /* the scancodes */
      scancodes = (gint8*) g_malloc0 (byte);
      b->special.scancodes = scancodes;
      if (scancodes == NULL) {
        g_warning ("insufficient memory allocating scancodes");
        goto end_rkl;
      }

      if (br_read (fd, scancodes, sizeof (gint8) * byte) != sizeof (gint8) * byte) {
        g_warning ("premature_read reading scancodes");
        goto end_rkl;
      }
    } else {
      b->special.scancodes = NULL;
      b->special.length = 0;
    }
  }
  success = TRUE;
end_rkl:
  if (success == FALSE) {
    free_layout (p, 1);
    p = NULL;
  }
  return p;
}

vkb_layout *read_specialchar_layout (binary_reader *fd, vkb_layout *p)
{
  gboolean success = FALSE;
  gint8  byte;
  int i, j;
  IM_LABEL *l;
  IM_BUTTON *b;
  vkb_scv_tab *t;

  if (!fd || fd->fd < 0)
    return NULL;

  if (p == NULL)
    return NULL;

  p->TOTAL_chars = p->num_rows = 0;
  p->labels = NULL;
  p->keys = NULL;
  p->num_keys_in_rows = NULL;

  /* total tabs */
  if (br_read (fd, &byte, sizeof (gint8)) == sizeof (gint8)) {
    if (byte <= 0)
      goto end_rsl;
  } else {
    g_warning ("premature_read reading number of tabs");
    goto end_rsl;
  }

  p->num_tabs = byte;
  p->tabs = (vkb_scv_tab *) g_malloc0 (byte * sizeof (vkb_scv_tab));
  if (!p->tabs) {
    g_warning ("insufficient memory allocating tabs");
    goto end_rsl;
  }

  t = p->tabs;
  for (i = 0; i < p->num_tabs; i ++, t ++) {
    /* label length */
    if (br_read (fd, &byte, sizeof (gint8)) == sizeof (gint8)) {
      if (byte <= 0)
        goto end_rsl;
    } else {
      g_warning ("premature_read reading label length");
      goto end_rsl;
    }
  
    t->label = (gchar*) g_malloc0 (byte * sizeof (gchar) + 1);
    if (t->label == NULL) {
      g_warning ("insufficient memory allocating label");
      goto end_rsl;
    }

    /* label */
    if (br_read (fd, t->label, sizeof (gchar) * byte) != (sizeof (gchar) * byte)) {
      g_warning ("premature_read reading label length");
      goto end_rsl;
    }

    /* total keys */
    if (br_read (fd, &byte, sizeof (gint8)) == sizeof (gint8)) {
      if (byte <= 0)
        goto end_rsl;
    } else {
      g_warning ("premature_read reading total keys");
      goto end_rsl;
    }

    t->num_keys = byte;

    /* number of columns */
    if (br_read (fd, &byte, sizeof (gint8)) == sizeof (gint8)) {
      if (byte <= 0)
        goto end_rsl;
    } else {
      g_warning ("premature_read reading number of columns");
      goto end_rsl;
    }

    t->num_columns = byte;

    /* number of rows */
    if (br_read (fd, &byte, sizeof (gint8)) == sizeof (gint8)) {
      if (byte <= 0)
        goto end_rsl;
    } else {
      g_warning ("premature_read reading number of rows");
      goto end_rsl;
    }

    t->num_rows = byte;

    t->keys = (IM_BUTTON *) 
      g_malloc0 (sizeof (IM_BUTTON) *  t->num_keys);
    if (t->keys == NULL) {
      g_warning ("insufficient memory allocating keys");
      goto end_rsl;
    }
    t->labels = (IM_LABEL *) 
      g_malloc0 (sizeof (IM_LABEL) *  t->num_keys);
    if (t->labels == NULL) {
      g_warning ("insufficient memory allocating labels");
      goto end_rsl;
    }

    b = t->keys;
    l = t->labels;
    for (j = 0; j < t->num_keys; j ++, l ++, b ++) {
      gchar *text;
      b->special.scancodes = NULL;
      b->special.length = 0;

      /* type */
      if (br_read (fd, &byte, sizeof (gint8)) == sizeof (gint8)) {
        b->type = byte;
      } else {
        g_warning ("premature_read reading key type");
        goto end_rsl;
      }

      /* length */
      if (br_read (fd, &byte, sizeof (gint8)) == sizeof (gint8)) {
        l->length = byte;
      } else {
        g_warning ("premature_read reading label length");
        goto end_rsl;
      }

      if (byte == 0) {
        l->text= NULL;
        continue;
      }

      /* the label */
      text = (gchar*) g_malloc0 (byte + 1);
      l->text = text;
      if (text == NULL) {
        g_warning ("insufficient memory allocation label");
        goto end_rsl;
      }

      if (br_read (fd, text, sizeof (gchar) * byte) != sizeof (gchar) * byte) {
        g_warning ("premature_read reading label");
        goto end_rsl;
      }     
    }
  }
  success = TRUE;
end_rsl:
  if (success == FALSE) {
    free_layout (p, 1);
    p = NULL;
  }
  return p;
}

vkb_layout_collection *imlayout_vkb_load_file0 (const gchar *filename, gboolean header_only)
{
  gboolean success = FALSE;
  int  i, num_layout;
  binary_reader *fd;
  gint8 byte;
  vkb_layout_collection *retval = NULL;
  vkb_layout *p;
  
  fd = br_open (filename);
  if (fd && fd->fd >= 0) {
    retval = read_header (fd);
    if (retval == NULL)
      goto end_load_file;
  
    if (header_only) {
      success = TRUE;
      goto end_load_file;
    }
    num_layout = retval->num_layouts;
    p = (vkb_layout *) retval->collection;
    for (i = 0; i < num_layout; i++, p++) {
      /* layout type */
      if (br_read (fd, &byte, sizeof (gint8)) == sizeof (gint8)) {
        if (byte < OSSO_VKB_LAYOUT_LOWERCASE || 
            byte >  OSSO_VKB_LAYOUT_SPECIAL)
          byte = OSSO_VKB_LAYOUT_LOWERCASE;
      } else {
        g_warning ("premature_read reading layout type");
        goto end_load_file;
      }

      p->type = (vkb_layout_type) byte;
      switch (p->type) {
        case OSSO_VKB_LAYOUT_LOWERCASE:
        case OSSO_VKB_LAYOUT_UPPERCASE:
          p = read_keyboard_layout (fd, p);
          break;
        case OSSO_VKB_LAYOUT_SPECIAL:
          p = read_specialchar_layout (fd, p);
          break;
      }
      
      if (p == NULL)
        goto end_load_file;
    }

    success = TRUE;
end_load_file:
    if (success == FALSE) {
      imlayout_vkb_free_layout_collection (retval);
      retval = NULL;
    }
    br_close (fd);
  } else {
    g_warning ("couldn't open vkb layout file: %s", filename);
  }

  return retval;
}

vkb_layout_collection *imlayout_vkb_load_file (const gchar *filename)
{
  return imlayout_vkb_load_file0 (filename, FALSE);
}


void imlayout_vkb_free_layout_collection (vkb_layout_collection *collection)
{
  vkb_layout *l;
  gint num_layout;

  if (!collection)
    return;

  l = collection->collection;
  num_layout = collection->num_layouts;
  
  free_layout (l, num_layout);
  if (collection->name != NULL)
    g_free (collection->name);

  if (collection->language != NULL)
    g_free (collection->language);

  if (collection->word_completion != NULL)
    g_free (collection->word_completion);

  g_free (collection);
  collection = NULL;
}

gchar **get_vkb_entry (const gchar *filename)
{
  gchar **retval = NULL;
  gchar *real_file = NULL;
  vkb_layout_collection *c = NULL;

  real_file = g_strdup_printf ("%s/%s", VKB_LAYOUT_DIR, filename);
  if ((c = imlayout_vkb_load_file0 (real_file, TRUE)) != NULL) {
    if (c->name != NULL && strlen (c->name) > 0 && 
        c->language != NULL) {
      retval = g_malloc0 (4 * sizeof (gchar *));
      retval [0] = g_strndup (filename, strlen (filename) - 4);
      retval [1] = g_strdup (c->name);
      retval [2] = g_strdup (c->language);
      retval [3] = g_strdup (c->word_completion);
    }
    imlayout_vkb_free_layout_collection (c);
  }
  g_free (real_file);
  return retval;
}

void init_xml (xml_data *xml)
{
  xml->name = xml->lang = xml->wc = NULL;
}

void callback_start (void *user_data, const XML_Char *name, 
  const XML_Char **attrs) 
{
  xml_data *xml = (xml_data *) user_data;
 
  if (strncmp ((char*) name, "keyboard", 8) == 0) {
    while (attrs && *attrs) {
      if (strncmp (attrs [0], "lang", 4) == 0) {
        xml->lang = g_strdup (attrs [1]); 
      } else if (strncmp (attrs [0], "name", 4) == 0) {
        xml->name = g_strdup (attrs [1]);
      } else if (strncmp (attrs [0], "wc", 2) == 0) {
        xml->wc = g_strdup (attrs [1]);
      } 

      attrs += 2;
    }
  }
}

gchar **get_xml_entry (const gchar *filename)
{
  gchar **retval = NULL;
  gchar *real_file = NULL;
  int file_in;
  xml_data xml;
  gboolean error = FALSE;

  init_xml (&xml);
  real_file = g_strdup_printf ("%s/%s", VKB_LAYOUT_DIR, filename);
  file_in = open (real_file, O_RDONLY);
  g_free (real_file);
  
  if (file_in > 0) {
    XML_Parser p;

    p = XML_ParserCreate (NULL);
    XML_SetElementHandler(p, callback_start, NULL);
    XML_SetUserData (p, &xml);

    while (1) {
      int bytes_read;

      void *buff = XML_GetBuffer (p, XML_BUFFER_SIZE);
      if (buff == NULL) {
        g_error ("Couldn't allocate expat buffer");
        break;
      }

      bytes_read = read (file_in, buff, XML_BUFFER_SIZE);
      if (!XML_ParseBuffer(p, bytes_read, bytes_read == 0)) {
        g_error ("Error parsing xml %s", 
            XML_ErrorString (XML_GetErrorCode (p)));
        error = TRUE;
        break;
      }
      if (bytes_read == 0)
        break;
    }

    XML_ParserFree (p);
    close (file_in);
  }
  if (!error) {
    retval = (gchar **) g_malloc0 (4 * sizeof (gchar *));
    retval [0] = g_strndup (filename, strlen (filename) - 4);
    if (xml.name != NULL)
      retval [1] = xml.name;
    else
      retval [1] = g_strdup (retval [0]);

    if (xml.lang != NULL)
      retval [2] = xml.lang;
    else
      retval [2] = g_strdup (retval [0]);

    if (xml.wc != NULL)
      retval [3] = xml.wc;
    else
      retval [3] = g_strdup (retval [0]);
  }
  return retval;
}

#ifdef TEST
#define VKB_LAYOUT_DIR "./"
#endif
GList *imlayout_vkb_get_layout_list ()
{
  GDir *dir = NULL;
  GList *retval = NULL;

  dir = g_dir_open (VKB_LAYOUT_DIR, 0, NULL);
  if (dir) {
    const gchar *filename;
    while ((filename= g_dir_read_name (dir)) != NULL) {
      gchar **entry = NULL;
      if (g_str_has_suffix (filename, ".vkb")) {
        entry = get_vkb_entry (filename);
      } else if (g_str_has_suffix (filename, ".xml")) {
        entry = get_xml_entry (filename);
      } else
        continue;

      if (entry != NULL)
        retval = g_list_append (retval, entry);
    }
    g_dir_close (dir);
  } 
    
  return retval;
}

void free_list_element (gpointer data, gpointer user_data)
{
  gchar **_data = (gchar **) data;
  if (data) {
    if (_data [0])
      g_free (_data [0]);

    if (_data [1])
      g_free (_data [1]);

    if (_data [2])
      g_free (_data [2]);

    if (_data [3])
      g_free (_data [3]);

    g_free (data);
  }

  data = NULL;
}

void imlayout_vkb_free_layout_list (GList *list)
{
  if (list) {
    g_list_foreach (list, free_list_element, NULL);
    g_list_free (list);
    list = NULL;
  }
}

#ifdef TEST

#define g_print printf
int main () {
  int i, k;
 
  for (i = 0; i < MAX_LANG; i++) {
    k = 0;
    vkb_layout_collection *c;
    vkb_layout *l, *lx;
    gint8 num_layout;
    vkb_layout_type type;
    gchar *filename = g_strdup_printf ("%s.vkb", langs [i]);
    c = imlayout_vkb_load_file (filename);
    if (!c) {
      g_print ("Layout invalid\n");
      continue;
    }
    g_free (filename);
    l = c->collection;
    type = c->numeric_layout;
    num_layout = c->num_layouts;
    lx = l;
    g_print ("Layout for %s (%s) with wc %s\n"\
          "number of layout %d\n"\
          "numeric layout %d\n",
          langs [i], 
          c->name, c->word_completion,
          num_layout, type
        );

    g_print ("%d %d",type, OSSO_VKB_LAYOUT_SPECIAL);
    if (lx->type == OSSO_VKB_LAYOUT_SPECIAL) {
      gint8 j = 0, k, m;
      vkb_scv_tab *t;
      IM_LABEL *lbl;
      IM_BUTTON *btn;

      t = lx->tabs;
      for (k = 0; k < lx->num_tabs; k++, t++) {
        g_print (
          "#TAB label %s\n"\
          "total keys %d\n"\
          "number of columns: %d\n"\
          "number of rows: %d\n",
          t->label,
          t->num_keys,
          t->num_columns,
          t->num_rows
          );

        g_print ("\nList of labels: ");

        m = 0;
        lbl = t->labels;
        btn = t->keys;
        while (m < t->num_keys) {
          gchar *c = g_strndup (lbl->text, lbl->length);
          g_print ("%s ", c);
          if (btn->type & KEY_TYPE_RAW) {
            IM_SPECIAL_KEY sk = btn->special;
            int xx = 0;
            g_print ("{ scan code = ");
            while (xx < sk.length) {
              g_print (" %d ", sk.scancodes [xx]);
              xx ++;
            }
            g_print ("} ");
          }
          g_free (c);
          lbl ++;
          m ++;
          btn++;
        }

        g_print ("\n");
      }
    } else 
    while (k < num_layout) {
      gint8 j = 0, m;
      IM_LABEL *lbl;
      IM_BUTTON *btn;
      gint8 *byte = lx->num_keys_in_rows;
      g_print ("Layout #%d (type %d)\n"\
        "total keys %d\n"\
        "number of rows: %d\n",
        k + 1, lx->type,
        lx->TOTAL_chars,
        lx->num_rows
        );
      g_print ("number of keys in each row: ");
      while (j < lx->num_rows) {
        g_print ("%d ", *byte);
        byte ++;
        j ++;
      }
      g_print ("\nList of labels: ");

      m = 0;
      lbl = lx->labels;
      btn = lx->keys;
      while (m < lx->TOTAL_chars) {
        gchar *c = g_strndup (lbl->text, lbl->length);
        g_print ("%s ", c);
        if (btn->type & KEY_TYPE_RAW) {
          IM_SPECIAL_KEY sk = btn->special;
          int xx = 0;
          g_print ("{ scan code = ");
          while (xx < sk.length) {
            g_print (" %d ", sk.scancodes [xx]);
            xx ++;
          }
          g_print ("} ");
        }
        g_free (c);
        lbl ++;
        m ++;
        btn++;
      }

      g_print ("\n");
      lx ++;
      k ++;
    }
    imlayout_vkb_free_layout_collection (c);

  }

  GList *list = imlayout_vkb_get_layout_list ();
  int _x;
  for (_x = 0; _x < g_list_length (list); _x++) {
    gchar **en = g_list_nth_data (list, _x);
    g_print ("%s %s %s %s\n", en [0], en [1], en [2], en [3]);
  }
  imlayout_vkb_free_layout_list (list);
  return 0;
}
#endif
