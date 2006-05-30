## Customization variables for installing Numeric

# You could put "-O4" etc. here.
extra_compile_args = []
extra_link_args = []
include_dirs = []

# If you want to use your system's LAPACK libraries, set use_system_lapack
# to 1, put the directories in lapack_library_dirs, and the libraries to
# link to in lapack_libraries.

# If use_system_lapack is false, f2c'd versions of the required routines
# will be used, except on Mac OS X, where the vecLib framework will be used
# if found.

use_system_lapack = 0
lapack_library_dirs = []
lapack_libraries = []
lapack_extra_link_args = []

# Example: using ATLAS
if 0:
    use_system_lapack = 1
    lapack_library_dirs = ['/usr/lib/atlas']
    lapack_libraries = ['lapack', 'cblas', 'f77blas', 'atlas', 'g2c']

# Example: Some Linux distributions have ATLAS, but not LAPACK (if you're
#          liblapack file is smaller than a megabyte, this is probably you)
#          Use our own f2c'd lapack libraries, but use ATLAS for BLAS.
if 0:
    use_system_lapack = 0
    use_system_blas = 1
    lapack_library_dirs = ['/usr/lib/atlas']
    lapack_libraries = ['cblas', 'f77blas', 'atlas', 'g2c']

# Set use_dotblas to 1 to use BLAS for the matrix multiplication routines.
# Put the directory that 'cblas.h' is in into dotblas_include_dirs
use_dotblas = 0
dotblas_include_dirs = []
dotblas_cblas_header = '<cblas.h>'
dotblas_library_dirs = lapack_library_dirs
dotblas_libraries = lapack_libraries
dotblas_extra_link_args = []

# Example: using ATLAS
if 0:
    use_dotblas = 1
    dotblas_include_dirs = ['/usr/include/atlas']
    dotblas_library_dirs = lapack_library_dirs
    dotblas_libraries = ['cblas', 'atlas', 'g2c']

# Example: using the Gnu Scientific Library's CBLAS interface
# This is useful if GSL was compiled against an optimized Fortran BLAS.
if 0:
    use_dotblas = 1
    dotblas_include_dirs = ['/usr/include/gsl/']
    dotblas_cblas_header = '<gsl_cblas.h>'
    dotblas_library_dirs = []
    dotblas_libraries = ['gslcblas']
