# Copyright (C) 2005 by Nokia corporation
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

import gobject
import gtk

class TreeModelRegistry(gobject.GObject):
  def __init__(self):
    gobject.GObject.__init__(self)

    self.models = {}
    self.names = {}

  def add(self, model, name):
    self.names[model] = name
    self.models[name] = model
    self.emit("model-added", name, model)
  
  def remove_by_name(self, name):
    model = self.models[name]
    del self.models[name]
    del self.names[model]
    self.emit("model-deleted", name)
  
  def remove(self, model):
    name = self.names[model]
    del self.models[name]
    del self.names[model]
    self.emit("model-deleted", name)
  
  def replace(self, old_model, new_model):
    name = self.names[old_model]
    del self.models[name]
    del self.names[old_model]
    self.names[new_model] = name
    self.models[name] = new_model
    self.emit("model-updated", name, new_model)
  
  def lookup_name(self, model):
    try:
      return self.names[model]
    except KeyError:
      return None
    
  def lookup_model(self, name):
    try:
      return self.models[name]
    except KeyError:
      return None
  
  def list_names(self):
    return self.names.values()
  
  def list_models(self):
    return self.models.values()

gobject.type_register(TreeModelRegistry)
gobject.signal_new("model-updated", TreeModelRegistry, gobject.SIGNAL_RUN_FIRST, None, [gobject.TYPE_STRING, gtk.TreeModel])
gobject.signal_new("model-added", TreeModelRegistry, gobject.SIGNAL_RUN_FIRST, None, [gobject.TYPE_STRING, gtk.TreeModel])
gobject.signal_new("model-deleted", TreeModelRegistry, gobject.SIGNAL_RUN_FIRST, None, [gobject.TYPE_STRING])

