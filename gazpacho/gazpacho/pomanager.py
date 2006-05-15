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
from xml.parsers import expat
from gazpacho.properties import prop_registry
import gobject
import os
import time

class Stack(list):
    push = list.append
    def peek(self):
        if self:
            return self[-1]
def odd(x):
    return bool(x & 1)

class PoManager:
   _po_file = None
   _path = None
  
   # Header Info
   _headers = {
        "Project-Id-Version": "PACKAGE VERSION\\n",
        "Report-Msgid-Bugs-To": "\\n",
        "POT-Creation-Date": None,
        "PO-Revision-Date": "2005-06-16 00:16+0300\\n",
        "Last-Translator": "Unknown  <unknown@unknown.un>\\n",
        "Language-Team": "Unknown  <unknown@unknown.un>\\n",
        "MIME-Version": "1.0\\n",
        "Content-Type": "text/plain; charset=utf-8\\n",
        "Content-Transfer-Encoding": "8bit\\n"}

   def __init__(self, path=None):
       self._stack = Stack()
       #self._state_stack = Stack()
       self._parser = expat.ParserCreate()
       self._parser.buffer_text = True
       self._parser.StartElementHandler = self._handle_startelement
       self._parser.EndElementHandler = self._handle_endelement
       self._parser.CharacterDataHandler = self._handle_characterdata
       self._parser.CommentHandler = self._handle_comment
       self._path = path

   # Expat callbacks
   def _handle_startelement(self, name, attrs):
       if name == 'property' and u'engineering_english' in attrs:
           prop = [name, attrs]
           self._stack.push(prop)

   def _handle_endelement(self, name):
       prop = self._stack.peek()
       if prop and prop[0] == name:
           self._stack.pop()
        
   def _handle_characterdata(self, data):
       if not self._po_file:
           return

       prop = self._stack.peek()
       if prop:
           name, attrs = prop
           print >> self._po_file, 'msgid', '\"%s\"' % data 
           print >> self._po_file, 'msgstr', '\"%s\"' % attrs[u'engineering_english']
           print >> self._po_file, '\n'

   def _handle_comment(self, data):
        lines = data.split('\n')

        if lines and lines[0] == ' EXTRA_STRINGS':
            lines.remove(lines[0])
            for line in lines:
                words = line.split('\"')
                if len (words) == 5:
                    print >> self._po_file, 'msgid', '\"%s\"' % words[1]
                    print >> self._po_file, 'msgstr', '\"%s\"' % words[3]
                    print >> self._po_file, '\n'

        elif lines and lines[0] == ' EXTRA_PLURAL_STRINGS':
            lines.remove(lines[0])
            for line in lines:
                words = line.split('\"')
                if len (words) >= 5 and odd(len(words)):
                    print >> self._po_file, 'msgid', '\"%s\"' % words[1]
                    print >> self._po_file, 'msgid_plural', '\"%s\"' % words[3]
                    i = 0
                    for j in range(5, len(words), 2):
                        print >> self._po_file, 'msgstr[%d] \"%s\"' % (i, words[j])
                        i += 1
                    print >> self._po_file, '\n'

   def set_file(self, path):
       self._path = path
   
   def generate_po(self, xml, path=None):
       print xml
       if path:
           self._path = path
       self._po_file = open(self._path, 'w')

       if not self._headers["POT-Creation-Date"]:
           self._headers["POT-Creation-Date"] = '%s\\n' % time.strftime("%a, %d %b %Y %H:%M:%S %z")
       self._headers["PO-Revision-Date"] = '%s\\n' % time.strftime("%a, %d %b %Y %H:%M:%S %z")

       for key in self._headers:
           print >> self._po_file, '\"%s: %s\"' % (key, self._headers[key])
       print >> self._po_file, '\n\n'

       self._parser.Parse(xml)
       self._po_file.close()
   
   def import_po(self, project, path=None):
       if path:
           self._path = path
       self._po_file = open(self._path, 'r')

       for widget in project.widgets:
           properties = prop_registry.list(gobject.type_name(widget),
                                           widget.get_parent())
           prop_id = None 
           self._po_file.seek(0)

           for line in self._po_file.readlines():
               words = line[:-1].split('\"')

               if len(words) >= 2:
                   if prop_id:
                       if words[0].strip() == 'msgstr':
                           widget.set_property(prop_id, words[1])
                           prop_id = None

                   else:
                       for prop in properties:
                           prop_id = prop.name
                           prop_eng_id = 'engineering_english_%s' % prop_id 
                           prop_eng_value = widget.get_data(prop_eng_id)
                   
                           if not (prop.readable and prop.translatable and
                                words[0].strip() == 'msgid' and
                                words[1] == prop_eng_value):
                                prop_id = None
                           else:
                                break
       msg_id = None
       self._po_file.seek(0)
       num_plurals = 0

       for line in self._po_file.readlines():
           words = line[:-1].split('\"')

           if len(words) >= 2:
               if msg_id:
                   if words[0].strip() == 'msgstr':
                       project.extra_strings[msg_id] = words[1]

                   elif words[0].strip() == 'msgid_plural':
                       project.extra_plural_strings[msg_id] = [words[1]]

                   elif words[0].strip() == 'msgstr[%d]' % num_plurals and \
                        msg_id in project.extra_plural_strings:
                       project.extra_plural_strings[msg_id].append(words[1])
                       num_plurals += 1
               
                   else:
                       msg_id = None
                       num_plurals = 0

               elif words[0].strip() == 'msgid':
                   msg_id = words[1]

           else:
               msg_id = None
               num_plurals = 0

           words = line[:-1].split(':', 1)

           if len(words) >= 2:
               if words[0][1:] in self._headers:
                  self._headers[words[0][1:]] = words[1][1:-1]

       self._po_file.close()

   def set_headers(self, headers):
       headers["POT-Creation-Date"] = self._headers["POT-Creation-Date"]

       self._headers = headers 

   def get_headers(self):
       return self._headers

#po_generator = PoManager('/home/zali/po/mpo')
#fp = open("/home/zali/anything.glade")
#po_generator.generate_po(fp.read())

