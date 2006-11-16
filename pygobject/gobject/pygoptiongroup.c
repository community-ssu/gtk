/* -*- Mode: C; c-basic-offset: 4 -*-
 * pygtk- Python bindings for the GTK toolkit.
 * Copyright (C) 2006  Johannes Hoelzl
 *
 *   pygoptiongroup.c: GOptionContext and GOptionGroup wrapper
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "pygobject-private.h"

static gboolean
check_if_owned(PyGOptionGroup *self)
{
    if (self->other_owner)
    {
        PyErr_SetString(PyExc_ValueError, "The GOptionGroup was not created by "
                        "gobject.OptionGroup(), so operation is not possible.");
        return TRUE;
    }
    return FALSE;
}


static void destroy_g_group(PyGOptionGroup *self)
{
    PyGILState_STATE state;
    state = pyg_gil_state_ensure();

    self->group = NULL;
    Py_CLEAR(self->callback);
    g_slist_foreach(self->strings, (GFunc) g_free, NULL);
    g_slist_free(self->strings);
    self->strings = NULL;
    
    if (self->is_in_context)
    {
        Py_DECREF(self);
    }
        
    pyg_gil_state_release(state);
}

static int
pyg_option_group_init(PyGOptionGroup *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "name", "description", "help_description", 
                              "callback", NULL };
    char *name, *description, *help_description;
    PyObject *callback;
    
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "zzzO:GOptionGroup.__init__",
                                     kwlist, &name, &description, 
                                     &help_description, &callback))
        return -1;
    
    self->group = g_option_group_new(name, description, help_description,
                                     self, (GDestroyNotify) destroy_g_group);
    self->other_owner = FALSE;
    self->is_in_context = FALSE;
    
    Py_INCREF(callback);
    self->callback = callback;
    
    return 0;
}

static void
pyg_option_group_dealloc(PyGOptionGroup *self)
{
    if (!self->other_owner && !self->is_in_context)
    {
        GOptionGroup *tmp = self->group;
        g_assert(tmp != NULL);
        self->group = NULL;
        g_option_group_free(tmp);
    }

    PyObject_Del(self);
}

static gboolean 
arg_func(const gchar *option_name,
         const gchar *value,
         PyGOptionGroup *self,
         GError *error)
{
    PyObject *ret;
    PyGILState_STATE state;

    state = pyg_gil_state_ensure();
  
    if (value == NULL)
        ret = PyObject_CallFunction(self->callback, "sOO", 
                                    option_name, Py_None, self);
    else
        ret = PyObject_CallFunction(self->callback, "ssO", 
                                    option_name, value, self);
    
    if (ret == NULL)
        PyErr_Print();

    pyg_gil_state_release(state);

    if (ret == NULL)
        return FALSE;

    Py_DECREF(ret);
    return TRUE;      
}

static PyObject *
pyg_option_group_add_entries(PyGOptionGroup *self, PyObject *args, 
                             PyObject *kwargs)
{
    static char *kwlist[] = { "entries", NULL };
    gssize entry_count, pos;
    PyObject *entry_tuple, *list;
    GOptionEntry *entries;
    
    if (check_if_owned(self)) return NULL;
    
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O:GOptionGroup.add_entries",
                                     kwlist, &list))
        return NULL;
    
    if (!PyList_Check(list))
    {
        PyErr_SetString(PyExc_TypeError, 
                        "GOptionGroup.add_entries expected a list of entries");
        return NULL;
    }

    entry_count = PyList_Size(list);
    if (entry_count == -1)
    {
        PyErr_SetString(PyExc_TypeError, 
                        "GOptionGroup.add_entries expected a list of entries");
        return NULL;
    }
    
    entries = g_new0(GOptionEntry, entry_count + 1);
    for (pos = 0; pos < entry_count; pos++)
    {
        gchar *long_name, *description, *arg_description;
        entry_tuple = PyList_GetItem(list, pos);
        if (!PyTuple_Check(entry_tuple))
        {
            PyErr_SetString(PyExc_TypeError, "GOptionGroup.add_entries "
                                             "expected a list of entries");
            g_free(entries);
            return NULL;
        }
        if (!PyArg_ParseTuple(entry_tuple, "scisz",
            &long_name,
            &(entries[pos].short_name),
            &(entries[pos].flags),
            &description,
            &arg_description))
        {
            PyErr_SetString(PyExc_TypeError, "GOptionGroup.add_entries "
                                             "expected a list of entries");
            g_free(entries);
            return NULL;
        }
        long_name = g_strdup(long_name);
        self->strings = g_slist_prepend(self->strings, long_name);
        entries[pos].long_name = long_name;

        description = g_strdup(description);
        self->strings = g_slist_prepend(self->strings, description);
        entries[pos].description = description;

        arg_description = g_strdup(arg_description);
        self->strings = g_slist_prepend(self->strings, arg_description);
        entries[pos].arg_description = arg_description;
        
        entries[pos].arg = G_OPTION_ARG_CALLBACK;
        entries[pos].arg_data = arg_func;
    }
    
    g_option_group_add_entries(self->group, entries);
    
    g_free(entries);

    Py_INCREF(Py_None);
    return Py_None;
}


static PyObject *
pyg_option_group_set_translation_domain(PyGOptionGroup *self, 
                                        PyObject *args, 
                                        PyObject *kwargs)
{
    static char *kwlist[] = { "domain", NULL };
    char *domain;
    if (check_if_owned(self)) return NULL;

    if (self->group == NULL)
    {
        PyErr_SetString(PyExc_RuntimeError, 
                        "The corresponding GOptionGroup was already freed, "
                        "probably through the release of GOptionContext");
        return NULL;
    }
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, 
                                     "z:GOptionGroup.set_translate_domain", 
                                     kwlist, &domain))
        return NULL;
    g_option_group_set_translation_domain(self->group, domain);
    Py_INCREF(Py_None);
    return Py_None;
}

static int
pyg_option_group_compare(PyGOptionGroup *self, PyGOptionGroup *group)
{
    if (self->group == group->group) return 0;
    if (self->group > group->group)
        return 1;
    return -1;
}

static PyMethodDef pyg_option_group_methods[] = {
    { "add_entries", (PyCFunction)pyg_option_group_add_entries, METH_VARARGS | METH_KEYWORDS },
    { "set_translation_domain", (PyCFunction)pyg_option_group_set_translation_domain, METH_VARARGS | METH_KEYWORDS },
    { NULL, NULL, 0 },
};

PyTypeObject PyGOptionGroup_Type = {
    PyObject_HEAD_INIT(NULL)
    0,
    "gobject.OptionGroup",
    sizeof(PyGOptionGroup),
    0,
    /* methods */
    (destructor)pyg_option_group_dealloc,
    (printfunc)0,
    (getattrfunc)0,
    (setattrfunc)0,
    (cmpfunc)pyg_option_group_compare,
    (reprfunc)0,
    0,
    0,
    0,
    (hashfunc)0,
    (ternaryfunc)0,
    (reprfunc)0,
    (getattrofunc)0,
    (setattrofunc)0,
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    NULL,
    (traverseproc)0,
    (inquiry)0,
    (richcmpfunc)0,
    0,
    (getiterfunc)0,
    (iternextfunc)0,
    pyg_option_group_methods,
    0,
    0,
    NULL,
    NULL,
    (descrgetfunc)0,
    (descrsetfunc)0,
    0,
    (initproc)pyg_option_group_init,
};

/**
 * pyg_option_group_transfer_group:
 * @group: a GOptionGroup wrapper
 *
 * This is used to transfer the GOptionGroup to a GOptionContext. After this
 * is called, the calle must handle the release of the GOptionGroup.
 *
 * When #NULL is returned, the GOptionGroup was already transfered.
 *
 * Returns: Either #NULL or the wrapped GOptionGroup.
 */
GOptionGroup *
pyg_option_group_transfer_group(PyGOptionGroup *self)
{
    if (self->is_in_context) return NULL;
    self->is_in_context = TRUE;
    
    /* Here we increase the reference count of the PyGOptionGroup, because now
     * the GOptionContext holds an reference to us (it is the userdata passed
     * to g_option_group_new().
     *
     * The GOptionGroup is freed with the GOptionContext.
     *
     * We set it here because if we would do this in the init method we would
     * hold two references and the PyGOptionGroup would never be freed.
     */
    Py_INCREF(self);
    
    return self->group;
}

/**
 * pyg_option_group_new:
 * @group: a GOptionGroup
 *
 * The returned GOptionGroup can't be used to set any hooks, translation domains
 * or add entries. It's only intend is, to use for GOptionContext.add_group().
 *
 * Returns: the GOptionGroup wrapper.
 */
PyObject * 
pyg_option_group_new (GOptionGroup *group)
{
    PyGOptionGroup *self;

    self = (PyGOptionGroup *)PyObject_NEW(PyGOptionGroup,
					  &PyGOptionGroup_Type);
    if (self == NULL)
	return NULL;

    self->group = group;
    self->other_owner = TRUE;
    self->is_in_context = FALSE;
        
    return (PyObject *)self;
}
