# file: inquiry.py
# auth: Albert Huang <albert@csail.mit.edu>
# desc: performs a simple device inquiry followed by a remote name request of
#       each discovered device
# $Id: inquiry.py,v 1.6 2006/02/24 20:30:15 albert Exp $
#

import sys
import bluetooth

print "performing inquiry..."

nearby_devices = bluetooth.discover_devices()

print "devices found: %s" % len(nearby_devices)
print "lookup up names..."

for addr in nearby_devices:
    name = bluetooth.lookup_name( addr )
    if name is None: name = "[couldn't lookup name]"
    print "%s - %s" % (addr, name)
