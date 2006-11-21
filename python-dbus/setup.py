import os
import sys
from subprocess import *

sys.path.append("dbus")

from distutils.core import setup
from distutils.extension import Extension
from distutils.command.clean import clean
from Pyrex.Distutils import build_ext

import extract

sys.path.append("test")
from dbus_python_check import dbus_python_check

def remove(filename):
    if os.path.exists(filename):
        os.remove(filename)

class full_clean(clean):
    def run(self):
        clean.run(self)
        remove("dbus/extract.pyo")
        remove("dbus/dbus_bindings.pxd")
        remove("dbus/dbus_bindings.c")
        remove("dbus/dbus_glib_bindings.c")
        remove("test/dbus_python_check.pyo")
        remove("ChangeLog")


def run(cmd):
    try:
        p = Popen(' '.join(cmd), shell=True,
                stdin=PIPE, stdout=PIPE, stderr=PIPE,
                close_fds=True
            )
    except Exception, e:
        print >>sys.stderr, "ERROR: running %s (%s)" % (' '.join(cmd), e)
        raise SystemExit

    return p.stdout.read().strip(), p.stderr.read().strip()


def check_package(names, parms):
    cmd = [ "pkg-config" ]
    if isinstance(parms, list):
        cmd += parms
    else:
        cmd.append(parms)

    if isinstance(names, list):
        cmd += names
    else:
        cmd.append(names)

    output, error = run(cmd)

    if error:
        print >>sys.stderr, "ERROR: checking %s:\n%s" % (names, error)
        raise SystemExit

    return output


includedirs_flag = ['-I.', '-Idbus/']
dbus_includes = ['.']
dbus_glib_includes = ['.']
dbus_libs = []
dbus_glib_libs = []

output = check_package("dbus-1", "--cflags").split()
dbus_includes.extend([ x.replace("-I", "") for x in output ])
includedirs_flag.extend(output)

output = check_package("dbus-1", "--libs-only-L").split()
dbus_libs.extend([ x.replace("-L", "") for x in output ])

output = check_package("dbus-glib-1", "--cflags").split()
dbus_glib_includes.extend([ x.replace("-I", "") for x in output ])
includedirs_flag.extend(output)

output = check_package("dbus-glib-1", "--libs-only-L").split()
dbus_glib_libs.extend([ x.replace("-L", "") for x in output ])

output = open("dbus/dbus_bindings.pxd", 'w')
extract.main("dbus/dbus_bindings.pxd.in", includedirs_flag, output)
output.close()

#create ChangeLog only if this is a git repo
if os.path.exists(".git"):
    output, error = run(["git-log", "--stat"])

    if error:
        print "ERROR: running git-log (%s)" % (error)
        raise SystemExit

    file = open("ChangeLog", "w")
    file.writelines(output)
    file.close()

long_desc = '''D-BUS is a message bus system, a simple way for applications to
talk to one another.

D-BUS supplies both a system daemon (for events such as "new hardware device
added" or "printer queue changed") and a per-user-login-session daemon (for
general IPC needs among user applications). Also, the message bus is built on
top of a general one-to-one message passing framework, which can be used by any
two apps to communicate directly (without going through the message bus daemon).
Currently the communicating applications are on one computer, but TCP/IP option
is available and remote support planned.'''

setup(
    name='dbus-python',
    version='0.71',
    description='D-Bus Python bindings',
    long_description=long_desc,
    url='http://dbus.freedesktop.org/',
    author='John (J5) Palmieri',
    author_email='johnp@redhat.com',
    maintainer='John (J5) Palmieri',
    maintainer_email='johnp@redhat.com',
    package_dir={'dbus':'dbus'},
    py_modules=[
        "dbus/_dbus",
        "dbus/exceptions",
        "dbus/glib",
        "dbus/__init__",
        "dbus/matchrules",
        "dbus/service",
        "dbus/types",
        "dbus/decorators",
        "dbus/introspect_parser",
        "dbus/proxies",
        "dbus/_util",
    ],
    ext_modules=[
        Extension("dbus/dbus_bindings", ["dbus/dbus_bindings.pyx"],
            include_dirs=dbus_includes,
            library_dirs=dbus_libs,
            libraries=["dbus-1"],

        ),
        Extension("dbus/dbus_glib_bindings", ["dbus/dbus_glib_bindings.pyx"],
            include_dirs=dbus_glib_includes,
            library_dirs=dbus_glib_libs,
            libraries=["dbus-glib-1", "dbus-1", "glib-2.0"],
            define_macros=[
                ('DBUS_API_SUBJECT_TO_CHANGE', '1')
            ],
        ),
    ],
    cmdclass={'build_ext': build_ext, 'clean': full_clean, 'check': dbus_python_check}
)

# vim:ts=4:sw=4:tw=80:si:ai:showmatch:et
