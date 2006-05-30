#include "Python.h"

#define _ARRAY_MODULE
#include "Numeric/arrayobject.h"
#define _UFUNC_MODULE
#include "Numeric/ufuncobject.h"

/* Table of functions defined in the module */

static PyMethodDef numpy_methods[] = {
    {NULL,  NULL}		/* sentinel */
};

/* Module initialization */

DL_EXPORT(void)
init_numpy(void)
{
    PyObject *m, *d;
    PyObject *c_api;
    static void *PyArray_API[PyArray_API_pointers];
    static void *Py_UFunc_API[PyUFunc_API_pointers];

    /* Create the module and add the functions */
    PyArray_Type.ob_type = &PyType_Type;
    PyUFunc_Type.ob_type = &PyType_Type;

    m = Py_InitModule("_numpy", numpy_methods);
    if (!m) goto err;
    d = PyModule_GetDict(m);
    if (!d) goto err;

    /* Initialize C API pointer arrays and store them in module */
    PyArray_API[PyArray_Type_NUM] = (void *)&PyArray_Type;
    PyArray_API[PyArray_SetNumericOps_NUM] = (void *)&PyArray_SetNumericOps;
    PyArray_API[PyArray_INCREF_NUM] = (void *)&PyArray_INCREF;
    PyArray_API[PyArray_XDECREF_NUM] = (void *)&PyArray_XDECREF;
#if 0
    PyArray_API[PyArrayError_NUM] = (void *)&PyArrayError;
#endif
    PyArray_API[PyArray_SetStringFunction_NUM] =
	(void *)&PyArray_SetStringFunction;
    PyArray_API[PyArray_DescrFromType_NUM] = (void *)&PyArray_DescrFromType;
    PyArray_API[PyArray_Cast_NUM] = (void *)&PyArray_Cast;
    PyArray_API[PyArray_CanCastSafely_NUM] = (void *)&PyArray_CanCastSafely;
    PyArray_API[PyArray_ObjectType_NUM] = (void *)&PyArray_ObjectType;
    PyArray_API[_PyArray_multiply_list_NUM] = (void *)&_PyArray_multiply_list;
    PyArray_API[PyArray_Size_NUM] = (void *)&PyArray_Size;
    PyArray_API[PyArray_FromDims_NUM] = (void *)&PyArray_FromDims;
    PyArray_API[PyArray_FromDimsAndData_NUM] = (void *)&PyArray_FromDimsAndData;
    PyArray_API[PyArray_FromDimsAndDataAndDescr_NUM] = (void *)&PyArray_FromDimsAndDataAndDescr;
    PyArray_API[PyArray_ContiguousFromObject_NUM] =
	(void *)&PyArray_ContiguousFromObject;
    PyArray_API[PyArray_CopyFromObject_NUM] = (void *)&PyArray_CopyFromObject;
    PyArray_API[PyArray_FromObject_NUM] = (void *)&PyArray_FromObject;
    PyArray_API[PyArray_Return_NUM] = (void *)&PyArray_Return;
    PyArray_API[PyArray_Reshape_NUM] = (void *)&PyArray_Reshape;
    PyArray_API[PyArray_Copy_NUM] = (void *)&PyArray_Copy;
    PyArray_API[PyArray_Take_NUM] = (void *)&PyArray_Take;
    PyArray_API[PyArray_Put_NUM] = (void *)&PyArray_Put;
    PyArray_API[PyArray_PutMask_NUM] = (void *)&PyArray_PutMask;
    PyArray_API[PyArray_CopyArray_NUM] = (void *)&PyArray_CopyArray;
    PyArray_API[PyArray_As1D_NUM] = (void *)&PyArray_As1D;
    PyArray_API[PyArray_As2D_NUM] = (void *)&PyArray_As2D;
    PyArray_API[PyArray_Free_NUM] = (void *)&PyArray_Free;
    PyArray_API[PyArray_ValidType_NUM] = (void *)&PyArray_ValidType;
    PyArray_API[PyArray_IntegerAsInt_NUM] = (void *)&PyArray_IntegerAsInt;
    c_api = PyCObject_FromVoidPtr((void *)PyArray_API, NULL);
    if (PyErr_Occurred()) goto err;
    PyDict_SetItemString(d, "_ARRAY_API", c_api);
    Py_DECREF(c_api);
    if (PyErr_Occurred()) goto err;
    Py_UFunc_API[PyUFunc_Type_NUM] = (void *)&PyUFunc_Type;
    Py_UFunc_API[PyUFunc_FromFuncAndData_NUM] = (void *)&PyUFunc_FromFuncAndData;
    Py_UFunc_API[PyUFunc_GenericFunction_NUM] = (void *)&PyUFunc_GenericFunction;
    Py_UFunc_API[PyUFunc_f_f_As_d_d_NUM] = (void *)&PyUFunc_f_f_As_d_d;
    Py_UFunc_API[PyUFunc_d_d_NUM] = (void *)&PyUFunc_d_d;
    Py_UFunc_API[PyUFunc_F_F_As_D_D_NUM] = (void *)&PyUFunc_F_F_As_D_D;
    Py_UFunc_API[PyUFunc_D_D_NUM] = (void *)&PyUFunc_D_D;
    Py_UFunc_API[PyUFunc_O_O_NUM] = (void *)&PyUFunc_O_O;
    Py_UFunc_API[PyUFunc_ff_f_As_dd_d_NUM] = (void *)&PyUFunc_ff_f_As_dd_d;
    Py_UFunc_API[PyUFunc_dd_d_NUM] = (void *)&PyUFunc_dd_d;
    Py_UFunc_API[PyUFunc_FF_F_As_DD_D_NUM] = (void *)&PyUFunc_FF_F_As_DD_D;
    Py_UFunc_API[PyUFunc_DD_D_NUM] = (void *)&PyUFunc_DD_D;
    Py_UFunc_API[PyUFunc_OO_O_NUM] = (void *)&PyUFunc_OO_O;
    Py_UFunc_API[PyUFunc_O_O_method_NUM] = (void *)&PyUFunc_O_O_method;
#if 0
    Py_UFunc_API[PyArray_Map_NUM] = (void *)&PyArray_Map;
#endif
    c_api = PyCObject_FromVoidPtr((void *)Py_UFunc_API, NULL);
    if (PyErr_Occurred()) goto err;
    PyDict_SetItemString(d, "_UFUNC_API", c_api);
    Py_DECREF(c_api);
    if (PyErr_Occurred()) goto err;
    return;
err:
	Py_FatalError("can't initialize module _numpy");
}
