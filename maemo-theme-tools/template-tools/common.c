/* 
 * GPL license, Copyright (c) 2006 by Nokia Corporation                       
 *                                                                            
 * Authors:                                                                   
 *      Michael Dominic Kostrzewa <michael.kostrzewa@nokia.com>               
 *                                                                            
 * This program is free software; you can redistribute it and/or modify it    
 * under the terms of the GNU General Public License as published by the      
 *  Free Software Foundation; either version 2, or (at your option) any later
 * version.                                                                   
 *                                                                            
 * This program is distributed in the hope that it will be useful, but        
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY 
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License   
 * for more details.                                                          
 *                                                                            
 * You should have received a copy of the GNU General Public License along    
 * with this program; if not, write to the Free Software Foundation, Inc., 59 
 * Temple Place - Suite 330, Boston, MA 02111-1307, USA.                      
 *
 */

#include "common.h"

/* Reads a given file and returns an allocated template. 
 * Returns NULL on failure AND prints the error message */
Template*                       read_template (gchar *template_file)
{
        GKeyFile *key_file = g_key_file_new ();
        Template *templ = g_new0 (Template, 1);
        gchar **element_keys = NULL;
        gchar **color_keys = NULL;
        guint element_count = 0;
        guint color_count = 0;
        int i;

        g_return_val_if_fail (templ != NULL, NULL);
        g_return_val_if_fail (key_file != NULL, NULL);

        if (! g_key_file_load_from_file (key_file, template_file, G_KEY_FILE_NONE, NULL)) {
                g_print ("ERROR: Failed to load and parse template file!\n");
                goto Error;
        }

        g_key_file_set_list_separator (key_file, ',');

        /* Do some basic checks... */
        if (! g_key_file_has_group (key_file, "Main")) {
                g_print ("ERROR: [Main] group not present, invalid template file!\n");
                goto Error;
        }

        if (! g_key_file_has_group (key_file, "Elements")) {
                g_print ("ERROR: [Elements] group not present, invalid template file!\n");
                goto Error;
        }

        /* More specific checks... */
        if (! g_key_file_has_key (key_file, "Main", "TemplateWidth", NULL)) {
                g_print ("ERROR: No TemplateWidth specified!\n");
                goto Error;
        }

        if (! g_key_file_has_key (key_file, "Main", "TemplateHeight", NULL)) {
                g_print ("ERROR: No TemplateHeight specified!\n");
                goto Error;
        }

        /* Extract our basic params */
        templ->Width = g_key_file_get_integer (key_file, "Main", "TemplateWidth", NULL);
        templ->Height = g_key_file_get_integer (key_file, "Main", "TemplateHeight", NULL);
        if (templ->Width == 0 || templ->Height == 0) {
                g_print ("ERROR: Bad template dimensions!\n");
                goto Error;
        }

        /* Get all the elements */
        element_keys = g_key_file_get_keys (key_file, "Elements", &element_count, NULL);
        if (element_keys == NULL || element_count == 0) {
                g_print ("ERROR: No elements found!\n");
                goto Error;
        }

        for (i = 0; i < element_count; i++) {
                Element *element = new_element_from_key (key_file, element_keys [i]);
                if (element == NULL) 
                        g_print ("WARNING: Failed to parse '%s'!\n", element_keys [i]);
                else 
                        templ->ElementList = g_slist_append (templ->ElementList, 
                                                             element);
        }

        /* Try to parse the colors, but only if we have some defined */
        if (g_key_file_has_group (key_file, "Colours")) {
                color_keys = g_key_file_get_keys (key_file, "Colours", &color_count, NULL);
                if (color_keys != NULL && color_count != 0) {

                        /* Now load all the colors... */
                        for (i = 0; i < color_count; i++) {
                                Color *color = new_color_from_key (key_file, color_keys [i]);
                                if (color == NULL) 
                                        g_print ("WARNING: Failed to parse '%s'!\n", color_keys [i]);
                                else 
                                        templ->ColorList = g_slist_append (templ->ColorList, 
                                                                           color);
                        }
                }
        }

        goto Done;

Error:
         if (templ != NULL)
                free_template (templ);

         templ = NULL;

Done:
        if (key_file != NULL)
                g_free (key_file);
        
        if (element_keys != NULL)
                g_strfreev (element_keys);

        if (color_keys != NULL)
                g_strfreev (color_keys);

        return templ;
}

/* Free the memory allocated by an element */
void                            free_element (Element *element)
{
        g_return_if_fail (element != NULL);
        if (element->Name != NULL)
                g_free (element->Name);

        g_free (element);
}

/* Free the memory allocated by the color */
void                            free_color (Color *color)
{
        g_return_if_fail (color != NULL);
        if (color->Name != NULL)
                g_free (color->Name);

        g_free (color);
}

/* Show some information about the template */
void                            show_template (Template *template)
{
        g_print ("Template width:     %d\n", template->Width);
        g_print ("Template height:    %d\n", template->Height);
        g_print ("Number of elements: %d\n", g_slist_length (template->ElementList));
        g_print ("Number of colours:  %d\n\n", g_slist_length (template->ColorList));
}

/* Free the memory allocated by a template */
void                            free_template (Template *templ)
{
        GSList *iterator = NULL;

        g_return_if_fail (templ != NULL);

        /* Free the elements */
        for (iterator = templ->ElementList; iterator; iterator = g_slist_next (iterator)) {
                Element *element = (Element *) iterator->data;
                free_element (element);
        }

        /* Free the colors */
        for (iterator = templ->ColorList; iterator; iterator = g_slist_next (iterator)) {
                Color *color = (Color *) iterator->data;
                free_color (color);
        }

        g_free (templ);
}

/* Create a new element from a given key name. 
 * Parses the list */
Element*                        new_element_from_key (GKeyFile *key_file, gchar *name)
{
        Element *element = NULL;
        guint size = 0;
        gint *vals = NULL;
        
        g_return_val_if_fail (key_file != NULL, NULL);

        if (name == NULL)
                goto Done;

        if (! g_key_file_has_key (key_file, "Elements", name, NULL))
                goto Done;

        size = 0;
        vals = g_key_file_get_integer_list (key_file, "Elements", name, &size, NULL);

        if (size != 4 || vals == NULL)
                goto Done;

        element = g_new0 (Element, 1);
        element->X = vals [0];
        element->Y = vals [1];
        element->Width = vals [2];
        element->Height = vals [3];
        element->Name = g_strdup (name);

Done:
        if (vals != NULL)
                g_free (vals);

        return element;
}

/* Create a new color from a given color name 
 * Parses the list */
Color*                          new_color_from_key (GKeyFile *key_file, gchar *name)
{
        Color *color = NULL;
        guint size = 0;
        gint *vals = NULL;
        
        g_return_val_if_fail (key_file != NULL, NULL);

        if (name == NULL)
                goto Done;

        if (! g_key_file_has_key (key_file, "Colours", name, NULL))
                goto Done;

        size = 0;
        vals = g_key_file_get_integer_list (key_file, "Colours", name, &size, NULL);

        if (size != 4 || vals == NULL)
                goto Done;

        color = g_new0 (Color, 1);
        color->X = vals [0] + (vals [2] / 2);
        color->Y = vals [1] + (vals [3] / 2);
        color->Name = g_strdup (name);

Done:
        if (vals != NULL)
                g_free (vals);

        return color;
}
