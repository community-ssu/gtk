import os, string
from distutils.core import setup
s = os.path.join('Lib','MA_version.py')
execfile(s)
setup (name = "MA",
       version=version,
       author="Paul F. Dubois",
       description = "Masked Array facility",
       maintainer = "Numerical Python Developers",
       maintainer_email = "numpy-developers@lists.sourceforge.net",
       url = "http://sourceforge.net/projects/numpy",
       extra_path = 'Numeric',
       packages = ['MA'],
       package_dir = {'MA': 'Lib'},
      )
print "MA Version", version
    
