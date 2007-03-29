/*
 * This file is part of python-hildondesktop
 *
 * Copyright (C) 2006 Nokia Corporation.
 *
 * Author:  Lucas Rocha <lucas.rocha@nokia.com>
 * Contact: Karoliina Salminen <karoliina.t.salminen@nokia.com>
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

#include <pygobject.h>
 
void hildondesktop_register_classes (PyObject *d); 
void hildondesktop_add_constants(PyObject *module, const gchar *strip_prefix);

extern PyMethodDef hildondesktop_functions[];

DL_EXPORT(void)
inithildondesktop(void)
{
  PyObject *m, *d;

  init_pygobject ();

  m = Py_InitModule ("hildondesktop", hildondesktop_functions);
  d = PyModule_GetDict (m);

  hildondesktop_register_classes (d);
  hildondesktop_add_constants (m, "HILDON_DESKTOP_");
  
  if (PyErr_Occurred ()) {
      Py_FatalError ("can't initialise module hildondesktop");
  }
}
