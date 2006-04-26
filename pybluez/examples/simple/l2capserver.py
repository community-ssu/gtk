# file: l2capclient.py
# auth: Calvin On <calvinon@csail.mit.edu>
# desc: Demo L2CAP server for pybluez.
# $Id: l2capserver.py,v 1.3 2006/02/24 20:30:15 albert Exp $
# 
# 03/31/2005: albert - modified to work with new API

import bluetooth

server_sock=bluetooth.BluetoothSocket( bluetooth.L2CAP )

port = 0x1001

server_sock.bind(("",port))
server_sock.listen(1)

client_sock,address = server_sock.accept()
print "Accepted connection from ",address

data = client_sock.recv(1024)
print "Data received:", data

while data:
    client_sock.send('Echo =>' + data)
    data = client_sock.recv(1024)
    print "Data received:",data

client_sock.close()
server_sock.close()
