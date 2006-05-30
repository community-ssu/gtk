#!/usr/bin/env python
# To use:
#       python setup.py install
# or:
#       python setup.py bdist_rpm (you'll end up with RPMs in dist)
#
# Optionally, you can run
#       python setup.py config
# first. This fixes a bug in LinearAlgebra on Cygwin (and possibly
# other platforms).
#
import os, sys

if not hasattr(sys, 'version_info') or sys.version_info < (2,0,0,'alpha',0):
    raise SystemExit, "Python 2.0 or later required to build Numeric."

from glob import glob
try:
    from setuptools import setup
    have_setuptools = 1
except ImportError:
    from distutils.core import setup
    have_setuptools = 0
from distutils.core import Extension
from distutils.command.config import config
from distutils.sysconfig import get_config_var, customize_compiler
from distutils.cygwinccompiler import CygwinCCompiler, Mingw32CCompiler
from distutils.bcppcompiler import BCPPCompiler
from distutils.msvccompiler import MSVCCompiler
try:
    from distutils.command.config import log
except:
    pass

# Run the configuration
class config_numpy(config):
    def run (self):
        # Get a compiler
        self._check_compiler()
        try: log.set_verbosity(0)
        except: pass
        self.dump_source = 0
        # Switch off optimization
        if isinstance(self.compiler, BCPPCompiler):
            self.compiler.compile_options.remove('/O2')
        elif isinstance(self.compiler, MSVCCompiler):
            self.compiler.compile_options.remove('/Ox')
            self.compiler.compile_options.remove('/GX')
        else:
            if isinstance(self.compiler, CygwinCCompiler):
                cc = 'gcc'
            elif isinstance(self.compiler, Mingw32CCompiler):
                cc = 'gcc -mno-cygwin'
            else: # UnixCCompiler
                cc = get_config_var('CC')
            if os.environ.has_key('CC'):
                cc = os.environ['CC']
            self.compiler.set_executable('compiler_so',cc)
        testcode = """\
#include "%s"
""" % os.path.join("Src","config.c")
        if self.try_run(testcode):
            print "Wrote config.h"
            if os.path.isfile(os.path.join("Src","config.h")):
                os.remove(os.path.join("Src","config.h"))
            os.rename("config.h",os.path.join("Src","config.h"))
        else:
            print "Configuration failed, using default compilation"
            if os.path.isfile(os.path.join("Src","config.h")):
                os.remove(os.path.join("Src","config.h"))

        # Restore usual compiler flags
        if isinstance(self.compiler, BCPPCompiler):
            self.compiler.compile_options.append('/O2')
        elif isinstance(self.compiler, MSVCCompiler):
            self.compiler.compile_options.append('/Ox')
            self.compiler.compile_options.append('/GX')
        else:
            customize_compiler(self.compiler)

def path(name):
    "Convert a /-separated pathname to one using the OS's path separator."
    splitted = name.split('/')
    return os.path.join(*splitted)

# Set up the default customizations, and then read customize.py for the
# user's customizations.
class customize:
    extra_compile_args = []
    extra_link_args = []
    include_dirs = []
    use_system_lapack = 0
    use_system_blas = 0
    lapack_library_dirs = []
    lapack_libraries = []
    lapack_extra_link_args = []
    use_dotblas = 0
    dotblas_include_dirs = []
    dotblas_cblas_header = '<cblas.h>'
    dotblas_library_dirs = []
    dotblas_extra_link_args = []
execfile('customize.py', customize.__dict__)

# Get all version numbers
execfile(path('Lib/numeric_version.py'))
numeric_version = version

execfile(path('Packages/MA/Lib/MA_version.py'))
MA_version = version

extra_compile_args = customize.extra_compile_args
extra_link_args = customize.extra_link_args
include_dirs = customize.include_dirs[:]
define_macros = []
undef_macros = []

umath_define_macros = [('HAVE_INVERSE_HYPERBOLIC','1')]
mathlibs = ['m']
undef_macros = []
# You might need to add a case here for your system
if sys.platform in ['win32']:
    mathlibs = []
    umath_define_macros = [('HAVE_INVERSE_HYPERBOLIC', '0')]
elif sys.platform in ['mac', 'beos5']:
    mathlibs = []

# Find out if the config.h file is available
if os.path.isfile(os.path.join("Src","config.h")):
    extra_compile_args.append('-DHAVE_CONFIG')

# For Mac OS X >= 10.2, an optimized BLAS and most of LAPACK (all the
# routines we need, at least) should already be installed
VECLIB_PATH = '/System/Library/Frameworks/vecLib.framework'
have_veclib = os.path.exists(VECLIB_PATH)

def extension(name, sources, **kw):
    def prepend(name, value, kw=kw):
        kw[name] = value + kw.get(name, [])
    prepend('extra_compile_args', extra_compile_args)
    prepend('extra_link_args', extra_link_args)
    prepend('define_macros', define_macros)
    prepend('undef_macros', undef_macros)
    return Extension(name, sources, **kw)

lapack_source = [path('Src/lapack_litemodule.c')]
lapack_link_args = customize.lapack_extra_link_args
if customize.use_system_lapack:
    pass
elif have_veclib:
    lapack_link_args.extend(['-framework', 'vecLib'])
else:
    lapack_source.extend([path('Src/f2c_lite.c'),
                          path('Src/zlapack_lite.c'),
                          path('Src/dlapack_lite.c'),
                          path('Src/dlamch.c'),
                         ])
    if not customize.use_system_blas:
        lapack_source.append(path('Src/blas_lite.c'))

lapack_ext = extension('lapack_lite', lapack_source,
                       library_dirs=customize.lapack_library_dirs,
                       libraries=customize.lapack_libraries,
                       extra_link_args=lapack_link_args)

dotblas_source = [path('Packages/dotblas/dotblas/_dotblas.c')]
if customize.use_dotblas:
    dotblas_ext = extension('_dotblas', dotblas_source,
                            include_dirs=customize.dotblas_include_dirs,
                            library_dirs=customize.dotblas_library_dirs,
                            libraries=customize.dotblas_libraries,
                            define_macros=[('CBLAS_HEADER',
                                            customize.dotblas_cblas_header)],
                            extra_link_args=customize.dotblas_extra_link_args,
                           )
elif have_veclib:
    dotblas_ext = extension('_dotblas', dotblas_source,
                            include_dirs=[os.path.join(VECLIB_PATH, 'Headers')],
                            define_macros=[('CBLAS_HEADER', '<cblas.h>')],
                            extra_link_args=['-framework', 'vecLib'],
                           )
else:
    dotblas_ext = None

# The packages are split in this way to allow future optional inclusion
# Numeric package
packages = ['']
package_dir = {'': 'Lib'}
include_dirs.append('Include')
ext_modules = [
    extension('_numpy',
              [path('Src/_numpymodule.c'),
               path('Src/arrayobject.c'),
               path('Src/ufuncobject.c')],
             ),
    extension('multiarray',
              [path('Src/multiarraymodule.c')],
             ),
    extension('umath',
              [path('Src/umathmodule.c')],
              libraries = mathlibs,
              define_macros = umath_define_macros,
             ),
    extension('arrayfns',
              [path('Src/arrayfnsmodule.c')],
             ),
    extension('ranlib',
              [path('Src/ranlibmodule.c'),
               path('Src/ranlib.c'),
               path('Src/com.c'),
               path('Src/linpack.c')],
             ),
    lapack_ext,
    ]

# add FFT package (optional)
packages.append('FFT')
package_dir['FFT'] = path('Packages/FFT/Lib')
include_dirs.append(path('Packages/FFT/Include'))
ext_modules.append(extension('FFT.fftpack',
                             [path('Packages/FFT/Src/fftpackmodule.c'),
                              path('Packages/FFT/Src/fftpack.c')]))

# add MA package (optional)
packages.append('MA')
package_dir['MA'] = path('Packages/MA/Lib')

# add RNG package (optional)
packages.append('RNG')
package_dir['RNG'] = path('Packages/RNG/Lib')
include_dirs.append(path('Packages/RNG/Include'))
ext_modules.append(extension('RNG.RNG',
                             [path('Packages/RNG/Src/RNGmodule.c'),
                              path('Packages/RNG/Src/ranf.c'),
                              path('Packages/RNG/Src/pmath_rng.c')]))
if dotblas_ext:
    packages.append('dotblas')
    package_dir['dotblas'] = path('Packages/dotblas/dotblas')
    ext_modules.append(dotblas_ext)

headers = glob(path('Include/Numeric/*.h'))
packages.append('Numeric_headers')
package_dir['Numeric_headers'] = path('Include')

cmdclass = {'config': config_numpy}
setuptools = {}
if have_setuptools:
    setuptools['zip_safe'] = 0
    setuptools['package_data'] = {
        'Numeric_headers' : ['Numeric/*.h'],
    }
    # By default, we don't want to install as an egg -- that would
    # screw backwards compatibility.
    from distutils.command.install import install
    cmdclass['install'] = install


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
rpm_build_text = \
    'env CFLAGS="$RPM_OPT_FLAGS" %s setup.py build\n' % sys.executable
rpm_script = open("rpm_build.sh", "w")
rpm_script.write(rpm_build_text)
rpm_script.close()

# Write rpm_install.sh pointing to this python
rpm_install_text = sys.executable + """ \
setup.py install --root=$RPM_BUILD_ROOT

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
       ext_modules = ext_modules,
       cmdclass = cmdclass,
       **setuptools
       )

print 'MA Version', MA_version
print 'Numeric Version', numeric_version
