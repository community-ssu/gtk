# file: rfcomm-server.py
# auth: Albert Huang <albert@csail.mit.edu>
# desc: simple demonstration of a server application that uses RFCOMM sockets
#
# $Id: rfcomm-server.py,v 1.4 2006/02/24 19:42:34 albert Exp $

from bluetooth import *

port = get_available_port( RFCOMM ) 

server_sock=BluetoothSocket( RFCOMM )
server_sock.bind(("",port))
server_sock.listen(1)

uuid = "94f39d29-7d6d-437d-973b-fba39e49d4ee"

advertise_service( server_sock, "FooBar service",
                   service_id = uuid,
                   service_classes = [ SERIAL_PORT_CLASS ],
                   profiles = [ SERIAL_PORT_PROFILE ] )
                   
print "Waiting for connection on RFCOMM channel %d" % port

client_sock, client_info = server_sock.accept()
print "Accepted connection from ", client_info

try:
    while True:
        data = client_sock.recv(1024)
        print "received [%s]" % data
except BluetoothError:
    pass

print "disconnected"

client_sock.close()
server_sock.close()
