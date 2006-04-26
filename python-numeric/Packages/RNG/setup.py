#!/usr/bin/env python

# Setup script for building the RNG extension to Python.

# To use:
#       python setup.py build
#     or this to build and install it
#       python setup.py install

import string,re

# Check for an advanced enough Distutils.
import distutils
from distutils.core import setup, Extension

setup (name = "RNG",
       version = "3.1",
       maintainer = "Paul Dubois",
       maintainer_email = "dubois@users.sourceforge.net",
       description = "Cray-like Random number package.",
       extra_path = 'Numeric',
       packages = ['RNG'],
       package_dir = {'RNG': 'Lib'},
       include_dirs=['Include','../../Include'],
       ext_modules=[Extension('RNG.RNG',
                       ['Src/RNGmodule.c',
                        'Src/ranf.c',
                        'Src/pmath_rng.c'],
                             )
                   ]
       )
