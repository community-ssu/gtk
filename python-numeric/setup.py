#!/usr/bin/env python
# To use:
#       python setup.py install
# or:
#       python setup.py bdist_rpm (you'll end up with RPMs in dist)
#
import os, sys, string, re
from glob import glob
if not hasattr(sys, 'version_info') or sys.version_info < (2,0,0,'alpha',0):
    raise SystemExit, "Python 2.0 or later required to build Numeric."
import distutils
from distutils.core import setup, Extension

# Get all version numbers
execfile(os.path.join('Lib','numeric_version.py'))
numeric_version = version

execfile(os.path.join('Packages', 'MA', 'Lib', 'MA_version.py'))
MA_version = version

headers = glob (os.path.join ("Include","Numeric","*.h"))
extra_compile_args = []  # You could put "-O4" etc. here.
mathlibs = ['m']
define_macros = [('HAVE_INVERSE_HYPERBOLIC',None)]
undef_macros = []
# You might need to add a case here for your system
if sys.platform in ['win32']:
    mathlibs = []
    define_macros = []
    undef_macros = ['HAVE_INVERSE_HYPERBOLIC']
elif sys.platform in ['mac', 'beos5']:
    mathlibs = []

# delete all but the first one in this list if using your own LAPACK/BLAS
sourcelist = [os.path.join('Src', 'lapack_litemodule.c'),
              os.path.join('Src', 'blas_lite.c'), 
              os.path.join('Src', 'f2c_lite.c'), 
              os.path.join('Src', 'zlapack_lite.c'),
              os.path.join('Src', 'dlapack_lite.c')
             ]
# set these to use your own BLAS;
sourcelist = [os.path.join('Src', 'lapack_litemodule.c')]

library_dirs_list = []
libraries_list = []

if os.environ.has_key('USE_LAPACK'):
    print "use system LAPACK/BLAS libraries"
    sourcelist = [os.path.join('Src', 'lapack_litemodule.c')]
    #libraries_list = ['lapack_atlas', 'cblas', 'f77blas', 'atlas', 'g2c']
    libraries_list = ['lapack', 'blas', 'g2c']
else:
    sourcelist.append(os.path.join('Src', 'blas_lite.c'))
    sourcelist.append(os.path.join('Src', 'f2c_lite.c'))
    sourcelist.append(os.path.join('Src', 'zlapack_lite.c'))
    sourcelist.append(os.path.join('Src', 'dlapack_lite.c'))

if os.environ.has_key('FLAG_FUNCTION_SECTIONS'):
    print "compile with -ffunction-sections"
    extra_compile_args.append('-ffunction-sections')

# set to true (1), if you also want BLAS optimized matrixmultiply/dot/innerproduct
use_dotblas = 0
if os.environ.has_key('USE_DOTBLAS'):
    use_dotblas = 1
else:
    use_dotblas = 0
include_dirs = []#'/usr/include/atlas']  
                   # You may need to set this to find cblas.h
                   #  e.g. on UNIX using ATLAS this should be ['/usr/include/atlas']
extra_link_args = []

# for MacOS X to link against vecLib if present
VECLIB_PATH = '/System/Library/Frameworks/vecLib.framework'
if os.path.exists(VECLIB_PATH):
    extra_link_args = ['-framework', 'vecLib']
    include_dirs = [os.path.join(VECLIB_PATH, 'Headers')]

# The packages are split in this way to allow future optional inclusion
# Numeric package
packages = ['']
package_dir = {'': 'Lib'}
include_dirs.append('Include')
ext_modules = [
    Extension('_numpy',
              [os.path.join('Src', '_numpymodule.c'),
               os.path.join('Src', 'arrayobject.c'),
               os.path.join('Src', 'ufuncobject.c')],
              extra_compile_args = extra_compile_args),
    Extension('multiarray',
              [os.path.join('Src', 'multiarraymodule.c')],
              extra_compile_args = extra_compile_args),
    Extension('umath',
              [os.path.join('Src', 'umathmodule.c')],
              libraries = mathlibs,
              define_macros = define_macros,
              undef_macros = undef_macros,
              extra_compile_args = extra_compile_args),
    Extension('arrayfns',
              [os.path.join('Src', 'arrayfnsmodule.c')],
              extra_compile_args = extra_compile_args),
    Extension('ranlib',
              [os.path.join('Src', 'ranlibmodule.c'),
               os.path.join('Src', 'ranlib.c'),
               os.path.join('Src', 'com.c'),
               os.path.join('Src', 'linpack.c')],
              extra_compile_args = extra_compile_args),
    Extension('lapack_lite', sourcelist,
              library_dirs = library_dirs_list,
              libraries = libraries_list,
              extra_link_args = extra_link_args,
              extra_compile_args = extra_compile_args) 
    ]

# add FFT package (optional)
packages.append('FFT')
package_dir['FFT'] = os.path.join('Packages','FFT','Lib')
include_dirs.append(os.path.join('Packages','FFT','Include'))
ext_modules.append(Extension('FFT.fftpack',
                             [os.path.join('Packages','FFT','Src', 'fftpackmodule.c'),
                              os.path.join('Packages', 'FFT', 'Src', 'fftpack.c')],
                             extra_compile_args = extra_compile_args))

# add MA package (optional) 
packages.append('MA')
package_dir['MA'] = os.path.join('Packages', 'MA', 'Lib')

# add RNG package (optional)
packages.append('RNG')
package_dir['RNG'] = os.path.join('Packages', 'RNG', 'Lib')
include_dirs.append(os.path.join('Packages', 'RNG', 'Include'))
ext_modules.append(Extension('RNG.RNG',
                             [os.path.join('Packages', 'RNG', 'Src', 'RNGmodule.c'),
                              os.path.join('Packages', 'RNG', 'Src', 'ranf.c'),
                              os.path.join('Packages', 'RNG', 'Src', 'pmath_rng.c')],
                             extra_compile_args = extra_compile_args))
# add dotblas package (optional)
if use_dotblas:
    packages.append('dotblas')
    package_dir['dotblas'] = os.path.join('Packages', 'dotblas', 'dotblas')
    ext_modules.append(Extension('_dotblas',
                                 [os.path.join('Packages', 'dotblas', 'dotblas', '_dotblas.c')],
                                 library_dirs = library_dirs_list,
                                 libraries = libraries_list,
                                 extra_compile_args=extra_compile_args))




long_description = """
Numerical Extension to Python with subpackages.

The authors and maintainers of the subpackages are: 

FFTPACK-3.1
        maintainer = "Numerical Python Developers"
        maintainer_email = "numpy-discussion@lists.sourceforge.net"
        description = "Fast Fourier Transforms"
        url = "http://numpy.sourceforge.net"

MA-%s
        author = "Paul F. Dubois"
        description = "Masked Array facility"
        maintainer = "Paul F. Dubois"
        maintainer_email = "dubois@users.sf.net"
        url = "http://sourceforge.net/projects/numpy"

RNG-3.1
        author = "Lee Busby, Paul F. Dubois, Fred Fritsch"
        maintainer = "Paul F. Dubois"
        maintainer_email = "dubois@users.sf.net"
        description = "Cray-like Random number package."
""" % (MA_version, )

# Oops, another bug in Distutils!?
# Write rpm_build.sh pointing to this python
rpm_build_text="""env CFLAGS="$RPM_OPT_FLAGS" %s setup.py build\n""" % sys.executable
rpm_script = open("rpm_build.sh", "w")
rpm_script.write(rpm_build_text)
rpm_script.close()

# Write rpm_install.sh pointing to this python
rpm_install_text=sys.executable +""" setup.py install --root=$RPM_BUILD_ROOT

cat >INSTALLED_FILES <<EOF
%doc Demo
EOF
find $RPM_BUILD_ROOT -type f | sed -e "s|$RPM_BUILD_ROOT||g" >>INSTALLED_FILES

""" 
rpm_script = open("rpm_install.sh", "w")
rpm_script.write(rpm_install_text)
rpm_script.close()

setup (name = "Numeric",
       version = numeric_version,
       maintainer = "Numerical Python Developers",
       maintainer_email = "numpy-developers@lists.sourceforge.net",
       description = "Numerical Extension to Python",
       long_description = long_description,
       url = "http://numpy.sourceforge.net",

       # distutils allows you to fix or fudge anything :-)
       extra_path = 'Numeric',
       packages = packages,
       package_dir = package_dir,
       headers = headers,
       include_dirs = include_dirs,
       ext_modules = ext_modules
       )

print 'MA Version', MA_version
print 'Numeric Version', numeric_version
