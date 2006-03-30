/* -*- Mode: C; c-basic-offset: 4 -*-
 * pygtk- Python bindings for the GTK toolkit.
 * Copyright (C) 1998-2003  James Henstridge
 *
 *   pygpointer.c: wrapper for GPointer
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

static void
pyg_pointer_dealloc(PyGPointer *self)
{
    self->ob_type->tp_free((PyObject *)self);
}

static int
pyg_pointer_compare(PyGPointer *self, PyGPointer *v)
{
    if (self->pointer == v->pointer) return 0;
    if (self->pointer > v->pointer)  return -1;
    return 1;
}

static long
pyg_pointer_hash(PyGPointer *self)
{
    return (long)self->pointer;
}

static PyObject *
pyg_pointer_repr(PyGPointer *self)
{
    gchar buf[128];

    g_snprintf(buf, sizeof(buf), "<%s at 0x%lx>", g_type_name(self->gtype),
	       (long)self->pointer);
    return PyString_FromString(buf);
}

static int
pyg_pointer_init(PyGPointer *self, PyObject *args, PyObject *kwargs)
{
    gchar buf[512];

    if (!PyArg_ParseTuple(args, ":GPointer.__init__"))
	return -1;

    self->pointer = NULL;
    self->gtype = 0;

    g_snprintf(buf, sizeof(buf), "%s can not be constructed", self->ob_type->tp_name);
    PyErr_SetString(PyExc_NotImplementedError, buf);
    return -1;
}

static void
pyg_pointer_free(PyObject *op)
{
  PyObject_FREE(op);
}

PyTypeObject PyGPointer_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                                  /* ob_size */
    "gobject.GPointer",                 /* tp_name */
    sizeof(PyGPointer),                 /* tp_basicsize */
    0,                                  /* tp_itemsize */
    /* methods */
    (destructor)pyg_pointer_dealloc,    /* tp_dealloc */
    (printfunc)0,                       /* tp_print */
    (getattrfunc)0,                     /* tp_getattr */
    (setattrfunc)0,                     /* tp_setattr */
    (cmpfunc)pyg_pointer_compare,       /* tp_compare */
    (reprfunc)pyg_pointer_repr,         /* tp_repr */
    0,                                  /* tp_as_number */
    0,                                  /* tp_as_sequence */
    0,                                  /* tp_as_mapping */
    (hashfunc)pyg_pointer_hash,         /* tp_hash */
    (ternaryfunc)0,                     /* tp_call */
    (reprfunc)0,                        /* tp_str */
    (getattrofunc)0,			/* tp_getattro */
    (setattrofunc)0,                    /* tp_setattro */
    0,					/* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,	/* tp_flags */
    NULL, /* Documentation string */
    (traverseproc)0,			/* tp_traverse */
    (inquiry)0,				/* tp_clear */
    (richcmpfunc)0,			/* tp_richcompare */
    0,					/* tp_weaklistoffset */
    (getiterfunc)0,			/* tp_iter */
    (iternextfunc)0,			/* tp_iternext */
    0,					/* tp_methods */
    0,					/* tp_members */
    0,					/* tp_getset */
    (PyTypeObject *)0,			/* tp_base */
    (PyObject *)0,			/* tp_dict */
    0,					/* tp_descr_get */
    0,					/* tp_descr_set */
    0,					/* tp_dictoffset */
    (initproc)pyg_pointer_init,		/* tp_init */
    (allocfunc)0,			/* tp_alloc */
    (newfunc)0,				/* tp_new */
    (freefunc)pyg_pointer_free,		/* tp_free */
    (inquiry)0,				/* tp_is_gc */
    (PyObject *)0,			/* tp_bases */
};

static GQuark pointer_type_id = 0;

/**
 * pyg_register_pointer:
 * @dict: the module dictionary to store the wrapper class.
 * @class_name: the Python name for the wrapper class.
 * @pointer_type: the GType of the pointer type being wrapped.
 * @type: the wrapper class.
 *
 * Registers a wrapper for a pointer type.  The wrapper class will be
 * a subclass of gobject.GPointer, and a reference to the wrapper
 * class will be stored in the provided module dictionary.
 */
void
pyg_register_pointer(PyObject *dict, const gchar *class_name,
		     GType pointer_type, PyTypeObject *type)
{
    PyObject *o;

    g_return_if_fail(dict != NULL);
    g_return_if_fail(class_name != NULL);
    g_return_if_fail(pointer_type != 0);

    if (!pointer_type_id)
      pointer_type_id = g_quark_from_static_string("PyGPointer::class");

    if (!type->tp_dealloc) type->tp_dealloc = (destructor)pyg_pointer_dealloc;

    type->ob_type = &PyType_Type;
    type->tp_base = &PyGPointer_Type;

    if (PyType_Ready(type) < 0) {
	g_warning("could not get type `%s' ready", type->tp_name);
	return;
    }

    PyDict_SetItemString(type->tp_dict, "__gtype__",
			 o=pyg_type_wrapper_new(pointer_type));
    Py_DECREF(o);

    g_type_set_qdata(pointer_type, pointer_type_id, type);

    PyDict_SetItemString(dict, (char *)class_name, (PyObject *)type);
}

/**
 * pyg_pointer_new:
 * @pointer_type: the GType of the pointer value.
 * @pointer: the pointer value.
 *
 * Creates a wrapper for a pointer value.  Since G_TYPE_POINTER types
 * don't register any information about how to copy/free them, there
 * is no guarantee that the pointer will remain valid, and there is
 * nothing registered to release the pointer when the pointer goes out
 * of scope.  This is why we don't recommend people use these types.
 *
 * Returns: the boxed wrapper.
 */
PyObject *
pyg_pointer_new(GType pointer_type, gpointer pointer)
{
    PyGILState_STATE state;
    PyGPointer *self;
    PyTypeObject *tp;
    g_return_val_if_fail(pointer_type != 0, NULL);

    state = pyg_gil_state_ensure();

    if (!pointer) {
	Py_INCREF(Py_None);
	pyg_gil_state_release(state);
	return Py_None;
    }

    tp = g_type_get_qdata(pointer_type, pointer_type_id);
    if (!tp)
	tp = (PyTypeObject *)&PyGPointer_Type; /* fallback */
    self = PyObject_NEW(PyGPointer, tp);

    pyg_gil_state_release(state);

    if (self == NULL)
	return NULL;

    self->pointer = pointer;
    self->gtype = pointer_type;

    return (PyObject *)self;
}
