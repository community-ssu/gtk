#!/usr/bin/env python

from distutils.core import setup, Extension
import sys
import os

if sys.platform == 'win32':
    # XXX what's a better way to add the Platform SDK include path?
    XP2_PSDK_PATH = r"\Program Files\Microsoft Platform SDK for Windows XP SP2"
    S03_PSDK_PATH = r"\Program files\Microsoft Platform SDK"
    PSDK_PATH = None
    for p in [ XP2_PSDK_PATH, S03_PSDK_PATH ]:
        if os.path.exists(p):
            PSDK_PATH = p
            break
    if PSDK_PATH is None:
        raise SystemExit("Can't find the Windows XP Platform SDK")
    
    extmod = Extension( '_msbt', 
                        include_dirs = ["%s\\Include" % PSDK_PATH],
                        library_dirs = ["%s\\Lib" % PSDK_PATH],
                        libraries = [ "WS2_32", "Irprops" ],
                        sources=['msbt\\_msbt.c'], )
elif sys.platform == 'linux2':
    extmod = Extension('_bluetooth',
		                 libraries = ['bluetooth'],
                         sources = ['bluez/btmodule.c', 'bluez/btsdp.c'])

setup (	name = 'PyBluez',
       	version = '0.9.1',
       	description = 'Bluetooth Python extension module',
       	author="Albert Huang",
     	author_email="ashuang@alum.mit.edu",
      	url="http://org.csail.mit.edu/pybluez",
       	ext_modules = [ extmod ],
        py_modules = [ 'bluetooth' ],

# for the python cheese shop
        classifiers = [ 'Development Status :: 4 - Beta',
            'License :: OSI Approved :: GNU General Public License (GPL)',
            'Programming Language :: Python',
            'Topic :: Communications' ],
        download_url = 'http://org.csail.mit.edu/pybluez/download.html',
        long_description = 'Bluetooth Python extension module to allow Python developers to use system Bluetooth resources. PyBluez works with GNU/Linux and Windows XP.',
        maintainer = 'Albert Huang',
        maintainer_email = 'albert@csail.mit.edu',
        license = 'GPL',
        )
