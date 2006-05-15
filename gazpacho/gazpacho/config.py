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

import os
from ConfigParser import ConfigParser

MAX_RECENT = 5

"""[General]
lastdirectory=..

[RecentProjects]
project0=..
project1=..
"""

class ConfigError(Exception):
    pass

class BaseConfig:
    def __init__(self, filename):
        self._filename = filename
        self._config = ConfigParser()
        self.open(self._filename)

    def open(self, filename):
        if os.path.exists(filename):
            self._config.read(filename)
        self._filename = filename

    def has_option(self, name, section='General'):
        return self._config.has_option(section, name)

    def get_option(self, name, section='General'):
        if not section in self.sections:
            raise ConfigError('Invalid section: %s' % section)

        if self._config.has_option(section, name):
            return self._config.get(section, name)

        raise ConfigError('%s does not have option: %s' %
                          (self._filename, name))

    def set_option(self, name, value, section='General'):
        if not section in self.sections:
            raise ConfigError('Invalid section: %s' % section)

        if not self._config.has_section(section):
            self._config.add_section(section)
            
        self._config.set(section, name, value)
        
    def save(self):
        filename = self._filename
        fp = open(filename, 'w')
        self._config.write(fp)

class GazpachoConfig(BaseConfig):
    sections = ['General', 'RecentProjects']
    def __init__(self):
        self.recent_projects = []
        self.lastdirectory = None
        BaseConfig.__init__(self, self.get_filename())

    def get_filename(self):
        home = os.path.join(os.path.expanduser('~'))
        projectdir = os.path.join(home, '.gazpacho')
        if not os.path.exists(projectdir):
            os.mkdir(projectdir)
        return os.path.join(home, '.gazpacho', 'config')
        
    def open(self, filename):
        BaseConfig.open(self, filename)

        if self.has_option('lastdirectory'):
            self.lastdirectory = self.get_option('lastdirectory')

        i = 0
        while True:
            name = 'project%d' % i
            if not self.has_option(name, 'RecentProjects'):
                break

            self.recent_projects.append(self.get_option(name, 'RecentProjects'))
            i += 1

    def save(self):
        self.set_option('lastdirectory', self.lastdirectory)
        for i, project in enumerate(self.recent_projects):
            self.set_option('project%d' % i, project, 'RecentProjects')
            
        BaseConfig.save(self)

    def set_lastdirectory(self, filename):
        if not os.path.isdir(filename):
            filename = os.path.dirname(filename)
        self.lastdirectory = filename
        
    def add_recent_project(self, path):
        if not path:
            return
        
        if not path in self.recent_projects:
            self.recent_projects.insert(0, path)

        if len(self.recent_projects) > MAX_RECENT:
            self.recent_projects = self.recent_projects[:MAX_RECENT]
