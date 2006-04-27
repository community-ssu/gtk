from distutils.core import setup, Extension

osso = Extension('osso',
            sources = [
                'osso.c',
                'osso-rpc.c',
                'osso-autosave.c',
                'osso-time-notification.c',
                'osso-mime.c',
                'osso-device-state.c',
                'osso-misc.c',
                'osso-helper.c',
                'osso-application.c',
                'osso-statusbar.c',
                'osso-system-note.c',
                'osso-state.c',
                'osso-plugin.c',
                ],
            libraries = [
                'osso',
                'dbus-1',
                'dbus-glib-1',
                'glib-2.0',
                'gobject-2.0',
                'gmodule-2.0',
                'atk-1.0',
                'pangoxft-1.0',
                'pangox-1.0',
                'pango-1.0',
                'gdk-x11-2.0',
                'gdk_pixbuf-2.0',
                'gtk-x11-2.0',
            ],
            include_dirs=[
                '/usr/include',
                '/usr/include/freetype2',
                '/usr/include/dbus-1.0',
                '/usr/include/glib-2.0',
                '/usr/include/atk-1.0',
                '/usr/include/pango-1.0',
                '/usr/include/gtk-2.0',
                '/usr/include/pygtk-2.0',
                '/usr/lib/dbus-1.0/include',
                '/usr/lib/glib-2.0/include',
                '/usr/lib/gtk-2.0/include',
                '/usr/X11R6/include',
            ],
            extra_compile_args=[
                '-Os',
                '-ansi',
                '-DXTHREADS',
                '-DXUSE_MTSAFE_API',
#                '-pedantic',
#                '-Wno-long-long',
#                '-g',
#                '-rdynamic',
            ],
        )

setup(
        name = 'osso',
        version = '0.1',
        description = 'Python bindings for libosso components.',
        author = 'Osvaldo Santana Neto',
        author_email = 'osvaldo.santana@indt.org.br',
        url = 'http://www.maemo.org',
        ext_modules = [osso]
)
