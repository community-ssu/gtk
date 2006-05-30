/*
  Python Multiarray Module -- A useful collection of functions for creating and
  operating on array objects.  In an ideal world this would be called
  arraymodule.c, but this can't be used due to conflits with the existing
  python array module (which this should replace).


  Copyright (c) 1995, 1996, 1997 Jim Hugunin, hugunin@mit.edu
  See file COPYING for details.

*/

#include "Python.h"
#include <string.h>
#include <math.h>

#include "Numeric/arrayobject.h"

static PyObject *MultiArrayError;

/*Sets the maximum number of dimensions in an array to 40.
  If you ever need to change this I'd love to know more about your arrays.
*/
#define MAX_DIMS 30
static int compare_lists(int *l1, int *l2, int n) {
    int i;
    for(i=0;i<n;i++) {
	if (l1[i] != l2[i]) return 0;
    }
    return 1;
}

/*op is a python object supporting the sequence interface.
  Its elements will be concatenated together to form a single
  multidimensional array.*/
extern PyObject *PyArray_Concatenate(PyObject *op) {
    PyArrayObject *ret, **mps;
    PyObject *otmp;
    int i, n, type_num, tmp, nd=0, new_dim;
    char *data;

    n = PySequence_Length(op);
    if (n == -1) {
	return NULL;
    }
    if (n == 0) {
	PyErr_SetString(PyExc_ValueError,
			"Concatenation of zero-length tuples is impossible.");
	return NULL;
    }

    ret = NULL;

    mps = (PyArrayObject **)malloc(n*sizeof(PyArrayObject *));
    if (mps == NULL) {
	PyErr_SetString(PyExc_MemoryError, "memory error");
	return NULL;
    }

    /* Make sure these arrays are legal to concatenate. */
    /* Must have same dimensions except d0, and have coercible type. */

    type_num = 0;
    for(i=0; i<n; i++) {
	otmp = PySequence_GetItem(op, i);
	type_num = PyArray_ObjectType(otmp, type_num);
	mps[i] = NULL;
	Py_XDECREF(otmp);
    }
    if (type_num == -1) {
	PyErr_SetString(PyExc_TypeError,
			"can't find common type for arrays to concatenate");
	goto fail;
    }

    for(i=0; i<n; i++) {
	if ((otmp = PySequence_GetItem(op, i)) == NULL) goto fail;
	mps[i] = (PyArrayObject*)
	    PyArray_ContiguousFromObject(otmp, type_num, 0, 0);
	Py_DECREF(otmp);
    }

    new_dim = 0;
    for(i=0; i<n; i++) {
	if (mps[i] == NULL) goto fail;
	if (i == 0) nd = mps[i]->nd;
	else {
	    if (nd != mps[i]->nd) {
		PyErr_SetString(PyExc_ValueError,
				"arrays must have same number of dimensions");
		goto fail;
	    }
	    if (!compare_lists(mps[0]->dimensions+1, mps[i]->dimensions+1, nd-1)) {
		PyErr_SetString(PyExc_ValueError,
				"array dimensions must agree except for d_0");
		goto fail;
	    }
	}
	if (nd == 0) {
	    PyErr_SetString(PyExc_ValueError,
			    "0d arrays can't be concatenated");
	    goto fail;
	}
	new_dim += mps[i]->dimensions[0];
    }

    tmp = mps[0]->dimensions[0];
    mps[0]->dimensions[0] = new_dim;
    ret = (PyArrayObject *)PyArray_FromDims(nd, mps[0]->dimensions,
					    type_num);
    mps[0]->dimensions[0] = tmp;

    if (ret == NULL) goto fail;

    data = ret->data;
    for(i=0; i<n; i++) {
	memmove(data, mps[i]->data, PyArray_NBYTES(mps[i]));
	data += PyArray_NBYTES(mps[i]);
    }

    PyArray_INCREF(ret);
    for(i=0; i<n; i++) Py_XDECREF(mps[i]);
    free(mps);
    return (PyObject *)ret;

 fail:
    Py_XDECREF(ret);
    for(i=0; i<n; i++) Py_XDECREF(mps[i]);
    free(mps);
    return NULL;
}

/* Check whether the given array is stored contiguously (row-wise) in
   memory. */
static int array_really_contiguous(PyArrayObject *ap) {
    int sd;
    int i;

    sd = ap->descr->elsize;
    for (i = ap->nd-1; i >= 0; --i) {
	/* contiguous by definition */
	if (ap->dimensions[i] == 0) return 1;

	if (ap->strides[i] != sd) return 0;
	sd *= ap->dimensions[i];
    }
    return 1;
}

/* Changed to be able to deal with non-contiguous arrays. */
extern PyObject *PyArray_Transpose(PyArrayObject *ap, PyObject *op) {
    long *axes, axis;
    int i, n;
    int *permutation = NULL;
    PyArrayObject *ret = NULL;

    if (op == Py_None) {
      n = ap->nd;
      permutation = (int *)malloc(n*sizeof(int));
      for(i=0; i<n; i++)
	permutation[i] = n-1-i;
    } else {
      if (PyArray_As1D(&op, (char **)&axes, &n, PyArray_LONG) == -1)
	return NULL;

      permutation = (int *)malloc(n*sizeof(int));

      for(i=0; i<n; i++) {
	axis = axes[i];
	if (axis < 0) axis = ap->nd+axis;
	if (axis < 0 || axis >= ap->nd) {
	  PyErr_SetString(PyExc_ValueError,
			  "invalid axis for this array");
	  goto fail;
	}
	permutation[i] = axis;
      }
    }

    /* this allocates memory for dimensions and strides (but fills them
       incorrectly), sets up descr, and points data at ap->data. */
    ret = (PyArrayObject *)PyArray_FromDimsAndData(n, permutation,
						   ap->descr->type_num,
						   ap->data);
    if (ret == NULL) goto fail;

    /* point at true owner of memory: */
    ret->base = (PyObject *)ap;
    Py_INCREF(ap);

    for(i=0; i<n; i++) {
	ret->dimensions[i] = ap->dimensions[permutation[i]];
	ret->strides[i] = ap->strides[permutation[i]];
    }
    if (array_really_contiguous(ret)) ret->flags |= CONTIGUOUS;
    else ret->flags &= ~CONTIGUOUS;

    if (op != Py_None)
      PyArray_Free(op, (char *)axes);
    free(permutation);
    return (PyObject *)ret;

 fail:
    Py_XDECREF(ret);
    if (permutation != NULL) free(permutation);
    if (op != Py_None)
      PyArray_Free(op, (char *)axes);
    return NULL;
}

extern PyObject *PyArray_Repeat(PyObject *aop, PyObject *op, int axis) {
    long *counts;
    int n, n_outer, i, j, k, chunk, total, tmp;
    PyArrayObject *ret=NULL, *ap;
    char *new_data, *old_data;

    ap = (PyArrayObject *)PyArray_ContiguousFromObject(aop,
						       PyArray_NOTYPE, 0, 0);

    if (axis < 0) axis = ap->nd+axis;
    if (axis < 0 || axis >= ap->nd) {
	PyErr_SetString(PyExc_ValueError, "axis is invalid");
	return NULL;
    }

    if (PyArray_As1D(&op, (char **)&counts, &n, PyArray_LONG) == -1)
	return NULL;

    if (n != ap->dimensions[axis]) {
	PyErr_SetString(PyExc_ValueError,
			"len(n) != a.shape[axis]");
	goto fail;
    }

    total = 0;
    for(j=0; j<n; j++) {
	if (counts[j] == -1) {
	    PyErr_SetString(PyExc_ValueError, "count < 0");
	    goto fail;
	}
	total += counts[j];
    }
    tmp = ap->dimensions[axis];
    ap->dimensions[axis] = total;
    ret = (PyArrayObject *)PyArray_FromDims(ap->nd, ap->dimensions,
					    ap->descr->type_num);
    ap->dimensions[axis] = tmp;

    if (ret == NULL) goto fail;

    new_data = ret->data;
    old_data = ap->data;

    chunk = ap->descr->elsize;
    for(i=axis+1; i<ap->nd; i++) {
	chunk *= ap->dimensions[i];
    }

    n_outer = 1;
    for(i=0; i<axis; i++) n_outer *= ap->dimensions[i];

    for(i=0; i<n_outer; i++) {
	for(j=0; j<n; j++) {
	    for(k=0; k<counts[j]; k++) {
		memmove(new_data, old_data, chunk);
		new_data += chunk;
	    }
	    old_data += chunk;
	}
    }

    PyArray_INCREF(ret);
    Py_XDECREF(ap);
    PyArray_Free(op, (char *)counts);

    return (PyObject *)ret;

 fail:
    Py_XDECREF(ap);
    Py_XDECREF(ret);
    PyArray_Free(op, (char *)counts);
    return NULL;
}


extern PyObject *PyArray_Choose(PyObject *ip, PyObject *op) {
    int i, n, *sizes, m, offset, elsize, type_num;
    char *ret_data;
    PyArrayObject **mps, *ap, *ret;
    PyObject *otmp;
    long *self_data, mi;
    ap = NULL;
    ret = NULL;

    n = PySequence_Length(op);

    mps = (PyArrayObject **)malloc(n*sizeof(PyArrayObject *));
    if (mps == NULL) {
	PyErr_SetString(PyExc_MemoryError, "memory error");
	return NULL;
    }

    sizes = (int *)malloc(n*sizeof(int));

    /* Figure out the right type for the new array */

    type_num = 0;
    for(i=0; i<n; i++) {
	otmp = PySequence_GetItem(op, i);
	type_num = PyArray_ObjectType(otmp, type_num);
	mps[i] = NULL;
	Py_XDECREF(otmp);
    }
    if (type_num == -1) {
	PyErr_SetString(PyExc_TypeError,
			"can't find common type for arrays to choose from");
	goto fail;
    }

    /* Make sure all arrays are real array objects. */
    for(i=0; i<n; i++) {
	if ((otmp = PySequence_GetItem(op, i)) == NULL)
	    goto fail;
	mps[i] = (PyArrayObject*)
	    PyArray_ContiguousFromObject(otmp, type_num,
					 0, 0);
	Py_DECREF(otmp);
    }

    ap = (PyArrayObject *)PyArray_ContiguousFromObject(ip,
						       PyArray_LONG, 0, 0);
    if (ap == NULL) goto fail;

    /* Check the dimensions of the arrays */
    for(i=0; i<n; i++) {
	if (mps[i] == NULL) goto fail;
	if (ap->nd < mps[i]->nd) {
	    PyErr_SetString(PyExc_ValueError,
			    "too many dimensions");
	    goto fail;
	}
	if (!compare_lists(ap->dimensions+(ap->nd-mps[i]->nd),
			   mps[i]->dimensions, mps[i]->nd)) {
	    PyErr_SetString(PyExc_ValueError,
			    "array dimensions must agree");
	    goto fail;
	}
	sizes[i] = PyArray_NBYTES(mps[i]);
    }

    ret = (PyArrayObject *)PyArray_FromDims(ap->nd, ap->dimensions,
					    type_num);
    if (ret == NULL) goto fail;

    elsize = ret->descr->elsize;
    m = PyArray_SIZE(ret);
    self_data = (long *)ap->data;
    ret_data = ret->data;

    for (i=0; i<m; i++) {
	mi = *self_data;
	if (mi < 0 || mi >= n) {
	    PyErr_SetString(PyExc_ValueError,
			    "invalid entry in choice array");
	    goto fail;
	}
	offset = i*elsize;
	if (offset >= sizes[mi]) {offset = offset % sizes[mi]; }
	memmove(ret_data, mps[mi]->data+offset, elsize);
	ret_data += elsize; self_data++;
    }

    PyArray_INCREF(ret);
    for(i=0; i<n; i++) Py_XDECREF(mps[i]);
    Py_DECREF(ap);
    free(mps);
    free(sizes);

    return (PyObject *)ret;

 fail:
    for(i=0; i<n; i++) Py_XDECREF(mps[i]);
    Py_XDECREF(ap);
    free(mps);
    free(sizes);
    Py_XDECREF(ret);
    return NULL;
}

#define COMPARE(fname, type) \
int fname(type *ip1, type *ip2) { return *ip1 < *ip2 ? -1 : *ip1 == *ip2 ? 0 : 1; }

COMPARE(DOUBLE_compare, double)
COMPARE(FLOAT_compare, float)
COMPARE(SHORT_compare, short)
COMPARE(UNSIGNEDSHORT_compare, unsigned short)
COMPARE(INT_compare, int)
COMPARE(UINT_compare, int)
COMPARE(LONG_compare, long)
COMPARE(BYTE_compare, signed char)
COMPARE(UNSIGNEDBYTE_compare, unsigned char)

int OBJECT_compare(PyObject **ip1, PyObject **ip2) {
    return PyObject_Compare(*ip1, *ip2);
}

typedef int (*CompareFunction) (const void *, const void *);

static CompareFunction compare_functions[] =
{
    NULL,
    (CompareFunction)UNSIGNEDBYTE_compare,
    (CompareFunction)BYTE_compare,
    (CompareFunction)SHORT_compare,
    (CompareFunction)UNSIGNEDSHORT_compare,
    (CompareFunction)INT_compare,
    (CompareFunction)UINT_compare,
    (CompareFunction)LONG_compare,
    (CompareFunction)FLOAT_compare,
    (CompareFunction)DOUBLE_compare,
    NULL,
    NULL,
    (CompareFunction)OBJECT_compare
};

extern PyObject *PyArray_Sort(PyObject *op) {
    PyArrayObject *ap;
    CompareFunction compare_func;
    char *ip;
    int i, n, m, elsize;

    ap = (PyArrayObject *)PyArray_CopyFromObject(op, PyArray_NOTYPE,
						 1, 0);
    if (ap == NULL) return NULL;

    compare_func = compare_functions[ap->descr->type_num];
    if (compare_func == NULL) {
	PyErr_SetString(PyExc_TypeError,
			"compare not supported for type");
	Py_XDECREF(ap);
	return NULL;
    }

    elsize = ap->descr->elsize;
    m = ap->dimensions[ap->nd-1];
    if (m == 0) {
	return PyArray_Return(ap);
    }
    n = PyArray_SIZE(ap)/m;
    for (ip = ap->data, i=0; i<n; i++, ip+=elsize*m) {
	qsort(ip, m, elsize, compare_func);
    }

    return PyArray_Return(ap);

}

/* Using these pseudo global varibales can not possibly be a good idea, but... */

static CompareFunction argsort_compare_func;
static char *argsort_data;
static int argsort_elsize;

static int argsort_static_compare(long *ip1, long *ip2) {
    return argsort_compare_func(argsort_data+(argsort_elsize * *ip1),
				argsort_data+(argsort_elsize * *ip2));
}

extern PyObject *PyArray_ArgSort(PyObject *op) {
    PyArrayObject *ap, *ret;
    long *ip;
    int i, j, n, m;

    ap = (PyArrayObject *)PyArray_ContiguousFromObject(op, PyArray_NOTYPE, 1, 0);
    if (ap == NULL) return NULL;

    ret = (PyArrayObject *)PyArray_FromDims(ap->nd, ap->dimensions, PyArray_LONG);
    if (ret == NULL) goto fail;

    argsort_compare_func = compare_functions[ap->descr->type_num];
    if (argsort_compare_func == NULL) {
	PyErr_SetString(PyExc_TypeError, "compare not supported for type");
	goto fail;
    }

    ip = (long *)ret->data;
    argsort_elsize = ap->descr->elsize;
    m = ap->dimensions[ap->nd-1];
    if (m == 0) {
	Py_XDECREF(ap);
	return PyArray_Return(ret);
    }
    n = PyArray_SIZE(ap)/m;
    argsort_data = ap->data;
    for (i=0; i<n; i++, ip+=m, argsort_data += m*argsort_elsize) {
	for(j=0; j<m; j++) ip[j] = j;
	qsort((char *)ip, m, sizeof(long),
	      (CompareFunction)argsort_static_compare);
    }

    Py_DECREF(ap);
    return PyArray_Return(ret);

 fail:
    Py_XDECREF(ap);
    Py_XDECREF(ret);
    return NULL;

}

static long local_where(char *ip, char *vp, int elsize, int elements, CompareFunction compare) {
    long min_i, max_i, i;
    int location;

    min_i = 0;
    max_i = elements;

    while (min_i != max_i) {
	i = (max_i-min_i)/2 + min_i;
	location = compare(ip, vp+elsize*i);
	if (location == 0) {
	    while (i > 0) {
		if (compare(ip, vp+elsize*(--i)) != 0) {
		    i = i+1; break;
		}
	    }
	    return i;
	}
	if (location < 0) {
	    max_i = i;
	} else {
	    min_i = i+1;
	}
    }
    return min_i;
}

extern PyObject *PyArray_BinarySearch(PyObject *op1, PyObject *op2) {
    PyArrayObject *ap1, *ap2, *ret;
    int m, n, i, elsize;
    char *ip;
    long *rp;
    CompareFunction compare_func;
    int typenum = 0;

    typenum = PyArray_ObjectType(op1, 0);
    typenum = PyArray_ObjectType(op2, typenum);
    ret = NULL;
    ap1 = (PyArrayObject *)PyArray_ContiguousFromObject(op1, typenum, 1, 1);
    if (ap1 == NULL) return NULL;
    ap2 = (PyArrayObject *)PyArray_ContiguousFromObject(op2, typenum, 0, 0);
    if (ap2 == NULL) goto fail;

    ret = (PyArrayObject *)PyArray_FromDims(ap2->nd, ap2->dimensions,
					    PyArray_LONG);
    if (ret == NULL) goto fail;

    compare_func = compare_functions[ap2->descr->type_num];
    if (compare_func == NULL) {
	PyErr_SetString(PyExc_TypeError, "compare not supported for type");
	goto fail;
    }

    elsize = ap1->descr->elsize;
    m = ap1->dimensions[ap1->nd-1];
    n = PyArray_Size((PyObject *)ap2);

    for (rp = (long *)ret->data, ip=ap2->data, i=0; i<n; i++, ip+=elsize, rp++) {
	*rp = local_where(ip, ap1->data, elsize, m, compare_func);
    }

    Py_DECREF(ap1);
    Py_DECREF(ap2);
    return PyArray_Return(ret);

 fail:
    Py_XDECREF(ap1);
    Py_XDECREF(ap2);
    Py_XDECREF(ret);
    return NULL;
}


/* Rather than build a generic inner product, this is just dot product. */

#define DOT_PRODUCT(name, number) \
	static void name(char *ip1, int is1, char *ip2, int is2, char *op, int n) { \
	number tmp=(number)0.0;  int i; \
	for(i=0;i<n;i++,ip1+=is1,ip2+=is2) { \
    tmp += *((number *)ip1) * *((number *)ip2); \
} \
	*((number *)op) = tmp; \
}

DOT_PRODUCT(SHORT_DotProduct, short)
DOT_PRODUCT(UNSIGNEDSHORT_DotProduct, unsigned short)
DOT_PRODUCT(INT_DotProduct, int)
DOT_PRODUCT(UINT_DotProduct, int)
DOT_PRODUCT(LONG_DotProduct, long)
DOT_PRODUCT(FLOAT_DotProduct, float)
DOT_PRODUCT(DOUBLE_DotProduct, double)

#define CDOT_PRODUCT(name, number) \
	static void name(char *ip1, int is1, char *ip2, int is2, char *op, int n) { \
	number tmpr=(number)0.0, tmpi=(number)0.0;  int i; \
	for(i=0;i<n;i++,ip1+=is1,ip2+=is2) { \
    tmpr += ((number *)ip1)[0] * ((number *)ip2)[0] - ((number *)ip1)[1] * ((number *)ip2)[1]; \
    tmpi += ((number *)ip1)[1] * ((number *)ip2)[0] + ((number *)ip1)[0] * ((number *)ip2)[1]; \
} \
	((number *)op)[0] = tmpr; ((number *)op)[1] = tmpi;\
}

     CDOT_PRODUCT(CFLOAT_DotProduct, float)
     CDOT_PRODUCT(CDOUBLE_DotProduct, double)

     static void OBJECT_DotProduct(char *ip1, int is1, char *ip2, int is2,
				   char *op, int n) {
	 int i;
	 PyObject *tmp1, *tmp2, *tmp=NULL;
	 PyObject **tmp3;
	 for(i=0;i<n;i++,ip1+=is1,ip2+=is2) {
	     tmp1 = PyNumber_Multiply(*((PyObject **)ip1), *((PyObject **)ip2));
	     if (!tmp1) { Py_XDECREF(tmp); return;}
	     if (i == 0) {
		 tmp = tmp1;
	     } else {
		 tmp2 = PyNumber_Add(tmp, tmp1);
		 Py_XDECREF(tmp);
		 Py_XDECREF(tmp1);
		 if (!tmp2) return;
		 tmp = tmp2;
	     }
	 }
         tmp3 = (PyObject**) op;
         tmp2 = *tmp3;
	 *((PyObject **)op) = tmp;
	 Py_XDECREF(tmp2);
     }

typedef void (DotFunction) (char *, int, char *, int, char *, int);

static DotFunction *matrixMultiplyFunctions[] = {
    NULL,
    NULL,
    NULL,
    SHORT_DotProduct,
    UNSIGNEDSHORT_DotProduct,
    INT_DotProduct,
    UINT_DotProduct,
    LONG_DotProduct,
    FLOAT_DotProduct,
    DOUBLE_DotProduct,
    CFLOAT_DotProduct,
    CDOUBLE_DotProduct,
    OBJECT_DotProduct
};

extern PyObject *PyArray_InnerProduct(PyObject *op1, PyObject *op2) {
    PyArrayObject *ap1, *ap2, *ret;
    int i, j, l, i1, i2, n1, n2;
    int typenum;
    int is1, is2, os;
    char *ip1, *ip2, *op;
    int dimensions[MAX_DIMS], nd;
    DotFunction *dot;

    typenum = PyArray_ObjectType(op1, 0);
    typenum = PyArray_ObjectType(op2, typenum);


    ret = NULL;
    ap1 = (PyArrayObject *)PyArray_ContiguousFromObject(op1, typenum, 0, 0);
    if (ap1 == NULL) return NULL;
    ap2 = (PyArrayObject *)PyArray_ContiguousFromObject(op2, typenum, 0, 0);
    if (ap2 == NULL) goto fail;

    if (ap1->nd == 0 || ap2->nd == 0) {
	PyErr_SetString(PyExc_TypeError, "scalar arguments not allowed");
	goto fail;
    }

    l = ap1->dimensions[ap1->nd-1];

    if (ap2->dimensions[ap2->nd-1] != l) {
	PyErr_SetString(PyExc_ValueError, "matrices are not aligned");
	goto fail;
    }

    if (l == 0) n1 = n2 = 0;
    else {
	n1 = PyArray_SIZE(ap1)/l;
	n2 = PyArray_SIZE(ap2)/l;
    }

    nd = ap1->nd+ap2->nd-2;
    j = 0;
    for(i=0; i<ap1->nd-1; i++) {
	dimensions[j++] = ap1->dimensions[i];
    }
    for(i=0; i<ap2->nd-1; i++) {
	dimensions[j++] = ap2->dimensions[i];
    }

    ret = (PyArrayObject *)PyArray_FromDims(nd, dimensions, typenum);
    if (ret == NULL) goto fail;

    dot = matrixMultiplyFunctions[(int)(ret->descr->type_num)];
    if (dot == NULL) {
	PyErr_SetString(PyExc_ValueError,
			"matrixMultiply not available for this type");
	goto fail;
    }

    is1 = ap1->strides[ap1->nd-1]; is2 = ap2->strides[ap2->nd-1];
    op = ret->data; os = ret->descr->elsize;

    ip1 = ap1->data;
    for(i1=0; i1<n1; i1++) {
	ip2 = ap2->data;
	for(i2=0; i2<n2; i2++) {
	    dot(ip1, is1, ip2, is2, op, l);
	    ip2 += is2*l;
	    op += os;
	}
	ip1 += is1*l;
    }


    Py_DECREF(ap1);
    Py_DECREF(ap2);
    return PyArray_Return(ret);

 fail:
    Py_XDECREF(ap1);
    Py_XDECREF(ap2);
    Py_XDECREF(ret);
    return NULL;
}

/* just like inner product but does the swapaxes stuff on the fly */
extern PyObject *PyArray_MatrixProduct(PyObject *op1, PyObject *op2) {
    PyArrayObject *ap1, *ap2, *ret;
    int i, j, l, i1, i2, n1, n2;
    int typenum;
    int is1, is2, os;
    char *ip1, *ip2, *op;
    int dimensions[MAX_DIMS], nd;
    DotFunction *dot;
    int matchDim, otherDim, is2r, is1r;

    typenum = PyArray_ObjectType(op1, 0);
    typenum = PyArray_ObjectType(op2, typenum);


    ret = NULL;
    ap1 = (PyArrayObject *)PyArray_ContiguousFromObject(op1, typenum, 0, 0);
    if (ap1 == NULL) return NULL;
    ap2 = (PyArrayObject *)PyArray_ContiguousFromObject(op2, typenum, 0, 0);
    if (ap2 == NULL) goto fail;

    if (ap1->nd == 0 || ap2->nd == 0) {
      /* handle the use of dot with scalers here */
	PyErr_SetString(PyExc_TypeError, "scalar arguments not allowed");
	goto fail;
    }

    l = ap1->dimensions[ap1->nd-1];
    if (ap2->nd > 1) {
      matchDim = ap2->nd - 2;
      otherDim = ap2->nd - 1;
    }
    else {
      matchDim = 0;
      otherDim = 0;
    }

    /* fprintf(stderr, "ap1->nd=%d ap2->nd=%d\n", ap1->nd, ap2->nd); */
    if (ap2->dimensions[matchDim] != l) {
	PyErr_SetString(PyExc_ValueError, "matrices are not aligned");
	goto fail;
    }

    if (l == 0) n1 = n2 = 0;
    else {
	n1 = PyArray_SIZE(ap1)/l;
	n2 = PyArray_SIZE(ap2)/l;
    }

    nd = ap1->nd+ap2->nd-2;
    j = 0;
    for(i=0; i<ap1->nd-1; i++) {
	dimensions[j++] = ap1->dimensions[i];
    }
    for(i=0; i<ap2->nd-2; i++) {
	dimensions[j++] = ap2->dimensions[i];
    }
    if(ap2->nd > 1) {
      dimensions[j++] = ap2->dimensions[ap2->nd-1];
    }
    /* fprintf(stderr, "nd=%d dimensions=", nd);
    for(i=0; i<j; i++)
      fprintf(stderr, "%d ", dimensions[i]);
    fprintf(stderr, "\n"); */

    ret = (PyArrayObject *)PyArray_FromDims(nd, dimensions, typenum);
    if (ret == NULL) goto fail;

    dot = matrixMultiplyFunctions[(int)(ret->descr->type_num)];
    if (dot == NULL) {
	PyErr_SetString(PyExc_ValueError,
			"matrixMultiply not available for this type");
	goto fail;
    }

    is1 = ap1->strides[ap1->nd-1]; is2 = ap2->strides[matchDim];
    if(ap1->nd > 1)
      is1r = ap1->strides[ap1->nd-2];
    else
      is1r = ap1->strides[ap1->nd-1];
    is2r = ap2->strides[otherDim];
    /* fprintf(stderr, "n1=%d n2=%d is1=%d is2=%d\n", n1, n2, is1, is2); */
    op = ret->data; os = ret->descr->elsize;

    ip1 = ap1->data;
    for(i1=0; i1<n1; i1++) {
	ip2 = ap2->data;
	for(i2=0; i2<n2; i2++) {
	    dot(ip1, is1, ip2, is2, op, l);
	    ip2 += is2r;
	    op += os;
	}
	ip1 += is1r;
    }


    Py_DECREF(ap1);
    Py_DECREF(ap2);
    return PyArray_Return(ret);

 fail:
    Py_XDECREF(ap1);
    Py_XDECREF(ap2);
    Py_XDECREF(ret);
    return NULL;
}

extern PyObject *PyArray_fastCopyAndTranspose(PyObject *op) {
  PyArrayObject *ap=NULL, *ret=NULL;
  int typenum, nd;
  int t;

  typenum = PyArray_ObjectType(op, 0);

  ap = (PyArrayObject *)PyArray_ContiguousFromObject(op, typenum, 0, 0);
  nd = ap->nd;

  if(nd == 1) {
    return PyArray_Copy(ap);
  }

  /* swap the dimensions and strides so that the copy will transpose */
  t = ap->strides[0];
  ap->strides[0] = ap->strides[1];
  ap->strides[1] = t;
  t = ap->dimensions[0];
  ap->dimensions[0] = ap->dimensions[1];
  ap->dimensions[1] = t;
  /* create the copy and transposing */
  ret = (PyArrayObject*)PyArray_Copy(ap);
  /* swap them back */
  t = ap->strides[0];
  ap->strides[0] = ap->strides[1];
  ap->strides[1] = t;
  t = ap->dimensions[0];
  ap->dimensions[0] = ap->dimensions[1];
  ap->dimensions[1] = t;

  /* PyArray_Free(tmp); */

  Py_DECREF(ap);
  return PyArray_Return(ret);
}

extern PyObject *PyArray_Correlate(PyObject *op1, PyObject *op2, int mode) {
    PyArrayObject *ap1, *ap2, *ret;
    int length;
    int i, n1, n2, n, n_left, n_right;
    int typenum;
    int is1, is2, os;
    char *ip1, *ip2, *op;
    DotFunction *dot;

    typenum = PyArray_ObjectType(op1, 0);
    typenum = PyArray_ObjectType(op2, typenum);

    ret = NULL;
    ap1 = (PyArrayObject *)PyArray_ContiguousFromObject(op1, typenum, 1, 1);
    if (ap1 == NULL) return NULL;
    ap2 = (PyArrayObject *)PyArray_ContiguousFromObject(op2, typenum, 1, 1);
    if (ap2 == NULL) goto fail;

    n1 = ap1->dimensions[ap1->nd-1];
    n2 = ap2->dimensions[ap2->nd-1];

    if (n1 < n2) { ret = ap1; ap1 = ap2; ap2 = ret; ret = NULL; i = n1;n1=n2;n2=i;}
    length = n1;
    n = n2;
    switch(mode) {
    case 0:
	length = length-n+1;
	n_left = n_right = 0;
	break;
    case 1:
	n_left = (int)(n/2);
	n_right = n-n_left-1;
	break;
    case 2:
	n_right = n-1;
	n_left = n-1;
	length = length+n-1;
	break;
    default:
	PyErr_SetString(PyExc_ValueError,
			"mode must be 0,1, or 2");
	goto fail;
    }

    ret = (PyArrayObject *)PyArray_FromDims(1, &length, typenum);
    if (ret == NULL) goto fail;


    dot = matrixMultiplyFunctions[(int)(ret->descr->type_num)];
    if (dot == NULL) {
	PyErr_SetString(PyExc_ValueError,
			"function not available for this type");
	goto fail;
    }

    is1 = ap1->strides[ap1->nd-1]; is2 = ap2->strides[ap2->nd-1];
    op = ret->data; os = ret->descr->elsize;

    ip1 = ap1->data; ip2 = ap2->data+n_left*is2;
    n = n-n_left;
    for(i=0; i<n_left; i++) {
	dot(ip1, is1, ip2, is2, op, n);
	n++;
	ip2 -= is2;
	op += os;
    }
    for(i=0; i<(n1-n2+1); i++) {
	dot(ip1, is1, ip2, is2, op, n);
	ip1 += is1;
	op += os;
    }
    for(i=0; i<n_right; i++) {
	n--;
	dot(ip1, is1, ip2, is2, op, n);
	ip1 += is1;
	op += os;
    }
    Py_DECREF(ap1);
    Py_DECREF(ap2);
    return PyArray_Return(ret);

 fail:
    Py_XDECREF(ap1);
    Py_XDECREF(ap2);
    Py_XDECREF(ret);
    return NULL;
}


typedef int (*ArgFunction) (void*, long, long*);

#define ARGFUNC(fname, type, op) \
	int fname(type *ip, long n, long *ap) { long i; type mp=ip[0]; *ap=0; \
for(i=1; i<n; i++) { if (ip[i] op mp) { mp = ip[i]; *ap=i; } } return 0;}

ARGFUNC(DOUBLE_argmax, double, >)
ARGFUNC(FLOAT_argmax, float, >)
ARGFUNC(LONG_argmax, long, >)
ARGFUNC(INT_argmax, int, >)
ARGFUNC(UINT_argmax, int, >)
ARGFUNC(SHORT_argmax, short, >)
ARGFUNC(UNSIGNEDSHORT_argmax, unsigned short, >)
ARGFUNC(BYTE_argmax, signed char, >)
ARGFUNC(UNSIGNEDBYTE_argmax, unsigned char, >)

int OBJECT_argmax(PyObject **ip, long n, long *ap) {
    long i; PyObject *mp=ip[0]; *ap=0;
    for(i=1; i<n; i++) {
	    if (PyObject_Compare(ip[i],mp) > 0) { mp = ip[i]; *ap=i; }
    }
    return 0;
}

static ArgFunction argmax_functions[] = {
    NULL,
    (ArgFunction) UNSIGNEDBYTE_argmax,
    (ArgFunction) BYTE_argmax,
    (ArgFunction)SHORT_argmax,
    (ArgFunction)UNSIGNEDSHORT_argmax,
    (ArgFunction)INT_argmax,
    (ArgFunction)UINT_argmax,
    (ArgFunction)LONG_argmax,
    (ArgFunction)FLOAT_argmax,
    (ArgFunction)DOUBLE_argmax,
    NULL,
    NULL,
    (ArgFunction)OBJECT_argmax
};

extern PyObject *PyArray_ArgMax(PyObject *op) {
    PyArrayObject *ap, *rp;
    ArgFunction arg_func;
    char *ip;
    int i, n, m, elsize;

    rp = NULL;
    ap = (PyArrayObject *)PyArray_ContiguousFromObject(op, PyArray_NOTYPE, 1, 0);
    if (ap == NULL) return NULL;

    arg_func = argmax_functions[ap->descr->type_num];
    if (arg_func == NULL) {
	PyErr_SetString(PyExc_TypeError, "type not ordered");
	goto fail;
    }
    rp = (PyArrayObject *)PyArray_FromDims(ap->nd-1, ap->dimensions, PyArray_LONG);
    if (rp == NULL) goto fail;


    elsize = ap->descr->elsize;
    m = ap->dimensions[ap->nd-1];
    if (m == 0) {
	PyErr_SetString(MultiArrayError, "Attempt to get argmax/argmin of an empty sequence??");
	goto fail;
    }
    n = PyArray_SIZE(ap)/m;
    for (ip = ap->data, i=0; i<n; i++, ip+=elsize*m) {
	arg_func(ip, m, ((long *)rp->data)+i);
    }
    Py_DECREF(ap);
    return PyArray_Return(rp);

 fail:
    Py_DECREF(ap);
    Py_XDECREF(rp);
    return NULL;
}


/*************
	      ARGFUNC(DOUBLE_argmin, double, <)
	      ARGFUNC(FLOAT_argmin, float, <)
	      ARGFUNC(LONG_argmin, long, <)
	      ARGFUNC(INT_argmin, int, <)
	      ARGFUNC(UINT_argmin, int, <)
	      ARGFUNC(SHORT_argmin, short, <)
              ARGFUNC(UNSIGNEDSHORT_argmin, unsigned short, <)

	      int OBJECT_argmin(PyObject **ip, long n, PyObject **mp, long *ap) {
	      long i; *mp=ip[0]; *ap=0;
	      for(i=1; i<n; i++) {
	      if (PyObject_Compare(ip[i],*mp) < 0) { *mp = ip[i]; *ap=i; }
	      }
	      }
********/


static char doc_array[] = "array(sequence, typecode=None, copy=1, savespace=0) will return a new array formed from the given (potentially nested) sequence with type given by typecode.  If no typecode is given, then the type will be determined as the minimum type required to hold the objects in sequence.  If copy is zero and sequence is already an array, a reference will be returned.  If savespace is nonzero, the new array will maintain its precision in operations.";

static PyObject *
array_array(PyObject *ignored, PyObject *args, PyObject *kws)
{
    PyObject *op, *ret=NULL, *tpo=Py_None;
    static char *kwd[]= {"data", "typecode", "copy", "savespace", NULL};
    char *tp=NULL;
    int copy=1;
    int savespace=0;
    int type;

    if(!PyArg_ParseTupleAndKeywords(args, kws, "O|Oii", kwd, &op, &tpo,
				    &copy, &savespace)) return NULL;

    if (tpo == Py_None) {
	type = PyArray_NOTYPE;
    } else {
	tp = PyString_AsString(tpo);
	if ((tp == NULL) || (PyString_Size(tpo) > 1) ) {
	    PyErr_SetString(PyExc_TypeError, "typecode argument must be a valid type.");
	    return NULL;
	}
	if (tp[0] == 0) type = PyArray_NOTYPE;
	else {
            type = tp[0];
            if (!PyArray_ValidType(type)) {
                PyErr_SetString(PyExc_TypeError, "typecode argument must be a valid type.");
                return NULL;
            }
        }
    }

    /* fast exit if simple call */
    if (PyArray_Check(op) && (copy==0) &&
	(savespace==PyArray_ISSPACESAVER(op))) {
	if ((type == PyArray_NOTYPE) ||
	    (type == ((PyArrayObject *)op)->descr->type_num )) {
	    Py_INCREF(op);
	    return op;
	}
    }

    if (savespace != 0)
	type |= SAVESPACEBIT;

    if (copy) {
	if ((ret = PyArray_CopyFromObject(op, type, 0, 0)) == NULL)
	    return NULL;
    } else {
	if ((ret = PyArray_FromObject(op, type, 0, 0)) == NULL)
	    return NULL;
    }

    if (savespace |= 0 || (PyArray_Check(op) && PyArray_ISSPACESAVER(op)))
	((PyArrayObject *)ret)->flags |= SAVESPACE;

    return ret;
}

static char doc_empty[] = "empty((d1,...,dn),typecode='l',savespace=0) will return a new array\n"\
    "of shape (d1,...,dn) and given type with all its entries uninitialized.  If savespace is\n" \
    "nonzero, the array will be a spacesaver array.  This can be faster than zeros.";

static PyObject *array_empty(PyObject *ignored, PyObject *args, PyObject *kwds) {

    PyObject *sequence;
    char type='l';
    int savespace=0;
    static char *kwlist[] = {"shape", "typecode", "savespace", NULL};
    PyObject *op;
    PyArray_Descr *descr;
    int i, nd, n, dims[MAX_DIMS];
    int sd;
    char *data;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O|ci", kwlist,
                                     &sequence, &type, &savespace)) {
	return NULL;
    }

    if ((descr = PyArray_DescrFromType((int)type)) == NULL)
        return NULL;

    if ((nd=PySequence_Length(sequence)) == -1) {
	PyErr_Clear();
	nd = 1;
	dims[0] = PyArray_IntegerAsInt(sequence);
        if (PyErr_Occurred()) return NULL;
    } else {
	if (nd > MAX_DIMS) {
	    fprintf(stderr, "Maximum number of dimensions = %d\n", MAX_DIMS);
	    PyErr_SetString(PyExc_ValueError, "Number of dimensions is too large");
	    return NULL;
	}
	for(i=0; i<nd; i++) {
	    if( (op=PySequence_GetItem(sequence,i))) {
		dims[i] = PyArray_IntegerAsInt(op);
		Py_DECREF(op);
	    }
	    if(PyErr_Occurred()) return NULL;
	}
    }

    sd = descr->elsize;
    for(i=nd-1;i>=0;i--)
    {
	if (dims[i] < 0) {
	    PyErr_SetString(PyExc_ValueError, "negative dimensions are not allowed");
            return NULL;
        }
        sd *= dims[i] ? dims[i] : 1;
    }
    /* Make sure we're alligned on ints. */
    sd += sizeof(int) - sd%sizeof(int);

    /* allocate memory */
    if ((data = (char *)malloc(sd)) == NULL) {
        PyErr_SetString(PyExc_MemoryError, "can't allocate memory for array");
        return NULL;
    }

    if ((op = PyArray_FromDimsAndDataAndDescr(nd, dims, descr, data))==NULL)
        return NULL;

    /* we own the data (i.e. this informs the delete function to free the memory) */
    ((PyArrayObject *)op)->flags |= OWN_DATA;
    if (savespace) ((PyArrayObject *)op)->flags |= SAVESPACE;

    if (descr->type_num == PyArray_OBJECT) {
	    PyObject **optr;
	    /* Fill it with Py_None */
	    n = PyArray_SIZE(((PyArrayObject *)op));
	    optr = (PyObject **)((PyArrayObject *)op)->data;
	    for(i=0; i<n; i++) {
		    Py_INCREF(Py_None);
		    *optr++ = Py_None;
	    }
    }

    return op;
}

static char doc_zeros[] = "zeros((d1,...,dn),typecode='l',savespace=0) will return a new array of shape (d1,...,dn) and type typecode with all it's entries initialized to zero.  If savespace is nonzero the array will be a spacesaver array.";

static PyObject *array_zeros(PyObject *ignored, PyObject *args, PyObject *kwds) {
    PyObject *op, *sequence, *tpo=Py_None;
    PyArrayObject *ret;
    char type_char='l';
    char *type = &type_char, *dptr;
    int i, nd, n, dimensions[MAX_DIMS];
    int savespace=0;
    static char all_zero[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    static char *kwlist[] = {"shape", "typecode", "savespace", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O|Oi", kwlist,
                                     &sequence, &tpo, &savespace)) {
	return NULL;
    }
    if (tpo != Py_None) {
	type = PyString_AsString(tpo);
	if (!type)
	    return NULL;
	if (!*type)
	    type = &type_char;
    }

    if ((nd=PySequence_Length(sequence)) == -1) {
	PyErr_Clear();
	nd = 1;
	dimensions[0] = PyArray_IntegerAsInt(sequence);
	if (PyErr_Occurred()) return NULL;
    } else {
	if (nd > MAX_DIMS) {
	    fprintf(stderr, "Maximum number of dimensions = %d\n", MAX_DIMS);
	    PyErr_SetString(PyExc_ValueError, "Number of dimensions is too large");
	    return NULL;
	}
	for(i=0; i<nd; i++) {
	    if( (op=PySequence_GetItem(sequence,i))) {
		dimensions[i] = PyArray_IntegerAsInt(op);
		Py_DECREF(op);
	    }
	    if(PyErr_Occurred()) return NULL;
	}
    }
    if ((ret = (PyArrayObject *)PyArray_FromDims(nd, dimensions, *type)) ==
	NULL) return NULL;

    if (memcmp(ret->descr->zero, all_zero, ret->descr->elsize) == 0) {
	memset(ret->data,0,PyArray_Size((PyObject *)ret)*ret->descr->elsize);
    } else {
	dptr = ret->data;
	n = PyArray_SIZE(ret);
	for(i=0; i<n; i++) {
	    memmove(dptr, ret->descr->zero, ret->descr->elsize);
	    dptr += ret->descr->elsize;
	}
    }
    PyArray_INCREF(ret);
    if (savespace) ret->flags |= SAVESPACE;
    return (PyObject *)ret;
}

static char doc_fromString[] = "fromstring(string, typecode='l', count=-1) returns a new 1d array initialized from the raw binary data in string.  If count is positive, the new array will have count elements, otherwise it's size is determined by the size of string.";

static PyObject *array_fromString(PyObject *ignored, PyObject *args, PyObject *keywds)
{
    PyArrayObject *ret;
    char *type_char = "l";
    char *type;
    char *data;
    int s, n;
    PyObject* nobj;
    PyArray_Descr *descr;
    static char *kwlist[] = {"string", "typecode", "count", NULL};
    nobj = (PyObject *) 0;
    type = type_char;
    n = -1;

    if (!PyArg_ParseTupleAndKeywords(args, keywds, "s#|si", kwlist, &data, &s, &type, &n)) {
	    return NULL;
    }

    if ((descr = PyArray_DescrFromType(*type)) == NULL) {
	return NULL;
    }

    if (n < 0 ) {
	if (s % descr->elsize != 0) {
	    PyErr_SetString(PyExc_ValueError, "string size must be a multiple of element size");
	    return NULL;
	}
	n = s/descr->elsize;
    } else {
	if (s < n*descr->elsize) {
	    PyErr_SetString(PyExc_ValueError, "string is smaller than requested size");
	    return NULL;
	}
    }

    if ((ret = (PyArrayObject *)PyArray_FromDims(1, &n, *type)) == NULL)
	return NULL;

    memmove(ret->data, data, n*ret->descr->elsize);
    PyArray_INCREF(ret);
    return (PyObject *)ret;
}

static char doc_take[] = "take(a, indices, axis=0).  Selects the elements in indices from array a along the given axis.";

static PyObject *array_take(PyObject *dummy, PyObject *args, PyObject *kwds) {
    int dimension;
    PyObject *a, *indices, *ret;
    static char *kwlist[] = {"array", "indices", "axis", NULL};

    dimension=0;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "OO|i", kwlist,
                                     &a, &indices, &dimension))
	return NULL;

    ret = PyArray_Take(a, indices, dimension);
    return ret;
}

static char doc_put[] = "put(a, indices, values) sets a.flat[n] = v[n] for each n in indices. v can be scalar or shorter than indices, will repeat.";

static PyObject *array_put(PyObject *dummy, PyObject *args) {
    PyObject *a, *indices, *ret, *values;

    if (!PyArg_ParseTuple(args, "OOO", &a, &indices, &values))
	return NULL;
    ret = PyArray_Put(a, indices, values);
    return ret;
}

static char doc_putmask[] = "putmask(a, mask, values) sets a.flat[n] = v[n] for each n where mask.flat[n] is true. v can be scalar.";

static PyObject *array_putmask(PyObject *dummy, PyObject *args) {
    PyObject *a, *mask, *ret, *values;

    if (!PyArg_ParseTuple(args, "OOO", &a, &mask, &values))
	return NULL;
    ret = PyArray_PutMask(a, mask, values);
    return ret;
}

static char doc_reshape[] = "reshape(a, (d1, d2, ..., dn)).  Change the shape of a to be an n-dimensional array with dimensions given by d1...dn.  Note: the size specified for the new array must be exactly equal to the size of the  old one or an error will occur.";

static PyObject *array_reshape(PyObject *dummy, PyObject *args) {
    PyObject *shape, *ret, *a0;
    PyArrayObject *a;

    if (!PyArg_ParseTuple(args, "OO", &a0, &shape)) return NULL;
    if ((a = (PyArrayObject *)PyArray_ContiguousFromObject(a0,
							   PyArray_NOTYPE,0,0))
	==NULL) return NULL;

    ret = PyArray_Reshape(a, shape);
    Py_DECREF(a);
    return ret;
}


static char doc_concatenate[] = "concatenate((a1,a2,...)).";

static PyObject *array_concatenate(PyObject *dummy, PyObject *args) {
    PyObject *a0;

    if (!PyArg_ParseTuple(args, "O", &a0)) return NULL;
    return PyArray_Concatenate(a0);
}


static char doc_transpose[] = "transpose(a, axes=None)";

static PyObject *array_transpose(PyObject *dummy, PyObject *args, PyObject *kwds) {
    PyObject *shape=Py_None, *ret, *a0;
    PyArrayObject *a;
    static char *kwlist[] = {"array", "axes", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O|O", kwlist,
                                     &a0, &shape)) return NULL;
    if ((a = (PyArrayObject *)PyArray_FromObject(a0,
						 PyArray_NOTYPE,0,0))
	==NULL) return NULL;

    ret = PyArray_Transpose(a, shape);
    Py_DECREF(a);
    return ret;
}


static char doc_repeat[] = "repeat(a, n, axis=0)";

static PyObject *array_repeat(PyObject *dummy, PyObject *args, PyObject *kwds) {
    PyObject *shape, *a0;
    int axis=0;
    static char *kwlist[] = {"array", "shape", "axis", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "OO|i", kwlist,
                                     &a0, &shape, &axis)) return NULL;

    return PyArray_Repeat(a0, shape, axis);
}

static char doc_choose[] = "choose(a, (b1,b2,...))";

static PyObject *array_choose(PyObject *dummy, PyObject *args) {
    PyObject *shape, *a0;

    if (!PyArg_ParseTuple(args, "OO", &a0, &shape)) return NULL;

    return PyArray_Choose(a0, shape);
}

static char doc_sort[] = "sort(a)";

static PyObject *array_sort(PyObject *dummy, PyObject *args) {
    PyObject *a0;

    if (!PyArg_ParseTuple(args, "O", &a0)) return NULL;

    return PyArray_Sort(a0);
}

static char doc_argsort[] = "argsort(a)";

static PyObject *array_argsort(PyObject *dummy, PyObject *args) {
    PyObject *a0;

    if (!PyArg_ParseTuple(args, "O", &a0)) return NULL;

    return PyArray_ArgSort(a0);
}

static char doc_binarysearch[] = "binarysearch(a,v)";

static PyObject *array_binarysearch(PyObject *dummy, PyObject *args) {
    PyObject *shape, *a0;

    if (!PyArg_ParseTuple(args, "OO", &a0, &shape)) return NULL;

    return PyArray_BinarySearch(a0, shape);
}

static char doc_innerproduct[] = "innerproduct(a,v)";

static PyObject *array_innerproduct(PyObject *dummy, PyObject *args) {
    PyObject *shape, *a0;

    if (!PyArg_ParseTuple(args, "OO", &a0, &shape)) return NULL;

    return PyArray_InnerProduct(a0, shape);
}

static char doc_matrixproduct[] = "matrixproduct(a,v)";

static PyObject *array_matrixproduct(PyObject *dummy, PyObject *args) {
    PyObject *shape, *a0;

    if (!PyArg_ParseTuple(args, "OO", &a0, &shape)) return NULL;

    return PyArray_MatrixProduct(a0, shape);
}

static char doc_fastCopyAndTranspose[] = "_fastCopyAndTranspose(a)";

static PyObject *array_fastCopyAndTranspose(PyObject *dummy, PyObject *args) {
    PyObject *a0;

    if (!PyArg_ParseTuple(args, "O", &a0)) return NULL;

    return PyArray_fastCopyAndTranspose(a0);
}

static char doc_correlate[] = "cross_correlate(a,v, mode=0)";

static PyObject *array_correlate(PyObject *dummy, PyObject *args, PyObject *kwds) {
    PyObject *shape, *a0;
    int mode=0;
    static char *kwlist[] = {"a", "v", "mode", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "OO|i", kwlist,
                                     &a0, &shape, &mode)) return NULL;

    return PyArray_Correlate(a0, shape, mode);
}

static char doc_argmax[] = "argmax(a)";

static PyObject *array_argmax(PyObject *dummy, PyObject *args) {
    PyObject *a0;

    if (!PyArg_ParseTuple(args, "O", &a0)) return NULL;

    return PyArray_ArgMax(a0);
}


static char doc_arange[] = "arange(start, stop=None, step=1, typecode=None)\n\n  Just like range() except it returns an array whose type can be\n specified by the keyword argument typecode.";

static PyObject *
array_arange(PyObject *ignored, PyObject *args, PyObject *kws) {
    PyObject *o_start=NULL, *o_stop=Py_None, *o_step=NULL;
    PyObject *tpo=Py_None, *range;
    static char *kwd[]= {"start", "stop", "step", "typecode", NULL};
    char *tp=NULL;
    char *rptr;
    int elsize;
    double start, stop, step, value;
    int type, length, i;
    int deftype = PyArray_LONG;
    PyArray_Descr *dbl_descr;


    if(!PyArg_ParseTupleAndKeywords(args, kws, "O|OOO", kwd, &o_start,
				    &o_stop, &o_step, &tpo)) return NULL;

    deftype = PyArray_ObjectType(o_start, deftype);
    if (o_stop != Py_None) {
	deftype = PyArray_ObjectType(o_stop, deftype);
    }
    if (o_step != NULL) {
	deftype = PyArray_ObjectType(o_step, deftype);
    }

    if (tpo == Py_None) {
	type = deftype;
    } else {
	tp = PyString_AsString(tpo);
	if (tp == NULL) {
	    PyErr_SetString(PyExc_TypeError, "typecode argument must be a string.");
	    return NULL;
	}
	if (tp[0] == 0) type = deftype;
	else type = tp[0];
    }

    start = PyFloat_AsDouble(o_start);
    if ((start == -1.0) && (PyErr_Occurred() != NULL)) return NULL;

    if (o_step == NULL) {
	step = 1;
    }
    else {
	step = PyFloat_AsDouble(o_step);
	if ((step == -1.0) && (PyErr_Occurred() != NULL)) return NULL;
    }

    if (o_stop == Py_None) {
	stop = start;
	start = 0;
    }
    else {
	stop = PyFloat_AsDouble(o_stop);
	if ((stop == -1.0) && (PyErr_Occurred() != NULL)) return NULL;
    }

    length = (int ) ceil((stop - start)/step);

    if (length <= 0) {
	length = 0;
	return PyArray_FromDims(1, &length, type);
    }

    range = PyArray_FromDims(1,&length, type);
    if (range == NULL) return NULL;
    dbl_descr = PyArray_DescrFromType(PyArray_DOUBLE);

    rptr = ((PyArrayObject *)range)->data;
    elsize = ((PyArrayObject *)range)->descr->elsize;
    type = ((PyArrayObject *)range)->descr->type_num;
    for (i=0; i < length; i++) {
        value = start + i*step;
	dbl_descr->cast[type]((char*)&value, 0, rptr, 0, 1);
	rptr += elsize;
    }

    return range;
}

/*****
      static char doc_arrayMap[] = "arrayMap(func, [a1,...,an])";

      static PyObject *array_arrayMap(PyObject *dummy, PyObject *args) {
      PyObject *shape, *a0;

      if (PyArg_ParseTuple(args, "OO", &a0, &shape) == NULL) return NULL;

      return PyArray_Map(a0, shape);
      }
*****/

static char doc_set_string_function[] = "set_string_function(f, repr=1) sets the python function f to be the function used to obtain a pretty printable string version of a array whenever a array is printed.  f(M) should expect a array argument M, and should return a string consisting of the desired representation of M for printing.";

static PyObject *array_set_string_function(PyObject *dummy, PyObject *args, PyObject *kwds) {
    PyObject *op;
    int repr=1;
    static char *kwlist[] = {"f", "repr", NULL};

    if(!PyArg_ParseTupleAndKeywords(args, kwds, "O|i", kwlist,
                                    &op, &repr)) return NULL;
    PyArray_SetStringFunction(op, repr);
    Py_INCREF(Py_None);
    return Py_None;
}

static struct PyMethodDef array_module_methods[] = {
    {"set_string_function", (PyCFunction)array_set_string_function,
                   METH_VARARGS|METH_KEYWORDS, doc_set_string_function},

    {"array",	(PyCFunction)array_array, METH_VARARGS|METH_KEYWORDS, doc_array},
    {"arange",  (PyCFunction)array_arange, METH_VARARGS|METH_KEYWORDS, doc_arange},
    {"zeros",	(PyCFunction)array_zeros, METH_VARARGS|METH_KEYWORDS, doc_zeros},
    {"empty",	(PyCFunction)array_empty, METH_VARARGS|METH_KEYWORDS, doc_empty},
    {"fromstring",(PyCFunction)array_fromString, METH_VARARGS|METH_KEYWORDS, doc_fromString},
    {"take",	(PyCFunction)array_take, METH_VARARGS|METH_KEYWORDS, doc_take},
    {"put",	(PyCFunction)array_put, METH_VARARGS, doc_put},
    {"putmask",	(PyCFunction)array_putmask, METH_VARARGS, doc_putmask},
    {"reshape",	(PyCFunction)array_reshape, METH_VARARGS, doc_reshape},
    {"concatenate",	(PyCFunction)array_concatenate, METH_VARARGS, doc_concatenate},
    {"transpose",	(PyCFunction)array_transpose, METH_VARARGS|METH_KEYWORDS, doc_transpose},
    {"repeat",	(PyCFunction)array_repeat, METH_VARARGS|METH_KEYWORDS, doc_repeat},
    {"choose",	(PyCFunction)array_choose, METH_VARARGS, doc_choose},

    {"sort",	(PyCFunction)array_sort, METH_VARARGS, doc_sort},
    {"argsort",	(PyCFunction)array_argsort, METH_VARARGS, doc_argsort},
    {"binarysearch",	(PyCFunction)array_binarysearch, METH_VARARGS, doc_binarysearch},

    {"argmax",	(PyCFunction)array_argmax, METH_VARARGS, doc_argmax},
    {"innerproduct",	(PyCFunction)array_innerproduct, METH_VARARGS, doc_innerproduct},
    {"matrixproduct",	(PyCFunction)array_matrixproduct, METH_VARARGS, doc_matrixproduct},
    {"_fastCopyAndTranspose", (PyCFunction)array_fastCopyAndTranspose, METH_VARARGS, doc_fastCopyAndTranspose},
    {"cross_correlate", (PyCFunction)array_correlate, METH_VARARGS | METH_KEYWORDS, doc_correlate},
    /*  {"arrayMap",	(PyCFunction)array_arrayMap, 1, doc_arrayMap},*/

    {NULL,		NULL, 0}		/* sentinel */
};

/* Initialization function for the module (*must* be called initArray) */

DL_EXPORT(void) initmultiarray(void) {
    PyObject *m, *d, *s, *one, *zero;
    int i;
    char *data;
    PyArray_Descr *descr;

    /* Create the module and add the functions */
    m = Py_InitModule("multiarray", array_module_methods);

    /* Import the array object */
    import_array();

    /* Add some symbolic constants to the module */
    d = PyModule_GetDict(m);

    MultiArrayError = PyErr_NewException("multiarray.error", NULL, NULL);
    PyDict_SetItemString (d, "error", MultiArrayError);

    s = PyString_FromString("0.30");
    PyDict_SetItemString(d, "__version__", s);
    Py_DECREF(s);
    PyDict_SetItemString(d, "arraytype", (PyObject *)&PyArray_Type);

    /*Load up the zero and one values for the types.*/
    one = PyInt_FromLong(1);
    zero = PyInt_FromLong(0);

    for(i=PyArray_CHAR; i<PyArray_NTYPES; i++) {
	descr = PyArray_DescrFromType(i);
	data = (char *)malloc(descr->elsize);
	memset(data, 0, descr->elsize);
	descr->setitem(one, data);
	descr->one = data;
	data = (char *)malloc(descr->elsize);
	memset(data, 0, descr->elsize);
	descr->setitem(zero, data);
	descr->zero = data;
    }
    Py_DECREF(zero);
    Py_DECREF(one);

    /* Check for errors */
    if (PyErr_Occurred())
	Py_FatalError("can't initialize module array");
}
