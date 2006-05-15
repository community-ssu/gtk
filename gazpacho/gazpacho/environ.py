#
# Copyright (C) 2005 by Async Open Source 
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
#

import os

import gazpacho.widgets

__all__ = ['environ']

try:
    # We can't use from .. import ... as module until pyflakes
    # can handle it properly
    from gazpacho import __installed__
    module = __installed__
except ImportError:
    try:
        from gazpacho import __uninstalled__
        module = __uninstalled__
    except ImportError:
        raise SystemExit("FATAL ERROR: Internal error, could not load"
                         "gazpacho.\n"
                         "Tried to start gazpacho but critical configuration "
                         "were are missing.\n")

class Environment:
    def __init__(self):
        self._variables = {}
        self._external = {}
        
        self._add_variable('pixmaps_dir')
        self._add_variable('resources_dir')
        self._add_variable('languages_dir')
        self._add_variable('catalogs_dir')
        self._add_variable('glade_dir')
        self._add_variable('docs_dir')

    def _add_variable(self, variable):
        value = getattr(module, variable, None)
        if not value:
            raise SystemExit("Configuration option %s was not found" % variable)
            
        self._variables[variable] = [value]
        
    def _add_external(self, name, variable, dirname):
        if not os.path.exists(dirname):
            return
        
        self._variables[variable].append(dirname)
        if not name in self._external:
            self._external[name] = {}
        self._external[name][variable] = dirname
        
    def find_object(self, name, kind, variable, warn=True):
        for dirname in self._variables[variable]:
            filename = os.path.join(dirname, name)
            if not os.path.exists(filename):
                continue

            if not os.access(filename, os.R_OK):
                continue
            
            return filename

    def find_pixmap(self, name):
        retval = self.find_object(name, 'pixmap', 'pixmaps_dir')
        if not retval:
            print 'Could not find pixmap: %s' % name
        return retval
    
    def find_catalog(self, name):
        retval = self.find_object(name, 'catalog', 'catalogs_dir')
        if not retval:
            print 'Could not find catalog: %s' % name
        return retval
    
    def find_glade(self, name):
        retval = self.find_object(name, 'glade', 'glade_dir')
        if not retval:
            print 'Could not find glade: %s' % name
        return retval
    
    def find_resource(self, resource, name):
        retval = self.find_object(name, 'resource', 'resources_dir', warn=False)
        if not retval: 
            filename = os.path.join(resource, name)
            retval = self.find_object(filename, 'resource', 'resources_dir')
            if not retval:
                print 'Could not find resource: %s' % name
                
        return retval

    def get_languages_dir(self):
        return self._variables['languages_dir'][0]

    def get_resources_dir(self, name):
        if name in self._external:
            return self._external[name]['resources_dir']
        
        return self._variables['resources_dir']

    def get_docs_dir(self):
        return self._variables['docs_dir'][0]
    
    def _parse_gazpacho_path(self):
        """
        Parses the environment variable GAZPACHO_PATH and
        process all the directories in there
        """
        for path in os.environ.get('GAZPACHO_PATH', '').split(':'):
            if not path:
                continue
            
            for filename in os.listdir(path):
                if not filename.endswith('.xml'):
                    continue
                name = filename[:-4]
                catalog = os.path.join(path, filename)
                self._add_external(name, 'pixmaps_dir',
                                   os.path.join(path, 'resources'))
                self._add_external(name, 'catalogs_dir', path)
                self._add_external(name, 'resources_dir',
                                   os.path.join(path, 'resources'))
                self._add_external(name, 'glade_dir', path)

                # library uses gazpacho.widgets as a base for plugins,
                # attach ourselves to it's __path__ so imports will work
                gazpacho.widgets.__path__.append(path)
        
    def get_catalogs(self):
        catalog_error = "" 

        self._parse_gazpacho_path()
        
        retval = {}
        dirs = []
        for dirname in self._variables['catalogs_dir']:
            try:
                dir_list = os.listdir(dirname)
            except OSError, e:
                if e.errno == 2:
                    catalog_error = ("The catalog directory %s does not exist" %
                                     dirname)
                elif e.errno == 13:
                    catalog_error = ("You do not have read permissions in the "
                                     "catalog directory %s" % dirname)
                else:
                    catalog_error = e.strerror
                    
            if catalog_error:
                raise EnvironmentError(catalog_error)

            for filename in dir_list:
                if not filename.endswith('.xml'):
                    continue
                retval[filename[:-4]] = os.path.join(dirname, filename)

        return retval
        
environ = Environment()
