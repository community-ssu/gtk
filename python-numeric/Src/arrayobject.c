/*
  Python Array Object -- Provide multidimensional arrays as a basic
  object type in python.

  Copyright (c) 1995, 1996, 1997 Jim Hugunin, hugunin@mit.edu
  See file COPYING for details.


  These arrays are primarily designed for supporting multidimensional,
  homogeneous arrays of basic C numeric types.  They also can support
  arrays of arbitrary Python Objects, if you are willing to sacrifice
  performance for heterogeneity.
*/

/* $Id: arrayobject.c,v 1.107 2005/11/09 19:43:39 teoliphant Exp $ */

#include "Python.h"
#include <stddef.h>
/*Silly trick to make dll library work right under Win32*/
#if defined(MS_WIN32) || defined(__CYGWIN__)
#undef DL_IMPORT
#define DL_IMPORT(RTYPE) __declspec(dllexport) RTYPE
#endif
#define _ARRAY_MODULE
#include "Numeric/arrayobject.h"
#define _UFUNC_MODULE
#include "Numeric/ufuncobject.h"

#define OBJECT(O) ((PyObject*)(O))

/* There are several places in the code where an array of dimensions is */
/* allocated statically.  This is the size of that static allocation.  I */
/* can't come up with any reasonable excuse for a larger array than this. */

#define MAX_DIMS 30

/* Helper Functions */
extern int _PyArray_multiply_list(int *l1, int n) {
    int s=1, i=0;
    while (i < n) s *= l1[i++];
    return s;
}
extern int _PyArray_compare_lists(int *l1, int *l2, int n) {
    int i;
    for(i=0;i<n;i++) {
	if (l1[i] != l2[i]) return 0;
    }
    return 1;
}

/* pyport.h guarantees INT_MAX */
#ifndef INT_MIN
#define INT_MIN (-INT_MAX - 1)
#endif

static PyObject *array_int(PyArrayObject *v);
/* Convert Python ints, Python longs, and array objects to a C int.
 * Checks for potential overflow in the cast to int if we're on an
 * architecture where ints aren't the same as longs.
 * This is important as dimensions, etc. in the PyArrayObject
 * structure are C ints.
 */
extern int
PyArray_IntegerAsInt(PyObject *o)
{
    long long_value = -1;
    if (!o) {
        PyErr_SetString(PyExc_TypeError, "an integer is required");
	return -1;
    }
    if (PyArray_Check(o)) {
        PyObject *as_int = array_int((PyArrayObject *)o);
        if (as_int == NULL) return -1;
        o = as_int;
    }
    else {
        Py_INCREF(o);
    }
    if (PyInt_Check(o)) {
	long_value = PyInt_AS_LONG(o);
    } else if (PyLong_Check(o)) {
	long_value = PyLong_AsLong(o);
    } else {
	PyErr_SetString(PyExc_TypeError, "an integer is required");
        Py_DECREF(o);
	return -1;
    }
    Py_DECREF(o);
#if (SIZEOF_LONG != SIZEOF_INT)
    if ((long_value < INT_MIN) || (long_value > INT_MAX)) {
	PyErr_SetString(PyExc_ValueError,
		"integer won't fit into a C int");
	return -1;
    }
#endif
    return (int) long_value;
}
#define error_converting(x)  (((x) == -1) && PyErr_Occurred())


/* These can be cleaned up */
#define SIZE(mp) (_PyArray_multiply_list((mp)->dimensions, (mp)->nd))
#define NBYTES(mp) ((mp)->descr->elsize * SIZE(mp))
/* Obviously this needs some work. */
#define ISCONTIGUOUS(m) ((m)->flags & CONTIGUOUS)

#define PyArray_CONTIGUOUS(m) (ISCONTIGUOUS(m) ? Py_INCREF(m), m : \
(PyArrayObject *)(PyArray_ContiguousFromObject((PyObject *)(m), \
(m)->descr->type_num, 0,0)))

int do_sliced_copy(char *dest, int *dest_strides, int *dest_dimensions,
		   int dest_nd, char *src, int *src_strides,
		   int *src_dimensions, int src_nd, int elsize,
		   int copies) {
	int i, j;

	if (src_nd == 0 && dest_nd == 0) {
		for(j=0; j<copies; j++) {
			memmove(dest, src, elsize);
			dest += elsize;
		}
		return 0;
	}

	if (dest_nd > src_nd) {
		for(i=0; i<*dest_dimensions; i++, dest += *dest_strides) {
			if (do_sliced_copy(dest, dest_strides+1,
					   dest_dimensions+1, dest_nd-1,
					   src, src_strides,
					   src_dimensions, src_nd,
					   elsize, copies) == -1)
			  return -1;
		}
		return 0;
	}

	if (dest_nd == 1) {
                /*               if (*dest_dimensions != *src_dimensions) {
                        PyErr_SetString(PyExc_ValueError,
                          "matrices are not aligned for copy");
                        return -1;
                }
                */
		for(i=0; i<*dest_dimensions; i++, src += *src_strides) {
			for(j=0; j<copies; j++) {
				memmove(dest, src, elsize);
				dest += *dest_strides;
			}
		}
	} else {
		for(i=0; i<*dest_dimensions; i++, dest += *dest_strides,
		      src += *src_strides) {
			if (do_sliced_copy(dest, dest_strides+1,
					   dest_dimensions+1, dest_nd-1,
					   src, src_strides+1,
					   src_dimensions+1, src_nd-1,
					   elsize, copies) == -1)
			  return -1;
		}
	}

	return 0;
}

int optimize_slices(int **dest_strides, int **dest_dimensions,
		    int *dest_nd, int **src_strides,
		    int **src_dimensions, int *src_nd,
		    int *elsize, int *copies) {
    while (*src_nd > 0) {
	if (((*dest_strides)[*dest_nd-1] == *elsize) &&
	    ((*src_strides)[*src_nd-1] == *elsize)) {
	    *elsize *= (*dest_dimensions)[*dest_nd-1];
	    *dest_nd-=1; *src_nd-=1;
	} else {
	    break;
	}
    }
    if (*src_nd == 0) {
	while (*dest_nd > 0) {
	    if (((*dest_strides)[*dest_nd-1] == *elsize)) {
		*copies *= (*dest_dimensions)[*dest_nd-1];
		*dest_nd-=1;
	    } else {
		break;
	    }
	}
    }
    return 0;
}

static char *contiguous_data(PyArrayObject *src) {
    int dest_strides[MAX_DIMS], *dest_strides_ptr;
    int *dest_dimensions=src->dimensions;
    int dest_nd=src->nd;
    int *src_strides = src->strides;
    int *src_dimensions=src->dimensions;
    int src_nd=src->nd;
    int elsize=src->descr->elsize;
    int copies=1;
    int ret, i;
    int stride=elsize;
    char *new_data;

    for(i=dest_nd-1; i>=0; i--) {
	dest_strides[i] = stride;
	stride *= dest_dimensions[i];
    }

    dest_strides_ptr = dest_strides;

    if (optimize_slices(&dest_strides_ptr, &dest_dimensions, &dest_nd,
			&src_strides, &src_dimensions, &src_nd,
			&elsize, &copies) == -1)
	return NULL;

    new_data = (char *)malloc(stride);

    ret = do_sliced_copy(new_data, dest_strides_ptr, dest_dimensions,
			 dest_nd, src->data, src_strides,
			 src_dimensions, src_nd, elsize, copies);

    if (ret != -1) { return new_data; }
    else { free(new_data); return NULL; }
}


/* Used for arrays of python objects to increment the reference count of */
/* every python object in the array. */
extern int PyArray_INCREF(PyArrayObject *mp) {
    int i, n;
    PyObject **data, **data2;

    if (mp->descr->type_num != PyArray_OBJECT) return 0;

    if (ISCONTIGUOUS(mp)) {
	data = (PyObject **)mp->data;
    } else {
	if ((data = (PyObject **)contiguous_data(mp)) == NULL)
	    return -1;
    }

    n = SIZE(mp);
    data2 = data;
    for(i=0; i<n; i++, data++) Py_XINCREF(*data);

    if (!ISCONTIGUOUS(mp)) free(data2);

    return 0;
}

extern int PyArray_XDECREF(PyArrayObject *mp) {
    int i, n;
    PyObject **data, **data2;

    if (mp->descr->type_num != PyArray_OBJECT) return 0;

    if (ISCONTIGUOUS(mp)) {
	data = (PyObject **)mp->data;
    } else {
	if ((data = (PyObject **)contiguous_data(mp)) == NULL)
	    return -1;
    }

    n = SIZE(mp);
    data2 = data;
    for(i=0; i<n; i++, data++) Py_XDECREF(*data);

    if (!ISCONTIGUOUS(mp)) free(data2);

    return 0;
}

/* Including a C file is admittedly ugly.  Look at the C file if you want to */
/* see something even uglier. This is computer generated code. */
#include "arraytypes.c"

char *index2ptr(PyArrayObject *mp, int i) {
    if (i==0 && (mp->nd == 0 || mp->dimensions[0] > 0))
	return mp->data;

    if (mp->nd>0 &&  i>0 && i < mp->dimensions[0]) {
	return mp->data+i*mp->strides[0];
    }
    PyErr_SetString(PyExc_IndexError,"index out of bounds");
    return NULL;
}

extern int PyArray_Size(PyObject *op) {
    if (PyArray_Check(op)) {
	return SIZE((PyArrayObject *)op);
    } else {
	return 0;
    }
}

int PyArray_CopyArray(PyArrayObject *dest, PyArrayObject *src) {
    int *dest_strides=dest->strides;
    int *dest_dimensions=dest->dimensions;
    int dest_nd=dest->nd;
    int *src_strides = src->strides;
    int *src_dimensions=src->dimensions;
    int src_nd=src->nd;
    int elsize=src->descr->elsize;
    int copies=1;
    int ret, i,j;

    if (src->nd > dest->nd) {
	PyErr_SetString(PyExc_ValueError,
			"array too large for destination");
	return -1;
    }
    if (dest->descr->type_num != src->descr->type_num) {
	PyErr_SetString(PyExc_ValueError,
			"can only copy from a array of the same type.");
	return -1;
    }

    /* Determine if src is "broadcastable" to dest */
    for (i=dest->nd-1, j=src->nd-1; j>=0; i--, j--) {
            if ((src_dimensions[j] != 1) && (dest_dimensions[i] != src_dimensions[j])) {
                    PyErr_SetString(PyExc_ValueError,
                                    "matrices are not aligned for copy");
                    return -1;
            }
    }


    if (optimize_slices(&dest_strides, &dest_dimensions, &dest_nd,
			&src_strides, &src_dimensions, &src_nd,
			&elsize, &copies) == -1)
	return -1;

    ret = do_sliced_copy(dest->data, dest_strides, dest_dimensions,
			 dest_nd, src->data, src_strides,
			 src_dimensions, src_nd, elsize, copies);

    if (ret != -1) { ret = PyArray_INCREF(dest); }
    return ret;
}

int PyArray_CopyObject(PyArrayObject *dest, PyObject *src_object) {
    PyArrayObject *src;
    PyObject *tmp;
    int ret, n_new, n_old;
    char *new_string;

    /* Special function added here to try and make arrays of strings
       work out. */
    if ((dest->descr->type_num == PyArray_CHAR) && dest->nd > 0
	&& PyString_Check(src_object)) {
	n_new = dest->dimensions[dest->nd-1];
	n_old = PyString_Size(src_object);
	if (n_new > n_old) {
	    new_string = (char *)malloc(n_new*sizeof(char));
	    memmove(new_string,
		   PyString_AS_STRING((PyStringObject *)src_object),
		   n_old);
	    memset(new_string+n_old, ' ', n_new-n_old);
	    tmp = PyString_FromStringAndSize(new_string,
					     n_new);
	    free(new_string);
	    src_object = tmp;
	}
    }
    src = (PyArrayObject *)PyArray_FromObject(src_object,
					      dest->descr->type_num, 0,
					      dest->nd);
    if (src == NULL) return -1;

    ret = PyArray_CopyArray(dest, src);
    Py_DECREF(src);
    return ret;
}

/* This is the basic array allocation function. */
PyObject *PyArray_FromDimsAndDataAndDescr(int nd, int *d,
					  PyArray_Descr *descr,
					  char *data) {
    PyArrayObject *self;
    int i,sd;
    int *dimensions, *strides;
    int flags=CONTIGUOUS | OWN_DIMENSIONS | OWN_STRIDES;

    dimensions = strides = NULL;

    if (nd < 0) {
	PyErr_SetString(PyExc_ValueError,
			"number of dimensions must be >= 0");
	return NULL;
    }

    if (nd > 0) {
	if ((dimensions = (int *)malloc(nd*sizeof(int))) == NULL) {
	    PyErr_SetString(PyExc_MemoryError, "can't allocate memory for array");
	    goto fail;
	}
	if ((strides = (int *)malloc(nd*sizeof(int))) == NULL) {
	    PyErr_SetString(PyExc_MemoryError, "can't allocate memory for array");
	    goto fail;
	}
	memmove(dimensions, d, sizeof(int)*nd);
    }

    sd = descr->elsize;
    for(i=nd-1;i>=0;i--) {
	if (flags & OWN_STRIDES) strides[i] = sd;
	if (dimensions[i] < 0) {
	    PyErr_SetString(PyExc_ValueError, "negative dimensions are not allowed");
	    goto fail;
	}
	/*
	   This may waste some space, but it seems to be
	   (unsurprisingly) unhealthy to allow strides that are
	   longer than sd.
	*/
	sd *= dimensions[i] ? dimensions[i] : 1;
	/*		sd *= dimensions[i]; */
    }

    /* Make sure we're aligned on ints. */
    sd += sizeof(int) - sd%sizeof(int);

    if (data == NULL) {
	if ((data = (char *)malloc(sd)) == NULL) {
	    PyErr_SetString(PyExc_MemoryError, "can't allocate memory for array");
	    goto fail;
	}
	flags |= OWN_DATA;
    }

    if((self = PyObject_NEW(PyArrayObject, &PyArray_Type)) == NULL) goto fail;
    if (flags & OWN_DATA) memset(data, 0, sd);

    self->data=data;
    self->dimensions = dimensions;
    self->strides = strides;
    self->nd=nd;
    self->descr=descr;
    self->base = (PyObject *)NULL;
    self->flags = flags;
    self->weakreflist = (PyObject *)NULL;

    return (PyObject*)self;

 fail:

    if (flags & OWN_DATA) free(data);
    if (dimensions != NULL) free(dimensions);
    if (strides != NULL) free(strides);
    return NULL;
}

PyObject *PyArray_FromDimsAndData(int nd, int *d, int type, char *data) {
    PyArray_Descr *descr;
    PyObject *op;
    char real_type;

    real_type = type;
    real_type = real_type & ~SAVESPACEBIT;
    if ((descr = PyArray_DescrFromType((int)real_type)) == NULL) return NULL;
    op = PyArray_FromDimsAndDataAndDescr(nd, d, descr, data);
    if (type & SAVESPACEBIT)
	((PyArrayObject *)op)->flags |= SAVESPACE;
    return op;
}

PyObject *PyArray_FromDims(int nd, int *d, int type) {
    PyArray_Descr *descr;
    PyObject *op;
    char real_type;

    real_type = type;
    real_type = real_type & ~SAVESPACEBIT;
    if ((descr = PyArray_DescrFromType((int)real_type)) == NULL) return NULL;
    op = PyArray_FromDimsAndDataAndDescr(nd, d, descr, NULL);
    if (type & SAVESPACEBIT)
	((PyArrayObject *)op)->flags |= SAVESPACE;
    return op;
}


extern PyObject *PyArray_Copy(PyArrayObject *m1) {
    PyArrayObject *ret =
	(PyArrayObject *)PyArray_FromDims(m1->nd, m1->dimensions, m1->descr->type_num);

    if (PyArray_CopyArray(ret, m1) == -1) return NULL;

    return (PyObject *)ret;
}


static PyObject * PyArray_Resize(PyArrayObject *self, PyObject *shape) {
    size_t oldsize, newsize;
    int new_nd, k, sd, n, elsize;
    int refcnt;
    int new_dimensions[MAX_DIMS];
    int new_strides[MAX_DIMS];
    int *dimptr, *strptr;
    char *new_data, *dptr;
    char all_zero[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    PyObject *val;

    if (!PyArray_ISCONTIGUOUS(self)) {
	PyErr_SetString(PyExc_ValueError, "resize only works on contiguous arrays");
	return NULL;
    }

    refcnt = (((PyObject *)self)->ob_refcnt);
    if ((refcnt > 2) || (self->base != NULL) || (self->weakreflist != NULL)) {
	PyErr_SetString(PyExc_ValueError, "cannot resize an array that has been referenced or is referencing\n  another array in this way.  Use the resize function.");
	return NULL;
    }

    /* One more check for safety --- it may not be necessary. */
    if (!((self->flags & OWN_DIMENSIONS) && (self->flags & OWN_STRIDES) && (self->flags & OWN_DATA))) {
	PyErr_SetString(PyExc_ValueError, "cannot resize this array:  it is referencing another array.");
	return NULL;
    }

    /* Argument needs to be a sequence or a single integer */
    if ((new_nd = PySequence_Size(shape)) == -1) {
	PyErr_Clear();
        newsize = PyArray_IntegerAsInt(shape);
	if (error_converting(newsize)) {
            PyErr_SetString(PyExc_TypeError, "new shape must be a sequence or a positive integer");
	    return NULL;
	} else if (newsize < 0) {
            PyErr_SetString(PyExc_ValueError, "negative dimensions are not allowed");
            return NULL;
        }
        new_nd = 1;
	new_dimensions[0] = newsize;
    }
    else {
	if (new_nd > MAX_DIMS) {
	    PyErr_SetString(PyExc_ValueError, "Too many dimensions.");
	    return NULL;
	}
	newsize = 1;
	for (k=0; k < new_nd; k++) {
	    val = PySequence_GetItem(shape, k);
	    if (val == NULL) return NULL;
	    new_dimensions[k] = PyArray_IntegerAsInt(val);
	    if (error_converting(new_dimensions[k])) {
		Py_DECREF(val);
		return NULL;
	    } else if (new_dimensions[k] < 0) {
		PyErr_SetString(PyExc_ValueError, "negative dimensions are not allowed");
		Py_DECREF(val);
		return NULL;
	    }
	    newsize *= new_dimensions[k];
	    Py_DECREF(val);
	}
    }
    if (newsize == 0) {
	PyErr_SetString(PyExc_ValueError, "Newsize is zero.  Cannot delete an array in this way.");
	return NULL;
    }
    oldsize = PyArray_SIZE(self);

    if (oldsize == newsize) return PyArray_Reshape(self, shape);
    /* make new_strides variable */
    sd = self->descr->elsize;
    for (k=new_nd-1; k >=0; k--) {
	new_strides[k] = sd;
	sd *= new_dimensions[k] ? new_dimensions[k] : 1;
    }

    /* Reallocate space */
    new_data = (char *)realloc(self->data, newsize*(self->descr->elsize));
    if (new_data == NULL) {
	PyErr_SetString(PyExc_MemoryError, "can't allocate memory for array.");
	return NULL;
    }
    self->data = new_data;

    if (newsize > oldsize) {  /* Fill with zeros */
	elsize = self->descr->elsize;
	if (memcmp(self->descr->zero, all_zero, elsize) == 0) {
	    memset(self->data + oldsize*elsize, 0, (newsize - oldsize)*elsize);
	} else {
	    dptr = self->data + oldsize*elsize;
	    n = newsize - oldsize;
	    for(k=0; k<n; k++) {
		memmove(dptr, self->descr->zero, elsize);
		dptr += elsize;
	    }
	}
    }

    if (self->nd != new_nd) {  /* Different number of dimensions. */
	self->nd = new_nd;

	/* Need new dimensions and strides arrays */
	dimptr = (int *)realloc(self->dimensions, new_nd*sizeof(int));
	if (dimptr == NULL) {
	    PyErr_SetString(PyExc_MemoryError, "can't allocate memory for array (array may be corrupted).");
	    return NULL;
	}
	self->dimensions = dimptr;

	strptr = (int *)realloc(self->strides, new_nd*sizeof(int));
	if (strptr == NULL) {
	    PyErr_SetString(PyExc_MemoryError, "can't allocate memory for array (array may be corrupted).");
	    return NULL;
	}
	self->strides = strptr;
    }

    memmove(self->dimensions, new_dimensions, new_nd*sizeof(int));
    memmove(self->strides, new_strides, new_nd*sizeof(int));

    Py_INCREF(Py_None);
    return Py_None;

}

static void array_dealloc(PyArrayObject *self) {

    if (self->weakreflist != NULL)
        PyObject_ClearWeakRefs((PyObject *)self);

    if(self->base) {
	Py_DECREF(self->base);
    }

    if (self->flags & OWN_DATA) {
	PyArray_XDECREF(self);
	free(self->data);
    }

    if (self->flags & OWN_DIMENSIONS && self->dimensions != NULL) free(self->dimensions);
    if (self->flags & OWN_STRIDES && self->strides != NULL) free(self->strides);

    PyObject_DEL(self);
}

/* Code to handle accessing Array objects as sequence objects */
static int array_length(PyArrayObject *self) {
    if (self->nd != 0) {
	return self->dimensions[0];
    } else {
	return 1; /* Because a[0] works on 0d arrays. */
    }
}


static PyObject *array_item(PyArrayObject *self, int i) {
    char *item;

    if ((item = index2ptr(self, i)) == NULL) return NULL;

    if(self->nd >= 1) {
	PyArrayObject *r;
        r = (PyArrayObject *)PyArray_FromDimsAndDataAndDescr(self->nd-1,
                                                             self->dimensions+1,
                                                             self->descr,
                                                             item);
        if (r == NULL) return NULL;
        memmove(r->strides, self->strides+1, sizeof(int)*(r->nd));
	r->base = (PyObject *)self;
	r->flags = (self->flags & (CONTIGUOUS | SAVESPACE));
        r->flags |= OWN_DIMENSIONS | OWN_STRIDES;
	Py_INCREF(self);
	return (PyObject*)r;
    } else {
	return self->descr->getitem(item);
    }
}

static PyObject *array_item_nice(PyArrayObject *self, int i) {
	PyObject *ret;
	ret = array_item(self, i);
	if (ret == NULL) {
	    return NULL;
	} else if (PyArray_Check(ret)) {
	    return PyArray_Return((PyArrayObject *)ret);
	} else {
	    return ret;
	}
}

extern PyObject * PyArray_Item(PyObject *op, int i) {
    if (PyArray_Check(op)) {
	return array_item((PyArrayObject *)op, i);
    } else {
	PyErr_SetString(PyExc_ValueError, "Not an array object");
	return NULL;
    }
}

extern PyObject *PyArray_Return(PyArrayObject *mp) {
    PyObject *op;

    if (PyErr_Occurred()) {
	if (mp != NULL) {Py_DECREF(mp);}
	return NULL;
    }
    if (mp->nd == 0) {
            if (mp->descr->type_num == PyArray_LONG || ((sizeof(int) == sizeof(long)) && mp->descr->type_num == PyArray_INT) || mp->descr->type_num == PyArray_DOUBLE || mp->descr->type_num == PyArray_CDOUBLE || mp->descr->type_num == PyArray_OBJECT ) {
	    op = mp->descr->getitem(mp->data);
            Py_DECREF(mp);
            return op;
            }
    }
    return (PyObject *)mp;
}

static PyObject *
array_slice(PyArrayObject *self, int ilow, int ihigh)
{
    PyArrayObject *r;
    int l;
    char *data;

    if (self->nd == 0) {
	PyErr_SetString(PyExc_ValueError, "can't slice a scalar");
	return NULL;
    }

    l=self->dimensions[0];
    if (ilow < 0) {
        ilow = 0;
    } else if (ilow > l) {
        ilow = l;
    }
    if (ihigh < ilow) {
        ihigh = ilow;
    } else if (ihigh > l) {
        ihigh = l;
    }

    if (ihigh != ilow) {
	data = index2ptr(self, ilow);
	if (data == NULL) return NULL;
    } else {
	data = self->data;
    }

    self->dimensions[0] = ihigh-ilow;
    r = (PyArrayObject *)PyArray_FromDimsAndDataAndDescr(self->nd,
                                                         self->dimensions,
                                                         self->descr, data);
    self->dimensions[0] = l;

    if (!ISCONTIGUOUS(self)) r->flags &= ~CONTIGUOUS;
    if (self->flags & SAVESPACE) r->flags |= SAVESPACE;
    memmove(r->strides, self->strides, sizeof(int)*self->nd);
    r->base = (PyObject *)self;
    Py_INCREF(self);

    return (PyObject *)r;
}

/* These will need some serious work when I feel like it. */
static int array_ass_item(PyArrayObject *self, int i, PyObject *v) {
    PyObject *c=NULL;
    PyArrayObject *tmp;
    char *item;
    int ret;

     if (v == NULL) {
	PyErr_SetString(PyExc_ValueError, "Can't delete array elements.");
	return -1;
    }

    if (i < 0) i = i+self->dimensions[0];

    if (self->nd > 1) {
	if((tmp = (PyArrayObject *)array_item(self, i)) == NULL) return -1;
	ret = PyArray_CopyObject(tmp, v);
	Py_DECREF(tmp);
	return ret;
    }

    if ((item = index2ptr(self, i)) == NULL) return -1;

    if(self->descr->type_num != PyArray_OBJECT && PyString_Check(v) && PyObject_Length(v) == 1) {
	char *s;
	if ((s=PyString_AsString(v)) == NULL) return -1;
	if(self->descr->type == 'c') {
	    ((char*)self->data)[i]=*s;
	    return 0;
	}
	if(s) c=PyInt_FromLong((long)*s);
	if(c) v=c;
    }

    self->descr->setitem(v, item);
    if(c) {Py_DECREF(c);}
    if(PyErr_Occurred()) return -1;
    return 0;
}


static int array_ass_slice(PyArrayObject *self, int ilow, int ihigh, PyObject *v) {
    int ret;
    PyArrayObject *tmp;

    if (v == NULL) {
	PyErr_SetString(PyExc_ValueError, "Can't delete array elements.");
	return -1;
    }
    if ((tmp = (PyArrayObject *)array_slice(self, ilow, ihigh)) == NULL) return -1;
    ret = PyArray_CopyObject(tmp, v);
    Py_DECREF(tmp);

    return ret;
}

/* -------------------------------------------------------------- */
static int
slice_coerce_index(PyObject *o, int *v)
{
    *v = PyArray_IntegerAsInt(o);
    if (error_converting(*v)) {
        PyErr_Clear();
        return 0;
    }
    return 1;
}

/* This is basically PySlice_GetIndicesEx, but with our coercion
 * of indices to integers (plus, that function is new in Python 2.3) */
static int
slice_GetIndices(PySliceObject *r, int length,
                 int *start, int *stop, int *step,
                 int *slicelength)
{
    int defstart, defstop;

    if (r->step == Py_None) {
        *step = 1;
    } else {
        if (!slice_coerce_index(r->step, step)) return -1;
        if (*step == 0) {
            PyErr_SetString(PyExc_ValueError, "slice step can not be zero");
            return -1;
        }
    }

    defstart = *step < 0 ? length - 1 : 0;
    defstop = *step < 0 ? -1 : length;

    if (r->start == Py_None) {
        *start = *step < 0 ? length-1 : 0;
    } else {
        if (!slice_coerce_index(r->start, start)) return -1;
        if (*start < 0) *start += length;
        if (*start < 0) *start = (*step < 0) ? -1 : 0;
        if (*start >= length) {
            *start = (*step < 0) ? length - 1 : length;
        }
    }

    if (r->stop == Py_None) {
        *stop = defstop;
    } else {
        if (!slice_coerce_index(r->stop, stop)) return -1;
        if (*stop < 0) *stop += length;
        if (*stop < 0) *stop = -1;
        if (*stop > length) *stop = length;
    }

    if ((*step < 0 && *stop >= *start) || (*step > 0 && *start >= *stop)) {
        *slicelength = 0;
    } else if (*step < 0) {
        *slicelength = (*stop - *start + 1) / (*step) + 1;
    } else {
        *slicelength = (*stop - *start - 1) / (*step) + 1;
    }

    return 0;
}

#define PseudoIndex -1
#define RubberIndex -2
#define SingleIndex -3

static int
parse_subindex(PyObject *op, int *step_size, int *n_steps, int max)
{
    int index;

    if (op == Py_None) {
        *n_steps = PseudoIndex;
        index = 0;
    } else if (op == Py_Ellipsis) {
        *n_steps = RubberIndex;
        index = 0;
    } else if (PySlice_Check(op)) {
        int stop;
        if (slice_GetIndices((PySliceObject *)op, max,
                             &index, &stop, step_size, n_steps) < 0) {
            if (!PyErr_Occurred()) {
                PyErr_SetString(PyExc_IndexError, "invalid slice");
            }
            goto fail;
        }
        if (*n_steps <= 0) {
            *n_steps = 0;
            *step_size = 1;
            index = 0;
        }
    } else {
        index = PyArray_IntegerAsInt(op);
        if (error_converting(index)) {
            PyErr_SetString(PyExc_IndexError,
                "each subindex must be either a slice, an integer, Ellipsis, or NewAxis");
            goto fail;
        }
        *n_steps = SingleIndex;
        *step_size = 0;
        if (index < 0) index += max;
        if (index >= max || index < 0) {
            PyErr_SetString(PyExc_IndexError, "invalid index");
            goto fail;
        }
    }
    return index;
fail:
    return -1;
}

static int parse_index(PyArrayObject *self, PyObject *op,
		       int *dimensions, int *strides, int *offset_ptr) {
    int i, j, n;
    int nd_old, nd_new, start, offset, n_add, n_pseudo;
    int step_size, n_steps;
    PyObject *op1=NULL;
    int is_slice;


    if (PySlice_Check(op) || op == Py_Ellipsis || op == Py_None) {
	n = 1;
	op1 = op;
	Py_INCREF(op);
	/* this relies on the fact that n==1 for loop below */
	is_slice = 1;
    }
    else {
	if (!PySequence_Check(op)) {
	    PyErr_SetString(PyExc_IndexError,
			    "index must be either an int or a sequence");
	    return -1;
	}
	n = PySequence_Length(op);
	is_slice = 0;
    }

    nd_old = nd_new = 0;

    /* IF we have at least two args with the last being a FLAG|FUNCTION
        do me...
    */

    offset = 0;
    for(i=0; i<n; i++) {
	if (!is_slice) {
	    if (!(op1=PySequence_GetItem(op, i))) {
		PyErr_SetString(PyExc_IndexError, "invalid index");
		return -1;
	    }
	}

	start = parse_subindex(op1, &step_size, &n_steps,
			       nd_old < self->nd ? self->dimensions[nd_old] : 0);
	Py_DECREF(op1);
	if (start == -1) break;

	if (n_steps == PseudoIndex) {
	    dimensions[nd_new] = 1; strides[nd_new] = 0; nd_new++;
	} else {
	    if (n_steps == RubberIndex) {
		for(j=i+1, n_pseudo=0; j<n; j++) {
		    op1 = PySequence_GetItem(op, j);
		    if (op1 == Py_None) n_pseudo++;
		    Py_DECREF(op1);
		}
		n_add = self->nd-(n-i-n_pseudo-1+nd_old);
		if (n_add < 0) {
		    PyErr_SetString(PyExc_IndexError, "too many indices");
		    return -1;
		}
		for(j=0; j<n_add; j++) {
		    dimensions[nd_new] = self->dimensions[nd_old];
		    strides[nd_new] = self->strides[nd_old];
		    nd_new++; nd_old++;
		}
	    } else {
		if (nd_old >= self->nd) {
		    PyErr_SetString(PyExc_IndexError, "too many indices");
		    return -1;
		}
		offset += self->strides[nd_old]*start;
		nd_old++;
		if (n_steps != SingleIndex) {
		    dimensions[nd_new] = n_steps;
		    strides[nd_new] = step_size*self->strides[nd_old-1];
		    nd_new++;
		}
	    }
	}
    }
    if (i < n) return -1;
    n_add = self->nd-nd_old;
    for(j=0; j<n_add; j++) {
	dimensions[nd_new] = self->dimensions[nd_old];
	strides[nd_new] = self->strides[nd_old];
	nd_new++; nd_old++;
    }
    *offset_ptr = offset;
    return nd_new;
}

/* Called when treating array object like a mapping -- called first from
   Python when using a[object] */
static PyObject *array_subscript(PyArrayObject *self, PyObject *op) {
    int dimensions[MAX_DIMS], strides[MAX_DIMS];
    int nd, offset, i, elsize;
    PyArrayObject *other;

    i = PyArray_IntegerAsInt(op);
    if (!error_converting(i)) {
	if (i < 0 && self->nd > 0) i = i+self->dimensions[0];
	return array_item(self, i);
    }
    PyErr_Clear();

    if ((nd = parse_index(self, op, dimensions, strides, &offset))
	== -1) {
	return NULL;
    }

    if ((other = (PyArrayObject *)PyArray_FromDimsAndDataAndDescr(nd,
								  dimensions,
								  self->descr,
								  self->data+offset)) == NULL) {
	return NULL;
    }
    memmove(other->strides, strides, sizeof(int)*other->nd);
    other->base = (PyObject *)self;
    Py_INCREF(self);

    elsize=other->descr->elsize;
    /* Check to see if other is CONTIGUOUS:  see if strides match
       dimensions */
    for (i=other->nd-1; i>=0; i--) {
	if (other->strides[i] == elsize) {
	    elsize *= other->dimensions[i];
	} else {
	    break;
	}
    }
    if (i >= 0) other->flags &= ~CONTIGUOUS;

    /* Maintain SAVESPACE flag on selection */
    if (self->flags & SAVESPACE) other->flags |= SAVESPACE;

    return (PyObject *)other;
}

static PyObject *array_subscript_nice(PyArrayObject *self, PyObject *op) {
    PyObject *ret;

    if ((ret = array_subscript(self, op)) == NULL) return NULL;
    if (PyArray_Check(ret)) return PyArray_Return((PyArrayObject *)ret);
    else return ret;
}


/* Another assignment hacked by using CopyObject.  */

static int array_ass_sub(PyArrayObject *self, PyObject *index, PyObject *op) {
    int ret;
    int i;
    PyArrayObject *tmp;

    if (op == NULL) {
	PyErr_SetString(PyExc_ValueError,
			"Can't delete array elements.");
	return -1;
    }

    i = PyArray_IntegerAsInt(index);
    if (!error_converting(i)) {
	return array_ass_item(self, i, op);
    }
    PyErr_Clear();

    if ((tmp = (PyArrayObject *)array_subscript(self, index)) == NULL)
	return -1;
    ret = PyArray_CopyObject(tmp, op);
    Py_DECREF(tmp);

    return ret;
}

static PyMappingMethods array_as_mapping = {
    (inquiry)array_length,		/*mp_length*/
    (binaryfunc)array_subscript_nice,   /*mp_subscript*/
    (objobjargproc)array_ass_sub,	/*mp_ass_subscript*/
};

/* Beginning of methods added by Scott N. Gunyan
   to alloy export of array as a Python buffer object */
static int array_getsegcount(PyArrayObject *self, int *lenp) {
    int i, elsize;
    int num_segments=1;

    if (lenp)
        *lenp = NBYTES(self);

    elsize = self->descr->elsize;
    /* Check how self is CONTIGUOUS:  see if strides match
       dimensions */
    for (i=self->nd-1; i>=0; i--) {
	if (self->strides[i] == elsize) {
	    elsize *= self->dimensions[i];
	} else {
	    break;
	}
    }
    if (i >= 0) {
        for (; i >= 0; i--) {
            num_segments *= self->dimensions[i];
	}
    }
    return num_segments;
}


/* this will probably need some work, could be faster */
int get_segment_pointer(PyArrayObject *self, int segment, int i) {
    if (i >= 0) {
    return ((segment%self->dimensions[i])*self->strides[i] +
            get_segment_pointer(self,segment/self->dimensions[i],i-1));
    } else {
        return 0;
    }
}



static int array_getreadbuf(PyArrayObject *self, int segment, void **ptrptr) {
    int num_segments, product=1, i=0;

    if ((segment < 0) ||
        (segment > (num_segments = array_getsegcount(self,NULL)))) {
        PyErr_SetString(PyExc_SystemError,
                        "Accessing non-existent array segment");
        return -1;
    }

    if (num_segments > 1) {
        /* find where the break in contiguity is */
        while ((product != num_segments) && (i < self->nd)) {
            product *= self->dimensions[i++];
        }
        /* now get the right segment pointer (i is indexing break in contiguity) */
        *ptrptr = self->data + get_segment_pointer(self,segment,i-1);
    } else {
        *ptrptr = self->data;
    }
    /* return the size in bytes of the segment (i is indexing break in contiguity) */
    return NBYTES(self);
}


static int array_getwritebuf(PyArrayObject *self, int segment, void **ptrptr) {
    return array_getreadbuf(self, segment, (void **) ptrptr);
}

static int array_getcharbuf(PyArrayObject *self, int segment, const char **ptrptr) {
    if (self->descr->type_num == PyArray_CHAR)
        return array_getreadbuf(self, segment, (void **) ptrptr);
    else {
        PyErr_SetString(PyExc_TypeError,
                        "Non-character array cannot be interpreted as character buffer.");
        return -1;
    }
}

static PyBufferProcs array_as_buffer = {
    (getreadbufferproc)array_getreadbuf,    /*bf_getreadbuffer*/
    (getwritebufferproc)array_getwritebuf,  /*bf_getwritebuffer*/
    (getsegcountproc)array_getsegcount,     /*bf_getsegcount*/
    (getcharbufferproc)array_getcharbuf,    /*bf_getcharbuffer*/
};
/* End methods added by Scott N. Gunyan for buffer extension */


/* -------------------------------------------------------------- */

typedef struct {
    PyUFuncObject *add,
	*subtract,
	*multiply,
	*divide,
	*remainder,
	*power,
	*negative,
	*absolute;
    PyUFuncObject *invert,
	*left_shift,
	*right_shift,
	*bitwise_and,
	*bitwise_xor,
	*bitwise_or;
    PyUFuncObject *less,     /* Added by Scott N. Gunyan */
        *less_equal,         /* for rich comparisons */
        *equal,
        *not_equal,
        *greater,
        *greater_equal;
    PyUFuncObject *floor_divide, /* Added by Bruce Sherwood */
        *true_divide;            /* for floor and true divide */
} NumericOps;

static NumericOps n_ops;

#define GET(op) n_ops.op = (PyUFuncObject *)PyDict_GetItemString(dict, #op)

int PyArray_SetNumericOps(PyObject *dict) {
    GET(add);
    GET(subtract);
    GET(multiply);
    GET(divide);
    GET(remainder);
    GET(power);
    GET(negative);
    GET(absolute);
    GET(invert);
    GET(left_shift);
    GET(right_shift);
    GET(bitwise_and);
    GET(bitwise_or);
    GET(bitwise_xor);
    GET(less);         /* Added by Scott N. Gunyan */
    GET(less_equal);   /* for rich comparisons */
    GET(equal);
    GET(not_equal);
    GET(greater);
    GET(greater_equal);
    GET(floor_divide);  /* Added by Bruce Sherwood */
    GET(true_divide);   /* for floor and true divide */
    return 0;
}

static int array_coerce(PyArrayObject **pm, PyObject **pw) {
    PyObject *new_op;
    if ((new_op = PyArray_FromObject(*pw, PyArray_NOTYPE, 0, 0))
	== NULL)
	return -1;
    Py_INCREF(*pm);
    *pw = new_op;
    return 0;

    /**Eliminate coercion at this stage.  Handled by umath now
       PyObject *new_op;
       char t1, t2;

       t1 = (*pm)->descr->type_num;
       t2 = PyArray_ObjectType(*pw, 0);

       if (PyArray_CanCastSafely(t2, t1)) {
       if ((new_op = PyArray_FromObject(*pw, t1, 0, 0)) == NULL) return -1;
       Py_INCREF(*pm);
       *pw = new_op;
       } else {
       if (PyArray_CanCastSafely(t1, t2)) {
       *pm = (PyArrayObject *)PyArray_Cast(*pm, t2);
       if ((new_op = PyArray_FromObject(*pw, t2, 0, 0)) == NULL) return -1;
       *pw = new_op;
       } else {
       PyErr_SetString(PyExc_TypeError, "cannot perform this operation on these types");
       return -1;
       }
       }
       return 0;
    ***/
}

static PyObject *PyUFunc_BinaryFunction(PyUFuncObject *s, PyArrayObject *mp1, PyObject *mp2) {
    PyObject *arglist;
    PyArrayObject *mps[3];

    arglist = Py_BuildValue("(OO)", mp1, mp2);
    mps[0] = mps[1] = mps[2] = NULL;
    if (PyUFunc_GenericFunction(s, arglist, mps) == -1) {
	Py_DECREF(arglist);
	Py_XDECREF(mps[0]); Py_XDECREF(mps[1]); Py_XDECREF(mps[2]);
	return NULL;
    }

    Py_DECREF(mps[0]); Py_DECREF(mps[1]);
    Py_DECREF(arglist);
    return PyArray_Return(mps[2]);
}

/*This method adds the augmented assignment*/
/*functionality that was made available in Python 2.0*/
static PyObject *PyUFunc_InplaceBinaryFunction(PyUFuncObject *s, PyArrayObject *mp1, PyObject *mp2) {
    PyObject *arglist;
    PyArrayObject *mps[3];

    arglist = Py_BuildValue("(OOO)", mp1, mp2, mp1);

    mps[0] = mps[1] = mps[2] = NULL;
    if (PyUFunc_GenericFunction(s, arglist, mps) == -1) {
	Py_DECREF(arglist);
	Py_XDECREF(mps[0]); Py_XDECREF(mps[1]); Py_XDECREF(mps[2]);
	return NULL;
    }

    Py_DECREF(mps[0]); Py_DECREF(mps[1]);
    Py_DECREF(arglist);
    return PyArray_Return(mps[2]);
}

static PyObject *PyUFunc_UnaryFunction(PyUFuncObject *s, PyArrayObject *mp1) {
    PyObject *arglist;
    PyArrayObject *mps[3];

    arglist = Py_BuildValue("(O)", mp1);

    mps[0] = mps[1] = NULL;
    if (PyUFunc_GenericFunction(s, arglist, mps) == -1) {
	Py_DECREF(arglist);
	Py_XDECREF(mps[0]); Py_XDECREF(mps[1]);
	return NULL;
    }

    Py_DECREF(mps[0]);
    Py_DECREF(arglist);
    return PyArray_Return(mps[1]);
}

static PyObject *array_add(PyArrayObject *m1, PyObject *m2) {
    return PyUFunc_BinaryFunction(n_ops.add, m1, m2);
}
static PyObject *array_subtract(PyArrayObject *m1, PyObject *m2) {
    return PyUFunc_BinaryFunction(n_ops.subtract, m1, m2);
}
static PyObject *array_multiply(PyArrayObject *m1, PyObject *m2) {
    return PyUFunc_BinaryFunction(n_ops.multiply, m1, m2);
}
static PyObject *array_divide(PyArrayObject *m1, PyObject *m2) {
    return PyUFunc_BinaryFunction(n_ops.divide, m1, m2);
}
static PyObject *array_remainder(PyArrayObject *m1, PyObject *m2) {
    return PyUFunc_BinaryFunction(n_ops.remainder, m1, m2);
}
static PyObject *array_power(PyArrayObject *m1, PyObject *m2) {
    return PyUFunc_BinaryFunction(n_ops.power, m1, m2);
}
static PyObject *array_negative(PyArrayObject *m1) {
    return PyUFunc_UnaryFunction(n_ops.negative, m1);
}
static PyObject *array_absolute(PyArrayObject *m1) {
    return PyUFunc_UnaryFunction(n_ops.absolute, m1);
}
static PyObject *array_invert(PyArrayObject *m1) {
    return PyUFunc_UnaryFunction(n_ops.invert, m1);
}
static PyObject *array_left_shift(PyArrayObject *m1, PyObject *m2) {
    return PyUFunc_BinaryFunction(n_ops.left_shift, m1, m2);
}
static PyObject *array_right_shift(PyArrayObject *m1, PyObject *m2) {
    return PyUFunc_BinaryFunction(n_ops.right_shift, m1, m2);
}
static PyObject *array_bitwise_and(PyArrayObject *m1, PyObject *m2) {
    return PyUFunc_BinaryFunction(n_ops.bitwise_and, m1, m2);
}
static PyObject *array_bitwise_or(PyArrayObject *m1, PyObject *m2) {
    return PyUFunc_BinaryFunction(n_ops.bitwise_or, m1, m2);
}
static PyObject *array_bitwise_xor(PyArrayObject *m1, PyObject *m2) {
    return PyUFunc_BinaryFunction(n_ops.bitwise_xor, m1, m2);
}


/*These methods add the augmented assignment*/
/*functionality that was made available in Python 2.0*/
static PyObject *array_inplace_add(PyArrayObject *m1, PyObject *m2) {
    return PyUFunc_InplaceBinaryFunction(n_ops.add, m1, m2);
}
static PyObject *array_inplace_subtract(PyArrayObject *m1, PyObject *m2) {
    return PyUFunc_InplaceBinaryFunction(n_ops.subtract, m1, m2);
}
static PyObject *array_inplace_multiply(PyArrayObject *m1, PyObject *m2) {
    return PyUFunc_InplaceBinaryFunction(n_ops.multiply, m1, m2);
}
static PyObject *array_inplace_divide(PyArrayObject *m1, PyObject *m2) {
    return PyUFunc_InplaceBinaryFunction(n_ops.divide, m1, m2);
}
static PyObject *array_inplace_remainder(PyArrayObject *m1, PyObject *m2) {
    return PyUFunc_InplaceBinaryFunction(n_ops.remainder, m1, m2);
}
static PyObject *array_inplace_power(PyArrayObject *m1, PyObject *m2) {
    return PyUFunc_InplaceBinaryFunction(n_ops.power, m1, m2);
}
static PyObject *array_inplace_left_shift(PyArrayObject *m1, PyObject *m2) {
    return PyUFunc_InplaceBinaryFunction(n_ops.left_shift, m1, m2);
}
static PyObject *array_inplace_right_shift(PyArrayObject *m1, PyObject *m2) {
    return PyUFunc_InplaceBinaryFunction(n_ops.right_shift, m1, m2);
}
static PyObject *array_inplace_bitwise_and(PyArrayObject *m1, PyObject *m2) {
    return PyUFunc_InplaceBinaryFunction(n_ops.bitwise_and, m1, m2);
}
static PyObject *array_inplace_bitwise_or(PyArrayObject *m1, PyObject *m2) {
    return PyUFunc_InplaceBinaryFunction(n_ops.bitwise_or, m1, m2);
}
static PyObject *array_inplace_bitwise_xor(PyArrayObject *m1, PyObject *m2) {
    return PyUFunc_InplaceBinaryFunction(n_ops.bitwise_xor, m1, m2);
}

/*Added by Bruce Sherwood Dec 2001*/
/*These methods add the floor and true division*/
/*functionality that was made available in Python 2.2*/
static PyObject *array_floor_divide(PyArrayObject *m1, PyObject *m2) {
    return PyUFunc_BinaryFunction(n_ops.floor_divide, m1, m2);
}
static PyObject *array_true_divide(PyArrayObject *m1, PyObject *m2) {
    return PyUFunc_BinaryFunction(n_ops.true_divide, m1, m2);
}
static PyObject *array_inplace_floor_divide(PyArrayObject *m1, PyObject *m2) {
    return PyUFunc_InplaceBinaryFunction(n_ops.floor_divide, m1, m2);
}
static PyObject *array_inplace_true_divide(PyArrayObject *m1, PyObject *m2) {
    return PyUFunc_InplaceBinaryFunction(n_ops.true_divide, m1, m2);
}
/*End of methods added by Bruce Sherwood*/

/* Array evaluates as "true" if any of the elements are non-zero */
static int array_nonzero(PyArrayObject *mp) {
    char *zero;
    PyArrayObject *self;
    char *data;
    int i, s, elsize;

    self = PyArray_CONTIGUOUS(mp);
    zero = self->descr->zero;

    s = SIZE(self);
    elsize = self->descr->elsize;
    data = self->data;
    for(i=0; i<s; i++, data+=elsize) {
	if (memcmp(zero, data, elsize) != 0) break;
    }

    Py_DECREF(self);

    return i!=s;
}

static PyObject *array_divmod(PyArrayObject *op1, PyObject *op2) {
    PyObject *divp, *modp, *result;

    divp = array_divide(op1, op2);
    if (divp == NULL) return NULL;
    modp = array_remainder(op1, op2);
    if (modp == NULL) {
	Py_DECREF(divp);
	return NULL;
    }
    result = Py_BuildValue("OO", divp, modp);
    Py_DECREF(divp);
    Py_DECREF(modp);
    return result;
}


/* methods added by Travis Oliphant, Jan 2000 */
PyObject *array_int(PyArrayObject *v) {
    PyObject *pv, *pv2;
    if (PyArray_SIZE(v) != 1) {
	PyErr_SetString(PyExc_TypeError, "only length-1 arrays can be converted to Python scalars.");
	return NULL;
    }
    pv = v->descr->getitem(v->data);
    if (pv == NULL) return NULL;
    if (pv->ob_type->tp_as_number == 0) {
	PyErr_SetString(PyExc_TypeError, "cannot convert to an int, scalar object is not a number.");
	Py_DECREF(pv);
	return NULL;
    }
    if (pv->ob_type->tp_as_number->nb_int == 0) {
	PyErr_SetString(PyExc_TypeError, "don't know how to convert scalar number to int");
	Py_DECREF(pv);
	return NULL;
    }

    pv2 = pv->ob_type->tp_as_number->nb_int(pv);
    Py_DECREF(pv);
    return pv2;
}

static PyObject *array_float(PyArrayObject *v) {
    PyObject *pv, *pv2;
    if (PyArray_SIZE(v) != 1) {
	PyErr_SetString(PyExc_TypeError, "only length-1 arrays can be converted to Python scalars.");
	return NULL;
    }
    pv = v->descr->getitem(v->data);
    if (pv == NULL) return NULL;
    if (pv->ob_type->tp_as_number == 0) {
	PyErr_SetString(PyExc_TypeError, "cannot convert to an int, scalar object is not a number.");
	Py_DECREF(pv);
	return NULL;
    }
    if (pv->ob_type->tp_as_number->nb_float == 0) {
	PyErr_SetString(PyExc_TypeError, "don't know how to convert scalar number to float");
	Py_DECREF(pv);
	return NULL;
    }
    pv2 = pv->ob_type->tp_as_number->nb_float(pv);
    Py_DECREF(pv);
    return pv2;
}

static PyObject *array_long(PyArrayObject *v) {
    PyObject *pv, *pv2;
    if (PyArray_SIZE(v) != 1) {
	PyErr_SetString(PyExc_TypeError, "only length-1 arrays can be converted to Python scalars.");
	return NULL;
    }
    pv = v->descr->getitem(v->data);
    if (pv->ob_type->tp_as_number == 0) {
	PyErr_SetString(PyExc_TypeError, "cannot convert to an int, scalar object is not a number.");
	return NULL;
    }
    if (pv->ob_type->tp_as_number->nb_long == 0) {
	PyErr_SetString(PyExc_TypeError, "don't know how to convert scalar number to long");
	return NULL;
    }
    pv2 = pv->ob_type->tp_as_number->nb_long(pv);
    Py_DECREF(pv);
    return pv2;
}

static PyObject *array_oct(PyArrayObject *v) {
    PyObject *pv, *pv2;
    if (PyArray_SIZE(v) != 1) {
	PyErr_SetString(PyExc_TypeError, "only length-1 arrays can be converted to Python scalars.");
	return NULL;
    }
    pv = v->descr->getitem(v->data);
    if (pv->ob_type->tp_as_number == 0) {
	PyErr_SetString(PyExc_TypeError, "cannot convert to an int, scalar object is not a number.");
	return NULL;
    }
    if (pv->ob_type->tp_as_number->nb_oct == 0) {
	PyErr_SetString(PyExc_TypeError, "don't know how to convert scalar number to oct");
	return NULL;
    }
    pv2 = pv->ob_type->tp_as_number->nb_oct(pv);
    Py_DECREF(pv);
    return pv2;
}

static PyObject *array_hex(PyArrayObject *v) {
    PyObject *pv, *pv2;
    if (PyArray_SIZE(v) != 1) {
	PyErr_SetString(PyExc_TypeError, "only length-1 arrays can be converted to Python scalars.");
	return NULL;
    }
    pv = v->descr->getitem(v->data);
    if (pv->ob_type->tp_as_number == 0) {
	PyErr_SetString(PyExc_TypeError, "cannot convert to an int, scalar object is not a number.");
	return NULL;
    }
    if (pv->ob_type->tp_as_number->nb_hex == 0) {
	PyErr_SetString(PyExc_TypeError, "don't know how to convert scalar number to hex");
	return NULL;
    }
    pv2 = pv->ob_type->tp_as_number->nb_hex(pv);
    Py_DECREF(pv);
    return pv2;
}

/* end of methods added by Travis Oliphant */


static PyNumberMethods array_as_number = {
    (binaryfunc)array_add,                  /*nb_add*/
    (binaryfunc)array_subtract,             /*nb_subtract*/
    (binaryfunc)array_multiply,             /*nb_multiply*/
    (binaryfunc)array_divide,               /*nb_divide*/
    (binaryfunc)array_remainder,            /*nb_remainder*/
    (binaryfunc)array_divmod,               /*nb_divmod*/
    (ternaryfunc)array_power,               /*nb_power*/
    (unaryfunc)array_negative,
    (unaryfunc)PyArray_Copy,                /*nb_pos*/
    (unaryfunc)array_absolute,              /*(unaryfunc)array_abs,*/
    (inquiry)array_nonzero,                 /*nb_nonzero*/
    (unaryfunc)array_invert,                /*nb_invert*/
    (binaryfunc)array_left_shift,           /*nb_lshift*/
    (binaryfunc)array_right_shift,          /*nb_rshift*/
    (binaryfunc)array_bitwise_and,          /*nb_and*/
    (binaryfunc)array_bitwise_xor,          /*nb_xor*/
    (binaryfunc)array_bitwise_or,           /*nb_or*/
    (coercion)array_coerce,                 /*nb_coerce*/
    (unaryfunc)array_int,                   /*nb_int*/
    (unaryfunc)array_long,                  /*nb_long*/
    (unaryfunc)array_float,                 /*nb_float*/
    (unaryfunc)array_oct,	            /*nb_oct*/
    (unaryfunc)array_hex,	            /*nb_hex*/

    /*This code adds augmented assignment functionality*/
    /*that was made available in Python 2.0*/
    (binaryfunc)array_inplace_add,          /*inplace_add*/
    (binaryfunc)array_inplace_subtract,     /*inplace_subtract*/
    (binaryfunc)array_inplace_multiply,     /*inplace_multiply*/
    (binaryfunc)array_inplace_divide,       /*inplace_divide*/
    (binaryfunc)array_inplace_remainder,    /*inplace_remainder*/
    (ternaryfunc)array_inplace_power,       /*inplace_power*/
    (binaryfunc)array_inplace_left_shift,   /*inplace_lshift*/
    (binaryfunc)array_inplace_right_shift,  /*inplace_rshift*/
    (binaryfunc)array_inplace_bitwise_and,  /*inplace_and*/
    (binaryfunc)array_inplace_bitwise_xor,  /*inplace_xor*/
    (binaryfunc)array_inplace_bitwise_or,   /*inplace_or*/

        /* Added in release 2.2 */
	/* The following require the Py_TPFLAGS_HAVE_CLASS flag */
#if PY_VERSION_HEX >= 0x02020000
	(binaryfunc)array_floor_divide,          /*nb_floor_divide*/
	(binaryfunc)array_true_divide,           /*nb_true_divide*/
	(binaryfunc)array_inplace_floor_divide,  /*nb_inplace_floor_divide*/
	(binaryfunc)array_inplace_true_divide,   /*nb_inplace_true_divide*/
#endif
};

static PySequenceMethods array_as_sequence = {
    (inquiry)array_length,		/*sq_length*/
    (binaryfunc)NULL, /*nb_add, concat is numeric add*/
    (intargfunc)NULL, /*nb_multiply, repeat is numeric multiply*/
    (intargfunc)array_item_nice,	/*sq_item*/
    (intintargfunc)array_slice,		/*sq_slice*/
    (intobjargproc)array_ass_item,	/*sq_ass_item*/
    (intintobjargproc)array_ass_slice,	/*sq_ass_slice*/
};

/* -------------------------------------------------------------- */

/*This ugly function quickly prints out an array.  It's likely to disappear */
/*in the near future. (or maybe not...) */
static int dump_data(char **string, int *n, int *max_n,
                     char *data, int nd, int *dimensions,
                     int *strides, PyArray_Descr *descr) {
    PyObject *op, *sp;
    char *ostring;
    int i, N;

#define CHECK_MEMORY if (*n >= *max_n-16) { *max_n *= 2; *string = (char *)realloc(*string, *max_n); }

    if (nd == 0) {

	if ((op = descr->getitem(data)) == NULL) return -1;
	sp = PyObject_Repr(op);
	if (sp == NULL) {Py_DECREF(op); return -1;}
	ostring = PyString_AsString(sp);
	N = PyString_Size(sp)*sizeof(char);
	*n += N;
	CHECK_MEMORY
	    memmove(*string+(*n-N), ostring, N);
	Py_DECREF(sp);
	Py_DECREF(op);
	return 0;
    } else {
	if (nd == 1 && descr->type_num == PyArray_CHAR) {
	    N = (dimensions[0]+2)*sizeof(char);
	    *n += N;
	    CHECK_MEMORY
		(*string)[*n-N] = '"';
	    memmove(*string+(*n-N+1), data, N-2);
	    (*string)[*n-1] = '"';
	    return 0;
	} else {
	    CHECK_MEMORY
		(*string)[*n] = '[';
	    *n += 1;
	    for(i=0; i<dimensions[0]; i++) {
		if (dump_data(string, n, max_n, data+(*strides)*i, nd-1, dimensions+1, strides+1, descr) < 0) return -1;
		CHECK_MEMORY
		    if (i<dimensions[0]-1) {
			(*string)[*n] = ',';
			(*string)[*n+1] = ' ';
			*n += 2;
		    }
	    }
	    CHECK_MEMORY
		(*string)[*n] = ']'; *n += 1;
	    return 0;
	}
    }

#undef CHECK_MEMORY
}

static PyObject * array_repr_builtin(PyArrayObject *self) {
    PyObject *ret;
    char *string;
    int n, max_n;

    max_n = NBYTES(self)*4*sizeof(char) + 7;

    if ((string = (char *)malloc(max_n)) == NULL) {
	PyErr_SetString(PyExc_MemoryError, "out of memory");
	return NULL;
    }

    n = 6;
    sprintf(string, "array(");

    if (dump_data(&string, &n, &max_n, self->data, self->nd, self->dimensions,
		  self->strides, self->descr) < 0) { free(string); return NULL; }

    sprintf(string+n, ", '%c')", self->descr->type);

    ret = PyString_FromStringAndSize(string, n+6);
    free(string);
    return ret;
}

static PyObject *PyArray_StrFunction=NULL;
static PyObject *PyArray_ReprFunction=NULL;

extern void PyArray_SetStringFunction(PyObject *op, int repr) {
    if (repr) {
	Py_XDECREF(PyArray_ReprFunction); /* Dispose of previous callback */
	Py_XINCREF(op); /* Add a reference to new callback */
	PyArray_ReprFunction = op; /* Remember new callback */
    } else {
	Py_XDECREF(PyArray_StrFunction); /* Dispose of previous callback */
	Py_XINCREF(op); /* Add a reference to new callback */
	PyArray_StrFunction = op; /* Remember new callback */
    }
}

static PyObject *array_repr(PyArrayObject *self) {
    PyObject *s, *arglist;

    if (PyArray_ReprFunction == NULL) {
	s = array_repr_builtin(self);
    } else {
	arglist = Py_BuildValue("(O)", self);
	s = PyEval_CallObject(PyArray_ReprFunction, arglist);
	Py_DECREF(arglist);
    }
    return s;
}

static PyObject *array_str(PyArrayObject *self) {
    PyObject *s, *arglist;

    if (PyArray_StrFunction == NULL) {
	s = array_repr(self);
    } else {
	arglist = Py_BuildValue("(O)", self);
	s = PyEval_CallObject(PyArray_StrFunction, arglist);
	Py_DECREF(arglist);
    }
    return s;
}

/* No longer implemented here.  Use the default in object.c
   static int array_print(PyArrayObject *self, FILE *fp, int flags) {
   PyObject *s;
   int r=-1, n;

   s = array_str(self);

   if (s == NULL || !PyString_Check(s)) {
   Py_XDECREF(s);
   return -1;
   }
   n = PyString_Size(s);
   r = fwrite(PyString_AsString(s), sizeof(char), n, fp);
   Py_DECREF(s);
   return r==n ? 0 : -1;
   }
*/

static void byte_swap_vector(void *p, int n, int size) {
    char *a, *b, c;

    switch(size) {
    case 2:
	for (a = (char*)p ; n > 0; n--, a += 1) {
	    b = a + 1;
	    c = *a; *a++ = *b; *b   = c;
	}
	break;
    case 4:
	for (a = (char*)p ; n > 0; n--, a += 2) {
	    b = a + 3;
	    c = *a; *a++ = *b; *b-- = c;
	    c = *a; *a++ = *b; *b   = c;
	}
	break;
    case 8:
	for (a = (char*)p ; n > 0; n--, a += 4) {
	    b = a + 7;
	    c = *a; *a++ = *b; *b-- = c;
	    c = *a; *a++ = *b; *b-- = c;
	    c = *a; *a++ = *b; *b-- = c;
	    c = *a; *a++ = *b; *b   = c;
	}
	break;
    default:
	break;
    }
}

static char doc_byteswapped[] = "m.byteswapped().  Swaps the bytes in the contents of array m.  Useful for reading data written by a machine with a different byte order.  Returns a new array with the byteswapped data.";

static PyObject * array_byteswap(PyArrayObject *self, PyObject *args) {
    PyArrayObject *ret;

    if (!PyArg_ParseTuple(args, "")) return NULL;

    if ((ret = (PyArrayObject *)PyArray_Copy(self)) == NULL) return NULL;

    if (self->descr->type_num < PyArray_CFLOAT) {
	byte_swap_vector(ret->data, SIZE(self), self->descr->elsize);
    } else {
	byte_swap_vector(ret->data, SIZE(self)*2, self->descr->elsize/2);
    }

    return (PyObject *)ret;
}

static char doc_tostring[] = "m.tostring().  Copy the data portion of the array to a python string and return that string.";

static PyObject *array_tostring(PyArrayObject *self, PyObject *args) {
    PyObject *so;
    PyArrayObject *mo;

    if (!PyArg_ParseTuple(args, "")) return NULL;

    if ((mo = PyArray_CONTIGUOUS(self)) ==NULL) return NULL;

    so = PyString_FromStringAndSize(mo->data, NBYTES(mo));

    Py_DECREF(mo);

    return so;
}


static PyObject *PyArray_ToList(PyObject *self) {
    PyObject *lp;
    PyObject *v;
    int sz, i;

    if (!PyArray_Check(self)) return self;

    if (((PyArrayObject *)self)->nd == 0) return ((PyArrayObject *)self)->descr->getitem(((PyArrayObject *)self)->data);

    sz = ((PyArrayObject *)self)->dimensions[0];
    lp = PyList_New(sz);

    for (i=0; i<sz; i++) {
	v=array_item((PyArrayObject *)self, i);
	PyList_SetItem(lp, i, PyArray_ToList(v));
	if (((PyArrayObject *)self)->nd>1){
	    Py_DECREF(v);
	}
    }

    return lp;
}


static char doc_tolist[] = "m.tolist().  Copy the data portion of the array to a hierarchical python list and return that list.";

static PyObject *array_tolist(PyArrayObject *self, PyObject *args) {
    if (!PyArg_ParseTuple(args, "")) return NULL;
    if (self->nd <= 0) {
	PyErr_SetString(PyExc_ValueError, "Can't convert a 0d array to a list");
	return NULL;
    }

    return PyArray_ToList((PyObject *)self);
}

static char doc_toscalar[] = "m.toscalar().  Copy the first data point of the array to a Python scalar and return it.";

static PyObject *array_toscalar(PyArrayObject *self, PyObject *args) {
    if (!PyArg_ParseTuple(args, "")) return NULL;
    return self->descr->getitem(self->data);
}

static char doc_cast[] = "m.astype(t).  Cast array m to type t.  t can be either a string representing a typecode, or a python type object of type int, float, or complex.";

PyObject *array_cast(PyArrayObject *self, PyObject *args) {
    PyObject *op;
    char typecode;

    if (!PyArg_ParseTuple(args, "O", &op)) return NULL;

    if (PyString_Check(op) && (PyString_Size(op) == 1)) {
        typecode = PyString_AS_STRING((PyStringObject *)op)[0];
        if (PyArray_ValidType(typecode)) return PyArray_Cast(self, typecode);
        PyErr_SetString(PyExc_ValueError, "Invalid type for array");
        return NULL;
    }

    if (PyType_Check(op)) {
	typecode = 'O';
	if (((PyTypeObject *)op) == &PyInt_Type) {
	    typecode = PyArray_LONG;
	}
	if (((PyTypeObject *)op) == &PyFloat_Type) {
	    typecode = PyArray_DOUBLE;
	}
	if (((PyTypeObject *)op) == &PyComplex_Type) {
	    typecode = PyArray_CDOUBLE;
	}
	return PyArray_Cast(self, typecode);
    }
    PyErr_SetString(PyExc_ValueError,
		    "type must be either a 1-length string, or a python type object");
    return NULL;
}

static char doc_typecode[] = "m.typecode().  Return the single character code indicating the type of the elements of m.";

PyObject *array_typecode(PyArrayObject *self, PyObject *args) {
    if (!PyArg_ParseTuple(args, "")) return NULL;

    return PyString_FromStringAndSize(&(self->descr->type), 1);
}

static char doc_itemsize[] = "m.itemsize().  Return the size in bytes of a single element of the array m.";

PyObject *array_itemsize(PyArrayObject *self, PyObject *args) {
    if (!PyArg_ParseTuple(args, "")) return NULL;

    return PyInt_FromLong(self->descr->elsize);
}

static char doc_contiguous[] = "m.contiguous().  Return true if the memory used by the array m is contiguous.  A non-contiguous array can be converted to a contiguous one by m.copy().";

PyObject *array_contiguous(PyArrayObject *self, PyObject *args) {
    if (!PyArg_ParseTuple(args, "")) return NULL;

    return PyInt_FromLong(ISCONTIGUOUS(self));
}

/* Added savespace methods Jan. 2000 -- Travis Oliphant */
static char doc_savespace[] = "m.savespace(flag=1).  Change the savespace parameter of the array m to flag.";

PyObject *array_savespace(PyArrayObject *self, PyObject *args, PyObject *kws)
{
    char flag=1;
    char *kwd[] = {"flag",NULL};

    if (!PyArg_ParseTupleAndKeywords(args,kws,"|b", kwd, &flag))
	return NULL;

    if (flag == 0)
	self->flags &= ~SAVESPACE;
    else
	self->flags |= SAVESPACE;

    Py_INCREF(Py_None);
    return Py_None;
}

static char doc_spacesaver[] = "m.spacesaver().  Return true if the spacesaving parameter is set for m.";

PyObject *array_spacesaver(PyArrayObject *self, PyObject *args) {
    if (!PyArg_ParseTuple(args, "")) return NULL;

    return PyInt_FromLong(PyArray_ISSPACESAVER(self) != 0);
}


static char doc_copy[] = "m.copy(). Return a copy of the array.";

PyObject *array_copy(PyArrayObject *self, PyObject *args) {
    if (!PyArg_ParseTuple(args, "")) return NULL;

    return PyArray_Copy(self);
}

static char doc_resize[] = "m.resize(new_shape). Return a resized version of the array.  Must be contiguous and not be referenced by other arrays.";

PyObject *array_resize(PyArrayObject *self, PyObject *args) {
    PyObject *new_shape;
    if (!PyArg_ParseTuple(args, "O", &new_shape)) return NULL;

    return PyArray_Resize(self, new_shape);
}

static char doc_deepcopy[] = "Used if copy.deepcopy is called on an array.";

PyObject *array_deepcopy(PyArrayObject *self, PyObject *args) {
    PyObject* visit;
    if (!PyArg_ParseTuple(args, "O", &visit)) return NULL;
	if (self->descr->type == 'O') {
	    PyErr_SetString(PyExc_TypeError,
	                    "Deep copy not implemented for Numerical arrays of type object.");
	    return 0;
	}
    return PyArray_Copy(self);
}


/* Changed by Travis Oliphant Jan, 2000 to allow comparison of scalars.*/
int array_compare(PyArrayObject *self, PyObject *other) {
    PyArrayObject *aother;
    PyObject *o1, *o2;
    int val, result;
    aother = (PyArrayObject *)other;
    if (self->nd != 0 || aother->nd != 0) {
	PyErr_SetString(PyExc_TypeError,
			"Comparison of multiarray objects other than rank-0 arrays is not implemented.");
	return -1;
    }
    o1 = self->descr->getitem(self->data);
    o2 = aother->descr->getitem(aother->data);
    if (o1 == NULL || o2 == NULL) return -1;
    val = PyObject_Cmp(o1,o2,&result);
    Py_DECREF(o1);
    Py_DECREF(o2);
    if (val < 0) {
	PyErr_SetString(PyExc_TypeError, "objects cannot be compared.");
	return -1;
    }
    return result;
}

/* Added by Scott N. Gunyan  Feb, 2001
   This will implement the new Rich Comparisons added in Python 2.1
   to allow returning an array object */
#if PY_VERSION_HEX >= 0x02010000
PyObject *array_richcompare(PyArrayObject *self, PyObject *other, int cmp_op) {
    PyObject *array_other, *result;
    PyObject *Py_Zero, *Py_One;

    switch (cmp_op)
        {
        case Py_LT:
            return PyUFunc_BinaryFunction(n_ops.less, self, other);
        case Py_LE:
            return PyUFunc_BinaryFunction(n_ops.less_equal, self, other);
        case Py_EQ:
	    /* Try to convert other to an array */
	    array_other = PyArray_FromObject(other, PyArray_NOTYPE, 0, 0);
	    /* If not successful, then return the integer object 0.
	       This fixes code that used to allow equality comparisons
	       between arrays and other objects which would give a result of 0
	    */
	    Py_Zero = PyInt_FromLong(0);
	    if ((array_other == NULL) || (array_other == Py_None)) {
		Py_XDECREF(array_other);
		PyErr_Clear();
		return Py_Zero;
	    }
	    result = PyUFunc_BinaryFunction(n_ops.equal, self, array_other);
	    /* If the comparison results in NULL, then the two array objects
               can not be compared together so return zero
	    */
	    Py_DECREF(array_other);
	    if (result == NULL) {
		PyErr_Clear();
		return Py_Zero;
	    }
	    Py_DECREF(Py_Zero);
	    return result;
        case Py_NE:
	    /* Try to convert other to an array */
	    array_other = PyArray_FromObject(other, PyArray_NOTYPE, 0, 0);
	    /* If not successful, then objects cannot be compared and cannot be
	       equal, therefore, return the integer object 1.
	    */
	    Py_One = PyInt_FromLong(1);
	    if ((array_other == NULL) || (array_other == Py_None)) {
		Py_XDECREF(array_other);
		PyErr_Clear();
		return Py_One;
	    }
	    result = PyUFunc_BinaryFunction(n_ops.not_equal, self, array_other);
	    Py_DECREF(array_other);
	    if (result == NULL) {
		PyErr_Clear();
		return Py_One;
	    }
	    Py_DECREF(Py_One);
	    return result;
        case Py_GT:
            return PyUFunc_BinaryFunction(n_ops.greater, self, other);
        case Py_GE:
            return PyUFunc_BinaryFunction(n_ops.greater_equal, self, other);
        }
    return NULL;
}
#endif

static struct PyMethodDef array_methods[] = {
    {"tostring",   (PyCFunction)array_tostring,	1, doc_tostring},
    {"tolist",   (PyCFunction)array_tolist,	1, doc_tolist},
    {"toscalar", (PyCFunction)array_toscalar,     METH_VARARGS, doc_toscalar},
    {"byteswapped",   (PyCFunction)array_byteswap,	1, doc_byteswapped},
    {"astype", (PyCFunction)array_cast, 1, doc_cast},
    {"typecode",   (PyCFunction)array_typecode, 1, doc_typecode},
    {"itemsize",   (PyCFunction)array_itemsize, 1, doc_itemsize},
    {"iscontiguous", (PyCFunction)array_contiguous, 1, doc_contiguous},
    {"savespace", (PyCFunction)array_savespace, METH_VARARGS | METH_KEYWORDS, doc_savespace},
    {"spacesaver", (PyCFunction)array_spacesaver, METH_VARARGS, doc_spacesaver},
    {"copy", (PyCFunction)array_copy, 1, doc_copy},  /* to make easy copies */
    {"resize", (PyCFunction)array_resize, 1, doc_resize}, /* fast resizing */
    {"__copy__", (PyCFunction)array_copy, 1, doc_copy},  /* for the copy module */
    {"__deepcopy__", (PyCFunction)array_deepcopy, 1, doc_deepcopy},  /* for the copy module */
    {NULL,		NULL}		/* sentinel */
};

static void
interface_struct_free (void *ptr, void *arr)
{
    PyArrayInterface *inter=ptr;
    
    Py_DECREF((PyObject *)arr);
    if (inter->nd != 0 && (sizeof(int) != sizeof(Py_intptr_t))) {
	free(inter->shape);
    }
    free(ptr);
}

/* ---------- */
static PyObject *array_getattr(PyArrayObject *self, char *name) {
    PyArrayObject *ret;

    if (strcmp(name, "real") == 0) {
	if (self->descr->type_num == PyArray_CFLOAT ||
	    self->descr->type_num == PyArray_CDOUBLE) {
	    ret = (PyArrayObject *)PyArray_FromDimsAndData(self->nd,
							   self->dimensions,
							   self->descr->type_num-2,
							   self->data);
	    if (ret == NULL) return NULL;
	    memmove(ret->strides, self->strides, sizeof(int)*ret->nd);
	    ret->flags &= ~CONTIGUOUS;
	    Py_INCREF(self);
	    ret->base = (PyObject *)self;
	    return (PyObject *)ret;
	} else {
	    ret = (PyArrayObject *)PyArray_FromDimsAndData(self->nd,
							   self->dimensions,
							   self->descr->type_num,
							   self->data);
	    if (ret == NULL) return NULL;
	    Py_INCREF(self);
	    ret->base = (PyObject *)self;
            return (PyObject *)ret;
	}
    }

    if ((strcmp(name, "imaginary") == 0) || (strcmp(name, "imag") == 0)) {
	if (self->descr->type_num == PyArray_CFLOAT ||
	    self->descr->type_num == PyArray_CDOUBLE) {
	    ret = (PyArrayObject *)PyArray_FromDimsAndData(self->nd,
							   self->dimensions,
							   self->descr->type_num-2,
							   self->data+self->descr->elsize/2);
	    if (ret == NULL) return NULL;
	    memmove(ret->strides, self->strides, sizeof(int)*ret->nd);
	    ret->flags &= ~CONTIGUOUS;
	    Py_INCREF(self);
	    ret->base = (PyObject *)self;
	    return (PyObject *)ret;
	} else {
	    PyErr_SetString(PyExc_ValueError, "No imaginary part to real array");
	    return NULL;
	}
    }

    if (strcmp(name, "flat") == 0) {
	int n;
	n = SIZE(self);
	if (!ISCONTIGUOUS(self)) {
	    PyErr_SetString(PyExc_ValueError, "flattened indexing only available for contiguous array");
	    return NULL;
	}
	ret = (PyArrayObject *)PyArray_FromDimsAndDataAndDescr(1, &n,
							       self->descr,
							       self->data);
	if (ret == NULL) return NULL;
	Py_INCREF(self);
	ret->base = (PyObject *)self;
	return (PyObject *)ret;
    }

    if (strcmp(name, "__array_struct__") == 0) {
        PyArrayInterface *inter;
        inter = (PyArrayInterface *)malloc(sizeof(PyArrayInterface));
        inter->version = 2;
        inter->nd = self->nd;
        if ((inter->nd == 0) || (sizeof(int) == sizeof(Py_intptr_t))) {
            inter->shape = (Py_intptr_t *)self->dimensions;
            inter->strides = (Py_intptr_t *)self->strides;
        }
	else {
	    int i;
	    inter->shape = (Py_intptr_t *)malloc(self->nd*2*sizeof(Py_intptr_t));
	    inter->strides = inter->shape + (self->nd);
	    for (i=0; i<self->nd; i++) {
		inter->shape[i] = self->dimensions[i];
		inter->strides[i] = self->strides[i];
	    }
	}
        inter->flags = (self->flags & CONTIGUOUS) | ALIGNED | NOTSWAPPED | WRITEABLE;
        inter->data = self->data;
        inter->itemsize = self->descr->elsize;
        
        switch(self->descr->type_num) {
        case PyArray_DOUBLE:
        case PyArray_FLOAT:
            inter->typekind = 'f';
	    break;
        case PyArray_UBYTE:
	case PyArray_UINT:
        case PyArray_USHORT:
	    inter->typekind = 'u';
	    break;
        case PyArray_LONG:
        case PyArray_INT:
        case PyArray_SBYTE:
        case PyArray_SHORT:
            inter->typekind = 'i';
	    break;
        case PyArray_CFLOAT:
        case PyArray_CDOUBLE:
	    inter->typekind = 'c';
	    break;
        case PyArray_CHAR:
            inter->typekind = 'S';
	    break;
        case PyArray_OBJECT:
            inter->typekind = 'O';
	    break;
	default:
	    inter->typekind = 'V';
        } 
        
        Py_INCREF(self);
        return PyCObject_FromVoidPtrAndDesc(inter, self, interface_struct_free);
    }
    
    if (strcmp(name, "__array_data__") == 0) {
        return Py_BuildValue("NN", 
                             PyString_FromFormat("%p", self->data),
                             PyInt_FromLong(0));
    }

    if ((strcmp(name,"shape") == 0) || (strcmp(name, "__array_shape__")) == 0) {
	PyObject *res, *oi;
	int i;
	res = PyTuple_New(self->nd);
	if (res == NULL) return NULL;
	for (i=0; i<self->nd; i++) {
	    oi = PyInt_FromLong(self->dimensions[i]);
	    if (!oi) { Py_DECREF(res); return NULL; }
	    PyTuple_SET_ITEM(res, i, oi);
	}
	return res;
    }

    if (strcmp(name, "__array_strides__") == 0) {
	PyObject *res, *oi;
	int i;

        if (ISCONTIGUOUS(self)) {
            Py_INCREF(Py_None);
            return Py_None;
        }

	res = PyTuple_New(self->nd);
	for (i=0; i<self->nd; i++) {
	    oi = PyInt_FromLong(self->strides[i]);
	    if (!oi) { Py_DECREF(res); return NULL; }
	    PyTuple_SET_ITEM(res, i, oi);
	}
	return res;
    }

    if (strcmp(name, "__array_typestr__") == 0) {
	int size = self->descr->elsize;
        unsigned long number = 1;
        char *s = (char *) &number;
        char endian;
        if (s[0] == 0) endian = '>';
        else endian = '<';

	switch(self->descr->type_num) {
	case PyArray_CHAR:
	    return PyString_FromString("|S1");
	case PyArray_UBYTE:
	case PyArray_USHORT:
	case PyArray_UINT:
            return PyString_FromFormat("%cu%d",endian,size);
	case PyArray_SBYTE:
	case PyArray_SHORT:
	case PyArray_INT:
	case PyArray_LONG:
	    return PyString_FromFormat("%ci%d",endian,size);
	case PyArray_FLOAT:
	case PyArray_DOUBLE:
	    return PyString_FromFormat("%cf%d",endian,size);
	case PyArray_CFLOAT:
	case PyArray_CDOUBLE:
	    return PyString_FromFormat("%cc%d",endian,size);
	case PyArray_OBJECT:
	    return PyString_FromFormat("|O%d",size);
	default:
	    return PyString_FromFormat("|V%d",size);
	}
    }

    return Py_FindMethod(array_methods, (PyObject *)self, name);
}

static int array_setattr(PyArrayObject *self, char *name, PyObject *op) {
    PyArrayObject *ap;
    int ret;

    if (strcmp(name, "shape") == 0) {
	/* This can be made more efficient by copying code from array_reshape if needed */
	if ((ap = (PyArrayObject *)PyArray_Reshape(self, op)) == NULL) return -1;
	if(self->flags & OWN_DIMENSIONS) free(self->dimensions);
	self->dimensions = ap->dimensions;
	if(self->flags & OWN_STRIDES) free(self->strides);
	self->strides = ap->strides;
	self->nd = ap->nd;
	self->flags &= ~(OWN_DIMENSIONS | OWN_STRIDES);
	self->flags |= ap->flags & (OWN_DIMENSIONS | OWN_STRIDES);
	ap->flags &= ~(OWN_DIMENSIONS | OWN_STRIDES);
	Py_DECREF(ap);
	return 0;
    }

    if (strcmp(name, "real") == 0) {
	if (self->descr->type_num == PyArray_CFLOAT ||
	    self->descr->type_num == PyArray_CDOUBLE) {
	    ap = (PyArrayObject *)PyArray_FromDimsAndData(self->nd,
							  self->dimensions,
							  self->descr->type_num-2,
							  self->data);
	    if (ap == NULL) return -1;
	    memmove(ap->strides, self->strides, sizeof(int)*ap->nd);
	    ap->flags &= ~CONTIGUOUS;
	    ret = PyArray_CopyObject(ap, op);
	    Py_DECREF(ap);
	    return ret;
	} else {
	    return PyArray_CopyObject(self, op);
	}
    }

    if ((strcmp(name, "imaginary") == 0) || (strcmp(name, "imag") == 0)) {
	if (self->descr->type_num == PyArray_CFLOAT ||
	    self->descr->type_num == PyArray_CDOUBLE) {
	    ap = (PyArrayObject *)PyArray_FromDimsAndData(self->nd,
							  self->dimensions,
							  self->descr->type_num-2,
							  self->data+self->descr->elsize/2);
	    if (ap == NULL) return -1;
	    memmove(ap->strides, self->strides, sizeof(int)*ap->nd);
	    ap->flags &= ~CONTIGUOUS;
	    ret = PyArray_CopyObject(ap, op);
	    Py_DECREF(ap);
	    return ret;
	} else {
	    PyErr_SetString(PyExc_ValueError, "No imaginary part to real array");
	    return -1;
	}
    }


    PyErr_SetString(PyExc_AttributeError, "Attribute does not exist or cannot be set");
    return -1;
}

static char Arraytype__doc__[] =
"A array object represents a multidimensional, homogeneous array of basic values.  It has the following data members, m.shape (the size of each dimension in the array), m.itemsize (the size (in bytes) of each element of the array), and m.typecode (a character representing the type of the matrices elements).  Matrices are sequence, mapping and numeric objects.  Sequence indexing is similar to lists, with single indices returning a reference that points to the old matrices data, and slices returning by copy.  A array is also allowed to be indexed by a sequence of items.  Each member of the sequence indexes the corresponding dimension of the array. Numeric operations operate on matrices in an element-wise fashion.";


PyTypeObject PyArray_Type = {
    PyObject_HEAD_INIT(0)
    0,				          /*ob_size*/
    "array",			          /*tp_name*/
    sizeof(PyArrayObject),	          /*tp_basicsize*/
    0,				          /*tp_itemsize*/
    /* methods */
    (destructor)array_dealloc,	          /*tp_dealloc*/
    (printfunc)NULL,		          /*tp_print*/
    (getattrfunc)array_getattr,	          /*tp_getattr*/
    (setattrfunc)array_setattr,	          /*tp_setattr*/
    (cmpfunc)array_compare,	          /*tp_compare*/
    (reprfunc)array_repr,	          /*tp_repr*/
    &array_as_number,		          /*tp_as_number*/
    &array_as_sequence,		          /*tp_as_sequence*/
    &array_as_mapping,		          /*tp_as_mapping*/
    (hashfunc)0,		          /*tp_hash*/
    (ternaryfunc)0,		          /*tp_call*/
    (reprfunc)array_str,	          /*tp_str*/

    /* Space for future expansion */
    (getattrofunc)0L,                     /*tp_getattro*/
    (setattrofunc)0L,                     /*tp_setattro*/
    (PyBufferProcs *)&array_as_buffer,    /*tp_as_buffer*/
    (Py_TPFLAGS_HAVE_INPLACEOPS |
#if PY_VERSION_HEX >= 0x02020000
         Py_TPFLAGS_HAVE_CLASS |
#endif
#if PY_VERSION_HEX >= 0x02010000
	 Py_TPFLAGS_HAVE_RICHCOMPARE |
         Py_TPFLAGS_HAVE_WEAKREFS |
#endif
     Py_TPFLAGS_HAVE_GETCHARBUFFER),      /*tp_flags*/
    /*Documentation string */
    Arraytype__doc__,                     /*tp_doc*/

#if PY_VERSION_HEX >= 0x02010000
    (traverseproc)0L,
    (inquiry)0L,
    (richcmpfunc)array_richcompare,       /*tp_richcompfunc*/
    offsetof(PyArrayObject, weakreflist), /*tp_weaklistoffset */
#endif
};


/* The rest of this code is to build the right kind of array from a python */
/* object. */

static int discover_depth(PyObject *s, int max, int stop_at_string) {
    int d=0;
    PyObject *e;


    if(max < 1) return -1;

    if(! PySequence_Check(s) || PyInstance_Check(s) || \
       PySequence_Length(s) < 0) {
	    PyErr_Clear(); return 0;
    }
    if (PyArray_Check(s) && (((PyArrayObject *)s)->nd) == 0) return 0;
    if(PyString_Check(s)) return stop_at_string ? 0:1;
    if (PySequence_Length(s) == 0) return 1;

    if ((e=PySequence_GetItem(s,0)) == NULL) return -1;
    if (e != s)
	{
	    d=discover_depth(e,max-1, stop_at_string);
	    if(d >= 0) d++;
	}
    Py_DECREF(e);
    return d;
}

static int discover_dimensions(PyObject *s, int nd, int *d, int check_it) {
    PyObject *e;
    int r, n, i, n_lower;

    n=PyObject_Length(s);
    *d = n;
    if(*d < 0) return -1;
    if(nd <= 1) return 0;
    n_lower = 0;
    for(i=0; i<n; i++) {
	if ((e=PySequence_GetItem(s,i)) == NULL) return -1;
	r=discover_dimensions(e,nd-1,d+1,check_it);
	Py_DECREF(e);

	if (r == -1) return -1;
        if (check_it && n_lower != 0 && n_lower != d[1]) {
                PyErr_SetString(PyExc_ValueError,
                                "inconsistent shape in sequence");
                return -1;
        }
	if (d[1] > n_lower) {
                n_lower = d[1];
        }

    }
    d[1] = n_lower;

    return 0;
}

#ifndef max
#define max(a,b) (a)>(b)?(a):(b);
#endif

static PyArray_Descr *_array_descr_fromstr(char *, int *);


static int
array_objecttype(PyObject *op, int minimum_type, int savespaceflag, int max)
{
    int l;
    PyObject *ip;
    int result;
    PyArray_Descr* descr;

    if (minimum_type == -1) return -1;

    if (max < 0) return PyArray_OBJECT;  /* either we are too deep or in a
					   recursive loop, give up */

    if (PyArray_Check(op))
	    return max((int)((PyArrayObject *)op)->descr->type_num,
		       minimum_type);
    
    if ((ip=PyObject_GetAttrString(op, "__array_typestr__"))!=NULL) {
	int swap=0;
	descr=NULL;
	if (PyString_Check(ip)) {
	    descr = _array_descr_fromstr(PyString_AS_STRING(ip), &swap);
	}   
	Py_DECREF(ip);
	if (descr) {
	    return max(minimum_type, descr->type_num);
	}
    }
    else PyErr_Clear();
    
    if ((ip=PyObject_GetAttrString(op, "__array_struct__")) != NULL) {
	PyArrayInterface *inter;
	char buf[40];
	int swap=0;
	descr=NULL;
	if (PyCObject_Check(ip)) {
	    inter=(PyArrayInterface *)PyCObject_AsVoidPtr(ip);
	    if (inter->version == 2) {
		snprintf(buf, 40, "|%c%d", inter->typekind, inter->itemsize);
		descr = _array_descr_fromstr(buf, &swap);
	    }
	}
	Py_DECREF(ip);
	if (descr) return max(minimum_type, descr->type_num);
    }
    else PyErr_Clear();

    if (PyObject_HasAttrString(op, "__array__")) {
        ip = PyObject_CallMethod(op, "__array__", NULL);
        if(ip && PyArray_Check(ip)) {
		result = max(minimum_type, (int)((PyArrayObject *)ip)->descr->type_num);
		Py_DECREF(ip);
		return result;
	}
	else Py_XDECREF(ip);
    }

	
    if (PyString_Check(op) || PyUnicode_Check(op)) {
	return max(minimum_type, (int)PyArray_CHAR);
    }

    if (PyInstance_Check(op)) return PyArray_OBJECT;

    if (PySequence_Check(op)) {
	l = PyObject_Length(op);
        if (l < 0 && PyErr_Occurred()) { PyErr_Clear(); return (int)PyArray_OBJECT;}
	if (l == 0 && minimum_type == 0) minimum_type = (savespaceflag ? PyArray_SHORT : PyArray_LONG);
	while (--l >= 0) {
	    ip = PySequence_GetItem(op, l);
	    if (ip==NULL) {PyErr_Clear(); return (int) PyArray_OBJECT;}
	    minimum_type = array_objecttype(ip, minimum_type,
					    savespaceflag, max-1);
	    Py_DECREF(ip);
	}
	return minimum_type;
    }

    if (PyInt_Check(op)) {
	return max(minimum_type, (int) (savespaceflag ? PyArray_SHORT: PyArray_LONG));
    } else {
	if (PyFloat_Check(op)) {
	    return max(minimum_type, (int) (savespaceflag ? PyArray_FLOAT : PyArray_DOUBLE));
	} else {
	    if (PyComplex_Check(op)) {
		return max(minimum_type, (int) (savespaceflag ? PyArray_CFLOAT : PyArray_CDOUBLE));
	    } else {
		return (int)PyArray_OBJECT;
	    }
	}
    }
}

static int Assign_Array(PyArrayObject *self, PyObject *v) {
    PyObject *e;
    int l, r;

    if (!PySequence_Check(v)) {
	PyErr_SetString(PyExc_ValueError,"assignment from non-sequence");
	return -1;
    }

    l=PyObject_Length(v);
    if(l < 0) return -1;

    while(--l >= 0)
	{
            e=PySequence_GetItem(v,l);
	    if (e == NULL) return -1;
	    r = PySequence_SetItem((PyObject*)self,l,e);
	    Py_DECREF(e);
	    if(r == -1) return -1;
	}
    return 0;
}

PyObject * Array_FromScalar(PyObject *, int);

static PyObject *
Array_FromSequence(PyObject *s, int type, int min_depth, int max_depth)
{
    PyArrayObject *r;
    int nd, *d;


    nd=discover_depth(s, MAX_DIMS+1, type == PyArray_OBJECT || type == 'O');
    if (nd==0)
	return Array_FromScalar(s, type);
    else if (nd < 0) {
	PyErr_SetString(PyExc_ValueError, "invalid input sequence");
	return NULL;
    }

    if ((max_depth && nd > max_depth) || (min_depth && nd < min_depth)) {
	PyErr_SetString(PyExc_ValueError, "invalid number of dimensions");
	return NULL;
    }

    if ((d=(int *)malloc(nd*sizeof(int))) == NULL) {
	PyErr_SetString(PyExc_MemoryError,"out of memory");
    }

    if(discover_dimensions(s,nd,d,(type != PyArray_CHAR)) == -1)
	{
	    free(d);
	    return NULL;
	}


    if (type == PyArray_CHAR && nd > 0 && d[nd-1] == 1) {
	nd = nd-1;
    }

    r=(PyArrayObject*)PyArray_FromDims(nd,d,type);
    free(d);
    if(! r) return NULL;
    if(Assign_Array(r,s) == -1)
	{
	    Py_DECREF(r);
	    return NULL;
	}
    return (PyObject*)r;
}

PyObject *Array_FromScalar(PyObject *op, int type) {
    PyArrayObject *ret;

    if ((ret = (PyArrayObject *)PyArray_FromDims(0, NULL, type)) == NULL)
	return NULL;

    ret->descr->setitem(op, ret->data);

    if (PyErr_Occurred()) {
	array_dealloc(ret);
	return NULL;
    } else {
	return (PyObject *)ret;
    }
}

int PyArray_ValidType(int type) {
    switch(type) {
    case 'c':
    case 'b':
    case '1':
    case 's':
    case 'w':
    case 'i':
    case 'u':
    case 'l':
    case 'f':
    case 'd':
    case 'F':
    case 'D':
    case 'O':
        break;
    default:
        if (type >= PyArray_NTYPES) return 0;
    }
    return 1;
}

PyObject *PyArray_Cast(PyArrayObject *mp, int type) {
    PyArrayObject *rp, *tmp;

    if (mp->descr->type_num == PyArray_OBJECT) {
	return PyArray_FromObject((PyObject *)mp, type, mp->nd, mp->nd);
    }

    if ((tmp = PyArray_CONTIGUOUS(mp)) == NULL) return NULL;

    rp = (PyArrayObject *)PyArray_FromDims(tmp->nd, tmp->dimensions, type);
    mp->descr->cast[(int)rp->descr->type_num](tmp->data, 1, rp->data, 1, SIZE(mp));

    Py_DECREF(tmp);
    return (PyObject *)rp;
}

static PyArray_Descr *
_array_descr_fromstr(char *str, int *swap)
{
    int type_num;
    char typechar;
    int size;
    unsigned long number = 1;
    char *s;
    const char msg[] = "unsupported typestring";

    s = (char *)&number;   /* s[0] == 0 implies big-endian */

    *swap = 0;

    if ((str[0] == '<') && (s[0] == 0)) *swap = 1;
    else if ((str[0] == '>') && (s[0] != 0)) *swap = 1;

    typechar = str[1];
    size = PyOS_strtol(str + 2, NULL, 10);
    switch (typechar) {
    case 'b':
    case 'u':
	if (size == sizeof(char))
	    type_num = PyArray_UBYTE;
#ifdef PyArray_UNSIGNED_TYPES
	else if (size == sizeof(short))
	    type_num = PyArray_USHORT;
	else if (size == sizeof(int))
	    type_num = PyArray_UINT;
#endif
	else {
	    PyErr_SetString(PyExc_ValueError, msg);
	    return NULL;
	}
	break;
    case 'i':
	if (size == sizeof(char))
	    type_num = PyArray_SBYTE;
	else if (size == sizeof(short))
	    type_num = PyArray_SHORT;
	else if (size == sizeof(long))
	    type_num = PyArray_LONG;
	else if (size == sizeof(int))
	    type_num = PyArray_INT;
	else {
	    PyErr_SetString(PyExc_ValueError, msg);
	    return NULL;
	}
	break;
    case 'f':
	if (size == sizeof(float))
	    type_num = PyArray_FLOAT;
	else if (size == sizeof(double))
	    type_num = PyArray_DOUBLE;
	else {
	    PyErr_SetString(PyExc_ValueError, msg);
	    return NULL;
	}
	break;
    case 'c':
	if (size == sizeof(float)*2)
	    type_num = PyArray_CFLOAT;
	else if (size == sizeof(double)*2)
	    type_num = PyArray_CDOUBLE;
	else {
	    PyErr_SetString(PyExc_ValueError, msg);
	    return NULL;
	}
	break;
    case 'O':
	if (size == sizeof(PyObject *))
	    type_num = PyArray_OBJECT;
	else {
	    PyErr_SetString(PyExc_ValueError, msg);
	    return NULL;
	}
	break;
    case 'S':
	if (size == 1)
	    type_num = PyArray_CHAR;
	else {
	    PyErr_SetString(PyExc_ValueError, msg);
	    return NULL;
	}
	break;
    default:
	PyErr_SetString(PyExc_ValueError, msg);
	return NULL;
    }

    return PyArray_DescrFromType(type_num);
}

static PyObject *
array_fromstructinterface(PyObject *input)
{
    PyArray_Descr *descr;
    int swap=0, i, n;
    char buf[40];
    PyArrayInterface *inter;
    PyObject *attr;
    PyArrayObject *ret;
    int dims[MAX_DIMS];
    
    attr = PyObject_GetAttrString(input, "__array_struct__");
    if (attr == NULL) {PyErr_Clear(); return Py_NotImplemented;}
    
    if (!PyCObject_Check(attr) ||                                       \
        ((inter=((PyArrayInterface *)PyCObject_AsVoidPtr(attr)))->version != 2)) {
        PyErr_SetString(PyExc_ValueError, "invalid __array_struct__");
        return NULL;
    }
    
    if ((inter->flags & (ALIGNED | WRITEABLE)) != (ALIGNED | WRITEABLE)) {
        PyErr_SetString(PyExc_ValueError, 
                        "cannot handle misaligned or not writeable arrays.");
        return NULL;
    }

    snprintf(buf, 40, "|%c%d", inter->typekind, inter->itemsize);
    if ((descr = _array_descr_fromstr(buf, &swap)) == NULL) {
        return NULL;
    }    
    n = inter->nd;
    for (i=0; i<n; i++) {
        dims[i] = (int) inter->shape[i];
    }
    ret = (PyArrayObject *)PyArray_FromDimsAndDataAndDescr(n, dims, descr, inter->data);
    if (ret == NULL) return NULL;
    Py_INCREF(input);
    ret->base = input;

    /* Adjust strides if called for */
    if (!(inter->flags & CONTIGUOUS)) {
        ret->flags &= ~CONTIGUOUS;
        for (i=0; i<n; i++) {
            ret->strides[i] = (int) inter->strides[i];
        }
    }

    if (!(inter->flags & NOTSWAPPED)) {
	PyObject *new;
	new = PyObject_CallMethod((PyObject *)ret, "byteswapped", "");
	Py_DECREF(ret);
	ret = (PyArrayObject *)new;
    }

    return (PyObject *)ret;    
}


static PyObject *
array_frominterface(PyObject *input)
{
    PyObject *attr=NULL, *item=NULL;
    PyObject *tstr, *shape;
    PyArrayObject *ret=NULL;
    PyArray_Descr *descr;
    char *data;
    int buffer_len;
    int res, i, n;
    int dims[MAX_DIMS], strides[MAX_DIMS];
    int swap;

    /* Get the memory from __array_data__ and __array_offset__ */
    /* Get the shape */
    /* Get the typestring -- ignore array_descr */
    /* Get the strides */

    shape = PyObject_GetAttrString(input, "__array_shape__");
    if (shape == NULL) {PyErr_Clear(); return Py_NotImplemented;}
    tstr = PyObject_GetAttrString(input, "__array_typestr__");
    if (tstr == NULL) {Py_DECREF(shape); PyErr_Clear(); return Py_NotImplemented;}

    attr = PyObject_GetAttrString(input, "__array_data__");
    if ((attr == NULL) || (attr==Py_None) || (!PyTuple_Check(attr))) {
	if (attr && (attr != Py_None)) item=attr;
	else item=input;
	res = PyObject_AsWriteBuffer(item, (void **)&data, 
				     &buffer_len);
        Py_XDECREF(attr);
	if (res < 0) return NULL;
        attr = PyObject_GetAttrString(input, "__array_offset__");
        if (attr) {
            long num = PyInt_AsLong(attr);
            if (error_converting(num)) {
                PyErr_SetString(PyExc_TypeError, 
                                "__array_offset__ "                     \
                                "must be an integer");
                return NULL;
            }
            data += num;
        }
        else PyErr_Clear();
    }
    else {
	if (PyTuple_GET_SIZE(attr) != 2) {
	    Py_DECREF(attr);
	    PyErr_SetString(PyExc_TypeError, 
			    "__array_data__ must return"		\
			    " a 2-tuple with ('data pointer "		\
			    "string', read-only flag)");
	    return NULL;
	}
	res = sscanf(PyString_AsString(PyTuple_GET_ITEM(attr,0)),
		     "%p", (void **)&data);
	if (res < 1) {
	    Py_DECREF(attr);
	    PyErr_SetString(PyExc_TypeError, 
			    "__array_data__ string cannot be "	\
			    "converted.");
	    return NULL;
	}
	if (PyObject_IsTrue(PyTuple_GET_ITEM(attr,1))) {
	    Py_DECREF(attr);
	    PyErr_SetString(PyExc_TypeError,
			    "cannot handle read-only data.");
	    return NULL;
	}
    }
    Py_XDECREF(attr);
    attr = PyObject_GetAttrString(input, "__array_typestr__");
    if (!attr || !PyString_Check(attr)) {
	PyErr_SetString(PyExc_TypeError, "__array_typestr__ must be a string.");
	Py_DECREF(attr);
	return NULL;
    }
    descr = _array_descr_fromstr(PyString_AS_STRING(attr), &swap);
    Py_DECREF(attr);
    if (descr==NULL) return NULL;

    attr = PyObject_GetAttrString(input, "__array_shape__");
    if (!attr || !PyTuple_Check(attr)) {
	PyErr_SetString(PyExc_TypeError, "__array_shape__ must be a tuple.");
	Py_DECREF(attr);
	return NULL;
    }
    n = PyTuple_GET_SIZE(attr);
    for (i=0; i<n; i++) {
	item = PyTuple_GET_ITEM(attr, i);
	dims[i] = PyArray_IntegerAsInt(item);
	if (error_converting(dims[i])) break;
    }
    Py_DECREF(attr);
    if (PyErr_Occurred()) return NULL;

    ret = (PyArrayObject *)PyArray_FromDimsAndDataAndDescr(n, dims, descr, data);
    if (ret == NULL) return NULL;
    Py_INCREF(input);
    ret->base = input;

    attr = PyObject_GetAttrString(input, "__array_strides__");
    if (attr != NULL && attr != Py_None) {
	if (!PyTuple_Check(attr)) {
	    PyErr_SetString(PyExc_TypeError, "__array_strides__ must be a tuple.");
	    Py_DECREF(attr);
	    return NULL;
	}
	if (n != PyTuple_GET_SIZE(attr)) {
	    PyErr_SetString(PyExc_ValueError, "mismatch in length of __array_strides__ and __array_shape__.");
	    Py_DECREF(attr);
	    return NULL;
	}
	for (i=0; i<n; i++) {
	    item = PyTuple_GET_ITEM(attr, i);
	    strides[i] = PyArray_IntegerAsInt(item);
	    if (error_converting(strides[i])) break;
	}
	Py_DECREF(attr);
	if (PyErr_Occurred()) return NULL;
	memcpy(ret->strides, strides, n*sizeof(int));
    }

    if (swap) {
	PyObject *new;
	new = PyObject_CallMethod((PyObject *)ret, "byteswapped", "");
	Py_DECREF(ret);
	ret = (PyArrayObject *)new;
    }

    return (PyObject *)ret;
}

#define ByCopy 1
#define Contiguous 2

PyObject *array_fromobject(PyObject *op_in, int type, int min_depth, int max_depth, int flags) {
    /* This is the main code to make a NumPy array from a Python Object.  It is
       called from lot's of different places which is why there are so many checks.
       The comments try to explain some of the checks. */

    PyObject *r, *op;
    char temp;
    int real_type;

    /* The type argument can have a (char-based) flag set which states that this array
       should be spacesaving */
    temp = type;
    temp &= ~SAVESPACEBIT;
    real_type = temp;       /* Type without the spacesaving flag  */

    r = NULL;

    /* Code that returns the object to convert for a non multiarray input object
       from the array_interface or the  __array__ attribute of the object. */
    if (!PyArray_Check(op_in)) {
        if ((op = array_fromstructinterface(op_in)) != Py_NotImplemented || \
            (op = array_frominterface(op_in)) != Py_NotImplemented) {
            if (op == NULL) return NULL;
	    if (real_type == PyArray_NOTYPE) {
		real_type = ((PyArrayObject *)op)->descr->type_num;
		if (type & SAVESPACEBIT) {
		    type = real_type | SAVESPACEBIT;
		}
	    }            
        }
	else if (PyObject_HasAttrString(op_in, "__array__")) {
	    if (real_type == PyArray_NOTYPE) {
		op = PyObject_CallMethod(op_in, "__array__", NULL);
	    } else {
		op = PyObject_CallMethod(op_in, "__array__", "c", real_type);
	    }
	    if (op == NULL) return NULL;
	    if (!PyArray_Check(op)) {
		    Py_DECREF(op);
		    PyErr_SetString(PyExc_TypeError,
				    "No array interface and __array__"\
				    " method not returning Numeric array.");
		    return NULL;
	    }
	}
	else {
	    op = op_in;
	    Py_INCREF(op);
	}
    }
    else {
	op = op_in;
	Py_INCREF(op);
    }

    /* If the typecode is NOTYPE get the real_type from the object and
       reset type with the SAVESPACEBIT */
    if (real_type == PyArray_NOTYPE) {
	real_type = array_objecttype(op, 0, type & SAVESPACEBIT, MAX_DIMS+1);
	if (type & SAVESPACEBIT) {
	    type = real_type | SAVESPACEBIT;
	}
    }
    if (real_type > PyArray_NTYPES) {
            PyArray_Descr *descr;
            descr = PyArray_DescrFromType(real_type);
            if (descr == NULL) return NULL;
            real_type = descr->type_num;
            if (type & SAVESPACEBIT) {
                    type = real_type | SAVESPACEBIT;
            }
    }

    /* Is input object an array but not an object array and we don't want an object
       array output */
    if (PyArray_Check(op) && (((PyArrayObject *)op)->descr->type_num != PyArray_OBJECT ||
			      real_type == PyArray_OBJECT || real_type == 'O')) {
	/* Is the desired output type the same as the input array type */
	if ((((PyArrayObject *)op)->descr->type_num == real_type ||
	     ((PyArrayObject *)op)->descr->type == real_type)) {
	    /* Should the output be copied (1) because it's a flag, (2) because a contiguous
	       array is requested */
	    if ((flags & ByCopy) || (flags&Contiguous && !ISCONTIGUOUS((PyArrayObject *)op))) {
		r = PyArray_Copy((PyArrayObject *)op);
	    }
	    /* If no copy then just increase the reference count and return the input */
	    else {
		Py_INCREF(op);
		r = op;
	    }
	}
	/* The desired output type is different than the input array type */
	else {
	    /* If the typecode is too big, (i.e. it is a character) make it a number. */
	    if (real_type > PyArray_NTYPES) real_type = PyArray_DescrFromType(real_type)->type_num;
	    /* Cast to the desired type if we can do it safely.  If the SAVESPACE
	       bit is set do it anyway.  Also cast if source is a rank-0 array to mimic
	       behavior with Python scalars */
	    if (PyArray_CanCastSafely(((PyArrayObject *)op)->descr->type_num, real_type) || (type & SAVESPACEBIT) || (((PyArrayObject *)op)->nd == 0))
		r = PyArray_Cast((PyArrayObject *)op, real_type);
	    else {
		PyErr_SetString(PyExc_TypeError, "Array can not be safely cast to required type");
		r = NULL;
	    }
	}
    }
    /* The input object is either not an array or it is an object array,
       or the desired output type is object. */
    else {
	if (PyUnicode_Check(op)) {
	    PyErr_SetString(PyExc_ValueError, "Unicode objects not supported.");
	    Py_DECREF(op);
	    return NULL;
	}
        if (!(PyInstance_Check(op)) && PySequence_Check(op)) {
                r = Array_FromSequence(op, real_type, min_depth, max_depth);
        }
        else {
                r = Array_FromScalar(op, real_type);
	}
    }
    Py_DECREF(op);
    /* If we didn't succeed return NULL */
    if (r == NULL) return NULL;

    if(!PyArray_Check(r)) {
	PyErr_SetString(PyExc_ValueError, "Internal error array_fromobject not producing an array");
	return NULL;
    }
    if (min_depth != 0 && ((PyArrayObject *)r)->nd < min_depth) {
	Py_DECREF(r);
	PyErr_SetString(PyExc_ValueError,
			"Object of too small depth for desired array");
	return NULL;
    }
    if (max_depth != 0 && ((PyArrayObject *)r)->nd > max_depth) {
	Py_DECREF(r);
	PyErr_SetString(PyExc_ValueError,
			"Object too deep for desired array");
	return NULL;
    }
    return r;
}


int PyArray_ObjectType(PyObject *op, int minimum_type) {
    return array_objecttype(op, minimum_type, 0, MAX_DIMS+1);
}

PyObject *PyArray_FromObject(PyObject *op, int type, int min_depth, int max_depth) {
    return array_fromobject(op, type, min_depth, max_depth, 0);
}
PyObject *PyArray_ContiguousFromObject(PyObject *op, int type, int min_depth, int max_depth) {
    return array_fromobject(op, type, min_depth, max_depth, Contiguous);
}
PyObject *PyArray_CopyFromObject(PyObject *op, int type, int min_depth, int max_depth) {
    return array_fromobject(op, type, min_depth, max_depth, ByCopy);
}

extern int PyArray_CanCastSafely(int fromtype, int totype) {

    if (fromtype == totype) return 1;
    if (totype == PyArray_OBJECT) return 1;
    if (fromtype == PyArray_OBJECT) return 0;

    switch(fromtype) {
    case PyArray_CHAR:
	return 0;
    case PyArray_UBYTE:
	return (totype >= PyArray_SHORT);
    case PyArray_SBYTE:
    case PyArray_SHORT:
        return (totype > fromtype) && (totype != PyArray_USHORT) && (totype != PyArray_UINT);
    case PyArray_USHORT:
	return (totype > fromtype);
    case PyArray_INT:
	return (totype >= PyArray_LONG) && (totype != PyArray_FLOAT) && (totype != PyArray_CFLOAT);
    case PyArray_UINT:
            return (totype > PyArray_FLOAT);
    case PyArray_LONG:
	/* Hopefully this will allow LONGs to be cast to INTs on 32 bit platforms where
	   they can be, but not on 64-bit platforms where it is not safe */
	if (sizeof(long) == sizeof(int))
	    return (totype == PyArray_INT) || (totype == PyArray_DOUBLE) || (totype == PyArray_CDOUBLE);
	else
	    return (totype == PyArray_DOUBLE) || (totype == PyArray_CDOUBLE);
    case PyArray_FLOAT:
	return (totype > fromtype);
    case PyArray_DOUBLE:
	return (totype == PyArray_CDOUBLE);
    case PyArray_CFLOAT:
	return (totype == PyArray_CDOUBLE);
    case PyArray_CDOUBLE:
	return 0;
    default:
	return 0;
    }
}

extern int PyArray_As1D(PyObject **op, char **ptr, int *d1, int typecode) {
    PyArrayObject *ap;

    if ((ap = (PyArrayObject *)PyArray_ContiguousFromObject(*op, typecode, 1,
							    1)) == NULL)
	return -1;

    *op = (PyObject *)ap;
    *ptr = ap->data;
    *d1 = ap->dimensions[0];
    return 0;
}

extern int PyArray_As2D(PyObject **op, char ***ptr, int *d1, int *d2, int typecode) {
    PyArrayObject *ap;
    int i, n;
    char **data;

    if ((ap = (PyArrayObject *)PyArray_ContiguousFromObject(*op, typecode, 2, 2)) == NULL)
	return -1;

    n = ap->dimensions[0];
    data = (char **)malloc(n*sizeof(char *));
    if (!data) {
        return -1;
    }
    for(i=0; i<n; i++) {
	data[i] = ap->data + i*ap->strides[0];
    }
    *op = (PyObject *)ap;
    *ptr = data;
    *d1 = ap->dimensions[0];
    *d2 = ap->dimensions[1];
    return 0;
}


extern int PyArray_Free(PyObject *op, char *ptr) {
    PyArrayObject *ap = (PyArrayObject *)op;
    int i, n;

    if (ap->nd > 2) return -1;
    if (ap->nd == 3) {
	n = ap->dimensions[0];
	for (i=0; i<n; i++) {
	    free(((char **)ptr)[i]);
	}
    }
    if (ap->nd >= 2) {
	free(ptr);
    }
    Py_DECREF(ap);
    return 0;
}

extern PyObject * PyArray_Reshape(PyArrayObject *self, PyObject *shape) {
    int i, n, s_original, i_unknown, s_known;
    int *dimensions;
    PyArrayObject *ret;

    if (!PyArray_ISCONTIGUOUS(self)) {
	PyErr_SetString(PyExc_ValueError, "reshape only works on contiguous arrays");
	return NULL;
    }

    if (PyArray_As1D(&shape, (char **)&dimensions, &n, PyArray_INT) == -1) return NULL;

    s_known = 1;
    i_unknown = -1;
    for(i=0; i<n; i++) {
	if (dimensions[i] < 0) {
	    if (i_unknown == -1) {
		i_unknown = i;
	    } else {
		PyErr_SetString(PyExc_ValueError, "can only specify one unknown dimension");
		goto fail;
	    }
	} else {
	    s_known *= dimensions[i];
	}
    }

    s_original = PyArray_SIZE(self);

    if (i_unknown >= 0) {
	if ((s_known == 0) || (s_original % s_known != 0)) {
	    PyErr_SetString(PyExc_ValueError, "total size of new array must be unchanged");
	    goto fail;
	}
	dimensions[i_unknown] = s_original/s_known;
    } else {
	if (s_original != s_known) {
	    PyErr_SetString(PyExc_ValueError, "total size of new array must be unchanged");
	    goto fail;
	}
    }
    if ((ret = (PyArrayObject *)PyArray_FromDimsAndDataAndDescr(n, dimensions,
								self->descr,
								self->data)) == NULL)
	goto fail;

    Py_INCREF(self);
    ret->base = (PyObject *)self;

    PyArray_Free(shape, (char *)dimensions);
    return (PyObject *)ret;

 fail:
    PyArray_Free(shape, (char *)dimensions);
    return NULL;
}


extern PyObject *PyArray_Take(PyObject *self0, PyObject *indices0, int axis) {
    PyArrayObject *self, *indices, *ret;
    int nd, shape[MAX_DIMS];
    int i, j, chunk, n, m, max_item;
    long tmp;
    char *src, *dest;

    indices = ret = NULL;
    self = (PyArrayObject *)PyArray_ContiguousFromObject(self0, PyArray_NOTYPE, 1, 0);
    if (self == NULL) return NULL;

    if (axis < 0) axis = axis + self->nd;
    if ((axis < 0) || (axis >= self->nd)) {
	PyErr_SetString(PyExc_ValueError, "Invalid axis for this array");
	goto fail;
    }

    indices = (PyArrayObject *)PyArray_ContiguousFromObject(indices0, PyArray_LONG, 1, 0);
    if (indices == NULL) goto fail;

    n = m = chunk = 1;
    nd = self->nd + indices->nd - 1;
    for (i=0; i< nd; i++) {
	if (i < axis) {
	    shape[i] = self->dimensions[i];
	    n *= shape[i];
	} else {
	    if (i < axis+indices->nd) {
		shape[i] = indices->dimensions[i-axis];
		m *= shape[i];
	    } else {
		shape[i] = self->dimensions[i-indices->nd+1];
		chunk *= shape[i];
	    }
	}
    }
    ret = (PyArrayObject *)PyArray_FromDims(nd, shape, self->descr->type_num);

    if (ret == NULL) goto fail;

    max_item = self->dimensions[axis];
    chunk = chunk * ret->descr->elsize;
    src = self->data;
    dest = ret->data;

    for(i=0; i<n; i++) {
	for(j=0; j<m; j++) {
	    tmp = ((long *)(indices->data))[j];
	    if (tmp < 0) tmp = tmp+max_item;
	    if ((tmp < 0) || (tmp >= max_item)) {
		PyErr_SetString(PyExc_IndexError, "Index out of range for array");
		goto fail;
	    }
	    memmove(dest, src+tmp*chunk, chunk);
	    dest += chunk;
	}
	src += chunk*max_item;
    }

    PyArray_INCREF(ret);

    Py_XDECREF(indices);
    Py_XDECREF(self);

    return (PyObject *)ret;


 fail:
    Py_XDECREF(ret);
    Py_XDECREF(indices);
    Py_XDECREF(self);
    return NULL;
}

extern PyObject *PyArray_Put(PyObject *self0, PyObject *indices0,
                             PyObject* values0) {
    PyArrayObject  *indices, *values, *self;
    int i, chunk, ni, max_item, nv;
    long tmp;
    char *src, *dest;

    indices = NULL;
    values = NULL;

    if (!PyArray_Check(self0)) {
	PyErr_SetString(PyExc_ValueError, "put: first argument must be an array");
	return NULL;
    }
    self = (PyArrayObject*) self0;
    if (!PyArray_ISCONTIGUOUS(self)) {
	PyErr_SetString(PyExc_ValueError, "put: first argument must be contiguous");
	return NULL;
    }
    max_item = PyArray_SIZE(self);
    dest = self->data;
    chunk = self->descr->elsize;

    indices = (PyArrayObject *)PyArray_ContiguousFromObject(indices0, PyArray_LONG, 0, 0);
    if (indices == NULL) goto fail;
    ni = PyArray_SIZE(indices);

    values = (PyArrayObject *)PyArray_ContiguousFromObject(values0,
							   self->descr->type, 0, 0);
    if (values == NULL) goto fail;
    nv = PyArray_SIZE(values);
    if (nv > 0) { /* nv == 0 for a null array */
        for(i=0; i<ni; i++) {
            src = values->data + chunk * (i % nv);
            tmp = ((long *)(indices->data))[i];
            if (tmp < 0) tmp = tmp+max_item;
            if ((tmp < 0) || (tmp >= max_item)) {
                PyErr_SetString(PyExc_IndexError, "Index out of range for array");
                goto fail;
            }
            if (self->descr->type == PyArray_OBJECT) {
                Py_INCREF(*((PyObject **)src));
                Py_XDECREF(*((PyObject **)(dest+tmp*chunk)));
            }
            memmove(dest + tmp * chunk, src, chunk);
        }
    }

    Py_XDECREF(values);
    Py_XDECREF(indices);
    Py_INCREF(Py_None);
    return Py_None;

 fail:
    Py_XDECREF(indices);
    Py_XDECREF(values);
    return NULL;
}

extern PyObject *PyArray_PutMask(PyObject *self0, PyObject *mask0,
                                 PyObject* values0) {
    PyArrayObject  *mask, *values, *self;
    int i, chunk, ni, max_item, nv;
    long tmp;
    char *src, *dest;

    mask = NULL;
    values = NULL;

    if (!PyArray_Check(self0)) {
	PyErr_SetString(PyExc_ValueError, "putmask: first argument must be an array");
	return NULL;
    }
    self = (PyArrayObject*) self0;
    if (!PyArray_ISCONTIGUOUS(self)) {
	PyErr_SetString(PyExc_ValueError, "putmask: first argument must be contiguous");
	return NULL;
    }
    max_item = PyArray_SIZE(self);
    dest = self->data;
    chunk = self->descr->elsize;

    mask = (PyArrayObject *)PyArray_ContiguousFromObject(mask0, PyArray_LONG, 0, 0);
    if (mask == NULL) goto fail;
    ni = PyArray_SIZE(mask);
    if (ni != max_item) {
	PyErr_SetString(PyExc_ValueError, "putmask: mask and data must be the same size.");
	goto fail;
    }

    values = (PyArrayObject *)PyArray_ContiguousFromObject(values0,
							   self->descr->type, 0, 0);
    if (values == NULL) goto fail;
    nv = PyArray_SIZE(values);   /* zero if null array */
    if (nv > 0) {
        for(i=0; i<ni; i++) {
            src = values->data + chunk * (i % nv);
            tmp = ((long *)(mask->data))[i];
            if (tmp) {
                if (self->descr->type == PyArray_OBJECT) {
                    Py_INCREF(*((PyObject **)src));
                    Py_XDECREF(*((PyObject **)(dest+tmp*chunk)));
                }
                memmove(dest + i * chunk, src, chunk);
            }
        }
    }

    Py_XDECREF(values);
    Py_XDECREF(mask);
    Py_INCREF(Py_None);
    return Py_None;

 fail:
    Py_XDECREF(mask);
    Py_XDECREF(values);
    return NULL;
}


/* This conversion function can be used with the "O&" argument for
   PyArg_ParseTuple, as the O! form will not work with subclasses of
   PyArray_Type  -- courtesy of Travis Oliphant. */

extern int PyArray_Converter(PyObject *object, PyObject **address) {
    if (PyArray_Check(object)) {
	*address = object;
	return 1;
    }
    else {
	PyErr_SetString(PyExc_TypeError,"expected Array object in one of the arguments");
	return 0;
    }
}
