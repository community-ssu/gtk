/*
 * This file is part of libimlayouts
 *
 * Copyright (C) 2005 Nokia Corporation.
 *
 * Contact: Mohammad Anwari <mdamt@maemo.org>
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



#include <string.h>
#include <stdlib.h>
#include <glib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <expat.h>
#include "imlayout_vkb.h"

typedef struct _xml_data
{
  int fd;
  gchar *filename, *name, *lang, *wc, *label;
  gint8 version;
  gint8 total_keys;
	gint8 num_tabs;
  gint8 num_rows;
  gint8 num_layout;
  gint8 numeric_layout;
  gint8 _layout_type;
	gint8 num_columns;
  GList *labels;
  GList *key_type;
  GList *num_keys_in_rows;
  GList *scancodes;
  GList *scancodes_size;
  int _row;
  gboolean _write;
	gboolean _text_written;
  gboolean _header_written;

	int num_tab_offset;
} xml_data;

#define BUFFER_SIZE 1024

void free_list_element (gpointer data, gpointer user_data)
{
	if (data != NULL)
		g_free (data);
}

void init_xml (xml_data *xml)
{ 
	xml->num_tab_offset = 0;
  xml->labels = xml->key_type = xml->num_keys_in_rows = NULL;
	xml->label = NULL;
  xml->_row = xml->num_rows = xml->total_keys = xml->num_columns =
    xml->num_layout = xml->num_tabs = 0;
  xml->numeric_layout = OSSO_VKB_LAYOUT_LOWERCASE;
  xml->scancodes = xml->scancodes_size = NULL;
  xml->_header_written = FALSE;
  xml->fd = -1;
}

void free_xml (xml_data *xml)
{
	//if (xml->label)
		//g_free (xml->label);

	if (xml->labels) {
		g_list_foreach (xml->labels, free_list_element, NULL);
		g_list_free (xml->labels);
	}

	if (xml->key_type) {
		g_list_foreach (xml->key_type, free_list_element, NULL);
		g_list_free (xml->key_type);
	}

	if (xml->num_keys_in_rows) {
		g_list_foreach (xml->num_keys_in_rows, free_list_element, NULL);
		g_list_free (xml->num_keys_in_rows);
	}

	if (xml->scancodes) {
		g_list_foreach (xml->scancodes, free_list_element, NULL);
		g_list_free (xml->scancodes);
	}

	if (xml->scancodes_size != NULL) {
    g_list_foreach (xml->scancodes_size, free_list_element, NULL);
		g_list_free (xml->scancodes_size);
	}

  xml->labels = xml->key_type = 
    xml->num_keys_in_rows = xml->scancodes = 
    xml->scancodes_size = NULL;
}

void write_header (xml_data *xml)
{
	int fd;
	gint8 l;

	if (xml->fd == -1) {
    fd = creat (xml->filename, S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH);
    if (fd == -1) {
      g_print ("Error generating file %s: %s", xml->filename, 
          strerror (errno));
      g_free (xml->filename);
      return;
    } 
 
    xml->fd = fd;
  } else {
    fd = xml->fd;
  }

	write (fd, &xml->version, sizeof (gint8));
	write (fd, &xml->num_layout, sizeof (gint8));
	write (fd, &xml->numeric_layout, sizeof (gint8));

	xml->num_tab_offset += 3;
	l = 0;
	if (xml->name)
		l = strlen (xml->name);

	xml->num_tab_offset += 1 + l;
	write (fd, &l, sizeof (gint8));
	if (l > 0)
		write (fd, xml->name, sizeof (gint8) * l);

	l = 0;
	if (xml->lang)
		l = strlen (xml->lang);

	xml->num_tab_offset += 1 + l;
	write (fd, &l, sizeof (gint8));
	if (l > 0)
		write (fd, xml->lang, sizeof (gint8) * l);

	l = 0;
	if (xml->wc) 
		l = strlen (xml->wc);

	xml->num_tab_offset += 1 + l;
	write (fd, &l, sizeof (gint8));
	if (l > 0)
		write (fd, xml->wc, sizeof (gint8) * l);

	if (xml->_layout_type == OSSO_VKB_LAYOUT_SPECIAL) {
		write (fd, &xml->_layout_type, sizeof (gint8));
		write (fd, &xml->num_tabs, sizeof (gint8));
		xml->num_tab_offset += 1;
	}

	g_print ("Generating file %s ", xml->filename);
	xml->_header_written = TRUE;
}

void write_vkb (xml_data *xml)
{
  int fd, j;
  int scancode_index = 0;

  if (xml->_header_written == FALSE) {
		write_header (xml);
  }

	if (xml->_layout_type == OSSO_VKB_LAYOUT_SPECIAL)
		return;

	fd = xml->fd;

  write (fd, &xml->_layout_type, sizeof (gint8));
  write (fd, &xml->total_keys, sizeof (gint8));
  write (fd, &xml->num_rows, sizeof (gint8));
  for (j = 0; j < g_list_length (xml->num_keys_in_rows); j ++) {
    gint8 *val = g_list_nth_data (xml->num_keys_in_rows, j);

    write (fd, val, sizeof (gint8));
  }
  for (j = 0; j < g_list_length (xml->labels); j ++) {
    gchar *temp;
    gint8 length = 0;
    gint8 *type = g_list_nth_data (xml->key_type, j);
    gchar *val = g_list_nth_data (xml->labels, j);
    temp = val;
    while (1) {
      if (temp [0] == 0)
        break;
      length ++;
      temp ++;
    }

    write (fd, type, sizeof (gint8));
    write (fd, &length, sizeof (gint8));
		if (length > 0)
			write (fd, val, sizeof (gint8) * length);
    
    if (*type & KEY_TYPE_RAW) {
      gint8 *size = g_list_nth_data (xml->scancodes_size, scancode_index);
      gint8 *codes = g_list_nth_data (xml->scancodes, scancode_index);
      write (fd, size, sizeof (gint8));
      write (fd, codes, sizeof (gint8) * *size);
      scancode_index ++;
    }
  }
}

void write_scv_tab (xml_data *xml)
{
  int fd, j;
	gint8 l;
  
	if (xml->_header_written == FALSE) {
		write_header (xml);
  }

	fd = xml->fd;
	l = strlen (xml->label);
	write (fd, &l, sizeof (gint8));
  write (fd, xml->label, sizeof (gint8) * l);
	
  write (fd, &xml->total_keys, sizeof (gint8));
	write (fd, &xml->num_columns, sizeof (gint8));
  write (fd, &xml->num_rows, sizeof (gint8));
  
  for (j = 0; j < g_list_length (xml->labels); j ++) {
    gchar *temp;
    gint8 length = 0;
    gint8 *type = g_list_nth_data (xml->key_type, j);
    gchar *val = g_list_nth_data (xml->labels, j);
    temp = val;
    if (temp != NULL)
			while (1) {
				if (temp [0] == 0)
					break;
				length ++;
				temp ++;
			}

    write (fd, type, sizeof (gint8));
    write (fd, &length, sizeof (gint8));
		if (length > 0)
			write (fd, val, sizeof (gint8) * length);
  }
}

gint8 *parse_scancode (const gchar *code, gint8 *size) 
{
  gint8 *retval = NULL;
  *size = 0;

  if (code != NULL) {
    gchar **array = g_strsplit (code, " ", 0);
    int i = 0;

    if (array) {
      while (1) {
        long result;
        if (array [i] == NULL)
          break;
  
        errno = 0;
        result = strtoll (array [i], (char **) NULL, 10);
        if (errno) {
          g_print ("Invalid number %s in scancode", array [i]);
        } else {
          *size = *size + 1;
          retval = realloc (retval, *size);
          if (retval)
            retval [*size - 1] = (gint8) result;
          else {
            g_print ("unable to add more scancode, memory exhausted");
            break;
          }
        }
        i++;
      }
      g_strfreev (array);
    }
  }
  return retval;
}

void callback_start (void *user_data, const XML_Char *name, 
  const XML_Char **attrs) 
{
  xml_data *xml = (xml_data *) user_data;

  xml->_write = FALSE;
  if (strncmp ((char*) name, "row", 3) == 0) {
    xml->num_rows ++;
    xml->_row = 0;
  } else if (strncmp ((char*) name, "keyboard", 8) == 0) {
    init_xml (xml);
    xml->lang = xml->name = xml->wc = NULL;
    while (attrs && *attrs) {
      if (strncmp (attrs [0], "file", 4) == 0) {
        if (xml->filename)
          g_free (xml->filename);

        xml->filename = g_strdup_printf ("%s.vkb", attrs [1]);
      } else if (strncmp (attrs [0], "lang", 4) == 0) {
        if (xml->filename == NULL)
          xml->filename = g_strdup_printf ("%s.vkb", attrs [1]);

        xml->lang = g_strdup (attrs [1]);
      } else if (strncmp (attrs [0], "numeric", 7) == 0) {
        if (strncmp (attrs [1], "UPPERCASE", 9) == 0) 
          xml->numeric_layout = OSSO_VKB_LAYOUT_UPPERCASE;
        else
          xml->numeric_layout = OSSO_VKB_LAYOUT_LOWERCASE;
      } else if (strncmp (attrs [0], "version", 7) == 0) {
        xml->version = atoi (attrs [1]);
      } else if (strncmp (attrs [0], "name", 4) == 0) {
        xml->name = g_strdup (attrs [1]);
      } else if (strncmp (attrs [0], "wc", 2) == 0) {
        xml->wc = g_strdup (attrs [1]);
      } 

      attrs += 2;
    }
  } else if (strncmp ((char*) name, "keypad", 6) == 0) {
    /* ignore */
  } else if (strncmp ((char*) name, "key", 3) == 0) {
    gint8 *type = g_malloc (sizeof (gint8));
    gint8 _type = 0;

    xml->_row ++;
    xml->total_keys ++; 
    while (attrs && *attrs) {
      if (strncmp (attrs [0], "dead", 4) == 0) 
        _type |= KEY_TYPE_DEAD;
      else if (strncmp (attrs [0], "alpha", 5) == 0) 
        _type |= KEY_TYPE_ALPHA;
      else if (strncmp (attrs [0], "numeric", 7) == 0) 
        _type |= KEY_TYPE_NUMERIC;
      else if (strncmp (attrs [0], "hexa", 4) == 0) 
        _type |= KEY_TYPE_HEXA;
      else if (strncmp (attrs [0], "tele", 4) == 0) 
        _type |= KEY_TYPE_TELE;
      else if (strncmp (attrs [0], "special", 7) == 0) 
        _type |= KEY_TYPE_SPECIAL;
      else if (strncmp (attrs [0], "whitespace", 10) == 0) 
        _type |= KEY_TYPE_WHITESPACE;
      else if (strncmp (attrs [0], "raw", 3) == 0) 
        _type |= KEY_TYPE_RAW;
      else if (strncmp (attrs [0], "scancode", 8) == 0 &&
        (_type & KEY_TYPE_RAW)) {

        gint8 _size = 0;
        gint8 *size = g_malloc (sizeof (gint8));
        gint8 *scancode = parse_scancode (attrs [1], &_size);
        size [0] = _size;

        xml->scancodes = g_list_append (xml->scancodes, scancode);
        xml->scancodes_size = g_list_append (xml->scancodes_size, size);
      }
      attrs += 2;
    }
    *type = _type;
    xml->key_type = g_list_append (xml->key_type, type);
    xml->_write = TRUE;
		xml->_text_written = FALSE;
  } else if (strncmp ((char*) name, "layout", 6) == 0) {
    xml->total_keys = 0;
    xml->num_rows = 0;
    xml->num_layout ++;
    while (attrs && *attrs) {
      if (strncmp (attrs [0], "type", 4) == 0) { 
        if (strncmp (attrs [1], "LOWERCASE", 9) == 0) 
          xml->_layout_type = OSSO_VKB_LAYOUT_LOWERCASE;
        else if (strncmp (attrs [1], "UPPERCASE", 9) == 0) 
          xml->_layout_type = OSSO_VKB_LAYOUT_UPPERCASE;
        else
          xml->_layout_type = OSSO_VKB_LAYOUT_SPECIAL; 
      }
      attrs += 2;
    }
  } else if (strncmp ((char*) name, "tab", 3) == 0) {
		xml->num_layout = 1;
		xml->total_keys = xml->num_rows = xml->num_columns = 0;
		xml->num_tabs ++;
		while (attrs && *attrs) {
      if (strncmp (attrs [0], "label", 5) == 0) { 
				xml->label = g_strdup (attrs [1]); 
      } else if (strncmp (attrs [0], "rows", 4) == 0) { 
				xml->num_rows = strtol (attrs [1], (char **) NULL, 10); 
      } else if (strncmp (attrs [0], "cols", 4) == 0) { 
				xml->num_columns = strtol (attrs [1], (char **) NULL, 10); 
      }
      attrs += 2;
    }
  }
}

void callback_end (void *user_data, const XML_Char *name) 
{
  xml_data *xml = (xml_data *) user_data;

  xml->_write = FALSE;
	if (strncmp ((char*) name, "tab", 3) == 0) {
    write_scv_tab (xml);
    free_xml (xml);
  } else if (strncmp ((char*) name, "layout", 6) == 0) {
    write_vkb (xml);
    free_xml (xml);
  } else if (strncmp ((char*) name, "keyboard", 8) == 0) {
		lseek (xml->fd, 1, SEEK_SET);
    write (xml->fd, &xml->num_layout, sizeof (gint8));
		if (xml->num_tabs > 0) {
			lseek (xml->fd, xml->num_tab_offset, SEEK_SET);
			write (xml->fd, &xml->num_tabs, sizeof (gint8));
		}
    close (xml->fd);
    free (xml->filename);

    xml->filename = NULL;
    if (xml->lang)
      g_free (xml->lang);
    if (xml->name)
      g_free (xml->name);
    if (xml->wc)
      g_free (xml->wc);

    g_print ("... done\n");
	} else if (strncmp ((char*) name, "keypad", 5) == 0) {
		/* ignore */
	} else if (strncmp ((char*) name, "key", 3) == 0) {
    if (xml->_text_written == FALSE) {
			xml->labels = g_list_append (xml->labels, NULL);
		}
  } else if (strncmp ((char*) name, "row", 3) == 0) {
    int *nr = g_malloc (sizeof (int));

    *nr = xml->_row;
    xml->num_keys_in_rows = g_list_append (xml->num_keys_in_rows, nr);
  }
	xml->_text_written = FALSE;
}

void callback_cdata(void *user_data, const XML_Char *data, int len) 
{
  xml_data *xml = (xml_data *) user_data;

  if (xml->_write) {
		xml->labels = g_list_append (xml->labels, g_strndup (data, len));
		xml->_text_written = TRUE;
	} 
}

void parse_xml (const gchar *filename)
{
  int file_in;
  xml_data xml;

  init_xml (&xml);
  file_in = open (filename, O_RDONLY);
  if (file_in > 0) {
    XML_Parser p;

    p = XML_ParserCreate (NULL);
    XML_SetElementHandler(p, callback_start, callback_end);
    XML_SetCharacterDataHandler(p, callback_cdata);
    XML_SetUserData (p, &xml);

    while (1) {
      int bytes_read;

      void *buff = XML_GetBuffer (p, BUFFER_SIZE);
      if (buff == NULL) {
        g_error ("Couldn't allocate expat buffer");
        break;
      }

      bytes_read = read (file_in, buff, BUFFER_SIZE);
      if (!XML_ParseBuffer(p, bytes_read, bytes_read == 0)) {
        g_error ("Error parsing xml %s", 
            XML_ErrorString (XML_GetErrorCode (p)));
      }
      if (bytes_read == 0)
        break;
    }

    XML_ParserFree (p);
    close (file_in);
  }

}

int main () {
  parse_xml ("keyboard.xml");
  return 0;
}

