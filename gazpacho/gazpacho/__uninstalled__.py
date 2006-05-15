#
# Copyright (C) 2005 Johan Dahlin
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

# All of these are relative to the root directory of the parent
basedir = os.path.dirname(os.path.dirname(__file__))
pixmaps_dir = os.path.abspath(os.path.join(basedir, "pixmaps"))
languages_dir = os.path.abspath(os.path.join(basedir, "locale"))
catalogs_dir = os.path.abspath(os.path.join(basedir, "catalogs"))
resources_dir = os.path.abspath(os.path.join(basedir, "resources"))
glade_dir = os.path.abspath(os.path.join(basedir, "glade"))
docs_dir = os.path.abspath(basedir)
