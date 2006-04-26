/*This module contributed by Doug Heisterkamp
Modified by Jim Hugunin
More modifications by Jeff Whitaker
*/


#include"Python.h"
#include"Numeric/arrayobject.h"

typedef struct { float r, i; } f2c_complex;
typedef struct { double r, i; } f2c_doublecomplex;
/* typedef long int (*L_fp)(); */

static PyObject *ErrorObject;

static PyObject *LapackError()
{  if (! ErrorObject)
      ErrorObject = PyString_FromString("LapackError");
   Py_INCREF(ErrorObject);
   return ErrorObject;
}

static PyObject *ErrorReturn(char *mes)
{  if (!ErrorObject)
      ErrorObject = PyString_FromString("LapackError");
   PyErr_SetString(ErrorObject,mes);
   return NULL;
}

#define TRY(E) if( ! (E)) return NULL

static int lapack_lite_CheckObject(PyObject *ob, int t, char *obname,
        char *tname, char *funname)
{       char buf[255];
        if (! PyArray_Check(ob))
        {       sprintf(buf,"Expected an array for parameter %s in lapack_dge.%s",obname, funname);
        ErrorReturn(buf);
        return 0;
    }
    if (!(((PyArrayObject *)ob)->flags & CONTIGUOUS)) 
    {    sprintf(buf,"Parameter %s is not contiguous in lapack_dge.%s",obname, funname);
        ErrorReturn(buf);
        return 0;
    }
    if (!(((PyArrayObject *)ob)->descr->type_num == t))
    {    sprintf(buf,"Parameter %s is not of type %s in lapack_lite.%s",obname,tname, funname);
        ErrorReturn(buf);
        return 0;
    }
    return 1;
}

#define CHDATA(p) ((char *) (((PyArrayObject *)p)->data))
#define SHDATA(p) ((short int *) (((PyArrayObject *)p)->data))
#define DDATA(p) ((double *) (((PyArrayObject *)p)->data))
#define FDATA(p) ((float *) (((PyArrayObject *)p)->data))
#define CDATA(p) ((f2c_complex *) (((PyArrayObject *)p)->data))
#define ZDATA(p) ((f2c_doublecomplex *) (((PyArrayObject *)p)->data))
#define IDATA(p) ((int *) (((PyArrayObject *)p)->data))



static PyObject *lapack_lite_dgeev(PyObject *self, PyObject *args)
{
int  lapack_lite_status__;
char jobvl;
char jobvr;
int n;
PyObject *a;
int lda;
PyObject *wr;
PyObject *wi;
PyObject *vl;
int ldvl;
PyObject *vr;
int ldvr;
PyObject *work;
int lwork;
int info;
TRY(PyArg_ParseTuple(args,"cciOiOOOiOiOii",&jobvl,&jobvr,&n,&a,&lda,&wr,&wi,&vl,&ldvl,&vr,&ldvr,&work,&lwork,&info));

TRY(lapack_lite_CheckObject(a,PyArray_DOUBLE,"a","PyArray_DOUBLE","dgeev")); 
TRY(lapack_lite_CheckObject(wr,PyArray_DOUBLE,"wr","PyArray_DOUBLE","dgeev")); 
TRY(lapack_lite_CheckObject(wi,PyArray_DOUBLE,"wi","PyArray_DOUBLE","dgeev")); 
TRY(lapack_lite_CheckObject(vl,PyArray_DOUBLE,"vl","PyArray_DOUBLE","dgeev")); 
TRY(lapack_lite_CheckObject(vr,PyArray_DOUBLE,"vr","PyArray_DOUBLE","dgeev")); 
TRY(lapack_lite_CheckObject(work,PyArray_DOUBLE,"work","PyArray_DOUBLE","dgeev")); 


#if defined(NO_APPEND_FORTRAN)
lapack_lite_status__ = dgeev(&jobvl,&jobvr,&n,DDATA(a),&lda,DDATA(wr),DDATA(wi),DDATA(vl),&ldvl,DDATA(vr),&ldvr,DDATA(work),&lwork,&info);
#else
lapack_lite_status__ = dgeev_(&jobvl,&jobvr,&n,DDATA(a),&lda,DDATA(wr),DDATA(wi),DDATA(vl),&ldvl,DDATA(vr),&ldvr,DDATA(work),&lwork,&info);
#endif

return Py_BuildValue("{s:i,s:c,s:c,s:i,s:i,s:i,s:i,s:i,s:i}","dgeev_",lapack_lite_status__,"jobvl",jobvl,"jobvr",jobvr,"n",n,"lda",lda,"ldvl",ldvl,"ldvr",ldvr,"lwork",lwork,"info",info);
}



static PyObject *lapack_lite_dsyevd(PyObject *self, PyObject *args)
{
  /*  Arguments */
  /*  ========= */
  
  char jobz;
  /*  JOBZ    (input) CHARACTER*1 */
  /*          = 'N':  Compute eigenvalues only; */
  /*          = 'V':  Compute eigenvalues and eigenvectors. */
  
  char uplo;
  /*  UPLO    (input) CHARACTER*1 */
  /*          = 'U':  Upper triangle of A is stored; */
  /*          = 'L':  Lower triangle of A is stored. */
  
  int n;
  /*  N       (input) INTEGER */
  /*          The order of the matrix A.  N >= 0. */
  
  PyObject *a;
  /*  A       (input/output) DOUBLE PRECISION array, dimension (LDA, N) */
  /*          On entry, the symmetric matrix A.  If UPLO = 'U', the */
  /*          leading N-by-N upper triangular part of A contains the */
  /*          upper triangular part of the matrix A.  If UPLO = 'L', */
  /*          the leading N-by-N lower triangular part of A contains */
  /*          the lower triangular part of the matrix A. */
  /*          On exit, if JOBZ = 'V', then if INFO = 0, A contains the */
  /*          orthonormal eigenvectors of the matrix A. */
  /*          If JOBZ = 'N', then on exit the lower triangle (if UPLO='L') */
  /*          or the upper triangle (if UPLO='U') of A, including the */
  /*          diagonal, is destroyed. */
  
  int lda;
  /*  LDA     (input) INTEGER */
  /*          The leading dimension of the array A.  LDA >= max(1,N). */
  
  PyObject *w;
  /*  W       (output) DOUBLE PRECISION array, dimension (N) */
  /*          If INFO = 0, the eigenvalues in ascending order. */
  
  PyObject *work;
  /*  WORK    (workspace/output) DOUBLE PRECISION array, dimension (LWORK) */
  /*          On exit, if INFO = 0, WORK(1) returns the optimal LWORK. */

  int lwork;
  /*  LWORK   (input) INTEGER */
  /*          The length of the array WORK.  LWORK >= max(1,3*N-1). */
  /*          For optimal efficiency, LWORK >= (NB+2)*N, */
  /*          where NB is the blocksize for DSYTRD returned by ILAENV. */

  PyObject *iwork;
  int liwork;
  
  int info;
  /*  INFO    (output) INTEGER */
  /*          = 0:  successful exit */
  /*          < 0:  if INFO = -i, the i-th argument had an illegal value */
  /*          > 0:  if INFO = i, the algorithm failed to converge; i */
  /*                off-diagonal elements of an intermediate tridiagonal */
  /*                form did not converge to zero. */
  
  
  
  
  int  lapack_lite_status__;


  
  TRY(PyArg_ParseTuple(args,"cciOiOOiOii",&jobz,&uplo,&n,&a,&lda,&w,&work,&lwork,&iwork,&liwork,&info));

  TRY(lapack_lite_CheckObject(a,PyArray_DOUBLE,"a","PyArray_DOUBLE","dsyevd")); 
  TRY(lapack_lite_CheckObject(w,PyArray_DOUBLE,"w","PyArray_DOUBLE","dsyevd")); 
  TRY(lapack_lite_CheckObject(work,PyArray_DOUBLE,"work","PyArray_DOUBLE","dsyevd")); 
  TRY(lapack_lite_CheckObject(iwork,PyArray_INT,"iwork","PyArray_INT","dsyevd")); 
  
  
#if defined(NO_APPEND_FORTRAN)
  lapack_lite_status__ = dsyevd(&jobz,&uplo,&n,DDATA(a),&lda,DDATA(w),DDATA(work),&lwork,IDATA(iwork),&liwork,&info);
#else
  lapack_lite_status__ = dsyevd_(&jobz,&uplo,&n,DDATA(a),&lda,DDATA(w),DDATA(work),&lwork,IDATA(iwork),&liwork,&info);
#endif
  
  return Py_BuildValue("{s:i,s:c,s:c,s:i,s:i,s:i,s:i,s:i}","dsyevd_",lapack_lite_status__,"jobz",jobz,"uplo",uplo,"n",n,"lda",lda,"lwork",lwork,"liwork",liwork,"info",info);
  
  
}

static PyObject *lapack_lite_zheevd(PyObject *self, PyObject *args)
{
  /*  Arguments */
  /*  ========= */
  
  char jobz;
  /*  JOBZ    (input) CHARACTER*1 */
  /*          = 'N':  Compute eigenvalues only; */
  /*          = 'V':  Compute eigenvalues and eigenvectors. */
  
  char uplo;
  /*  UPLO    (input) CHARACTER*1 */
  /*          = 'U':  Upper triangle of A is stored; */
  /*          = 'L':  Lower triangle of A is stored. */
  
  int n;
  /*  N       (input) INTEGER */
  /*          The order of the matrix A.  N >= 0. */
  
  PyObject *a;
  /*  A       (input/output) COMPLEX*16 array, dimension (LDA, N) */
  /*          On entry, the Hermitian matrix A.  If UPLO = 'U', the */
  /*          leading N-by-N upper triangular part of A contains the */
  /*          upper triangular part of the matrix A.  If UPLO = 'L', */
  /*          the leading N-by-N lower triangular part of A contains */
  /*          the lower triangular part of the matrix A. */
  /*          On exit, if JOBZ = 'V', then if INFO = 0, A contains the */
  /*          orthonormal eigenvectors of the matrix A. */
  /*          If JOBZ = 'N', then on exit the lower triangle (if UPLO='L') */
  /*          or the upper triangle (if UPLO='U') of A, including the */
  /*          diagonal, is destroyed. */
  
  int lda;
  /*  LDA     (input) INTEGER */
  /*          The leading dimension of the array A.  LDA >= max(1,N). */
  
  PyObject *w;
  /*  W       (output) DOUBLE PRECISION array, dimension (N) */
  /*          If INFO = 0, the eigenvalues in ascending order. */
  
  PyObject *work;
  /*  WORK    (workspace/output) COMPLEX*16 array, dimension (LWORK) */
  /*          On exit, if INFO = 0, WORK(1) returns the optimal LWORK. */

  int lwork;
  /*  LWORK   (input) INTEGER */
  /*          The length of the array WORK.  LWORK >= max(1,3*N-1). */
  /*          For optimal efficiency, LWORK >= (NB+2)*N, */
  /*          where NB is the blocksize for DSYTRD returned by ILAENV. */
  
  PyObject *rwork;
  /*  RWORK   (workspace) DOUBLE PRECISION array, dimension (max(1, 3*N-2)) */
  int lrwork;

  PyObject *iwork;
  int liwork;
  
  int info;
  /*  INFO    (output) INTEGER */
  /*          = 0:  successful exit */
  /*          < 0:  if INFO = -i, the i-th argument had an illegal value */
  /*          > 0:  if INFO = i, the algorithm failed to converge; i */
  /*                off-diagonal elements of an intermediate tridiagonal */
  /*                form did not converge to zero. */
  
  
  
  
  int  lapack_lite_status__;


  
  TRY(PyArg_ParseTuple(args,"cciOiOOiOiOii",&jobz,&uplo,&n,&a,&lda,&w,&work,&lwork,&rwork,&lrwork,&iwork,&liwork,&info));

  TRY(lapack_lite_CheckObject(a,PyArray_CDOUBLE,"a","PyArray_CDOUBLE","zheevd")); 
  TRY(lapack_lite_CheckObject(w,PyArray_DOUBLE,"w","PyArray_DOUBLE","zheevd")); 
  TRY(lapack_lite_CheckObject(work,PyArray_CDOUBLE,"work","PyArray_CDOUBLE","zheevd")); 
  TRY(lapack_lite_CheckObject(w,PyArray_DOUBLE,"rwork","PyArray_DOUBLE","zheevd")); 
  TRY(lapack_lite_CheckObject(iwork,PyArray_INT,"iwork","PyArray_INT","zheevd")); 
  
  
#if defined(NO_APPEND_FORTRAN)
  lapack_lite_status__ = zheevd(&jobz,&uplo,&n,DDATA(a),&lda,DDATA(w),DDATA(work),&lwork,DDATA(rwork),&lrwork,IDATA(iwork),&liwork,&info);
#else
  lapack_lite_status__ = zheevd_(&jobz,&uplo,&n,DDATA(a),&lda,DDATA(w),DDATA(work),&lwork,DDATA(rwork),&lrwork,IDATA(iwork),&liwork,&info);
#endif
  
  return Py_BuildValue("{s:i,s:c,s:c,s:i,s:i,s:i,s:i,s:i,s:i}","zheevd_",lapack_lite_status__,"jobz",jobz,"uplo",uplo,"n",n,"lda",lda,"lwork",lwork,"lrwork",lrwork,"liwork",liwork,"info",info);
  
  
}


static PyObject *lapack_lite_dgelsd(PyObject *self, PyObject *args)
{
int  lapack_lite_status__;
int m;
int n;
int nrhs;
PyObject *a;
int lda;
PyObject *b;
int ldb;
PyObject *s;
double rcond;
int rank;
PyObject *work;
PyObject *iwork;
int lwork;
int info;
TRY(PyArg_ParseTuple(args,"iiiOiOiOdiOiOi",&m,&n,&nrhs,&a,&lda,&b,&ldb,&s,&rcond,&rank,&work,&lwork,&iwork,&info));

TRY(lapack_lite_CheckObject(a,PyArray_DOUBLE,"a","PyArray_DOUBLE","dgelsd")); 
TRY(lapack_lite_CheckObject(b,PyArray_DOUBLE,"b","PyArray_DOUBLE","dgelsd")); 
TRY(lapack_lite_CheckObject(s,PyArray_DOUBLE,"s","PyArray_DOUBLE","dgelsd")); 
TRY(lapack_lite_CheckObject(work,PyArray_DOUBLE,"work","PyArray_DOUBLE","dgelsd")); 
TRY(lapack_lite_CheckObject(iwork,PyArray_INT,"iwork","PyArray_INT","dgelsd")); 


#if defined(NO_APPEND_FORTRAN)
lapack_lite_status__ = dgelsd(&m,&n,&nrhs,DDATA(a),&lda,DDATA(b),&ldb,DDATA(s),&rcond,&rank,DDATA(work),&lwork,IDATA(iwork),&info);
#else
lapack_lite_status__ = dgelsd_(&m,&n,&nrhs,DDATA(a),&lda,DDATA(b),&ldb,DDATA(s),&rcond,&rank,DDATA(work),&lwork,IDATA(iwork),&info);
#endif

return Py_BuildValue("{s:i,s:i,s:i,s:i,s:i,s:i,s:d,s:i,s:i,s:i}","dgelsd_",lapack_lite_status__,"m",m,"n",n,"nrhs",nrhs,"lda",lda,"ldb",ldb,"rcond",rcond,"rank",rank,"lwork",lwork,"info",info);
}





static PyObject *lapack_lite_dgesv(PyObject *self, PyObject *args)
{
int  lapack_lite_status__;
int n;
int nrhs;
PyObject *a;
int lda;
PyObject *ipiv;
PyObject *b;
int ldb;
int info;
TRY(PyArg_ParseTuple(args,"iiOiOOii",&n,&nrhs,&a,&lda,&ipiv,&b,&ldb,&info));

TRY(lapack_lite_CheckObject(a,PyArray_DOUBLE,"a","PyArray_DOUBLE","dgesv")); 
TRY(lapack_lite_CheckObject(ipiv,PyArray_INT,"ipiv","PyArray_INT","dgesv")); 
TRY(lapack_lite_CheckObject(b,PyArray_DOUBLE,"b","PyArray_DOUBLE","dgesv")); 


#if defined(NO_APPEND_FORTRAN)
lapack_lite_status__ = dgesv(&n,&nrhs,DDATA(a),&lda,IDATA(ipiv),DDATA(b),&ldb,&info);
#else
lapack_lite_status__ = dgesv_(&n,&nrhs,DDATA(a),&lda,IDATA(ipiv),DDATA(b),&ldb,&info);
#endif

return Py_BuildValue("{s:i,s:i,s:i,s:i,s:i,s:i}","dgesv_",lapack_lite_status__,"n",n,"nrhs",nrhs,"lda",lda,"ldb",ldb,"info",info);
}





static PyObject *lapack_lite_dgesdd(PyObject *self, PyObject *args)
{
int  lapack_lite_status__;
char jobz;
int m;
int n;
PyObject *a;
int lda;
PyObject *s;
PyObject *u;
int ldu;
PyObject *vt;
int ldvt;
PyObject *work;
int lwork;
PyObject *iwork;
int info;
TRY(PyArg_ParseTuple(args,"ciiOiOOiOiOiOi",&jobz,&m,&n,&a,&lda,&s,&u,&ldu,&vt,&ldvt,&work,&lwork,&iwork,&info));

TRY(lapack_lite_CheckObject(a,PyArray_DOUBLE,"a","PyArray_DOUBLE","dgesdd")); 
TRY(lapack_lite_CheckObject(s,PyArray_DOUBLE,"s","PyArray_DOUBLE","dgesdd")); 
TRY(lapack_lite_CheckObject(u,PyArray_DOUBLE,"u","PyArray_DOUBLE","dgesdd")); 
TRY(lapack_lite_CheckObject(vt,PyArray_DOUBLE,"vt","PyArray_DOUBLE","dgesdd")); 
TRY(lapack_lite_CheckObject(work,PyArray_DOUBLE,"work","PyArray_DOUBLE","dgesdd")); 
TRY(lapack_lite_CheckObject(iwork,PyArray_INT,"iwork","PyArray_INT","dgesdd")); 


#if defined(NO_APPEND_FORTRAN)
lapack_lite_status__ = dgesdd(&jobz,&m,&n,DDATA(a),&lda,DDATA(s),DDATA(u),&ldu,DDATA(vt),&ldvt,DDATA(work),&lwork,&iwork,&info);
#else
lapack_lite_status__ = dgesdd_(&jobz,&m,&n,DDATA(a),&lda,DDATA(s),DDATA(u),&ldu,DDATA(vt),&ldvt,DDATA(work),&lwork,IDATA(iwork),&info);
#endif

return Py_BuildValue("{s:i,s:c,s:i,s:i,s:i,s:i,s:i,s:i,s:i}","dgesdd_",lapack_lite_status__,"jobz",jobz,"m",m,"n",n,"lda",lda,"ldu",ldu,"ldvt",ldvt,"lwork",lwork,"info",info);
}



static PyObject *lapack_lite_dgetrf(PyObject *self, PyObject *args)
{
int  lapack_lite_status__;
int m;
int n;
PyObject *a;
int lda;
PyObject *ipiv;
int info;
TRY(PyArg_ParseTuple(args,"iiOiOi",&m,&n,&a,&lda,&ipiv,&info));

TRY(lapack_lite_CheckObject(a,PyArray_DOUBLE,"a","PyArray_DOUBLE","dgetrf")); 
TRY(lapack_lite_CheckObject(ipiv,PyArray_INT,"ipiv","PyArray_INT","dgetrf")); 


#if defined(NO_APPEND_FORTRAN)
lapack_lite_status__ = dgetrf(&m,&n,DDATA(a),&lda,IDATA(ipiv),&info);
#else
lapack_lite_status__ = dgetrf_(&m,&n,DDATA(a),&lda,IDATA(ipiv),&info);
#endif

return Py_BuildValue("{s:i,s:i,s:i,s:i,s:i}","dgetrf_",lapack_lite_status__,"m",m,"n",n,"lda",lda,"info",info);
}

static PyObject *lapack_lite_dpotrf(PyObject *self, PyObject *args)
{
int  lapack_lite_status__;
int n;
PyObject *a;
int lda;
char uplo;
int info;

TRY(PyArg_ParseTuple(args,"ciOii",&uplo,&n,&a,&lda,&info));
TRY(lapack_lite_CheckObject(a,PyArray_DOUBLE,"a","PyArray_DOUBLE","dpotrf")); 

#if defined(NO_APPEND_FORTRAN)
lapack_lite_status__ = dpotrf(&uplo,&n,DDATA(a),&lda,&info);
#else
lapack_lite_status__ = dpotrf_(&uplo,&n,DDATA(a),&lda,&info);
#endif

return Py_BuildValue("{s:i,s:i,s:i,s:i}","dpotrf_",lapack_lite_status__,"n",n,"lda",lda,"info",info);
}
 
static PyObject *lapack_lite_zgeev(PyObject *self, PyObject *args)
{
int  lapack_lite_status__;
char jobvl;
char jobvr;
int n;
PyObject *a;
int lda;
PyObject *w;
PyObject *vl;
int ldvl;
PyObject *vr;
int ldvr;
PyObject *work;
int lwork;
PyObject *rwork;
int info;
TRY(PyArg_ParseTuple(args,"cciOiOOiOiOiOi",&jobvl,&jobvr,&n,&a,&lda,&w,&vl,&ldvl,&vr,&ldvr,&work,&lwork,&rwork,&info));

TRY(lapack_lite_CheckObject(a,PyArray_CDOUBLE,"a","PyArray_CDOUBLE","zgeev")); 
TRY(lapack_lite_CheckObject(w,PyArray_CDOUBLE,"w","PyArray_CDOUBLE","zgeev")); 
TRY(lapack_lite_CheckObject(vl,PyArray_CDOUBLE,"vl","PyArray_CDOUBLE","zgeev")); 
TRY(lapack_lite_CheckObject(vr,PyArray_CDOUBLE,"vr","PyArray_CDOUBLE","zgeev")); 
TRY(lapack_lite_CheckObject(work,PyArray_CDOUBLE,"work","PyArray_CDOUBLE","zgeev")); 
TRY(lapack_lite_CheckObject(rwork,PyArray_DOUBLE,"rwork","PyArray_DOUBLE","zgeev")); 


#if defined(NO_APPEND_FORTRAN)
lapack_lite_status__ = zgeev(&jobvl,&jobvr,&n,ZDATA(a),&lda,ZDATA(w),ZDATA(vl),&ldvl,ZDATA(vr),&ldvr,ZDATA(work),&lwork,DDATA(rwork),&info);
#else
lapack_lite_status__ = zgeev_(&jobvl,&jobvr,&n,ZDATA(a),&lda,ZDATA(w),ZDATA(vl),&ldvl,ZDATA(vr),&ldvr,ZDATA(work),&lwork,DDATA(rwork),&info);
#endif

return Py_BuildValue("{s:i,s:c,s:c,s:i,s:i,s:i,s:i,s:i,s:i}","zgeev_",lapack_lite_status__,"jobvl",jobvl,"jobvr",jobvr,"n",n,"lda",lda,"ldvl",ldvl,"ldvr",ldvr,"lwork",lwork,"info",info);
}




static PyObject *lapack_lite_zgelsd(PyObject *self, PyObject *args)
{
int  lapack_lite_status__;
int m;
int n;
int nrhs;
PyObject *a;
int lda;
PyObject *b;
int ldb;
PyObject *s;
double rcond;
int rank;
PyObject *work;
int lwork;
PyObject *rwork;
PyObject *iwork;
int info;
TRY(PyArg_ParseTuple(args,"iiiOiOiOdiOiOOi",&m,&n,&nrhs,&a,&lda,&b,&ldb,&s,&rcond,&rank,&work,&lwork,&rwork,&iwork,&info));

TRY(lapack_lite_CheckObject(a,PyArray_CDOUBLE,"a","PyArray_CDOUBLE","zgelsd")); 
TRY(lapack_lite_CheckObject(b,PyArray_CDOUBLE,"b","PyArray_CDOUBLE","zgelsd")); 
TRY(lapack_lite_CheckObject(s,PyArray_DOUBLE,"s","PyArray_DOUBLE","zgelsd")); 
TRY(lapack_lite_CheckObject(work,PyArray_CDOUBLE,"work","PyArray_CDOUBLE","zgelsd")); 
TRY(lapack_lite_CheckObject(rwork,PyArray_DOUBLE,"rwork","PyArray_DOUBLE","zgelsd")); 
TRY(lapack_lite_CheckObject(iwork,PyArray_INT,"iwork","PyArray_INT","zgelsd")); 


#if defined(NO_APPEND_FORTRAN)
lapack_lite_status__ = zgelsd(&m,&n,&nrhs,ZDATA(a),&lda,ZDATA(b),&ldb,DDATA(s),&rcond,&rank,ZDATA(work),&lwork,DDATA(rwork),IDATA(iwork),&info);
#else
lapack_lite_status__ = zgelsd_(&m,&n,&nrhs,ZDATA(a),&lda,ZDATA(b),&ldb,DDATA(s),&rcond,&rank,ZDATA(work),&lwork,DDATA(rwork),IDATA(iwork),&info);
#endif

return Py_BuildValue("{s:i,s:i,s:i,s:i,s:i,s:i,s:i,s:i,s:i}","zgelsd_",lapack_lite_status__,"m",m,"n",n,"nrhs",nrhs,"lda",lda,"ldb",ldb,"rank",rank,"lwork",lwork,"info",info);
}



static PyObject *lapack_lite_zgesv(PyObject *self, PyObject *args)
{
int  lapack_lite_status__;
int n;
int nrhs;
PyObject *a;
int lda;
PyObject *ipiv;
PyObject *b;
int ldb;
int info;
TRY(PyArg_ParseTuple(args,"iiOiOOii",&n,&nrhs,&a,&lda,&ipiv,&b,&ldb,&info));

TRY(lapack_lite_CheckObject(a,PyArray_CDOUBLE,"a","PyArray_CDOUBLE","zgesv")); 
TRY(lapack_lite_CheckObject(ipiv,PyArray_INT,"ipiv","PyArray_INT","zgesv")); 
TRY(lapack_lite_CheckObject(b,PyArray_CDOUBLE,"b","PyArray_CDOUBLE","zgesv")); 


#if defined(NO_APPEND_FORTRAN)
lapack_lite_status__ = zgesv(&n,&nrhs,ZDATA(a),&lda,IDATA(ipiv),ZDATA(b),&ldb,&info);
#else
lapack_lite_status__ = zgesv_(&n,&nrhs,ZDATA(a),&lda,IDATA(ipiv),ZDATA(b),&ldb,&info);
#endif

return Py_BuildValue("{s:i,s:i,s:i,s:i,s:i,s:i}","zgesv_",lapack_lite_status__,"n",n,"nrhs",nrhs,"lda",lda,"ldb",ldb,"info",info);
}





static PyObject *lapack_lite_zgesdd(PyObject *self, PyObject *args)
{
int  lapack_lite_status__;
char jobz;
int m;
int n;
PyObject *a;
int lda;
PyObject *s;
PyObject *u;
int ldu;
PyObject *vt;
int ldvt;
PyObject *work;
int lwork;
PyObject *rwork;
PyObject *iwork;
int info;
TRY(PyArg_ParseTuple(args,"ciiOiOOiOiOiOOi",&jobz,&m,&n,&a,&lda,&s,&u,&ldu,&vt,&ldvt,&work,&lwork,&rwork,&iwork,&info));

TRY(lapack_lite_CheckObject(a,PyArray_CDOUBLE,"a","PyArray_CDOUBLE","zgesdd")); 
TRY(lapack_lite_CheckObject(s,PyArray_DOUBLE,"s","PyArray_DOUBLE","zgesdd")); 
TRY(lapack_lite_CheckObject(u,PyArray_CDOUBLE,"u","PyArray_CDOUBLE","zgesdd")); 
TRY(lapack_lite_CheckObject(vt,PyArray_CDOUBLE,"vt","PyArray_CDOUBLE","zgesdd")); 
TRY(lapack_lite_CheckObject(work,PyArray_CDOUBLE,"work","PyArray_CDOUBLE","zgesdd")); 
TRY(lapack_lite_CheckObject(rwork,PyArray_DOUBLE,"rwork","PyArray_DOUBLE","zgesdd")); 
TRY(lapack_lite_CheckObject(iwork,PyArray_INT,"iwork","PyArray_INT","zgesdd")); 


#if defined(NO_APPEND_FORTRAN)
lapack_lite_status__ = zgesdd(&jobz,&m,&n,ZDATA(a),&lda,DDATA(s),ZDATA(u),&ldu,ZDATA(vt),&ldvt,ZDATA(work),&lwork,DDATA(rwork),IDATA(iwork),&info);
#else
lapack_lite_status__ = zgesdd_(&jobz,&m,&n,ZDATA(a),&lda,DDATA(s),ZDATA(u),&ldu,ZDATA(vt),&ldvt,ZDATA(work),&lwork,DDATA(rwork),IDATA(iwork),&info);
#endif

return Py_BuildValue("{s:i,s:c,s:i,s:i,s:i,s:i,s:i,s:i,s:i}","zgesdd_",lapack_lite_status__,"jobz",jobz,"m",m,"n",n,"lda",lda,"ldu",ldu,"ldvt",ldvt,"lwork",lwork,"info",info);
}




static PyObject *lapack_lite_zgetrf(PyObject *self, PyObject *args)
{
int  lapack_lite_status__;
int m;
int n;
PyObject *a;
int lda;
PyObject *ipiv;
int info;
TRY(PyArg_ParseTuple(args,"iiOiOi",&m,&n,&a,&lda,&ipiv,&info));

TRY(lapack_lite_CheckObject(a,PyArray_CDOUBLE,"a","PyArray_CDOUBLE","zgetrf")); 
TRY(lapack_lite_CheckObject(ipiv,PyArray_INT,"ipiv","PyArray_INT","zgetrf")); 


#if defined(NO_APPEND_FORTRAN)
lapack_lite_status__ = zgetrf(&m,&n,ZDATA(a),&lda,IDATA(ipiv),&info);
#else
lapack_lite_status__ = zgetrf_(&m,&n,ZDATA(a),&lda,IDATA(ipiv),&info);
#endif

return Py_BuildValue("{s:i,s:i,s:i,s:i,s:i}","zgetrf_",lapack_lite_status__,"m",m,"n",n,"lda",lda,"info",info);
}

static PyObject *lapack_lite_zpotrf(PyObject *self, PyObject *args)
{
int  lapack_lite_status__;
int n;
PyObject *a;
int lda;
char uplo;
int info;

TRY(PyArg_ParseTuple(args,"ciOii",&uplo,&n,&a,&lda,&info));
TRY(lapack_lite_CheckObject(a,PyArray_CDOUBLE,"a","PyArray_CDOUBLE","zpotrf")); 

#if defined(NO_APPEND_FORTRAN)
lapack_lite_status__ = zpotrf(&uplo,&n,ZDATA(a),&lda,&info);
#else
lapack_lite_status__ = zpotrf_(&uplo,&n,ZDATA(a),&lda,&info);
#endif

return Py_BuildValue("{s:i,s:i,s:i,s:i}","zpotrf_",lapack_lite_status__,"n",n,"lda",lda,"info",info);
}

static struct PyMethodDef lapack_lite_module_methods[] = {
{"zheevd",lapack_lite_zheevd,1},
{"dsyevd",lapack_lite_dsyevd,1},
{"dgeev",lapack_lite_dgeev,1},
{"dgelsd",lapack_lite_dgelsd,1},
{"dgesv",lapack_lite_dgesv,1},
{"dgesdd",lapack_lite_dgesdd,1},
{"dgetrf",lapack_lite_dgetrf,1},
{"dpotrf",lapack_lite_dpotrf,1},
{"zgeev",lapack_lite_zgeev,1},
{"zgelsd",lapack_lite_zgelsd,1},
{"zgesv",lapack_lite_zgesv,1},
{"zgesdd",lapack_lite_zgesdd,1},
{"zgetrf",lapack_lite_zgetrf,1},
{"zpotrf",lapack_lite_zpotrf,1},
{ NULL,NULL,0}
};


static PyObject *lapack_liteError;

DL_EXPORT(void) initlapack_lite()
{ PyObject *m,*d;
m = Py_InitModule("lapack_lite",lapack_lite_module_methods);
import_array();
d = PyModule_GetDict(m);
ErrorObject = LapackError();
PyDict_SetItemString(d,"LapackError",ErrorObject);
}

