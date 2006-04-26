from distutils.core import setup
from distutils.sysconfig import get_python_lib
import os
from Pyrex.Compiler.Version import version

compiler_dir = os.path.join(get_python_lib(prefix=''), 'Pyrex/Compiler')

if os.name == "posix":
    scripts = ["bin/pyrexc"]
else:
    scripts = ["pyrexc.py"]

setup(
  name = 'Pyrex', 
  version = version,
  url = 'http://www.cosc.canterbury.ac.nz/~greg/python/Pyrex/',
  author = 'Greg Ewing',
  author_email = 'greg@cosc.canterbury.ac.nz',
  scripts = scripts,
  packages=[
    'Pyrex',
    'Pyrex.Compiler',
    'Pyrex.Distutils',
    'Pyrex.Mac',
    'Pyrex.Plex'
    ],
  data_files=[
    (compiler_dir, ['Pyrex/Compiler/Lexicon.pickle'])
    ]
  )

