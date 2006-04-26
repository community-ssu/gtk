import os, sys, string, re
from glob import glob

# Check for an advanced enough Distutils.
import distutils
from distutils.core import setup, Extension
setup (name = "FFTPACK",
       version = "3.1",
       maintainer = "Numerical Python Developers",
       maintainer_email = "numpy-discussion@lists.sourceforge.net",
       description = "Fast Fourier Transforms",
       url = "http://numpy.sourceforge.net",
       extra_path= 'Numeric',
       packages = ['FFT'],
       package_dir = {'FFT': 'Lib'},
       include_dirs = ['Include','../../Include'],
       ext_modules = [ Extension('FFT.fftpack',
                                ['Src/fftpackmodule.c',
                                 'Src/fftpack.c']),
                     ]
       )
