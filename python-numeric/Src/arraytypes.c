#include "Python.h"
#include "Numeric/arrayobject.h"

#ifndef CHAR_BIT
#define CHAR_BIT 8
#endif

#ifndef INT_BIT
#define INT_BIT (CHAR_BIT * sizeof(int))
#endif

#ifndef INT_MAX
#define INT_MAX ((1 << (INT_BIT-2)) + ((1 << (INT_BIT-2))-1))
#endif

static void CHAR_to_CHAR(char *ip, int ipstep, char *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {*op = (char)*ip;}}

static void CHAR_to_UBYTE(char *ip, int ipstep, unsigned char *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {*op = (unsigned char)*ip;}}

static void CHAR_to_SBYTE(char *ip, int ipstep, signed char *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {*op = (signed char)*ip;}}

static void CHAR_to_SHORT(char *ip, int ipstep, short *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {*op = (short)*ip;}}

static void CHAR_to_USHORT(char *ip, int ipstep, unsigned short *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {*op = (unsigned short)*ip;}}

static void CHAR_to_INT(char *ip, int ipstep, int *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {*op = (int)*ip;}}

static void CHAR_to_UINT(char *ip, int ipstep, unsigned int *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {*op = (unsigned int)*ip;}}

static void CHAR_to_LONG(char *ip, int ipstep, long *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {*op = (long)*ip;}}

static void CHAR_to_FLOAT(char *ip, int ipstep, float *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {*op = (float)*ip;}}

static void CHAR_to_DOUBLE(char *ip, int ipstep, double *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {*op = (double)*ip;}}

static void CHAR_to_CFLOAT(char *ip, int ipstep, float *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep*2) {op[0] = (float)*ip; op[1]=(float)0.0;}}

static void CHAR_to_CDOUBLE(char *ip, int ipstep, double *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep*2) {op[0] = (double)*ip; op[1]=0.0;}}

static void CHAR_to_OBJECT(char *ip, int ipstep, PyObject  **op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {op[0] = PyString_FromStringAndSize(ip,1);}}

static PyObject * CHAR_getitem(char *ip) {return PyString_FromStringAndSize(ip,1);}

static int CHAR_setitem(PyObject *op, char *ov) {*((char *)ov)=(char)PyInt_AsLong(op);return PyErr_Occurred() ? -1:0;}

static PyArray_Descr CHAR_Descr = { {

    (PyArray_VectorUnaryFunc*)CHAR_to_CHAR,
    (PyArray_VectorUnaryFunc*)CHAR_to_UBYTE,
    (PyArray_VectorUnaryFunc*)CHAR_to_SBYTE,
    (PyArray_VectorUnaryFunc*)CHAR_to_SHORT,
    (PyArray_VectorUnaryFunc*)CHAR_to_USHORT,
    (PyArray_VectorUnaryFunc*)CHAR_to_INT,
    (PyArray_VectorUnaryFunc*)CHAR_to_UINT,
    (PyArray_VectorUnaryFunc*)CHAR_to_LONG,
    (PyArray_VectorUnaryFunc*)CHAR_to_FLOAT,
    (PyArray_VectorUnaryFunc*)CHAR_to_DOUBLE,
    (PyArray_VectorUnaryFunc*)CHAR_to_CFLOAT,
    (PyArray_VectorUnaryFunc*)CHAR_to_CDOUBLE,
    (PyArray_VectorUnaryFunc*)CHAR_to_OBJECT,
},
				    CHAR_getitem, CHAR_setitem,
				    PyArray_CHAR, sizeof(char), NULL, NULL, 'c'};


static void UBYTE_to_CHAR(unsigned char *ip, int ipstep, char *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {*op = (char)*ip;}}

static void UBYTE_to_UBYTE(unsigned char *ip, int ipstep, unsigned char *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {*op = (unsigned char)*ip;}}

static void UBYTE_to_SBYTE(unsigned char *ip, int ipstep, signed char *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {*op = (signed char)*ip;}}

static void UBYTE_to_SHORT(unsigned char *ip, int ipstep, short *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {*op = (short)*ip;}}

static void UBYTE_to_USHORT(unsigned char *ip, int ipstep, unsigned short *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {*op = (unsigned short)*ip;}}

static void UBYTE_to_INT(unsigned char *ip, int ipstep, int *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {*op = (int)*ip;}}

static void UBYTE_to_UINT(unsigned char *ip, int ipstep, unsigned int *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {*op = (unsigned int)*ip;}}

static void UBYTE_to_LONG(unsigned char *ip, int ipstep, long *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {*op = (long)*ip;}}

static void UBYTE_to_FLOAT(unsigned char *ip, int ipstep, float *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {*op = (float)*ip;}}

static void UBYTE_to_DOUBLE(unsigned char *ip, int ipstep, double *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {*op = (double)*ip;}}

static void UBYTE_to_CFLOAT(unsigned char *ip, int ipstep, float *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep*2) {op[0] = (float)*ip; op[1]=0.0;}}

static void UBYTE_to_CDOUBLE(unsigned char *ip, int ipstep, double *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep*2) {op[0] = (double)*ip; op[1]=0.0;}}

static void UBYTE_to_OBJECT(unsigned char *ip, int ipstep, PyObject  **op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {op[0] = PyInt_FromLong((long)*((unsigned char *)ip));}}

static PyObject * UBYTE_getitem(char *ip) {return PyInt_FromLong((long)*((unsigned char *)ip));}

static int UBYTE_setitem(PyObject *op, char *ov) {*((unsigned char *)ov)=(unsigned char)PyInt_AsLong(op);return PyErr_Occurred() ? -1:0;}

static PyArray_Descr UBYTE_Descr = { {

    (PyArray_VectorUnaryFunc*)UBYTE_to_CHAR,
    (PyArray_VectorUnaryFunc*)UBYTE_to_UBYTE,
    (PyArray_VectorUnaryFunc*)UBYTE_to_SBYTE,
    (PyArray_VectorUnaryFunc*)UBYTE_to_SHORT,
    (PyArray_VectorUnaryFunc*)UBYTE_to_USHORT,
    (PyArray_VectorUnaryFunc*)UBYTE_to_INT,
    (PyArray_VectorUnaryFunc*)UBYTE_to_UINT,
    (PyArray_VectorUnaryFunc*)UBYTE_to_LONG,
    (PyArray_VectorUnaryFunc*)UBYTE_to_FLOAT,
    (PyArray_VectorUnaryFunc*)UBYTE_to_DOUBLE,
    (PyArray_VectorUnaryFunc*)UBYTE_to_CFLOAT,
    (PyArray_VectorUnaryFunc*)UBYTE_to_CDOUBLE,
    (PyArray_VectorUnaryFunc*)UBYTE_to_OBJECT,
},
				     UBYTE_getitem, UBYTE_setitem,
				     PyArray_UBYTE, sizeof(unsigned char), NULL, NULL, 'b'};


static void SBYTE_to_CHAR(signed char *ip, int ipstep, char *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {*op = (char)*ip;}}

static void SBYTE_to_UBYTE(signed char *ip, int ipstep, unsigned char *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {*op = (unsigned char)*ip;}}

static void SBYTE_to_SBYTE(signed char *ip, int ipstep, signed char *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {*op = (signed char)*ip;}}

static void SBYTE_to_SHORT(signed char *ip, int ipstep, short *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {*op = (short)*ip;}}

static void SBYTE_to_USHORT(signed char *ip, int ipstep, unsigned short *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {*op = (unsigned short)*ip;}}

static void SBYTE_to_INT(signed char *ip, int ipstep, int *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {*op = (int)*ip;}}

static void SBYTE_to_UINT(signed char *ip, int ipstep, unsigned int *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {*op = (unsigned int)*ip;}}

static void SBYTE_to_LONG(signed char *ip, int ipstep, long *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {*op = (long)*ip;}}

static void SBYTE_to_FLOAT(signed char *ip, int ipstep, float *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {*op = (float)*ip;}}

static void SBYTE_to_DOUBLE(signed char *ip, int ipstep, double *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {*op = (double)*ip;}}

static void SBYTE_to_CFLOAT(signed char *ip, int ipstep, float *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep*2) {op[0] = (float)*ip; op[1]=0.0;}}

static void SBYTE_to_CDOUBLE(signed char *ip, int ipstep, double *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep*2) {op[0] = (double)*ip; op[1]=0.0;}}

static void SBYTE_to_OBJECT(signed char *ip, int ipstep, PyObject  **op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {op[0] = PyInt_FromLong((long)*((signed char *)ip));}}

static PyObject * SBYTE_getitem(char *ip) {return PyInt_FromLong((long)*((signed char *)ip));}

static int SBYTE_setitem(PyObject *op, char *ov) {*((signed char *)ov)=(signed char)PyInt_AsLong(op);return PyErr_Occurred() ? -1:0;}

static PyArray_Descr SBYTE_Descr = { {

    (PyArray_VectorUnaryFunc*)SBYTE_to_CHAR,
    (PyArray_VectorUnaryFunc*)SBYTE_to_UBYTE,
    (PyArray_VectorUnaryFunc*)SBYTE_to_SBYTE,
    (PyArray_VectorUnaryFunc*)SBYTE_to_SHORT,
    (PyArray_VectorUnaryFunc*)SBYTE_to_USHORT,
    (PyArray_VectorUnaryFunc*)SBYTE_to_INT,
    (PyArray_VectorUnaryFunc*)SBYTE_to_UINT,
    (PyArray_VectorUnaryFunc*)SBYTE_to_LONG,
    (PyArray_VectorUnaryFunc*)SBYTE_to_FLOAT,
    (PyArray_VectorUnaryFunc*)SBYTE_to_DOUBLE,
    (PyArray_VectorUnaryFunc*)SBYTE_to_CFLOAT,
    (PyArray_VectorUnaryFunc*)SBYTE_to_CDOUBLE,
    (PyArray_VectorUnaryFunc*)SBYTE_to_OBJECT,
},
				     SBYTE_getitem, SBYTE_setitem,
				     PyArray_SBYTE, sizeof(signed char), NULL, NULL, '1'};


static void USHORT_to_CHAR(unsigned short *ip, int ipstep, char *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {*op = (char)*ip;}}

static void USHORT_to_UBYTE(unsigned short *ip, int ipstep, unsigned char *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {*op = (unsigned char)*ip;}}

static void USHORT_to_SBYTE(unsigned short *ip, int ipstep, signed char *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {*op = (signed char)*ip;}}

static void USHORT_to_SHORT(unsigned short *ip, int ipstep, short *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {*op = (short)*ip;}}

static void USHORT_to_USHORT(unsigned short *ip, int ipstep, unsigned short *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {*op = (unsigned short)*ip;}}

static void USHORT_to_INT(unsigned short *ip, int ipstep, int *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {*op = (int)*ip;}}

static void USHORT_to_UINT(unsigned short *ip, int ipstep, unsigned int *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {*op = (unsigned int)*ip;}}

static void USHORT_to_LONG(unsigned short *ip, int ipstep, long *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {*op = (long)*ip;}}

static void USHORT_to_FLOAT(unsigned short *ip, int ipstep, float *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {*op = (float)*ip;}}

static void USHORT_to_DOUBLE(unsigned short *ip, int ipstep, double *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {*op = (double)*ip;}}

static void USHORT_to_CFLOAT(unsigned short *ip, int ipstep, float *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep*2) {op[0] = (float)*ip; op[1]=0.0;}}

static void USHORT_to_CDOUBLE(unsigned short *ip, int ipstep, double *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep*2) {op[0] = (double)*ip; op[1]=0.0;}}

static void USHORT_to_OBJECT(unsigned short *ip, int ipstep, PyObject  **op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {op[0] = PyInt_FromLong((long)*((unsigned short *)ip));}}

static PyObject * USHORT_getitem(char *ip) {return PyInt_FromLong((long)*((unsigned short *)ip));}

static int USHORT_setitem(PyObject *op, char *ov) {*((unsigned short *)ov)=(unsigned short)PyInt_AsLong(op);return PyErr_Occurred() ? -1:0;}

static PyArray_Descr USHORT_Descr = { {

	(PyArray_VectorUnaryFunc*)USHORT_to_CHAR,
	(PyArray_VectorUnaryFunc*)USHORT_to_UBYTE,
	(PyArray_VectorUnaryFunc*)USHORT_to_SBYTE,
	(PyArray_VectorUnaryFunc*)USHORT_to_SHORT,
	(PyArray_VectorUnaryFunc*)USHORT_to_USHORT,
	(PyArray_VectorUnaryFunc*)USHORT_to_INT,
	(PyArray_VectorUnaryFunc*)USHORT_to_UINT,
	(PyArray_VectorUnaryFunc*)USHORT_to_LONG,
	(PyArray_VectorUnaryFunc*)USHORT_to_FLOAT,
	(PyArray_VectorUnaryFunc*)USHORT_to_DOUBLE,
	(PyArray_VectorUnaryFunc*)USHORT_to_CFLOAT,
	(PyArray_VectorUnaryFunc*)USHORT_to_CDOUBLE,
	(PyArray_VectorUnaryFunc*)USHORT_to_OBJECT,
},
	USHORT_getitem, USHORT_setitem,
	PyArray_USHORT, sizeof(unsigned short), NULL, NULL, 'w'};


static void SHORT_to_CHAR(short *ip, int ipstep, char *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {*op = (char)*ip;}}

static void SHORT_to_UBYTE(short *ip, int ipstep, unsigned char *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {*op = (unsigned char)*ip;}}

static void SHORT_to_SBYTE(short *ip, int ipstep, signed char *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {*op = (signed char)*ip;}}

static void SHORT_to_SHORT(short *ip, int ipstep, short *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {*op = (short)*ip;}}

static void SHORT_to_USHORT(short *ip, int ipstep, unsigned short *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {*op = (unsigned short)*ip;}}

static void SHORT_to_INT(short *ip, int ipstep, int *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {*op = (int)*ip;}}

static void SHORT_to_UINT(short *ip, int ipstep, unsigned int *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {*op = (unsigned int)*ip;}}

static void SHORT_to_LONG(short *ip, int ipstep, long *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {*op = (long)*ip;}}

static void SHORT_to_FLOAT(short *ip, int ipstep, float *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {*op = (float)*ip;}}

static void SHORT_to_DOUBLE(short *ip, int ipstep, double *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {*op = (double)*ip;}}

static void SHORT_to_CFLOAT(short *ip, int ipstep, float *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep*2) {op[0] = (float)*ip; op[1]=0.0;}}

static void SHORT_to_CDOUBLE(short *ip, int ipstep, double *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep*2) {op[0] = (double)*ip; op[1]=0.0;}}

static void SHORT_to_OBJECT(short *ip, int ipstep, PyObject  **op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {op[0] = PyInt_FromLong((long)*((short *)ip));}}

static PyObject * SHORT_getitem(char *ip) {return PyInt_FromLong((long)*((short *)ip));}

static int SHORT_setitem(PyObject *op, char *ov) {*((short *)ov)=(short)PyInt_AsLong(op);return PyErr_Occurred() ? -1:0;}

static PyArray_Descr SHORT_Descr = { {

    (PyArray_VectorUnaryFunc*)SHORT_to_CHAR,
    (PyArray_VectorUnaryFunc*)SHORT_to_UBYTE,
    (PyArray_VectorUnaryFunc*)SHORT_to_SBYTE,
    (PyArray_VectorUnaryFunc*)SHORT_to_SHORT,
    (PyArray_VectorUnaryFunc*)SHORT_to_USHORT,
    (PyArray_VectorUnaryFunc*)SHORT_to_INT,
    (PyArray_VectorUnaryFunc*)SHORT_to_UINT,
    (PyArray_VectorUnaryFunc*)SHORT_to_LONG,
    (PyArray_VectorUnaryFunc*)SHORT_to_FLOAT,
    (PyArray_VectorUnaryFunc*)SHORT_to_DOUBLE,
    (PyArray_VectorUnaryFunc*)SHORT_to_CFLOAT,
    (PyArray_VectorUnaryFunc*)SHORT_to_CDOUBLE,
    (PyArray_VectorUnaryFunc*)SHORT_to_OBJECT,
},
				     SHORT_getitem, SHORT_setitem,
				     PyArray_SHORT, sizeof(short), NULL, NULL, 's'};


static void INT_to_CHAR(int *ip, int ipstep, char *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {*op = (char)*ip;}}

static void INT_to_UBYTE(int *ip, int ipstep, unsigned char *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {*op = (unsigned char)*ip;}}

static void INT_to_SBYTE(int *ip, int ipstep, signed char *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {*op = (signed char)*ip;}}

static void INT_to_SHORT(int *ip, int ipstep, short *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {*op = (short)*ip;}}

static void INT_to_USHORT(int *ip, int ipstep, unsigned short *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {*op = (unsigned short)*ip;}}

static void INT_to_INT(int *ip, int ipstep, int *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {*op = (int)*ip;}}

static void INT_to_UINT(int *ip, int ipstep, unsigned int *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {*op = (unsigned int)*ip;}}

static void INT_to_LONG(int *ip, int ipstep, long *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {*op = (long)*ip;}}

static void INT_to_FLOAT(int *ip, int ipstep, float *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {*op = (float)*ip;}}

static void INT_to_DOUBLE(int *ip, int ipstep, double *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {*op = (double)*ip;}}

static void INT_to_CFLOAT(int *ip, int ipstep, float *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep*2) {op[0] = (float)*ip; op[1]=0.0;}}

static void INT_to_CDOUBLE(int *ip, int ipstep, double *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep*2) {op[0] = (double)*ip; op[1]=0.0;}}

static void INT_to_OBJECT(int *ip, int ipstep, PyObject  **op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {op[0] = PyInt_FromLong((long)*((int *)ip));}}

static PyObject * INT_getitem(char *ip) {return PyInt_FromLong((long)*((int *)ip));}

static int INT_setitem(PyObject *op, char *ov) {*((int *)ov)=PyInt_AsLong(op);return PyErr_Occurred() ? -1:0;}

static PyArray_Descr INT_Descr = { {

    (PyArray_VectorUnaryFunc*)INT_to_CHAR,
    (PyArray_VectorUnaryFunc*)INT_to_UBYTE,
    (PyArray_VectorUnaryFunc*)INT_to_SBYTE,
    (PyArray_VectorUnaryFunc*)INT_to_SHORT,
    (PyArray_VectorUnaryFunc*)INT_to_USHORT,
    (PyArray_VectorUnaryFunc*)INT_to_INT,
    (PyArray_VectorUnaryFunc*)INT_to_UINT,
    (PyArray_VectorUnaryFunc*)INT_to_LONG,
    (PyArray_VectorUnaryFunc*)INT_to_FLOAT,
    (PyArray_VectorUnaryFunc*)INT_to_DOUBLE,
    (PyArray_VectorUnaryFunc*)INT_to_CFLOAT,
    (PyArray_VectorUnaryFunc*)INT_to_CDOUBLE,
    (PyArray_VectorUnaryFunc*)INT_to_OBJECT,
},
				   INT_getitem, INT_setitem,
				   PyArray_INT, sizeof(int), NULL, NULL, 'i'};

static void UINT_to_CHAR(unsigned int *ip, int ipstep, char *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {*op = (char)*ip;}}

static void UINT_to_UBYTE(unsigned int *ip, int ipstep, unsigned char *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {*op = (unsigned char)*ip;}}

static void UINT_to_SBYTE(unsigned int *ip, int ipstep, signed char *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {*op = (signed char)*ip;}}

static void UINT_to_SHORT(unsigned int *ip, int ipstep, short *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {*op = (short)*ip;}}

static void UINT_to_USHORT(unsigned int *ip, int ipstep, unsigned short *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {*op = (unsigned short)*ip;}}

static void UINT_to_INT(unsigned int *ip, int ipstep, int *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {*op = (int)*ip;}}

static void UINT_to_UINT(unsigned int *ip, int ipstep, unsigned int *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {*op = (unsigned int)*ip;}}

static void UINT_to_LONG(unsigned int *ip, int ipstep, long *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {*op = (long)*ip;}}

static void UINT_to_FLOAT(unsigned int *ip, int ipstep, float *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {*op = (float)*ip;}}

static void UINT_to_DOUBLE(unsigned int *ip, int ipstep, double *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {*op = (double)*ip;}}

static void UINT_to_CFLOAT(unsigned int *ip, int ipstep, float *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep*2) {op[0] = (float)*ip; op[1]=0.0;}}

static void UINT_to_CDOUBLE(unsigned int *ip, int ipstep, double *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep*2) {op[0] = (double)*ip; op[1]=0.0;}}

static void UINT_to_OBJECT(unsigned int *ip, int ipstep, PyObject  **op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {op[0] = PyInt_FromLong((long)*ip);}}

static PyObject * UINT_getitem(char *ip) {
	if (*((unsigned int *)ip) > (unsigned int) INT_MAX) {
		return PyLong_FromUnsignedLong((unsigned long)*((unsigned int *)ip));
	} else {
		return PyInt_FromLong((long)*((unsigned int *)ip));
	}
}

static int UINT_setitem(PyObject *op, char *ov) {
	if (PyLong_Check(op)) {
		*((unsigned int *)ov)=(unsigned int)PyLong_AsUnsignedLong(op);
	} else {
		*((unsigned int *)ov)=(unsigned int)PyInt_AsLong(op);
	}
	return PyErr_Occurred() ? -1:0;
}

static PyArray_Descr UINT_Descr = { {

    (PyArray_VectorUnaryFunc*)UINT_to_CHAR,
    (PyArray_VectorUnaryFunc*)UINT_to_UBYTE,
    (PyArray_VectorUnaryFunc*)UINT_to_SBYTE,
    (PyArray_VectorUnaryFunc*)UINT_to_SHORT,
    (PyArray_VectorUnaryFunc*)UINT_to_USHORT,
    (PyArray_VectorUnaryFunc*)UINT_to_INT,
    (PyArray_VectorUnaryFunc*)UINT_to_UINT,
    (PyArray_VectorUnaryFunc*)UINT_to_LONG,
    (PyArray_VectorUnaryFunc*)UINT_to_FLOAT,
    (PyArray_VectorUnaryFunc*)UINT_to_DOUBLE,
    (PyArray_VectorUnaryFunc*)UINT_to_CFLOAT,
    (PyArray_VectorUnaryFunc*)UINT_to_CDOUBLE,
    (PyArray_VectorUnaryFunc*)UINT_to_OBJECT,
},
				   UINT_getitem, UINT_setitem,
				   PyArray_UINT, sizeof(unsigned int), NULL, NULL, 'u'};


static void LONG_to_CHAR(long *ip, int ipstep, char *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {*op = (char)*ip;}}

static void LONG_to_UBYTE(long *ip, int ipstep, unsigned char *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {*op = (unsigned char)*ip;}}

static void LONG_to_SBYTE(long *ip, int ipstep, signed char *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {*op = (signed char)*ip;}}

static void LONG_to_SHORT(long *ip, int ipstep, short *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {*op = (short)*ip;}}

static void LONG_to_USHORT(long *ip, int ipstep, unsigned short *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {*op = (unsigned short)*ip;}}

static void LONG_to_INT(long *ip, int ipstep, int *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {*op = (int)*ip;}}

static void LONG_to_UINT(long *ip, int ipstep, unsigned int *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {*op = (unsigned int)*ip;}}

static void LONG_to_LONG(long *ip, int ipstep, long *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {*op = (long)*ip;}}

static void LONG_to_FLOAT(long *ip, int ipstep, float *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {*op = (float)*ip;}}

static void LONG_to_DOUBLE(long *ip, int ipstep, double *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {*op = (double)*ip;}}

static void LONG_to_CFLOAT(long *ip, int ipstep, float *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep*2) {op[0] = (float)*ip; op[1]=0.0;}}

static void LONG_to_CDOUBLE(long *ip, int ipstep, double *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep*2) {op[0] = (double)*ip; op[1]=0.0;}}

static void LONG_to_OBJECT(long *ip, int ipstep, PyObject  **op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {op[0] = PyInt_FromLong((long)*((long *)ip));}}

static PyObject * LONG_getitem(char *ip) {return PyInt_FromLong((long)*((long *)ip));}

static int LONG_setitem(PyObject *op, char *ov) {*((long *)ov)=PyInt_AsLong(op);return PyErr_Occurred() ? -1:0;}

static PyArray_Descr LONG_Descr = { {

    (PyArray_VectorUnaryFunc*)LONG_to_CHAR,
    (PyArray_VectorUnaryFunc*)LONG_to_UBYTE,
    (PyArray_VectorUnaryFunc*)LONG_to_SBYTE,
    (PyArray_VectorUnaryFunc*)LONG_to_SHORT,
    (PyArray_VectorUnaryFunc*)LONG_to_USHORT,
    (PyArray_VectorUnaryFunc*)LONG_to_INT,
    (PyArray_VectorUnaryFunc*)LONG_to_UINT,
    (PyArray_VectorUnaryFunc*)LONG_to_LONG,
    (PyArray_VectorUnaryFunc*)LONG_to_FLOAT,
    (PyArray_VectorUnaryFunc*)LONG_to_DOUBLE,
    (PyArray_VectorUnaryFunc*)LONG_to_CFLOAT,
    (PyArray_VectorUnaryFunc*)LONG_to_CDOUBLE,
    (PyArray_VectorUnaryFunc*)LONG_to_OBJECT,
},
				    LONG_getitem, LONG_setitem,
				    PyArray_LONG, sizeof(long), NULL, NULL, 'l'};


static void FLOAT_to_CHAR(float *ip, int ipstep, char *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {*op = (char)*ip;}}

static void FLOAT_to_UBYTE(float *ip, int ipstep, unsigned char *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {*op = (unsigned char)*ip;}}

static void FLOAT_to_SBYTE(float *ip, int ipstep, signed char *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {*op = (signed char)*ip;}}

static void FLOAT_to_SHORT(float *ip, int ipstep, short *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {*op = (short)*ip;}}

static void FLOAT_to_USHORT(float *ip, int ipstep, unsigned short *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {*op = (unsigned short)*ip;}}

static void FLOAT_to_INT(float *ip, int ipstep, int *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {*op = (int)*ip;}}

static void FLOAT_to_UINT(float *ip, int ipstep, unsigned int *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {*op = (unsigned int)*ip;}}

static void FLOAT_to_LONG(float *ip, int ipstep, long *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {*op = (long)*ip;}}

static void FLOAT_to_FLOAT(float *ip, int ipstep, float *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {*op = (float)*ip;}}

static void FLOAT_to_DOUBLE(float *ip, int ipstep, double *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {*op = (double)*ip;}}

static void FLOAT_to_CFLOAT(float *ip, int ipstep, float *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep*2) {op[0] = (float)*ip; op[1]=0.0;}}

static void FLOAT_to_CDOUBLE(float *ip, int ipstep, double *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep*2) {op[0] = (double)*ip; op[1]=0.0;}}

static void FLOAT_to_OBJECT(float *ip, int ipstep, PyObject  **op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {op[0] = PyFloat_FromDouble((double)*((float *)ip));}}

static PyObject * FLOAT_getitem(char *ip) {return PyFloat_FromDouble((double)*((float *)ip));}

static int FLOAT_setitem(PyObject *op, char *ov) {*((float *)ov)=(float)PyFloat_AsDouble(op);return PyErr_Occurred() ? -1:0;}

static PyArray_Descr FLOAT_Descr = { {

    (PyArray_VectorUnaryFunc*)FLOAT_to_CHAR,
    (PyArray_VectorUnaryFunc*)FLOAT_to_UBYTE,
    (PyArray_VectorUnaryFunc*)FLOAT_to_SBYTE,
    (PyArray_VectorUnaryFunc*)FLOAT_to_SHORT,
    (PyArray_VectorUnaryFunc*)FLOAT_to_USHORT,
    (PyArray_VectorUnaryFunc*)FLOAT_to_INT,
    (PyArray_VectorUnaryFunc*)FLOAT_to_UINT,
    (PyArray_VectorUnaryFunc*)FLOAT_to_LONG,
    (PyArray_VectorUnaryFunc*)FLOAT_to_FLOAT,
    (PyArray_VectorUnaryFunc*)FLOAT_to_DOUBLE,
    (PyArray_VectorUnaryFunc*)FLOAT_to_CFLOAT,
    (PyArray_VectorUnaryFunc*)FLOAT_to_CDOUBLE,
    (PyArray_VectorUnaryFunc*)FLOAT_to_OBJECT,
},
				     FLOAT_getitem, FLOAT_setitem,
				     PyArray_FLOAT, sizeof(float), NULL, NULL, 'f'};


static void DOUBLE_to_CHAR(double *ip, int ipstep, char *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {*op = (char)*ip;}}

static void DOUBLE_to_UBYTE(double *ip, int ipstep, unsigned char *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {*op = (unsigned char)*ip;}}

static void DOUBLE_to_SBYTE(double *ip, int ipstep, signed char *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {*op = (signed char)*ip;}}

static void DOUBLE_to_SHORT(double *ip, int ipstep, short *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {*op = (short)*ip;}}

static void DOUBLE_to_USHORT(double *ip, int ipstep, unsigned short *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {*op = (unsigned short)*ip;}}

static void DOUBLE_to_INT(double *ip, int ipstep, int *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {*op = (int)*ip;}}

static void DOUBLE_to_UINT(double *ip, int ipstep, unsigned int *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {*op = (unsigned int)*ip;}}

static void DOUBLE_to_LONG(double *ip, int ipstep, long *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {*op = (long)*ip;}}

static void DOUBLE_to_FLOAT(double *ip, int ipstep, float *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {*op = (float)*ip;}}

static void DOUBLE_to_DOUBLE(double *ip, int ipstep, double *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {*op = (double)*ip;}}

static void DOUBLE_to_CFLOAT(double *ip, int ipstep, float *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep*2) {op[0] = (float)*ip; op[1]=0.0;}}

static void DOUBLE_to_CDOUBLE(double *ip, int ipstep, double *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep*2) {op[0] = (double)*ip; op[1]=0.0;}}

static void DOUBLE_to_OBJECT(double *ip, int ipstep, PyObject  **op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep,op+=opstep) {op[0] = PyFloat_FromDouble((double)*((double *)ip));}}

static PyObject * DOUBLE_getitem(char *ip) {return PyFloat_FromDouble((double)*((double *)ip));}

static int DOUBLE_setitem(PyObject *op, char *ov) {*((double *)ov)=PyFloat_AsDouble(op);return PyErr_Occurred() ? -1:0;}

static PyArray_Descr DOUBLE_Descr = { {

    (PyArray_VectorUnaryFunc*)DOUBLE_to_CHAR,
    (PyArray_VectorUnaryFunc*)DOUBLE_to_UBYTE,
    (PyArray_VectorUnaryFunc*)DOUBLE_to_SBYTE,
    (PyArray_VectorUnaryFunc*)DOUBLE_to_SHORT,
    (PyArray_VectorUnaryFunc*)DOUBLE_to_USHORT,
    (PyArray_VectorUnaryFunc*)DOUBLE_to_INT,
    (PyArray_VectorUnaryFunc*)DOUBLE_to_UINT,
    (PyArray_VectorUnaryFunc*)DOUBLE_to_LONG,
    (PyArray_VectorUnaryFunc*)DOUBLE_to_FLOAT,
    (PyArray_VectorUnaryFunc*)DOUBLE_to_DOUBLE,
    (PyArray_VectorUnaryFunc*)DOUBLE_to_CFLOAT,
    (PyArray_VectorUnaryFunc*)DOUBLE_to_CDOUBLE,
    (PyArray_VectorUnaryFunc*)DOUBLE_to_OBJECT,
},
				      DOUBLE_getitem, DOUBLE_setitem,
				      PyArray_DOUBLE, sizeof(double), NULL, NULL, 'd'};


static void CFLOAT_to_CHAR(float *ip, int ipstep, char *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep*2,op+=opstep) {*op = (char)*ip; }}

static void CFLOAT_to_UBYTE(float *ip, int ipstep, unsigned char *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep*2,op+=opstep) {*op = (unsigned char)*ip; }}

static void CFLOAT_to_SBYTE(float *ip, int ipstep, signed char *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep*2,op+=opstep) {*op = (signed char)*ip; }}

static void CFLOAT_to_SHORT(float *ip, int ipstep, short *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep*2,op+=opstep) {*op = (short)*ip; }}

static void CFLOAT_to_USHORT(float *ip, int ipstep, unsigned short *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep*2,op+=opstep) {*op = (unsigned short)*ip; }}

static void CFLOAT_to_INT(float *ip, int ipstep, int *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep*2,op+=opstep) {*op = (int)*ip; }}

static void CFLOAT_to_UINT(float *ip, int ipstep, unsigned int *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep*2,op+=opstep) {*op = (unsigned int)*ip; }}

static void CFLOAT_to_LONG(float *ip, int ipstep, long *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep*2,op+=opstep) {*op = (long)*ip; }}

static void CFLOAT_to_FLOAT(float *ip, int ipstep, float *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep*2,op+=opstep) {*op = (float)*ip; }}

static void CFLOAT_to_DOUBLE(float *ip, int ipstep, double *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep*2,op+=opstep) {*op = (double)*ip; }}

static void CFLOAT_to_CFLOAT(float *ip, int ipstep, float *op, int opstep, int n)
{int i; for(i=0;i<2*n;i++,ip+=ipstep,op+=opstep) {*op = (float)*ip;}}

static void CFLOAT_to_CDOUBLE(float *ip, int ipstep, double *op, int opstep, int n)
{int i; for(i=0;i<2*n;i++,ip+=ipstep,op+=opstep) {*op = (double)*ip;}}

static void CFLOAT_to_OBJECT(float *ip, int ipstep, PyObject  **op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=2*ipstep,op+=opstep) {op[0] = PyComplex_FromDoubles((double)((float *)ip)[0], (double)((float *)ip)[1]);}}

static PyObject * CFLOAT_getitem(char *ip) {return PyComplex_FromDoubles((double)((float *)ip)[0], (double)((float *)ip)[1]);}

static int CFLOAT_setitem(PyObject *op, char *ov)
{
    Py_complex oop;
    PyObject *op2;
    if PyScalarArray_Check(op) op2 = ((PyArrayObject *)op)->descr->getitem(((PyArrayObject *)op)->data);
    else { op2 = op; Py_INCREF(op);}
    oop = PyComplex_AsCComplex (op2);
    Py_DECREF(op2);
    if (PyErr_Occurred()) return -1;
    ((float *)ov)[0]= (float) oop.real;
    ((float *)ov)[1]= (float) oop.imag;
    return 0;
}

static PyArray_Descr CFLOAT_Descr = { {

    (PyArray_VectorUnaryFunc*)CFLOAT_to_CHAR,
    (PyArray_VectorUnaryFunc*)CFLOAT_to_UBYTE,
    (PyArray_VectorUnaryFunc*)CFLOAT_to_SBYTE,
    (PyArray_VectorUnaryFunc*)CFLOAT_to_SHORT,
    (PyArray_VectorUnaryFunc*)CFLOAT_to_USHORT,
    (PyArray_VectorUnaryFunc*)CFLOAT_to_INT,
    (PyArray_VectorUnaryFunc*)CFLOAT_to_UINT,
    (PyArray_VectorUnaryFunc*)CFLOAT_to_LONG,
    (PyArray_VectorUnaryFunc*)CFLOAT_to_FLOAT,
    (PyArray_VectorUnaryFunc*)CFLOAT_to_DOUBLE,
    (PyArray_VectorUnaryFunc*)CFLOAT_to_CFLOAT,
    (PyArray_VectorUnaryFunc*)CFLOAT_to_CDOUBLE,
    (PyArray_VectorUnaryFunc*)CFLOAT_to_OBJECT,
},
				      CFLOAT_getitem, CFLOAT_setitem,
				      PyArray_CFLOAT, 2*sizeof(float), NULL, NULL, 'F'};


static void CDOUBLE_to_CHAR(double *ip, int ipstep, char *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep*2,op+=opstep) {*op = (char)*ip; }}

static void CDOUBLE_to_UBYTE(double *ip, int ipstep, unsigned char *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep*2,op+=opstep) {*op = (unsigned char)*ip; }}

static void CDOUBLE_to_SBYTE(double *ip, int ipstep, signed char *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep*2,op+=opstep) {*op = (signed char)*ip; }}

static void CDOUBLE_to_SHORT(double *ip, int ipstep, short *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep*2,op+=opstep) {*op = (short)*ip; }}

static void CDOUBLE_to_USHORT(double *ip, int ipstep, unsigned short *op, int opstep, int n)
     {int i; for(i=0;i<n;i++,ip+=ipstep*2,op+=opstep) {*op = (unsigned short)*ip; }}

static void CDOUBLE_to_INT(double *ip, int ipstep, int *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep*2,op+=opstep) {*op = (int)*ip; }}

static void CDOUBLE_to_UINT(double *ip, int ipstep, unsigned int *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep*2,op+=opstep) {*op = (unsigned int)*ip; }}

static void CDOUBLE_to_LONG(double *ip, int ipstep, long *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep*2,op+=opstep) {*op = (long)*ip; }}

static void CDOUBLE_to_FLOAT(double *ip, int ipstep, float *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep*2,op+=opstep) {*op = (float)*ip; }}

static void CDOUBLE_to_DOUBLE(double *ip, int ipstep, double *op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=ipstep*2,op+=opstep) {*op = (double)*ip; }}

static void CDOUBLE_to_CFLOAT(double *ip, int ipstep, float *op, int opstep, int n)
{int i; for(i=0;i<2*n;i++,ip+=ipstep,op+=opstep) {*op = (float)*ip;}}

static void CDOUBLE_to_CDOUBLE(double *ip, int ipstep, double *op, int opstep, int n)
{int i; for(i=0;i<2*n;i++,ip+=ipstep,op+=opstep) {*op = (double)*ip;}}

static void CDOUBLE_to_OBJECT(double *ip, int ipstep, PyObject  **op, int opstep, int n)
{int i; for(i=0;i<n;i++,ip+=2*ipstep,op+=opstep) {op[0] = PyComplex_FromDoubles((double)((double *)ip)[0], (double)((double *)ip)[1]);}}

static PyObject * CDOUBLE_getitem(char *ip) {return PyComplex_FromDoubles((double)((double *)ip)[0], (double)((double *)ip)[1]);}

static int CDOUBLE_setitem(PyObject *op, char *ov) {
    Py_complex oop;
    PyObject *op2;
    if PyScalarArray_Check(op) op2 = ((PyArrayObject *)op)->descr->getitem(((PyArrayObject *)op)->data);
    else { op2 = op; Py_INCREF(op);}
    oop = PyComplex_AsCComplex (op2);
    Py_DECREF(op2);
    if (PyErr_Occurred()) return -1;
    ((double *)ov)[0]= (double) oop.real;
    ((double *)ov)[1]= (double) oop.imag;
    return 0;
}

static PyArray_Descr CDOUBLE_Descr = { {

    (PyArray_VectorUnaryFunc*)CDOUBLE_to_CHAR,
    (PyArray_VectorUnaryFunc*)CDOUBLE_to_UBYTE,
    (PyArray_VectorUnaryFunc*)CDOUBLE_to_SBYTE,
    (PyArray_VectorUnaryFunc*)CDOUBLE_to_SHORT,
    (PyArray_VectorUnaryFunc*)CDOUBLE_to_USHORT,
    (PyArray_VectorUnaryFunc*)CDOUBLE_to_INT,
    (PyArray_VectorUnaryFunc*)CDOUBLE_to_UINT,
    (PyArray_VectorUnaryFunc*)CDOUBLE_to_LONG,
    (PyArray_VectorUnaryFunc*)CDOUBLE_to_FLOAT,
    (PyArray_VectorUnaryFunc*)CDOUBLE_to_DOUBLE,
    (PyArray_VectorUnaryFunc*)CDOUBLE_to_CFLOAT,
    (PyArray_VectorUnaryFunc*)CDOUBLE_to_CDOUBLE,
    (PyArray_VectorUnaryFunc*)CDOUBLE_to_OBJECT,
},
				       CDOUBLE_getitem, CDOUBLE_setitem,
				       PyArray_CDOUBLE, 2*sizeof(double), NULL, NULL, 'D'};


static PyObject * OBJECT_getitem(char *ip) {return Py_INCREF(*(PyObject **)ip), *(PyObject **)ip;}

static int OBJECT_setitem(PyObject *op, char *ov) {if (*(PyObject **)ov != NULL) {Py_DECREF(*(PyObject **)ov);} Py_INCREF(op); *(PyObject **)ov = op;return PyErr_Occurred() ? -1:0;}

static PyArray_Descr OBJECT_Descr = { {

    (PyArray_VectorUnaryFunc*)NULL,
    (PyArray_VectorUnaryFunc*)NULL,
    (PyArray_VectorUnaryFunc*)NULL,
    (PyArray_VectorUnaryFunc*)NULL,
    (PyArray_VectorUnaryFunc*)NULL,
    (PyArray_VectorUnaryFunc*)NULL,
    (PyArray_VectorUnaryFunc*)NULL,
    (PyArray_VectorUnaryFunc*)NULL,
    (PyArray_VectorUnaryFunc*)NULL,
    (PyArray_VectorUnaryFunc*)NULL,
    (PyArray_VectorUnaryFunc*)NULL,
    (PyArray_VectorUnaryFunc*)NULL,
},
				      OBJECT_getitem, OBJECT_setitem,
				      PyArray_OBJECT, sizeof(PyObject*), NULL, NULL, 'O'};


static PyArray_Descr *descrs[] = {
    &CHAR_Descr,
    &UBYTE_Descr,
    &SBYTE_Descr,
    &SHORT_Descr,
    &USHORT_Descr,
    &INT_Descr,
    &UINT_Descr,
    &LONG_Descr,
    &FLOAT_Descr,
    &DOUBLE_Descr,
    &CFLOAT_Descr,
    &CDOUBLE_Descr,
    &OBJECT_Descr,
};

PyArray_Descr *PyArray_DescrFromType(int type) {
	if (type < PyArray_NTYPES) {
		return descrs[type];
	} else {
		switch(type) {
		case 'c': return descrs[PyArray_CHAR];
		case 'b': return descrs[PyArray_UBYTE];
		case '1': return descrs[PyArray_SBYTE];
		case 's': return descrs[PyArray_SHORT];
		case 'w': return descrs[PyArray_USHORT];
		case 'i': return descrs[PyArray_INT];
		case 'u': return descrs[PyArray_UINT];
		case 'l': return descrs[PyArray_LONG];
		case 'f': return descrs[PyArray_FLOAT];
		case 'd': return descrs[PyArray_DOUBLE];
		case 'F': return descrs[PyArray_CFLOAT];
		case 'D': return descrs[PyArray_CDOUBLE];
		case 'O': return descrs[PyArray_OBJECT];

		default:
			PyErr_SetString(PyExc_ValueError, "Invalid type for array");
			return NULL;
		}
	}
}
