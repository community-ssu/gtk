/*
  Python Universal Functions Object -- Math for all types, plus fast arrays math

  Copyright (c) 1995, 1996, 1997 Jim Hugunin, hugunin@mit.edu
  See file COPYING for details.

  Full description

  This supports mathematical (and boolean) functions on arrays and other python
  objects.  Math on large arrays of basic C types is rather efficient.

*/

#include "Python.h"
#if defined(MS_WIN32) || defined(__CYGWIN__)
#undef DL_IMPORT
#define DL_IMPORT(RTYPE) __declspec(dllexport) RTYPE
#endif

#define _ARRAY_MODULE
#include "Numeric/arrayobject.h"
#define _UFUNC_MODULE
#include "Numeric/ufuncobject.h"


/* ----------------------------------------------------- */
#include <errno.h>

#ifdef i860
/* Cray APP has bogus definition of HUGE_VAL in <math.h> */
#undef HUGE_VAL
#endif

/* You might need to define this to get things to work on your machine.
   #define HAVE_FINITE
**********************/

#ifdef HAVE_FINITE
#define CHECK(x) if (errno == 0 && !finite(x)) errno = ERANGE
#else
#ifdef HUGE_VAL
#define CHECK(x) if (errno == 0 && !(-HUGE_VAL <= (x) && (x) <= HUGE_VAL)) errno = ERANGE
#else
#define CHECK(x) /* Don't know how to check */
#endif
#endif

typedef double DoubleBinaryFunc(double x, double y);
typedef Py_complex ComplexBinaryFunc(Py_complex x, Py_complex y);

extern void PyUFunc_ff_f_As_dd_d(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2];
    char *ip1=args[0], *ip2=args[1], *op=args[2];
    int n=dimensions[0];

    for(i=0; i<n; i++, ip1+=is1, ip2+=is2, op+=os) {
	*(float *)op = (float)((DoubleBinaryFunc *)func)((double)*(float *)ip1, (double)*(float *)ip2);
    }
}

extern void PyUFunc_dd_d(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2];
    char *ip1=args[0], *ip2=args[1], *op=args[2];
    int n=dimensions[0];

    for(i=0; i<n; i++, ip1+=is1, ip2+=is2, op+=os) {
	*(double *)op = ((DoubleBinaryFunc *)func)(*(double *)ip1, *(double *)ip2);
    }
}

extern void PyUFunc_FF_F_As_DD_D(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2];
    char *ip1=args[0], *ip2=args[1], *op=args[2];
    int n=dimensions[0];
    Py_complex x, y;

    for(i=0; i<n; i++, ip1+=is1, ip2+=is2, op+=os) {
	x.real = ((float *)ip1)[0]; x.imag = ((float *)ip1)[1];
	y.real = ((float *)ip2)[0]; y.imag = ((float *)ip2)[1];
	x = ((ComplexBinaryFunc *)func)(x, y);
	((float *)op)[0] = (float)x.real;
	((float *)op)[1] = (float)x.imag;
    }
}

extern void PyUFunc_DD_D(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2];
    char *ip1=args[0], *ip2=args[1], *op=args[2];
    int n=dimensions[0];
    Py_complex x,y;

    for(i=0; i<n; i++, ip1+=is1, ip2+=is2, op+=os) {
	x.real = ((double *)ip1)[0]; x.imag = ((double *)ip1)[1];
	y.real = ((double *)ip2)[0]; y.imag = ((double *)ip2)[1];
	x = ((ComplexBinaryFunc *)func)(x, y);
	((double *)op)[0] = x.real;
	((double *)op)[1] = x.imag;
    }
}

extern void PyUFunc_OO_O(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2];
    char *ip1=args[0], *ip2=args[1], *op=args[2];
    int n=dimensions[0];
    PyObject *tmp;
    PyObject *x1, *x2;

    for(i=0; i<n; i++, ip1+=is1, ip2+=is2, op+=os) {
        x1 = *((PyObject **)ip1);
        x2 = *((PyObject **)ip2);
        if ((x1 == NULL) || (x2 == NULL)) return;
        if ( (void *) func == (void *) PyNumber_Power)
            tmp = ((ternaryfunc)func)(x1, x2, Py_None);
        else
	    tmp = ((binaryfunc)func)(*(PyObject **)ip1, *(PyObject **)ip2);
	if (PyErr_Occurred()) return;
	if (*((PyObject **)op) != NULL) {Py_DECREF(*((PyObject **)op));}
	*((PyObject **)op) = tmp;
    }
}



typedef double DoubleUnaryFunc(double x);
typedef Py_complex ComplexUnaryFunc(Py_complex x);

extern void PyUFunc_f_f_As_d_d(char **args, int *dimensions, int *steps, void *func) {
    int i;
    char *ip1=args[0], *op=args[1];
    for(i=0; i<*dimensions; i++, ip1+=steps[0], op+=steps[1]) {
	*(float *)op = (float)((DoubleUnaryFunc *)func)((double)*(float *)ip1);
    }
}

extern void PyUFunc_d_d(char **args, int *dimensions, int *steps, void *func) {
    int i;
    char *ip1=args[0], *op=args[1];
    for(i=0; i<*dimensions; i++, ip1+=steps[0], op+=steps[1]) {
	*(double *)op = ((DoubleUnaryFunc *)func)(*(double *)ip1);
    }
}

extern void PyUFunc_F_F_As_D_D(char **args, int *dimensions, int *steps, void *func) {
    int i; Py_complex x;
    char *ip1=args[0], *op=args[1];
    for(i=0; i<*dimensions; i++, ip1+=steps[0], op+=steps[1]) {
	x.real = ((float *)ip1)[0]; x.imag = ((float *)ip1)[1];
	x = ((ComplexUnaryFunc *)func)(x);
	((float *)op)[0] = (float)x.real;
	((float *)op)[1] = (float)x.imag;
    }
}

extern void PyUFunc_D_D(char **args, int *dimensions, int *steps, void *func) {
    int i; Py_complex x;
    char *ip1=args[0], *op=args[1];
    for(i=0; i<*dimensions; i++, ip1+=steps[0], op+=steps[1]) {
	x.real = ((double *)ip1)[0]; x.imag = ((double *)ip1)[1];
	x = ((ComplexUnaryFunc *)func)(x);
	((double *)op)[0] = x.real;
	((double *)op)[1] = x.imag;
    }
}

extern void PyUFunc_O_O(char **args, int *dimensions, int *steps, void *func) {
    int i; PyObject *tmp;
    char *ip1=args[0], *op=args[1];
    PyObject *x1;

    for(i=0; i<*dimensions; i++, ip1+=steps[0], op+=steps[1]) {
        x1 = *((PyObject **)ip1);
        if (x1 == NULL) return;
        tmp = ((unaryfunc)func)(x1);
	if (*((PyObject **)op) != NULL) {Py_DECREF(*((PyObject **)op));}
	*((PyObject **)op) = tmp;
    }
}

extern void PyUFunc_O_O_method(char **args, int *dimensions, int *steps, void *func) {
    int i; PyObject *tmp, *meth, *arglist;
    char *ip1=args[0], *op=args[1];
    for(i=0; i<*dimensions; i++, ip1+=steps[0], op+=steps[1]) {
	meth = PyObject_GetAttrString(*(PyObject **)ip1, (char *)func);
	if (meth != NULL) {
	    arglist = PyTuple_New(0);
	    tmp = PyEval_CallObject(meth, arglist);
	    Py_DECREF(arglist);
	    if (*((PyObject **)op) != NULL) {Py_DECREF(*((PyObject **)op));}
	    *((PyObject **)op) = tmp;
	    Py_DECREF(meth);
	}
    }
}


/* ---------------------------------------------------------------- */

/****
     so...here I'll do outer, inner, reduce, accumulate and direct implementations of these functions.
     pulling this out of the array code can only be a good thing for aesthetic reasons.
****/
#ifndef max
#define max(x,y) (x)>(y)?(x):(y)
#endif
#ifndef min
#define min(x,y) (x)>(y)?(y):(x)
#endif
#define MAX_DIMS 30
#define MAX_ARGS 10

static int compare_lists(int *l1, int *l2, int n) {
    int i;
    for(i=0;i<n;i++) {
	if (l1[i] != l2[i]) return 0;
    }
    return 1;
}

int get_stride(PyArrayObject *mp, int d) {
    return mp->strides[d];
    /******
	   int i, stride = mp->descr->elsize;
	   for(i=d+1; i<mp->nd; i++) stride *= mp->dimensions[i];
	   return stride;
    ******/
}


static int select_types(PyUFuncObject *self, char *arg_types, void **data, PyUFuncGenericFunction *function) {
    int i=0, j;
    char largest_savespace = 0, real_type;

    for (j=0; j<self->nin; j++) {
	real_type = arg_types[j] & ~((char )SAVESPACEBIT);
	if ((arg_types[j] & SAVESPACEBIT) && (real_type > largest_savespace))
	    largest_savespace = real_type;
    }

    if (largest_savespace == 0) {
	while (i<self->ntypes && arg_types[0] > self->types[i*self->nargs]) i++;
	for(;i<self->ntypes; i++) {
	    for(j=0; j<self->nin; j++) {
		if (!PyArray_CanCastSafely(arg_types[j], self->types[i*self->nargs+j])) break;
	    }
	    if (j == self->nin) break;
	}
	if(i>=self->ntypes) {
	    PyErr_SetString(PyExc_TypeError,
			    "function not supported for these types, and can't coerce to supported types");
	    return -1;
	}
	for(j=0; j<self->nargs; j++)
	    arg_types[j] = (self->types[i*self->nargs+j] & ~((char )SAVESPACEBIT));
    }
    else {
	while(i<self->ntypes && largest_savespace > self->types[i*self->nargs]) i++;
	if (i>=self->ntypes || largest_savespace < self->types[i*self->nargs]) {
	    PyErr_SetString(PyExc_TypeError,
			    "function not supported for the spacesaver array with the largest typecode.");
	    return -1;
	}

	for(j=0; j<self->nargs; j++)  /* Input arguments */
	    arg_types[j] = (self->types[i*self->nargs+j] | SAVESPACEBIT);
    }

    *data = self->data[i];
    *function = self->functions[i];

    return 0;
}

int setup_matrices(PyUFuncObject *self, PyObject *args,  PyUFuncGenericFunction *function, void **data,
		   PyArrayObject **mps, char *arg_types) {
    int nargs, i;

    nargs = PyTuple_Size(args);
    if ((nargs != self->nin) && (nargs != self->nin+self->nout)) {
	PyErr_SetString(PyExc_ValueError, "invalid number of arguments");
	return -1;
    }

    /* Determine the types of the input arguments. */
    for(i=0; i<self->nin; i++) {
	arg_types[i] = (char)PyArray_ObjectType(PyTuple_GET_ITEM(args, i), 0);
	if (PyArray_Check(PyTuple_GET_ITEM(args,i)) &&
	    PyArray_ISSPACESAVER(PyTuple_GET_ITEM(args,i)))
	    arg_types[i] |= SAVESPACEBIT;
    }

    /* Select an appropriate function for these argument types. */
    if (select_types(self, arg_types, data, function) == -1) return -1;

    /* Coerce input arguments to the right types. */
    for(i=0; i<self->nin; i++) {
	if ((mps[i] = (PyArrayObject *)PyArray_FromObject(PyTuple_GET_ITEM(args,
									   i),
							  arg_types[i], 0, 0)) == NULL) {
	    return -1;
	}
    }

    /* Check the return arguments, and INCREF. */
    for(i = self->nin;i<nargs; i++) {
	mps[i] = (PyArrayObject *)PyTuple_GET_ITEM(args, i);
	Py_INCREF(mps[i]);
	if (!PyArray_Check((PyObject *)mps[i])) {
	    PyErr_SetString(PyExc_TypeError, "return arrays must be of arraytype");
	    return -1;
	}
	if (mps[i]->descr->type_num != (arg_types[i] & ~((char )SAVESPACEBIT))) {
	    PyErr_SetString(PyExc_TypeError, "return array has incorrect type");
	    return -1;
	}
    }

    return nargs;
}

int setup_return(PyUFuncObject *self, int nd, int *dimensions, int steps[MAX_DIMS][MAX_ARGS],
		 PyArrayObject **mps, char *arg_types) {
    int i, j;


    /* Initialize the return matrices, or check if provided. */
    for(i=self->nin; i<self->nargs; i++) {
	if (mps[i] == NULL) {
	    if ((mps[i] = (PyArrayObject *)PyArray_FromDims(nd, dimensions,
							    arg_types[i])) == NULL)
		return -1;
	} else {
	    if (mps[i]->nd < nd || !compare_lists(mps[i]->dimensions, dimensions, nd)) {
		PyErr_SetString(PyExc_ValueError, "invalid return array shape");
		return -1;
	    }
	}
	for(j=0; j<mps[i]->nd; j++) {
	    steps[j][i] = get_stride(mps[i], j+mps[i]->nd-nd);
	}
	/* Small hack to keep purify happy (no UMR's for 0d array's) */
	if (mps[i]->nd == 0) steps[0][i] = 0;
    }
    return 0;
}

int optimize_loop(int steps[MAX_DIMS][MAX_ARGS], int *loop_n, int n_loops) {
    int j, tmp;

#define swap(x, y) tmp = (x), (x) = (y), (y) = tmp

    /* Here should go some code to "compress" the loops. */

    if (n_loops > 1 && (loop_n[n_loops-1] < loop_n[n_loops-2]) ) {
	swap(loop_n[n_loops-1], loop_n[n_loops-2]);
	for (j=0; j<MAX_ARGS; j++) {
	    swap(steps[n_loops-1][j], steps[n_loops-2][j]);
	}
    }
    return n_loops;

#undef swap
}


int setup_loop(PyUFuncObject *self, PyObject *args, PyUFuncGenericFunction *function, void **data,
	       int steps[MAX_DIMS][MAX_ARGS], int *loop_n, PyArrayObject **mps) {
    int i, j, nargs, nd, n_loops, tmp;
    int dimensions[MAX_DIMS];
    char arg_types[MAX_ARGS];

    nargs = setup_matrices(self, args, function, data, mps, arg_types);
    if (nargs < 0) return -1;

    /* The return matrices will have the same number of dimensions as the largest input array. */
    for(i=0, nd=0; i<self->nin; i++) nd = max(nd, mps[i]->nd);

    /* Setup the loop. This can be optimized later. */
    n_loops = 0;

    for(i=0; i<nd; i++) {
	dimensions[i] = 1;
	for(j=0; j<self->nin; j++) {
	    if (i + mps[j]->nd-nd  >= 0) tmp = mps[j]->dimensions[i + mps[j]->nd-nd];
	    else tmp = 1;

	    if (tmp == 1) {
		steps[n_loops][j] = 0;
	    } else {
		if (dimensions[i] == 1) dimensions[i] = tmp;
		else if (dimensions[i] != tmp) {
		    PyErr_SetString(PyExc_ValueError, "frames are not aligned");
		    return -1;
		}
		steps[n_loops][j] = get_stride(mps[j], i + mps[j]->nd-nd);
	    }
	}
	loop_n[n_loops] = dimensions[i];
	n_loops++;
    }

    /* Small hack to keep purify happy (no UMR's for 0d array's) */
    if (nd == 0) {
	for(j=0; j<self->nin; j++) steps[0][j] = 0;
    }

    if (setup_return(self, nd, dimensions, steps, mps, arg_types) == -1) return -1;

    n_loops = optimize_loop(steps, loop_n, n_loops);

    return n_loops;
}

static void math_error(void) {
    if (errno == EDOM)
	PyErr_SetString(PyExc_ValueError, "math domain error");
    else if (errno == ERANGE)
	PyErr_SetString(PyExc_OverflowError, "math range error");
    else
	PyErr_SetString(PyExc_ValueError, "unexpected math error");
}

/* Get rid of this.  Why is this needed?

void check_array(PyArrayObject *ap) {
    double *data;
    int i, n;

    if (ap->descr->type_num == PyArray_DOUBLE || ap->descr->type_num == PyArray_CDOUBLE) {
	data = (double *)ap->data;
	n = PyArray_Size((PyObject *)ap);
	if (ap->descr->type_num == PyArray_CDOUBLE) n *= 2;

	for(i=0; i<n; i++) CHECK(data[i]);
    }
}

*/

int PyUFunc_GenericFunction(PyUFuncObject *self, PyObject *args, PyArrayObject **mps) {
    int steps[MAX_DIMS][MAX_ARGS];
    int i, loop, n_loops, loop_i[MAX_DIMS], loop_n[MAX_DIMS];
    char *pointers[MAX_ARGS], *resets[MAX_DIMS][MAX_ARGS];
    void *data;
    PyUFuncGenericFunction function;

    if (self == NULL) {
	PyErr_SetString(PyExc_ValueError, "function not supported");
	return -1;
    }

    n_loops = setup_loop(self, args, &function, &data, steps, loop_n, mps);
    if (n_loops == -1) return -1;

    for(i=0; i<self->nargs; i++) pointers[i] = mps[i]->data;

    errno = 0;
    if (n_loops == 0) {
	n_loops = 1;
	function(pointers, &n_loops, steps[0], data);
    } else {
	/* This is the inner loop to actually do the computation. */
	loop=-1;
	while(1) {
	    while (loop < n_loops-2) {
		loop++;
		loop_i[loop] = 0;
		for(i=0; i<self->nin+self->nout; i++) { resets[loop][i] = pointers[i]; }
	    }

	    function(pointers, loop_n+(n_loops-1), steps[n_loops-1], data);

	    while (loop >= 0 && !(++loop_i[loop] < loop_n[loop]) && loop >= 0) loop--;
	    if (loop < 0) break;
	    for(i=0; i<self->nin+self->nout; i++) { pointers[i] = resets[loop][i] + steps[loop][i]*loop_i[loop]; }
	}
    }
    if (PyErr_Occurred()) return -1;

    if (self->check_return && errno != 0) {math_error(); return -1;}

    return 0;
}

PyObject * PyUFunc_GenericReduction(PyUFuncObject *self, PyObject *args, PyObject *kwds, int accumulate) {
    int steps[MAX_DIMS][MAX_ARGS];
    int i, j, loop, n_loops, loop_i[MAX_DIMS], loop_n[MAX_DIMS];
    char *pointers[MAX_ARGS], *resets[MAX_DIMS][MAX_ARGS], *dptr, *d;
    void *data;
    PyUFuncGenericFunction function;
    int i0, dimension;
    char arg_types[MAX_ARGS];
    PyArrayObject *mp, *ret;
    PyObject *op;
    long zero=0;
    int one = 1;
    PyArrayObject *indices;
    static char *kwlist[] = {"array", "axis", NULL};


    if (self == NULL) {
	PyErr_SetString(PyExc_ValueError, "function not supported");
	return NULL;
    }

    dimension = 0;
    if(!PyArg_ParseTupleAndKeywords(args, kwds, "O|i", kwlist,
                                    &op, &dimension)) return NULL;


    /* Determine the types of the input arguments. */
    arg_types[0] = (char)PyArray_ObjectType(PyTuple_GET_ITEM(args, 0), 0);
    arg_types[1] = arg_types[0];

    /* Select an appropriate function for these argument types. */
    if (select_types(self, arg_types, &data, &function) == -1) return NULL;

    if (!((arg_types[2]==arg_types[0]) && (arg_types[2]==arg_types[1]))) {
	PyErr_SetString(PyExc_ValueError, "reduce only supported for "\
			"functions with identical input and output types.");
	return NULL;
    }


    /* Coerce input arguments to the right types. */
    if ((mp = (PyArrayObject *)PyArray_FromObject(op, arg_types[0], 0, 0)) ==
	NULL) return NULL;

    if (dimension < 0) dimension += mp->nd;
    if (dimension < 0 || dimension >= mp->nd) {
	PyErr_SetString(PyExc_ValueError, "dimension not in array");
	return NULL;
    }

    /*Deal with 0 size arrays by returning the appropriate identity*/
    if (mp->dimensions[dimension] == 0) {
	if (self->identity == PyUFunc_None) {
	    PyErr_SetString(PyExc_ValueError,
			    "zero size array to ufunc without identity");
	    return NULL;
	}
	if (self->identity == PyUFunc_One) {
	    d = mp->descr->one;
	} else {
	    d = mp->descr->zero;
	}

	for(j=0, i=0; i<mp->nd; i++) {
	    if (i != dimension) loop_i[j++] = mp->dimensions[i];
	}

	ret = (PyArrayObject *)PyArray_FromDims(mp->nd-1, loop_i,
						mp->descr->type_num);
	j = mp->descr->elsize;
	dptr = ret->data;

	for(i=0; i<PyArray_SIZE(ret); i++) {
	    memmove(dptr, d, j);
	    dptr += j;
	}
	Py_DECREF(mp);
	return PyArray_Return(ret);
    }


    if (accumulate) {
	if ((ret = (PyArrayObject *)PyArray_Copy(mp)) == NULL) return NULL;
    } else {
	indices = (PyArrayObject *)PyArray_FromDimsAndData(1, &one, PyArray_LONG, (char *)&zero);
	if ((ret = (PyArrayObject *)PyArray_Take((PyObject *)mp, (PyObject
								  *)indices,
						 dimension)) ==
	    NULL) return NULL;
	Py_DECREF(indices);

	ret->nd -= 1;
	for(i=dimension; i < ret->nd; i++) {
	    ret->dimensions[i] = ret->dimensions[i+1];
	    ret->strides[i] = ret->strides[i+1];
	}
    }

    if (mp->dimensions[dimension] == 1) {
	Py_DECREF(mp);
	return PyArray_Return(ret);
    }

    /* Setup the loop. This can be optimized later. */
    n_loops = mp->nd;

    for(j=0, i0=0; j<n_loops; j++) {
	loop_n[j] = mp->dimensions[j];
	if (j == dimension) loop_n[j]--;

	if (j != dimension || accumulate) {
	    steps[j][0] = get_stride(ret, i0);
	    i0++;
	} else {
	    steps[j][0] = 0;
	}
	steps[j][1] = get_stride(mp, j);
	steps[j][2] = steps[j][0];
    }

    pointers[0] = ret->data;
    pointers[1] = mp->data+steps[dimension][1];
    pointers[2] = ret->data+steps[dimension][2];

    if (n_loops == 0) {
	PyErr_SetString(PyExc_ValueError, "can't reduce");
	return NULL;
    }

    /* This is the inner loop to actually do the computation. */
    loop=-1;
    while(1) {
	while (loop < n_loops-2) {
	    loop++;
	    loop_i[loop] = 0;
	    for(i=0; i<self->nin+self->nout; i++) { resets[loop][i] = pointers[i]; }
	}

	function(pointers, loop_n+(n_loops-1), steps[n_loops-1], data);

	while (loop >= 0 && !(++loop_i[loop] < loop_n[loop]) && loop >= 0) loop--;
	if (loop < 0) break;
	for(i=0; i<self->nin+self->nout; i++) { pointers[i] = resets[loop][i] + steps[loop][i]*loop_i[loop]; }
    }

    Py_DECREF(mp);

    /* Cleanup the returned matrices so that scalars will be returned as
       python scalars */

    if (PyErr_Occurred ()) {
	Py_DECREF(ret);
	return NULL;
    }

    return PyArray_Return(ret);
}

PyObject * PyUFunc_GenericReduceAt(PyUFuncObject *self, PyObject *args, int accumulate) {
    int steps[MAX_DIMS][MAX_ARGS];
    int i, j, loop, n_loops, loop_i[MAX_DIMS], loop_n[MAX_DIMS];
    char *pointers[MAX_ARGS], *resets[MAX_DIMS][MAX_ARGS];
    void *data;
    PyUFuncGenericFunction function;
    int i0, ni, os=1, cnt;
    char arg_types[MAX_ARGS];
    PyArrayObject *mp, *ret;
    PyObject *op, *op1;
    long *ip;

    if (self == NULL) {
	PyErr_SetString(PyExc_ValueError, "function not supported");
	return NULL;
    }

    mp = ret = NULL;
    if(!PyArg_ParseTuple(args, "OO", &op, &op1)) return NULL;

    if (PyArray_As1D(&op1, (char **)&ip, &ni, PyArray_LONG) == -1) return NULL;

    /* Determine the types of the input arguments. */
    arg_types[0] = (char)PyArray_ObjectType(op, 0);
    arg_types[1] = arg_types[0];

    /* Select an appropriate function for these argument types. */
    if (select_types(self, arg_types, &data, &function) == -1) goto fail;

    if (!((arg_types[2]==arg_types[0]) && (arg_types[2]==arg_types[1]))) {
	PyErr_SetString(PyExc_ValueError, "reduce only supported for "\
			"functions with identical input and output types.");
	return NULL;
    }


    /* Coerce input arguments to the right types. */
    if ((mp = (PyArrayObject *)PyArray_FromObject(op, arg_types[0], 0, 0)) ==
	NULL) goto fail;

    if (accumulate) {
	if ((ret = (PyArrayObject *)PyArray_Copy(mp)) == NULL) goto fail;
    } else {
	if ((ret = (PyArrayObject *)PyArray_Take((PyObject *)mp, op1, -1))
	    == NULL) goto
			 fail;

    }

    /* Setup the loop. This can be optimized later. */
    n_loops = mp->nd;

    /* Check to be sure that the indices are legal. */
    for(i=0; i<ni; i++) {
	if (ip[i] < 0 || ip[i] >= mp->dimensions[mp->nd-1]) {
	    PyErr_SetString(PyExc_IndexError, "invalid index to reduceAt");
	    goto fail;
	}
    }

    for(j=0, i0=0; j<n_loops; j++) {
	loop_n[j] = mp->dimensions[j];
	/* if (j == dimension) loop_n[j]--; Not relevant for reduceAt */

	if (j != mp->nd-1 || accumulate) {
	    steps[j][0] = get_stride(ret, i0);
	    i0++;
	} else {
	    steps[j][0] = 0;
	}
	os = get_stride(ret, i0);
	steps[j][1] = get_stride(mp, j);
	steps[j][2] = steps[j][0];
    }

    pointers[0] = ret->data;
    pointers[1] = mp->data+steps[mp->nd-1][1];
    pointers[2] = ret->data+steps[mp->nd-1][2];

    if (n_loops == 0) {
	PyErr_SetString(PyExc_ValueError, "can't reduce");
	goto fail;
    }

    /* This is the inner loop to actually do the computation. */
    loop=-1;
    while(1) {
	while (loop < n_loops-2) {
	    loop++;
	    loop_i[loop] = 0;
	    for(i=0; i<self->nin+self->nout; i++) { resets[loop][i] = pointers[i]; }
	}

	cnt = ip[0]-1;
	for(i=0; i<ni; i++) {
	    pointers[1] += (cnt+1)*steps[n_loops-1][1];
	    if (i < ni-1) {
		cnt = ip[i+1]-ip[i]-1;
	    } else {
		cnt = loop_n[n_loops-1]-ip[i]-1;
	    }
	    function(pointers, &cnt, steps[n_loops-1], data);
	    pointers[0] += os;
	    pointers[2] += os;
	}

	while (loop >= 0 && !(++loop_i[loop] < loop_n[loop]) && loop >= 0) loop--;
	if (loop < 0) break;
	for(i=0; i<self->nin+self->nout; i++) { pointers[i] = resets[loop][i] + steps[loop][i]*loop_i[loop]; }
    }

    PyArray_Free(op1, (char *)ip);
    Py_DECREF(mp);


    if (PyErr_Occurred ()) {
	Py_DECREF(ret);
	return NULL;
    }

    return PyArray_Return(ret);

 fail:
    PyArray_Free(op1, (char *)ip);
    Py_XDECREF(mp);
    Py_XDECREF(ret);
    return NULL;
}



/* ---------- */

static PyObject *ufunc_generic_call(PyUFuncObject *self, PyObject *args) {
    int i;
    PyTupleObject *ret;
    PyArrayObject *mps[MAX_ARGS];

    /* Initialize all array objects to NULL to make cleanup easier if something goes wrong. */
    for(i=0; i<self->nargs; i++) mps[i] = NULL;

    if (PyUFunc_GenericFunction(self, args, mps) == -1) {
	for(i=0; i<self->nargs; i++) {
            Py_XDECREF(mps[i]);
        }
	return NULL;
    }

    for(i=0; i<self->nin; i++) Py_DECREF(mps[i]);

    if (self->nout == 1) {
	return PyArray_Return(mps[self->nin]);
    } else {
	ret = (PyTupleObject *)PyTuple_New(self->nout);
	for(i=0; i<self->nout; i++) {
	    PyTuple_SET_ITEM(ret, i, PyArray_Return(mps[i+self->nin]));
	}
	return (PyObject *)ret;
    }
}

PyObject *PyUFunc_FromFuncAndData(PyUFuncGenericFunction *func, void **data, char *types, int ntypes,
				  int nin, int nout, int identity, char *name, char *doc, int check_return) {
    PyUFuncObject *self;

    if ((self = PyObject_NEW(PyUFuncObject, &PyUFunc_Type)) == NULL) return NULL;

    self->nin = nin;
    self->nout = nout;
    self->nargs = nin+nout;
    self->identity = identity;

    self->functions = func;
    self->data = data;
    self->types = types;
    self->ntypes = ntypes;
    self->attributes = 0;
    /* if (self->nin == 2) self->attributes = 1; */

    self->ranks = NULL;

    if (name == NULL) self->name = "?";
    else self->name = name;

    self->doc = doc;

    self->check_return = check_return;

    return (PyObject *)self;
}

static void
ufunc_dealloc(PyUFuncObject *self)
{
    if (self->ranks != NULL) free(self->ranks);
    PyObject_DEL(self);
}

static int
ufunc_compare(PyUFuncObject *v, PyUFuncObject *w)
{
    return -1;
}

static PyObject *
ufunc_repr(PyUFuncObject *self)
{
    char buf[100];

    sprintf(buf, "<ufunc '%.50s'>", self->name);

    return PyString_FromString(buf);
}

static PyObject *
ufunc_call(PyUFuncObject *self, PyObject *args)
{
    return ufunc_generic_call(self, args);
}

/* -------------------------------------------------------- */

PyObject *ufunc_outer(PyUFuncObject *self, PyObject *args) {
    int i;
    PyObject *ret;
    PyArrayObject *ap1, *ap2, *ap_new;
    PyObject *new_args, *tmp;
    int dimensions[MAX_DIMS];

    if(self->nin != 2) {
	PyErr_SetString(PyExc_ValueError,
			"outer product only supported for binary functions");
	return NULL;
    }

    if (PySequence_Length(args) != 2) {
	PyErr_SetString(PyExc_ValueError,
			"exactly two arguments expected");
	return NULL;
    }

    tmp = PySequence_GetItem(args, 0);
    if (tmp == NULL) return NULL;
    ap1 = (PyArrayObject *)PyArray_ContiguousFromObject(tmp, PyArray_NOTYPE, 0, 0);
    Py_DECREF(tmp);
    if (ap1 == NULL) return NULL;

    tmp = PySequence_GetItem(args, 1);
    if (tmp == NULL) return NULL;
    ap2 = (PyArrayObject *)PyArray_FromObject(tmp, PyArray_NOTYPE, 0, 0);
    Py_DECREF(tmp);
    if (ap2 == NULL) return NULL;

    memmove((char *)dimensions, ap1->dimensions, ap1->nd*sizeof(int));
    for(i=0;i<ap2->nd; i++) {
	dimensions[ap1->nd+i] = 1;
    }
    ap_new = (PyArrayObject *)PyArray_FromDims(ap1->nd+ap2->nd, dimensions,
					       ap1->descr->type_num);
    memmove(ap_new->data, ap1->data, PyArray_NBYTES(ap1));

    new_args = Py_BuildValue("(OO)", ap_new, ap2);
    Py_DECREF(ap1);
    Py_DECREF(ap2);
    Py_DECREF(ap_new);

    ret = ufunc_generic_call(self, new_args);
    Py_DECREF(new_args);
    return ret;
}


PyObject *ufunc_reduce(PyUFuncObject *self, PyObject *args, PyObject *kwds) {
    if (self->nin != 2) {
	PyErr_SetString(PyExc_ValueError,
			"reduce only supported for binary functions");
	return NULL;
    }
    if (self->nout != 1) {
	PyErr_SetString(PyExc_ValueError,
			"reduce only supported for functions returning a single value");
	return NULL;
    }

    return PyUFunc_GenericReduction(self, args, kwds, 0);
}

PyObject *ufunc_accumulate(PyUFuncObject *self, PyObject *args, PyObject *kwds) {
    if (self->nin != 2) {
	PyErr_SetString(PyExc_ValueError,
			"accumulate only supported for binary functions");
	return NULL;
    }
    if (self->nout != 1) {
	PyErr_SetString(PyExc_ValueError,
			"accumulate only supported for functions returning a single value");
	return NULL;
    }

    return PyUFunc_GenericReduction(self, args, kwds, 1);
}

PyObject *ufunc_reduceAt(PyUFuncObject *self, PyObject *args) {
    if (self->nin != 2) {
	PyErr_SetString(PyExc_ValueError,
			"reduceAt only supported for binary functions");
	return NULL;
    }
    if (self->nout != 1) {
	PyErr_SetString(PyExc_ValueError,
			"reduceAt only supported for functions returning a single value");
	return NULL;
    }

    return PyUFunc_GenericReduceAt(self, args, 0);
}


static struct PyMethodDef ufunc_methods[] = {
    {"reduce",  (PyCFunction)ufunc_reduce, METH_VARARGS | METH_KEYWORDS},
    {"accumulate",  (PyCFunction)ufunc_accumulate, METH_VARARGS | METH_KEYWORDS},
    {"reduceat",  (PyCFunction)ufunc_reduceAt, METH_VARARGS},

    {"outer", (PyCFunction)ufunc_outer, METH_VARARGS},
    {NULL,		NULL}		/* sentinel */
};


static PyObject *
ufunc_getattr(PyUFuncObject *self, char *name)
{
    if (strcmp(name, "__doc__") == 0) {
	char *doc = self->doc;
	if (doc != NULL)
	    return PyString_FromString(doc);
	Py_INCREF(Py_None);
	return Py_None;
    }
    /* XXXX Add your own getattr code here */
    return Py_FindMethod(ufunc_methods, (PyObject *)self, name);
}

static int
ufunc_setattr(PyUFuncObject *self, char *name, PyObject *v)
{
    /* XXXX Add your own setattr code here */
    return -1;
}

static char Ofunctype__doc__[] =
"Optimized functions make it possible to implement arithmetic with matrices efficiently"
;

PyTypeObject PyUFunc_Type = {
    PyObject_HEAD_INIT(0)
    0,				/*ob_size*/
    "ufunc",			/*tp_name*/
    sizeof(PyUFuncObject),		/*tp_basicsize*/
    0,				/*tp_itemsize*/
    /* methods */
    (destructor)ufunc_dealloc,	/*tp_dealloc*/
    (printfunc)0,		/*tp_print*/
    (getattrfunc)ufunc_getattr,	/*tp_getattr*/
    (setattrfunc)ufunc_setattr,	/*tp_setattr*/
    (cmpfunc)ufunc_compare,		/*tp_compare*/
    (reprfunc)ufunc_repr,		/*tp_repr*/
    0,			/*tp_as_number*/
    0,		/*tp_as_sequence*/
    0,		/*tp_as_mapping*/
    (hashfunc)0,		/*tp_hash*/
    (ternaryfunc)ufunc_call,		/*tp_call*/
    (reprfunc)ufunc_repr,		/*tp_str*/

    /* Space for future expansion */
    0L,0L,0L,0L,
    Ofunctype__doc__ /* Documentation string */
};

/* End of code for ufunc objects */
/* -------------------------------------------------------- */
