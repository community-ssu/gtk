# Copyright (C) 2004,2005 by SICEm S.L.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

import gettext
import optparse
import os
import sys

_ = gettext.gettext

PYGTK_REQUIRED = (2, 6, 0)

def open_project(app, filename, profile):
    if not profile:
        return app.open_project(filename)

    print 'profiling'
    import hotshot
    from hotshot import stats
    prof = hotshot.Profile("gazpacho.prof")
    retval = prof.runcall(app.open_project, filename)
    prof.close()

    s = stats.load("gazpacho.prof")
    s.strip_dirs()
    s.sort_stats('time', 'calls')
    s.print_stats(25)

def run_batch(app, command):
    ns = dict(new=lambda w: app.create(w))
    eval(command, {}, ns)

def run_console(app):
    try:
        import readline
        import rlcompleter
        assert rlcompleter
        readline.parse_and_bind('tab: complete')
    except ImportError:
        pass
    
    project = app.get_current_project()

    import code
    ia = code.InteractiveConsole(locals=locals())
    ia.interact()
    
def check_gtk(debug):
    if debug:
        print 'Loading PyGTK...'
    
    try:
        import gtk
    except ImportError:
        try:
            import pygtk
            # This modifies sys.path
            pygtk.require('2.0') 
            # Try again now when pygtk is imported
            import gtk
        except ImportError:
            raise SystemExit("PyGTK is required to run Gazpacho")
        
    import gobject
    if (os.path.dirname(os.path.dirname(gtk.__file__)) !=
        os.path.dirname(gobject.__file__)):
        print ('WARNING: GTK+ and GObject modules are not loaded from '
               'the same prefix')

    if debug:
        print 'Python:\t', '.'.join(map(str, sys.version_info[:3]))
        print 'GTK+:\t', '.'.join(map(str, gtk.gtk_version))
        print 'PyGTK:\t', '.'.join(map(str, gtk.pygtk_version))

    if gtk.pygtk_version < PYGTK_REQUIRED:
        raise SystemExit("PyGTK 2.6.0 or higher required to run Gazpacho")

    if gtk.pygtk_version >= (2, 7, 0):
        if gtk.pygtk_version in ((2, 7, 0), (2, 7, 1)):
            raise SystemExit("PyGTK 2.7.0 and 2.7.1 are buggy "
                             "Upgrade to 2.7.2 or downgrade to 2.6.x")
        
        if debug:
            print 'Using PyGTK 2.7.x, ignoring deprecation warnings'
            
        # Ignore deprecation warnings when using 2.7.x
        import warnings
        warnings.filterwarnings('ignore', category=DeprecationWarning)
    
def launch(options, filenames=[]):
    # Do this before importing application, which imports gtk
    check_gtk(debug=options.debug)

    # Delay imports, so command line parsing is not slowed down
    from gazpacho.application import Application
    from gazpacho.debugwindow import DebugWindow, show
    
    gazpacho = Application()

    if not options.debug:
        DebugWindow.application = gazpacho
        sys.excepthook = show

    for filename in filenames:
        if not os.path.exists(filename):
            raise SystemExit('%s: no such a file or directory' % filename)
        
        if not os.access(filename, os.R_OK):
            raise SystemExit('Could not open file %s: Permission denied.' %
                             filename)
        open_project(gazpacho, filename, options.profile)

    if options.update:
        for project in gazpacho.get_projects():
            project.save(project.path)

        return
    
    # If no filenames were specified, open up an empty project
    if not filenames:
        gazpacho.new_project()
    
    if options.batch:
        run_batch(gazpacho, options.batch)
        return
    elif options.console:
        run_console(gazpacho)
        return

    gazpacho.run()

def main(args=[]):
    from gazpacho.environ import environ
    gettext.bindtextdomain('gazpacho', environ.get_languages_dir())
    # When is this required, seems to work fine without in Python 2.[34]
    #gettext.bind_textdomain_codeset('utf-8')
    gettext.textdomain('gazpacho')
 
    parser = optparse.OptionParser()
    parser.add_option('', '--profile',
                      action="store_true", 
                      dest="profile",
                      help=_("Turn on profiling support"))
    parser.add_option('', '--debug',
                      action="store_true", 
                      dest="debug",
                      help=_("Turn on pdb debugging support"))
    parser.add_option('', '--batch',
                      action="store", 
                      dest="batch",
                      help=_("Batch command"))
    parser.add_option('-u', '--update',
                      action="store_true", 
                      dest="update",
                      help=_("Load glade file and save it"))
    parser.add_option('-c', '--console',
                      action="store_true", 
                      dest="console",
                      help=_("Start up a console"))
    
    options, args = parser.parse_args(args)

    if options.batch or options.console:
        options.debug = True
        
    filenames = []
    if len(args) >= 2:
        filenames = [os.path.abspath(name) for name in args[1:]]

    launch(options, filenames)
