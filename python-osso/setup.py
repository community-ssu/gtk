from distutils.core import setup, Extension

osso = Extension('osso',
            sources = ['osso.c'],
            libraries = [
                'osso',
                'dbus-glib-1',
                'dbus-1',
                'glib-2.0',
            ],
            include_dirs=[
                '/usr/include',
                '/usr/include/dbus-1.0',
                '/usr/lib/dbus-1.0/include',
                '/usr/include/glib-2.0',
                '/usr/lib/glib-2.0/include',
            ],
            extra_compile_args=[
                 '-Os',
                 '-rdynamic',
                 '-g',
#                '-pedantic',
#                '-ansi',
#                '-Wno-long-long',
            ],
        )

setup(name = 'osso', ext_modules = [osso])
