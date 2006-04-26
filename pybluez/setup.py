#!/usr/bin/env python
# Test setup script for installing bluez module.
#

from distutils.core import setup, Extension

module1 = Extension('_bluetooth',
		    libraries = ['bluetooth'],
            sources = ['src/btmodule.c', 'src/btsdp.c'])

setup (	name = 'PyBluez',
       	version = '0.6.1',
       	description = 'python wrappers around bluez',
       	author="Albert Huang, Calvin On",
     	author_email="albert@csail.mit.edu, calvinon@mit.edu",
      	url="http://org.csail.mit.edu/pybluez",
       	ext_modules = [module1],
        py_modules = [ 'bluetooth' ],

# for the python cheese shop
        classifiers = [ 'Development Status :: 4 - Beta',
            'License :: OSI Approved :: GNU General Public License (GPL)',
            'Operating System :: POSIX :: Linux',
            'Programming Language :: Python',
            'Topic :: Communications' ],
        download_url = 'http://org.csail.mit.edu/pybluez/download.html',
        long_description = 'python wrappers around bluez to allow python developers to use system bluetooth resources. PyBluez works on machines running the GNU/Linux operating system and the bluez bluetooth stack.',
        maintainer = 'Albert Huang',
        maintainer_email = 'albert@csail.mit.edu',
        license = 'GPL',
        )
