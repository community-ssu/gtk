from xml.sax import make_parser, handler
import cStringIO

class IntrospectionHandler(handler.ContentHandler):
    def __init__(self, method_map):
        self._map = method_map
        self._name = []

    def startElement(self, name, attrs):
        if name in ['interface', 'method']:
            self._name.append(str(attrs['name']))
        elif name == 'arg':
            direction = attrs.get('direction', 'in')
            if direction == 'in':
                try:
                    self._map['.'.join(self._name)] += str(attrs['type'])
                except KeyError, e:
                    self._map['.'.join(self._name)] = str(attrs['type'])

    def endElement(self, name):
        if name == 'method':
            if not self._map.has_key('.'.join(self._name)):
                self._map['.'.join(self._name)] = '' # no args
        if name in ['interface', 'method']:
            self._name.pop()
 
def process_introspection_data(data):
    method_map = {}
    parser = make_parser()
    parser.setContentHandler(IntrospectionHandler(method_map))
    stream = cStringIO.StringIO(data.encode('utf-8'))
    parser.parse(stream)
    return method_map
