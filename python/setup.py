########################################################################
#
#       License: MIT
#       Created: September 22, 2010
#       Author:  Francesc Alted - faltet@pytables.org
#
########################################################################

import sys, os

from distutils.core import Extension
from distutils.core import setup

########### Check versions ##########

# Check for Python
if sys.version_info[0] == 2:
    if sys.version_info[1] < 6:
        exit_with_error("You need Python 2.6 or greater to install blosc!")
elif sys.version_info[0] == 3:
    if sys.version_info[1] < 1:
        exit_with_error("You need Python 3.1 or greater to install blosc!")
else:
    exit_with_error("You need Python 2.6/3.1 or greater to install blosc!")

########### End of checks ##########


# Blosc version
VERSION = open('VERSION').read().strip()
# Create the version.py file
open('blosc/version.py', 'w').write('__version__ = "%s"\n' % VERSION)


# Global variables
CFLAGS = os.environ.get('CFLAGS', '').split()
LFLAGS = os.environ.get('LFLAGS', '').split()
lib_dirs = []
libs = []
inc_dirs = ['../blosc']
optional_libs = []

# Handle --lflags=[FLAGS] --cflags=[FLAGS]
args = sys.argv[:]
for arg in args:
    if arg.find('--lflags=') == 0:
        LFLAGS = arg.split('=')[1].split()
        sys.argv.remove(arg)
    elif arg.find('--cflags=') == 0:
        CFLAGS = arg.split('=')[1].split()
        sys.argv.remove(arg)

# Add -msse2 flag for optimizing shuffle in Blosc
if os.name == 'posix':
    CFLAGS.append("-msse2")

def_macros = [('NDEBUG', 1)]


classifiers = """\
Development Status :: 4 - Beta
Intended Audience :: Developers
Intended Audience :: Information Technology
Intended Audience :: Science/Research
License :: OSI Approved :: BSD License
Programming Language :: Python
Topic :: Compression
Topic :: Software Development :: Libraries :: Python Modules
Operating System :: Microsoft :: Windows
Operating System :: Unix
"""
setup(name = "blosc",
      version = VERSION,
      description = 'Blosc data compressor',
      long_description = """\

Blosc is a high performance compressor optimized for binary data.

""",
      classifiers = filter(None, classifiers.split("\n")),
      author = 'Francesc Alted',
      author_email = 'faltet@pytables.org',
      maintainer = 'Francesc Alted',
      maintainer_email = 'faltet@pytables.org',
      url = 'http://blosc.pytables.org',
      license = 'http://www.opensource.org/licenses/mit-license.php',
      download_url = 'http://blosc.pytables.org/sources/stable/blosc-%s.tar.gz' % VERSION,
      platforms = ['any'],
      ext_modules = [
        Extension( "blosc.blosc_extension",
                   include_dirs=inc_dirs,
                   define_macros=def_macros,
                   sources = [ "blosc/blosc_extension.c",
                               "../blosc/blosc.c", "../blosc/blosclz.c",
                               "../blosc/shuffle.c" ],
                   depends = [ "../blosc/blosc.h", "../blosc/blosclz.h",
                               "../blosc/shuffle.h" ],
                   library_dirs=lib_dirs,
                   libraries=libs,
                   extra_link_args=LFLAGS,
                   extra_compile_args=CFLAGS ),
        ],
      packages = ['blosc'],

)
