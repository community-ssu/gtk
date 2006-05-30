==========
dotblas.py
==========

copyright (c) 2003 Richard Everson <R.M.Everson@exeter.ac.uk> and
Alexander Schmolck <A.Schmolck@exeter.ex.ac.uk>


.. contents::

License
-------
same as Numeric

Overview
--------

This module provides BLAS-optimized replacements of Numeric.dot (==
Numeric.matrixmultiply) and Numeric.innerproduct. The BLAS libraries
supplied by machine vendors are highly optimised and very efficient.
The ATLAS team [2]_ provide very good machine optimised BLAS libraries
which can be built or downloaded for most machines.


If you are concerned about speed, this dot() can give speed ups of
about 80 times (!) on 1000x1000 matrix multiplication of single floats
and 40 times for double floats. BLAS routines exist only for 32 & 64
bit float and complex types; so you will experience no speedup for
integer types (which will be deferred to Numeric.dot).


      from Numeric import *
      from dotblas import dot

where you formerly had::

      from Numeric import *

To use the dotblas version for your whole program you can also do:

      import Numeric
      import dotblas
      Numeric.dot = dotblas.dot

Or, possibly even simpler, you can just append::

      from dotblas import dot

to ``<python-path>/site-packages/Numeric/Numeric.py``, so that all code that
imports Numeric will use it by default.

Finally, if you don't know whether dotblas will be available on all the
machines where you whish to run your code, you can simply use a try-except
statement like this::

      import Numeric
      try:
          import dotblas
          Numeric.dot = dotblas.dot
       if we can't find the fast dotblas, just use the normal dot
      except ImportError: pass



Installation
------------

Make sure you have a BLAS library.  If you haven't got a vendor
supplied version, get one from ATLAS [1]_.  They have a number of
prebuilt ones, and you may well be able to find a Linux version to
suit you with rpmfind [3]_. Obviously you also need Numeric (22.0
or later recommended). If all those are installed correctly::
  
  python setup.py install
  
should be enough. Otherwise, you might have to edit setup.py, to
change the default settings for library paths etc.


Timing
------

The script ``profileDot.py`` will do some (fairly rudimentary) timings of the
new `dot` compared with the variants available in Numeric.

.. [1]  http://www.numpy.org

.. [2]  http://math-atlas.sourceforge.net/

.. [3]  http://rpmfind.net/linux/rpm2html/search.php?query=blas


Download
--------

dotblas-1.0a.tar.gz_

.. _dotblas-1.0a.tar.gz: dotblas-1.0a.tar.gz