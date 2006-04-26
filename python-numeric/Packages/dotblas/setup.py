#!/usr/bin/env python
"""Setup script for the dotblas module distribution."""
__revision__ = "$Version$"

from distutils.core import setup, Extension

# Set this to point at your BLAS/ATLAS libraries
blas_dirs_list = ['/usr/lib/']
blas_libraries_list = ['lapack','cblas','f77blas', 'atlas', 'g2c']

setup (# Distribution meta-data
       name = "dotblas",
       version = "1.0a",
       description = "Provides a BLAS optimised dot product for Numeric arrays",
       author = "Richard Everson",
       author_email = "R.M.Everson@exeter.ac.uk",
       url = "http://www.dcs.ex.ac.uk/people/reverson",
       license = "MIT",
       # Description of the modules and packages in the distribution
       packages = ['dotblas'],
       ext_modules = 
           [Extension('_dotblas', ['dotblas/_dotblas.c'],
                      library_dirs=blas_dirs_list ,
                      libraries=blas_libraries_list,
                      ),
           ]
      )
