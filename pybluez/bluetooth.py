"""
bluetooth module
"""

# file: bluetooth.py
# auth: Albert Huang <albert@csail.mit.edu>
# desc: wrappers around the _bluetooth native python extension
# $Id: bluetooth.py,v 1.15 2006/02/24 20:30:15 albert Exp $

import _bluetooth
import _bluetooth as bt
import struct
import array
import fcntl

btsock = bt.btsocket

_constants = [ 'HCI', 'RFCOMM', 'L2CAP', 'SCO', 'SOL_L2CAP', 'SOL_RFCOMM', \
    'L2CAP_OPTIONS' ]
for _c in _constants: exec "%s = bt.%s" % (_c, _c)
del _constants

# Copied from Bluez's ${includedir}/bluetooth/sdp.h

# Service class identifiers of standard services and service groups
SDP_SERVER_CLASS = "1000"
BROWSE_GRP_DESC_CLASS = "1001"
PUBLIC_BROWSE_GROUP = "1002"
SERIAL_PORT_CLASS = "1101"
LAN_ACCESS_CLASS = "1102"
DIALUP_NET_CLASS = "1103"
IRMC_SYNC_CLASS = "1104"
OBEX_OBJPUSH_CLASS = "1105"
OBEX_FILETRANS_CLASS = "1106"
IRMC_SYNC_CMD_CLASS = "1107"
HEADSET_CLASS = "1108"
CORDLESS_TELEPHONY_CLASS = "1109"
AUDIO_SOURCE_CLASS = "110a"
AUDIO_SINK_CLASS = "110b"
AV_REMOTE_TARGET_CLASS = "110c"
ADVANCED_AUDIO_CLASS = "110d"
AV_REMOTE_CLASS = "110e"
VIDEO_CONF_CLASS = "110f"
INTERCOM_CLASS = "1110"
FAX_CLASS = "1111"
HEADSET_AGW_CLASS = "1112"
WAP_CLASS = "1113"
WAP_CLIENT_CLASS = "1114"
PANU_CLASS = "1115"
NAP_CLASS = "1116"
GN_CLASS = "1117"
DIRECT_PRINTING_CLASS = "1118"
REFERENCE_PRINTING_CLASS = "1119"
IMAGING_CLASS = "111a"
IMAGING_RESPONDER_CLASS = "111b"
IMAGING_ARCHIVE_CLASS = "111c"
IMAGING_REFOBJS_CLASS = "111d"
HANDSFREE_CLASS = "111e"
HANDSFREE_AGW_CLASS = "111f"
DIRECT_PRT_REFOBJS_CLASS = "1120"
REFLECTED_UI_CLASS = "1121"
BASIC_PRINTING_CLASS = "1122"
PRINTING_STATUS_CLASS = "1123"
HID_CLASS = "1124"
HCR_CLASS = "1125"
HCR_PRINT_CLASS = "1126"
HCR_SCAN_CLASS = "1127"
CIP_CLASS = "1128"
VIDEO_CONF_GW_CLASS = "1129"
UDI_MT_CLASS = "112a"
UDI_TA_CLASS = "112b"
AV_CLASS = "112c"
SAP_CLASS = "112d"
PNP_INFO_CLASS = "1200"
GENERIC_NETWORKING_CLASS = "1201"
GENERIC_FILETRANS_CLASS = "1202"
GENERIC_AUDIO_CLASS = "1203"
GENERIC_TELEPHONY_CLASS = "1204"
UPNP_CLASS = "1205"
UPNP_IP_CLASS = "1206"
UPNP_PAN_CLASS = "1300"
UPNP_LAP_CLASS = "1301"
UPNP_L2CAP_CLASS = "1302"
VIDEO_SOURCE_CLASS = "1303"
VIDEO_SINK_CLASS = "1304"

# standard profile descriptor identifiers

SDP_SERVER_PROFILE = ( SDP_SERVER_CLASS, 0x0100 )
BROWSE_GRP_DESC_PROFILE = ( BROWSE_GRP_DESC_CLASS, 0x0100 )
SERIAL_PORT_PROFILE = ( SERIAL_PORT_CLASS, 0x0100 )
LAN_ACCESS_PROFILE = ( LAN_ACCESS_CLASS, 0x0100 )
DIALUP_NET_PROFILE = ( DIALUP_NET_CLASS, 0x0100 )
IRMC_SYNC_PROFILE = ( IRMC_SYNC_CLASS, 0x0100 )
OBEX_OBJPUSH_PROFILE = ( OBEX_OBJPUSH_CLASS, 0x0100 )
OBEX_FILETRANS_PROFILE = ( OBEX_FILETRANS_CLASS, 0x0100 )
IRMC_SYNC_CMD_PROFILE = ( IRMC_SYNC_CMD_CLASS, 0x0100 )
HEADSET_PROFILE = ( HEADSET_CLASS, 0x0100 )
CORDLESS_TELEPHONY_PROFILE = ( CORDLESS_TELEPHONY_CLASS, 0x0100 )
AUDIO_SOURCE_PROFILE = ( AUDIO_SOURCE_CLASS, 0x0100 )
AUDIO_SINK_PROFILE = ( AUDIO_SINK_CLASS, 0x0100 )
AV_REMOTE_TARGET_PROFILE = ( AV_REMOTE_TARGET_CLASS, 0x0100 )
ADVANCED_AUDIO_PROFILE = ( ADVANCED_AUDIO_CLASS, 0x0100 )
AV_REMOTE_PROFILE = ( AV_REMOTE_CLASS, 0x0100 )
VIDEO_CONF_PROFILE = ( VIDEO_CONF_CLASS, 0x0100 )
INTERCOM_PROFILE = ( INTERCOM_CLASS, 0x0100 )
FAX_PROFILE = ( FAX_CLASS, 0x0100 )
HEADSET_AGW_PROFILE = ( HEADSET_AGW_CLASS, 0x0100 )
WAP_PROFILE = ( WAP_CLASS, 0x0100 )
WAP_CLIENT_PROFILE = ( WAP_CLIENT_CLASS, 0x0100 )
PANU_PROFILE = ( PANU_CLASS, 0x0100 )
NAP_PROFILE = ( NAP_CLASS, 0x0100 )
GN_PROFILE = ( GN_CLASS, 0x0100 )
DIRECT_PRINTING_PROFILE = ( DIRECT_PRINTING_CLASS, 0x0100 )
REFERENCE_PRINTING_PROFILE = ( REFERENCE_PRINTING_CLASS, 0x0100 )
IMAGING_PROFILE = ( IMAGING_CLASS, 0x0100 )
IMAGING_RESPONDER_PROFILE = ( IMAGING_RESPONDER_CLASS, 0x0100 )
IMAGING_ARCHIVE_PROFILE = ( IMAGING_ARCHIVE_CLASS, 0x0100 )
IMAGING_REFOBJS_PROFILE = ( IMAGING_REFOBJS_CLASS, 0x0100 )
HANDSFREE_PROFILE = ( HANDSFREE_CLASS, 0x0100 )
HANDSFREE_AGW_PROFILE = ( HANDSFREE_AGW_CLASS, 0x0100 )
DIRECT_PRT_REFOBJS_PROFILE = ( DIRECT_PRT_REFOBJS_CLASS, 0x0100 )
REFLECTED_UI_PROFILE = ( REFLECTED_UI_CLASS, 0x0100 )
BASIC_PRINTING_PROFILE = ( BASIC_PRINTING_CLASS, 0x0100 )
PRINTING_STATUS_PROFILE = ( PRINTING_STATUS_CLASS, 0x0100 )
HID_PROFILE = ( HID_CLASS, 0x0100 )
HCR_PROFILE = ( HCR_SCAN_CLASS, 0x0100 )
HCR_PRINT_PROFILE = ( HCR_PRINT_CLASS, 0x0100 )
HCR_SCAN_PROFILE = ( HCR_SCAN_CLASS, 0x0100 )
CIP_PROFILE = ( CIP_CLASS, 0x0100 )
VIDEO_CONF_GW_PROFILE = ( VIDEO_CONF_GW_CLASS, 0x0100 )
UDI_MT_PROFILE = ( UDI_MT_CLASS, 0x0100 )
UDI_TA_PROFILE = ( UDI_TA_CLASS, 0x0100 )
AV_PROFILE = ( AV_CLASS, 0x0100 )
SAP_PROFILE = ( SAP_CLASS, 0x0100 )
PNP_INFO_PROFILE = ( PNP_INFO_CLASS, 0x0100 )
GENERIC_NETWORKING_PROFILE = ( GENERIC_NETWORKING_CLASS, 0x0100 )
GENERIC_FILETRANS_PROFILE = ( GENERIC_FILETRANS_CLASS, 0x0100 )
GENERIC_AUDIO_PROFILE = ( GENERIC_AUDIO_CLASS, 0x0100 )
GENERIC_TELEPHONY_PROFILE = ( GENERIC_TELEPHONY_CLASS, 0x0100 )
UPNP_PROFILE = ( UPNP_CLASS, 0x0100 )
UPNP_IP_PROFILE = ( UPNP_IP_CLASS, 0x0100 )
UPNP_PAN_PROFILE = ( UPNP_PAN_CLASS, 0x0100 )
UPNP_LAP_PROFILE = ( UPNP_LAP_CLASS, 0x0100 )
UPNP_L2CAP_PROFILE = ( UPNP_L2CAP_CLASS, 0x0100 )
VIDEO_SOURCE_PROFILE = ( VIDEO_SOURCE_CLASS, 0x0100 )
VIDEO_SINK_PROFILE = ( VIDEO_SINK_CLASS, 0x0100 )

class BluetoothError(IOError):
    pass

def discover_devices(duration=8, flush_cache=True):
    """
    performs a bluetooth device discovery using the first available bluetooth
    resource.

    returns a list of bluetooth addresses.

    duration=8 
        how long, in units of 1.28 seconds, to search for devices.  To find as
        many devices as possible, you should set this to at least 8.
    flush_cache=True 
        if set to False, then discover_devices may return devices found during
        previous discoveries.
    """

    sock = _gethcisock()
    try:
        results = bt.hci_inquiry(sock, duration=8, flush_cache=True)
    except bt.error:
        sock.close()
        raise BluetoothError("error communicating with local bluetooth device")

    sock.close()
    return results

def lookup_name(address, timeout=10):
    """
    Tries to determine the friendly name (human readable) of the device with
    the specified bluetooth address.  Returns the name on success, and None
    on failure.

    timeout=10   how many seconds to search before giving up.
    """
    _checkaddr(address)

    sock = _gethcisock()
    timeoutms = int(timeout * 1000)
    try: 
        name = bt.hci_read_remote_name( sock, address, timeoutms )
    except bt.error, e:
        # name lookup failed.  either a timeout, or I/O error
        name = None
    sock.close()
    return name

def is_valid_address(address):
    """
    returns True if address is a valid bluetooth address

    valid address are always strings of the form XX:XX:XX:XX:XX:XX
    where X is a hexadecimal character.  For example,
        01:23:45:67:89:AB is a valid address, but
        IN:VA:LI:DA:DD:RE is not
    """
    try:
        pairs = address.split(":")
        if len(pairs) != 6: return False
        for b in pairs: int(b, 16)
    except:
        return False
    return True

def is_valid_uuid(uuid):
    """
    is_valid_uuid(uuid) -> bool

    returns True if uuid is a valid UUID.

    valid UUIDs are always strings taking one of the following forms:
        XXXX
        XXXXXXXX
        XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX
    where each X is a hexadecimal digit (case insensitive)
    """
    try:
        if len(uuid) == 4:
            if int( uuid, 16 ) < 0: return False
        elif len(uuid) == 8:
            if int( uuid, 16 ) < 0: return False
        elif len(uuid) == 36:
            pieces = uuid.split("-")
            if len(pieces) != 5 or \
                    len(pieces[0]) != 8 or \
                    len(pieces[1]) != 4 or \
                    len(pieces[2]) != 4 or \
                    len(pieces[3]) != 4 or \
                    len(pieces[4]) != 12:
                return False
            [ int( p, 16 ) for p in pieces ]
        else:
            return False
    except ValueError: 
        return False
    except TypeError:
        return False
    return True

def get_available_port( protocol ):
    """
    finds and returns an available port for the specified protocol.  protocol 
    must be either L2CAP or RFCOMM, and None if no available port is found.

    It is possible that somebody will bind to the returned port in between the
    time it is returned and the time you use it, so be careful.

    This is a major hack.  In Linux kernel versions 2.6.7 and higher, you can
    use PSM/channel 0 when binding a socket to be dynamically assigned a port
    number (e.g.  sock.bind(("",0)) )
    """
    if protocol == RFCOMM:
        for channel in range(1,31):
            s = BluetoothSocket( RFCOMM )
            try:
                s.bind( ("", channel) )
                s.close()
                return channel
            except:
                s.close()
    elif protocol == L2CAP:
        for psm in range(0x1001,0x8000,2):
            s = BluetoothSocket( L2CAP )
            try:
                s.bind( ("", psm) )
                s.close()
                return psm
            except:
                s.close()
    else:
        raise ValueError("protocol must either RFCOMM or L2CAP")

_socketmethods = ( 'bind', 'connect', 'connect_ex', 'close',
        'dup', 'fileno', 'getpeername', 'getsockname', 'gettimeout',
        'getsockopt', 'listen', 'makefile', 'recv', 'recvfrom', 'sendall',
        'send', 'sendto', 'setblocking', 'setsockopt', 'settimeout', 'shutdown'
        )

def set_packet_timeout( address, timeout ):
    """
    Adjusts the ACL flush timeout for the ACL connection to the specified
    device.  This means that all L2CAP and RFCOMM data being sent to that
    device will be dropped if not acknowledged in timeout milliseconds (maximum
    1280).  A timeout of 0 means to never drop packets.

    Since this affects all Bluetooth connections to that device, and not just
    those initiated by this process or PyBluez, a call to this method requires
    superuser privileges.

    You must have an active connection to the specified device before invoking
    this method
    """
    n = round(timeout / 0.625)
    _write_flush_timeout( address, n )

def set_l2cap_mtu( sock, mtu ):
    """set_l2cap_mtu( sock, mtu )

    Adjusts the MTU for the specified L2CAP socket.  This method needs to be
    invoked on both sides of the connection for it to work!  The default mtu
    that all L2CAP connections start with is 672 bytes.

    mtu must be between 48 and 65535, inclusive.
    """
    s = sock.getsockopt( SOL_L2CAP, L2CAP_OPTIONS, 7 )
    o,i,f,m = struct.unpack("HHHB", s)
    s = struct.pack("HHHB", mtu, mtu, f, m)
    sock.setsockopt( SOL_L2CAP, L2CAP_OPTIONS, s)

class BluetoothSocket:
    __doc__ = _bluetooth.btsocket.__doc__

    def __init__(self, proto = RFCOMM, _sock=None):
        if _sock is None:
            _sock = bt.btsocket(proto)
        self._sock = _sock
        self._proto = proto

    def dup(self):
        """dup() -> socket object

        Return a new socket object connected to the same system resource."""
        return BluetoothSocket(proto=self._proto, _sock=self._sock)

    def accept(self):
        try:
            client, addr = self._sock.accept()
        except _bluetooth.error, e:
            raise BluetoothError(str(e))
        newsock = BluetoothSocket(self._proto, client)
        return (newsock, addr)
    accept.__doc__ = _bluetooth.btsocket.accept.__doc__

    # import methods from the wraapped socket object
    _s = ("""def %s(self, *args, **kwargs): 
    try: 
        return self._sock.%s(*args, **kwargs)
    except _bluetooth.error, e:
        raise BluetoothError(str(e))
%s.__doc__ = _bluetooth.btsocket.%s.__doc__\n""")
    for _m in _socketmethods:
        exec _s % (_m, _m, _m, _m)
    del _m, _s
    
class DeviceDiscoverer:
    """
    Skeleton class for finer control of the device discovery process.

    To implement asynchronous device discovery (e.g. if you want to do 
    something *as soon as* a device is discovered), subclass DeviceDiscoverer
    and override device_discovered() and inquiry_complete()
    """
    def __init__(self):
        self.sock = None
        self.is_inquiring = False
        self.lookup_names = False

        self.names_to_find = {}
        self.names_found = {}

    def find_devices(self, lookup_names=True, 
            duration=8, 
            flush_cache=True):
        """
        start_inquiry( lookup_names=True, service_name=None, 
                       duration=8, flush_cache=True )

        Call this method to initiate the device discovery process

        lookup_names - set to True if you want to lookup the user-friendly 
                       names for each device found.

        service_name - set to the name of a service you're looking for.
                       only devices with a service of this name will be 
                       returned in device_discovered() NOT YET IMPLEMENTED


        ADVANCED PARAMETERS:  (don't change these unless you know what 
                            you're doing)

        duration - the number of 1.2 second units to spend searching for
                   bluetooth devices.  If lookup_names is True, then the 
                   inquiry process can take a lot longer.

        flush_cache - return devices discovered in previous inquiries
        """
        if self.is_inquiring:
            raise BluetoothError("Already inquiring!")

        self.lookup_names = lookup_names

        self.sock = _gethcisock()
        flt = bt.hci_filter_new()
        bt.hci_filter_all_events(flt)
        bt.hci_filter_set_ptype(flt, bt.HCI_EVENT_PKT)

        try:
            self.sock.setsockopt( bt.SOL_HCI, bt.HCI_FILTER, flt )
        except:
            raise BluetoothError("problem with local bluetooth device.")

        # send the inquiry command
        duration = 4
        max_responses = 255
        cmd_pkt = struct.pack("BBBBB", 0x33, 0x8b, 0x9e, \
                duration, max_responses)

        self.pre_inquiry()
        
        try:
            bt.hci_send_cmd( self.sock, bt.OGF_LINK_CTL, \
                    bt.OCF_INQUIRY, cmd_pkt)
        except:
            raise BluetoothError("problem with local bluetooth device.")

        self.is_inquiring = True

        self.names_to_find = {}
        self.names_found = {}

    def cancel_inquiry(self):
        """
        Call this method to cancel an inquiry in process.  inquiry_complete 
        will still be called.
        """
        self.names_to_find = {}

        if self.is_inquiring:
            try:
                bt.hci_send_cmd( self.sock, bt.OGF_LINK_CTL, \
                        bt.OCF_INQUIRY_CANCEL )
                self.sock.close()
                self.sock = None
            except:
                raise BluetoothError("error canceling inquiry")
            self.is_inquiring = False

    def process_inquiry(self):
        """
        Repeatedly calls process_event() until the device inquiry has 
        completed.
        """
        while self.is_inquiring or len(self.names_to_find) > 0:
            self.process_event()

    def process_event(self):
        """
        Waits for one event to happen, and proceses it.  The event will be
        either a device discovery, or an inquiry completion.
        """
        self._process_hci_event()

    def _process_hci_event(self):
        if self.sock is None: return
        # voodoo magic!!!
        pkt = self.sock.recv(255)
        ptype, event, plen = struct.unpack("BBB", pkt[:3])
        pkt = pkt[3:]
        if event == bt.EVT_INQUIRY_RESULT:
            nrsp = struct.unpack("B", pkt[0])[0]
            for i in range(nrsp):
                addr = bt.ba2str( pkt[1+6*i:1+6*i+6] )
                psrm = pkt[ 1+6*nrsp+i ]
                pspm = pkt[ 1+7*nrsp+i ]
                devclass_raw = pkt[1+9*nrsp+3*i:1+9*nrsp+3*i+3]
                devclass = struct.unpack("!I", "\0%s" % devclass_raw)[0]
                clockoff = pkt[1+12*nrsp+2*i:1+12*nrsp+2*i+2]

                self._device_discovered( addr, devclass, psrm, pspm, clockoff )
        elif event == bt.EVT_INQUIRY_RESULT_WITH_RSSI:
            nrsp = struct.unpack("B", pkt[0])[0]
            for i in range(nrsp):
                addr = bt.ba2str( pkt[1+6*i:1+6*i+6] )
                psrm = pkt[ 1+6*nrsp+i ]
                pspm = pkt[ 1+7*nrsp+i ]
                devclass_raw = pkt[1+8*nrsp+3*i:1+8*nrsp+3*i+3]
                devclass = struct.unpack("!I", "\0%s" % devclass_raw)[0]
                clockoff = pkt[1+11*nrsp+2*i:1+11*nrsp+2*i+2]
                rssi = struct.unpack("b", pkt[1+13*nrsp+i])[0]

                self._device_discovered( addr, devclass, psrm, pspm, clockoff )
        elif event == bt.EVT_INQUIRY_COMPLETE:
            self.is_inquiring = False
            if len(self.names_to_find) == 0:
#                print "inquiry complete (evt_inquiry_complete)"
                self.sock.close()
                self.inquiry_complete()
            else:
                self._send_next_name_req()

        elif event == bt.EVT_CMD_STATUS:
            status, ncmd, opcode = struct.unpack("BBH", pkt[:4])
            if status != 0:
                self.is_inquiring = False
                self.sock.close()
                
#                print "inquiry complete (bad status 0x%X 0x%X 0x%X)" % \
#                        (status, ncmd, opcode)
                self.names_to_find = {}
                self.inquiry_complete()
        elif event == bt.EVT_REMOTE_NAME_REQ_COMPLETE:
            status = struct.unpack("B", pkt[0])[0]
            addr = bt.ba2str( pkt[1:7] )
            if status == 0:
                try:
                    name = pkt[7:].split('\0')[0]
                except IndexError:
                    name = '' 
                if addr in self.names_to_find:
                    device_class = self.names_to_find[addr][0]
                    self.device_discovered( addr, device_class, name )
                    del self.names_to_find[addr]
                    self.names_found[addr] = ( device_class, name )
                else:
                    pass
            else:
                if addr in self.names_to_find: del self.names_to_find[addr]
                # XXX should we do something when a name lookup fails?
#                print "name req unsuccessful %s - %s" % (addr, status)

            if len(self.names_to_find) == 0:
                self.is_inquiring = False
                self.sock.close()
                self.inquiry_complete()
#                print "inquiry complete (name req complete)"
            else:
                self._send_next_name_req()
        else:
            pass
#            print "unrecognized packet type 0x%02x" % ptype

    def _device_discovered(self, address, device_class, psrm, pspm, clockoff):
        if self.lookup_names:
            if address not in self.names_found and \
                address not in self.names_to_find:
            
                self.names_to_find[address] = \
                    (device_class, psrm, pspm, clockoff)
        else:
            self.device_discovered(address, device_class, None)

    def _send_next_name_req(self):
        assert len(self.names_to_find) > 0
        address = self.names_to_find.keys()[0]
        device_class, psrm, pspm, clockoff = self.names_to_find[address]
        bdaddr = bt.str2ba( address )
        
        cmd_pkt = "%s%s\0%s" % (bdaddr, psrm, clockoff)

        try:
            bt.hci_send_cmd( self.sock, bt.OGF_LINK_CTL, \
                    bt.OCF_REMOTE_NAME_REQ, cmd_pkt)
        except Exception, e:
            raise BluetoothError("error request name of %s - %s" % 
                    (address, str(e)))

    def fileno(self):
        if not self.sock: return None
        return self.sock.fileno()

    def pre_inquiry(self):
        """
        Called just after start_inquiry is invoked, but just before the inquiry
        is started.

        This method exists to be overriden
        """

    def device_discovered(self, address, device_class, name):
        """
        Called when a bluetooth device is discovered.

        address is the bluetooth address of the device

        device_class is the Class of Device, as specified in [1]
                     passed in as a 3-byte string

        name is the user-friendly name of the device if lookup_names was set
             when the inquiry was started.  otherwise None
        
        This method exists to be overriden.

        [1] https://www.bluetooth.org/foundry/assignnumb/document/baseband
        """
        if name:
            print "found: %s - %s (class 0x%X)" % (address, name, device_class)
        else:
            print "found: %s (class 0x%X)" % (address, device_class)

    def inquiry_complete(self):
        """
        Called when an inquiry started by start_inquiry has completed.
        """
        print "inquiry complete"

# ============== SDP service registration and unregistration ============
def advertise_service( sock, name, service_id = "", service_classes = [], \
        profiles = [], provider = "", description = "" ):
    """
    Advertises a service with the local SDP server.  sock must be a bound,
    listening socket.  name should be the name of the service, and service_id 
    (if specified) should be a string of the form 
    "XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXX", where each 'X' is a hexadecimal digit.

    service_classes is a list of service classes whose this service belongs to.
    Each class service is a 16-bit UUID in the form "XXXX", where each 'X' is a
    hexadecimal digit, or a 128-bit UUID in the form
    "XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX". There are some constants for
    standard services, e.g. SERIAL_PORT_CLASS that equals to "1101". Some class
    constants:

    SERIAL_PORT_CLASS        LAN_ACCESS_CLASS         DIALUP_NET_CLASS 
    HEADSET_CLASS            CORDLESS_TELEPHONY_CLASS AUDIO_SOURCE_CLASS
    AUDIO_SINK_CLASS         PANU_CLASS               NAP_CLASS
    GN_CLASS

    profiles is a list of service profiles that thie service fulfills. Each
    profile is a tuple with ( uuid, version ). Most standard profiles use
    standard classes as UUIDs. PyBluez offers a list of standard profiles,
    for example SERIAL_PORT_PROFILE. All standard profiles have the same
    name as the classes, except that _CLASS suffix is replaced by _PROFILE.

    provider is a text string specifying the provider of the service

    description is a text string describing the service

    A note on working with Symbian smartphones:
        bt_discover in Python for Series 60 will only detect service records
        with service class SERIAL_PORT_CLASS and profile SERIAL_PORT_PROFILE

    """
    if service_id != "" and not is_valid_uuid( service_id ):
        raise ValueError("invalid UUID specified for service_id")
    for uuid in service_classes:
        if not is_valid_uuid( uuid ):
            raise ValueError("invalid UUID specified in service_classes")
    for uuid, version in profiles:
        if not is_valid_uuid( uuid ):
            raise ValueError("invalid UUID specified in profiles")
    try:
        _bluetooth.sdp_advertise_service( sock._sock, name, service_id, \
                service_classes, profiles, provider, description )
    except _bluetooth.error, e:
        raise BluetoothError(str(e))

def stop_advertising( sock ):
    """
    Instructs the local SDP server to stop advertising the service associated
    with sock.  You should typically call this right before you close sock.
    """
    try:
        _bluetooth.sdp_stop_advertising( sock._sock )
    except _bluetooth.error, e:
        raise BluetoothError(str(e))

def find_service( name = None, uuid = None, address = None ):
    """
    find_service( name = None, uuid = None, address = None )

    Searches for SDP services that match the specified criteria and returns
    the search results.  If no criteria are specified, then returns a list of
    all nearby services detected.  If more than one is specified, then
    the search results will match all the criteria specified.  If uuid is
    specified, it must be either a 16-bit UUID in the form "XXXX", where each
    'X' is a hexadecimal digit, or as a 128-bit UUID in the form
    "XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX".  A special case of address is
    "localhost", which will search for services on the local machine.

    The search results will be a list of dictionaries.  Each dictionary
    represents a search match and will have the following key/value pairs:

      host          - the bluetooth address of the device advertising the
                      service
      name          - the name of the service being advertised
      description   - a description of the service being advertised
      provider      - the name of the person/organization providing the service
      protocol      - either 'RFCOMM', 'L2CAP', None if the protocol was not
                      specified, or 'UNKNOWN' if the protocol was specified but
                      unrecognized
      port          - the L2CAP PSM # if the protocol is 'L2CAP', the RFCOMM
                      channel # if the protocol is 'RFCOMM', or None if it
                      wasn't specified
      service-classes - a list of service class IDs (UUID strings).  possibly
                        empty
      profiles        - a list of profiles - (UUID, version) pairs - the
                        service claims to support.  possibly empty.
      service-id      - the Service ID of the service.  None if it wasn't set
                        See the Bluetooth spec for the difference between
                        Service ID and Service Class ID List
    """
    if not address:
        devices = discover_devices()
    else:
        devices = [ address ]

    results = []

    if uuid is not None and not is_valid_uuid(uuid):
        raise ValueError("invalid UUID")

    try:
        for addr in devices:
            try:
                s = _bluetooth.SDPSession()
                s.connect( addr )
                matches = []
                if uuid is not None:
                    matches = s.search(uuid)
                else:
                    matches = s.browse()
            except _bluetooth.error:
                continue

            if name is not None:
                matches = filter( lambda s: s.get( "name", "" ) == name, \
                        matches )

            for m in matches:
                m["host"] = addr

            results.extend(matches)
    except _bluetooth.error, e:
        raise BluetoothError(str(e))

    return results
    
# ================= internal methods =================
def _gethcisock( device_id = -1):
    try:
        sock = bt.hci_open_dev( device_id )
    except:
        raise BluetoothError("error accessing bluetooth device")
    return sock

def _checkaddr(addr):
    if not is_valid_address(addr): 
        raise BluetoothError("%s is not a valid Bluetooth address" % addr)

def _get_acl_conn_handle(hci_sock, addr):
    hci_fd = hci_sock.fileno()
    reqstr = struct.pack( "6sB17s", bt.str2ba(addr), bt.ACL_LINK, "\0" * 17)
    request = array.array( "c", reqstr )
    try:
        fcntl.ioctl( hci_fd, bt.HCIGETCONNINFO, request, 1 )
    except IOError, e:
        raise BluetoothError("There is no ACL connection to %s" % addr)

    handle = struct.unpack("8xH14x", request.tostring())[0]
    return handle

def _write_flush_timeout( addr, timeout ):
    hci_sock = bt.hci_open_dev()
    # get the ACL connection handle to the remote device
    handle = _get_acl_conn_handle(hci_sock, addr)
    pkt = struct.pack("HH", handle, bt.htobs(timeout))
    response = bt.hci_send_req(hci_sock, bt.OGF_HOST_CTL, 
        0x0028, bt.EVT_CMD_COMPLETE, 3, pkt)
    status = struct.unpack("B", response[0])[0]
    rhandle = struct.unpack("H", response[1:3])[0]
    assert rhandle == handle 
    assert status == 0

def _read_flush_timeout( addr ):
    hci_sock = bt.hci_open_dev()
    # get the ACL connection handle to the remote device
    handle = _get_acl_conn_handle(hci_sock, addr)
    pkt = struct.pack("H", handle)
    response = bt.hci_send_req(hci_sock, bt.OGF_HOST_CTL, 
        0x0027, bt.EVT_CMD_COMPLETE, 5, pkt)
    status = struct.unpack("B", response[0])[0]
    rhandle = struct.unpack("H", response[1:3])[0]
    assert rhandle == handle
    assert status == 0
    fto = struct.unpack("H", response[3:5])[0]
    return fto


if __name__ == "__main__":
    print "performing device discovery"
    print discover_devices()
