import sys
import struct
import binascii

if sys.platform == "win32":
    import _msbt as bt
    bt.initwinsock()
elif sys.platform == "linux2":
    import _bluetooth as _bt
    import array
    import fcntl
    _constants = [ 'HCI', 'RFCOMM', 'L2CAP', 'SCO', 'SOL_L2CAP', 'SOL_RFCOMM',\
        'L2CAP_OPTIONS' ]
    for _c in _constants: exec "%s = _bt.%s" % (_c, _c)
    del _constants

L2CAP=0
RFCOMM=3

PORT_ANY=0

# Service Class IDs
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

# Bluetooth Profile Descriptors
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

# Universal Service Attribute IDs
SERVICE_RECORD_HANDLE_ATTRID = 0x0000
SERVICE_CLASS_ID_LIST_ATTRID = 0x0001
SERVICE_RECORD_STATE_ATTRID = 0x0002
SERVICE_ID_ATTRID = 0x0003
PROTOCOL_DESCRIPTOR_LIST_ATTRID = 0x0004
BROWSE_GROUP_LIST_ATTRID = 0x0005
LANGUAGE_BASE_ATTRID_LIST_ATTRID = 0x0006
SERVICE_INFO_TIME_TO_LIVE_ATTRID = 0x0007
SERVICE_AVAILABILITY_ATTRID = 0x0008
BLUETOOTH_PROFILE_DESCRIPTOR_LIST_ATTRID = 0x0009
DOCUMENTATION_URL_ATTRID = 0x000a
CLIENT_EXECUTABLE_URL_ATTRID = 0x000b
ICON_URL_ATTRID = 0x000c
SERVICE_NAME_ATTRID = 0x0100
SERVICE_DESCRIPTION_ATTRID = 0x0101
PROVIDER_NAME_ATTRID = 0x0102

# Protocol UUIDs
SDP_UUID       = "0001"
UDP_UUID       = "0002"
RFCOMM_UUID    = "0003"
TCP_UUID       = "0004"
TCS_BIN_UUID   = "0005"
TCS_AT_UUID    = "0006"
OBEX_UUID      = "0008"
IP_UUID        = "0009"
FTP_UUID       = "000a"
HTTP_UUID      = "000c"
WSP_UUID       = "000e"
BNEP_UUID      = "000f"
UPNP_UUID      = "0010"
HIDP_UUID      = "0011"
HCRP_CTRL_UUID = "0012"
HCRP_DATA_UUID = "0014"
HCRP_NOTE_UUID = "0016"
AVCTP_UUID     = "0017"
AVDTP_UUID     = "0019"
CMTP_UUID      = "001b"
UDI_UUID       = "001d"
L2CAP_UUID     = "0100"

def is_valid_address( s ):
    """
    returns True if address is a valid Bluetooth address

    valid address are always strings of the form XX:XX:XX:XX:XX:XX
    where X is a hexadecimal character.  For example,
        01:23:45:67:89:AB is a valid address, but
        IN:VA:LI:DA:DD:RE is not
    """
    try:
        pairs = s.split(":")
        if len(pairs) != 6: return False
        for b in pairs: int(b, 16)
    except:
        return False
    return True

def is_valid_uuid(uuid):
    """
    is_valid_uuid(uuid) -> bool

    returns True if uuid is a valid 128-bit UUID.

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

def to_full_uuid(uuid):
    """
    converts a short 16-bit or 32-bit reserved UUID to a full 128-bit Bluetooth
    UUID.
    """
    if not is_valid_uuid(uuid): raise ValueError("invalid UUID")
    if len(uuid) == 4:
        return "0000%s-0000-1000-8000-00805F9B34FB" % uuid
    elif len(uuid) == 8:
        return "%s-0000-1000-8000-00805F9B34FB" % uuid
    else:
        return uuid

def discover_devices(duration=8, flush_cache=True, lookup_names=False):
    """
    performs a bluetooth device discovery using the first available bluetooth
    resource.

    if lookup_names is False, returns a list of bluetooth addresses.
    if lookup_names is True, returns a list of (address, name) tuples

    duration=8 
        how long, in units of 1.28 seconds, to search for devices.  To find as
        many devices as possible, you should set this to at least 8.  
        Linux only
    flush_cache=True 
        if set to False, then discover_devices may return devices found during
        previous discoveries.
    lookup_names=False
        if set to True, then discover_devices also attempts to lookup the
        display name of each detected device.
    """
    if sys.platform == "linux2":
        sock = _gethcisock()
        try:
            results = _bt.hci_inquiry(sock, duration=8, flush_cache=True)
        except _bt.error:
            sock.close()
            raise BluetoothError("error communicating with local "
            "bluetooth adapter")

        if lookup_names:
            pairs = []
            for addr in results:
                timeoutms = int(10 * 1000)
                try: 
                    name = _bt.hci_read_remote_name( sock, addr, timeoutms )
                except _bt.error, e:
                    # name lookup failed.  either a timeout, or I/O error
                    continue
                pairs.append( (addr, name) )
            sock.close()
            return pairs
        else:
            sock.close()
            return results
    elif sys.platform == "win32":
        return bt.discover_devices(flush_cache, lookup_names)

class BluetoothError(IOError):
    pass

def lookup_name(address, timeout=10):
    """
    Linux only

    Tries to determine the friendly name (human readable) of the device with
    the specified bluetooth address.  Returns the name on success, and None
    on failure.

    timeout=10   how many seconds to search before giving up.
    """
    if sys.platform == "linux2":
        _checkaddr(address)

        sock = _gethcisock()
        timeoutms = int(timeout * 1000)
        try: 
            name = _bt.hci_read_remote_name( sock, address, timeoutms )
        except _bt.error, e:
            # name lookup failed.  either a timeout, or I/O error
            name = None
        sock.close()
        return name
    elif sys.platform == "win32":
        if not is_valid_address(address): 
            raise ValueError("Invalid Bluetooth address")

        return bt.lookup_name( address )

def get_available_port( protocol ):
    """
    deprecated.  bind to PORT_ANY instead.
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
    if sys.platform != "linux2":
        raise NotImplementedException("set packet timeout NYI")
    n = round(timeout / 0.625)
    _write_flush_timeout( address, n )

def set_l2cap_mtu( sock, mtu ):
    """set_l2cap_mtu( sock, mtu )

    Adjusts the MTU for the specified L2CAP socket.  This method needs to be
    invoked on both sides of the connection for it to work!  The default mtu
    that all L2CAP connections start with is 672 bytes.

    mtu must be between 48 and 65535, inclusive.
    """
    if sys.platform != "linux2":
        raise NotImplementedException("set L2CAP MTU NYI")
    s = sock.getsockopt( SOL_L2CAP, L2CAP_OPTIONS, 7 )
    # XXX should these be "<HHHB"?
    o,i,f,m = struct.unpack("HHHB", s)
    s = struct.pack("HHHB", mtu, mtu, f, m)
    sock.setsockopt( SOL_L2CAP, L2CAP_OPTIONS, s)

class BluetoothSocket:
    if sys.platform == "linux2":
        __doc__ = _bt.btsocket.__doc__

        def __init__(self, proto = RFCOMM, _sock=None):
            if _sock is None:
                _sock = _bt.btsocket(proto)
            self._sock = _sock
            self._proto = proto

        def dup(self):
            """dup() -> socket object

            Return a new socket object connected to the same system resource."""
            return BluetoothSocket(proto=self._proto, _sock=self._sock)

        def accept(self):
            try:
                client, addr = self._sock.accept()
            except _bt.error, e:
                raise BluetoothError(str(e))
            newsock = BluetoothSocket(self._proto, client)
            return (newsock, addr)
        accept.__doc__ = _bt.btsocket.accept.__doc__

        def bind(self, addrport):
            if self._proto == RFCOMM or self._proto == L2CAP:
                addr, port = addrport
                if port == 0: addrport = (addr, get_available_port(self._proto))
            return self._sock.bind(addrport)

        # import methods from the wraapped socket object
        _s = ("""def %s(self, *args, **kwargs): 
        try: 
            return self._sock.%s(*args, **kwargs)
        except _bt.error, e:
            raise BluetoothError(str(e))
        %s.__doc__ = _bt.btsocket.%s.__doc__\n""")
        for _m in ( 'connect', 'connect_ex', 'close',
            'fileno', 'getpeername', 'getsockname', 'gettimeout',
            'getsockopt', 'listen', 'makefile', 'recv', 'recvfrom', 'sendall',
            'send', 'sendto', 'setblocking', 'setsockopt', 'settimeout', 
            'shutdown'):
            exec _s % (_m, _m, _m, _m)
        del _m, _s
    elif sys.platform == "win32":
        def __init__(self, proto = RFCOMM, sockfd = None):
            if proto not in [ RFCOMM ]:
                raise ValueError("invalid protocol")

            if sockfd:
                self._sockfd = sockfd
            else:
                self._sockfd = bt.socket( bt.SOCK_STREAM, bt.BTHPROTO_RFCOMM )
            self._proto = proto

            # used by advertise_service and stop_advertising
            self._sdp_handle = None
            self._raw_sdp_record = None

            # used to track if in blocking or non-blocking mode (FIONBIO appears
            # write only)
            self._blocking = True
            self._timeout = False

        def bind(self, addrport):
            if self._proto == RFCOMM:
                addr, port = addrport

                if port == 0: port = bt.BT_PORT_ANY
                status = bt.bind( self._sockfd, addr, port )

        def listen(self, backlog):
            bt.listen( self._sockfd, backlog )

        def accept(self):
            clientfd, addr, port = bt.accept( self._sockfd )
            client = BluetoothSocket( self._proto, sockfd=clientfd )
            return client, (addr, port)

        def connect(self, addrport):
            addr, port = addrport
            bt.connect( self._sockfd, addr, port )

        def send(self, data):
            return bt.send( self._sockfd, data )

        def recv(self, numbytes):
            return bt.recv( self._sockfd, numbytes )

        def close(self):
            return bt.close( self._sockfd )

        def getsockname(self):
            return bt.getsockname( self._sockfd )

        def setblocking(self, blocking):
            s = bt.setblocking( self._sockfd, blocking )
            self._blocking = blocking

        def settimeout(self, timeout):
            if timeout < 0: raise ValueError("invalid timeout")

            if timeout == 0:
                self.setblocking( False )
            else:
                self.setblocking( True )
                # XXX this doesn't look correct
                timeout = 0 # winsock timeout still needs to be set 0

            s = bt.settimeout( self._sockfd, timeout )
            self._timeout = timeout

        def gettimeout(self):    
            if self._blocking and not self._timeout: return None
            return bt.gettimeout( self._sockfd )

        def fileno(self):
            return self._sockfd

        def dup(self):
            return BluetoothSocket( self._proto, sockfd=bt.dup( self._sockfd) )

        def makefile(self):
            # TODO
            raise "Not yet implemented"


# =============== parsing and constructing raw SDP records ============

def sdp_parse_size_desc(data):
    dts = struct.unpack("B", data[0])[0]
    dtype, dsizedesc = dts >> 3, dts & 0x7
    dstart = 1
    if   dtype == 0:     dsize = 0
    elif dsizedesc == 0: dsize = 1
    elif dsizedesc == 1: dsize = 2
    elif dsizedesc == 2: dsize = 4
    elif dsizedesc == 3: dsize = 8
    elif dsizedesc == 4: dsize = 16
    elif dsizedesc == 5:
        dsize = struct.unpack("B", data[1])[0]
        dstart += 1
    elif dsizedesc == 6:
        dsize = struct.unpack("!H", data[1:3])[0]
        dstart += 2
    elif dsizedesc == 7:
        dsize == struct.unpack("!I", data[1:5])[0]
        dstart += 4

    if dtype > 8:
        raise ValueError("Invalid TypeSizeDescriptor byte %s %d, %d" \
                % (binascii.hexlify(data[0]), dtype, dsizedesc))

    return dtype, dsize, dstart

def sdp_parse_uuid(data, size):
    if size == 2:
        return binascii.hexlify( data )
    elif size == 4:
        return binascii.hexlify( data )
    elif size == 16:
        return "%08X-%04X-%04X-%04X-%04X%08X" % struct.unpack("!IHHHHI", data)
    else: return ValueError("invalid UUID size")

def sdp_parse_int(data, size, signed):
    fmts = { 1 : "!b" , 2 : "!h" , 4 : "!i" , 8 : "!q" , 16 : "!qq" }
    fmt = fmts[size]
    if not signed: fmt = fmt.upper()
    if fmt in [ "!qq", "!QQ" ]:
        upp, low = struct.unpack("!QQ", data)
        result = ( upp << 64 ) | low
        if signed:
            result=-((~(result-1))&0x7FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFL)
        return result
    else:
        return struct.unpack(fmt, data)[0]

def sdp_parse_data_elementSequence(data):
    result = []
    pos = 0
    datalen = len(data)
    while pos < datalen:
        rtype, rval, consumed = sdp_parse_data_element(data[pos:])
        pos += consumed
        result.append( (rtype, rval) )
    return result

def sdp_parse_data_element(data):
    dtype, dsize, dstart = sdp_parse_size_desc(data)
    elem = data[dstart:dstart+dsize]

    if dtype == 0:
        rtype, rval = "Nil", None
    elif dtype == 1:
        rtype, rval = "UInt%d"%(dsize*8), sdp_parse_int(elem, dsize, False)
    elif dtype == 2:
        rtype, rval = "SInt%d"%(dsize*8), sdp_parse_int(elem, dsize, True)
    elif dtype == 3:
        rtype, rval = "UUID", sdp_parse_uuid(elem, dsize)
    elif dtype == 4:
        rtype, rval = "String", elem
    elif dtype == 5:
        rtype, rval = "Bool", (struct.unpack("B", elem)[0] != 0)
    elif dtype == 6:
        rtype, rval = "ElemSeq", sdp_parse_data_elementSequence( elem )
    elif dtype == 7:
        rtype, rval = "AltElemSeq", sdp_parse_data_elementSequence( elem )
    elif dtype == 8:
        rtype, rval = "URL", elem

    return rtype, rval, dstart+dsize

def sdp_parse_raw_record(data):
    dtype, dsize, dstart = sdp_parse_size_desc(data)
    assert dtype == 6

    pos = dstart
    datalen = len(data)
    record = {}
    while pos < datalen:
        type, attrid, consumed = sdp_parse_data_element(data[pos:])
        assert type == "UInt16"
        pos += consumed
        type, attrval, consumed = sdp_parse_data_element(data[pos:])
        pos += consumed
        record[attrid] = attrval
    return record

def sdp_make_data_element( type, value ):
    def maketsd(tdesc, sdesc):
        return struct.pack("B", (tdesc << 3) | sdesc)
    def maketsdl(tdesc, size):
        if   size < (1<<8):  return struct.pack("!BB", tdesc << 3 | 5, size)
        elif size < (1<<16): return struct.pack("!BH", tdesc << 3 | 6, size)
        else:                return struct.pack("!BI", tdesc << 3 | 7, size)

    easyinttypes = { "UInt8"   : (1, 0, "!B"),  "UInt16"  : (1, 1, "!H"),
                     "UInt32"  : (1, 2, "!I"),  "UInt64"  : (1, 3, "!Q"),
                     "SInt8"   : (2, 0, "!b"),  "SInt16"  : (2, 1, "!h"),
                     "SInt32"  : (2, 2, "!i"),  "SInt64"  : (2, 3, "!q"),
                     }

    if type == "Nil": 
        return maketsd( 0, 0 )
    elif type in easyinttypes:
        tdesc, sdesc, fmt = easyinttypes[type]
        return maketsd(tdesc, sdesc) + struct.pack(fmt, value)
    elif type == "UInt128":
        ts = maketsd( 1, 4 )
        upper = ts >> 64
        lower = (ts & 0xFFFFFFFFFFFFFFFFL)
        return ts + struct.pack("!QQ", upper, lower)
    elif type == "SInt128":
        ts = maketsd( 2, 4 )
        # FIXME
        raise NotImplementedException("128-bit signed int NYI!")
    elif type == "UUID":
        if len(value) == 4:
            return maketsd( 3, 1 ) + binascii.unhexlify(value)
        elif len(value) == 8:
            return maketsd( 3, 2 ) + binascii.unhexlify(value)
        elif len(value) == 36:
            return maketsd( 3, 4 ) + binascii.unhexlify(value.replace("-",""))
    elif type == "String":
        return maketsdl( 4, len(value) ) + value
    elif type == "Bool":
        return maketsd(5,0) + (value and "\x01" or "\x00")
    elif type == "ElemSeq":
        packedseq = ""
        for subtype, subval in value:
            nextelem = sdp_make_data_element( subtype, subval )
            packedseq = packedseq + nextelem
        return maketsdl( 6, len(packedseq)) + packedseq
    elif type == "AltElemSeq":
        packedseq = ""
        for subtype, subval in value:
            packedseq = packedseq + sdp_make_data_element( subtype, subval )
        return maketsdl( 7, len(packedseq) ) + packedseq
    elif type == "URL":
        return maketsdl( 8, len(value) ) + value
    else:
        raise ValueError("invalid type %s" % type)

# ============== SDP service registration and unregistration ============

def advertise_service( sock, name, service_id = "", service_classes = [], \
        profiles = [], provider = "", description = "", protocols = [] ):
    """
    Advertises a service with the local SDP server.  sock must be a bound,
    listening socket.  name should be the name of the service, and service_id 
    (if specified) should be a string of the form 
    "XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX", where each 'X' is a hexadecimal
    digit.

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
        if not is_valid_uuid(uuid) or  version < 0 or  version > 0xFFFF:
            raise ValueError("Invalid Profile Descriptor")
    for uuid in protocols:
        if not is_valid_uuid( uuid ):
            raise ValueError("invalid UUID specified in protocols")        

    if sys.platform == "linux2":
        try:
            _bt.sdp_advertise_service( sock._sock, name, service_id, \
                    service_classes, profiles, provider, description, \
                    protocols )
        except _bt.error, e:
            raise BluetoothError(str(e))
    elif sys.platform == "win32":
        if sock._raw_sdp_record is not None:
            raise IOError("service already advertised")

        avpairs = []

        # service UUID
        if len(service_id) > 0:
            avpairs.append( ("UInt16", SERVICE_ID_ATTRID) )
            avpairs.append( ("UUID", service_id) )

        # service class list
        if len(service_classes) > 0:
            seq = [ ("UUID", svc_class) for svc_class in service_classes ]
            avpairs.append( ("UInt16", SERVICE_CLASS_ID_LIST_ATTRID) )
            avpairs.append( ("ElemSeq", seq) )

        # set protocol and port information
        assert sock._proto == RFCOMM
        addr, port = sock.getsockname()
        avpairs.append(("UInt16", PROTOCOL_DESCRIPTOR_LIST_ATTRID))
        l2cap_pd = ("ElemSeq", (("UUID", L2CAP_UUID), ))
        rfcomm_pd = ("ElemSeq", (("UUID", RFCOMM_UUID), ("UInt8", port)))
        proto_list = [ l2cap_pd, rfcomm_pd ]
        for proto_uuid in protocols:
            proto_list.append( ("ElemSeq", (("UUID", proto_uuid), )) )
        avpairs.append(("ElemSeq", proto_list ))

        # make the service publicly browseable
        avpairs.append(("UInt16", BROWSE_GROUP_LIST_ATTRID))
        avpairs.append(("ElemSeq", (("UUID", PUBLIC_BROWSE_GROUP),)))

        # profile descriptor list
        if len(profiles) > 0:
            seq = [ ("ElemSeq", (("UUID",uuid), ("UInt16",version))) \
                    for uuid, version in profiles ]
            avpairs.append( ("UInt16", 
                BLUETOOTH_PROFILE_DESCRIPTOR_LIST_ATTRID) )
            avpairs.append( ("ElemSeq", seq) )

        # service name
        avpairs.append(("UInt16", SERVICE_NAME_ATTRID))
        avpairs.append(("String", name))

        # service description
        if len(description) > 0:
            avpairs.append(("UInt16", SERVICE_DESCRIPTION_ATTRID))
            avpairs.append(("String", description))
        
        # service provider
        if len(provider) > 0:
            avpairs.append(("UInt16", PROVIDER_NAME_ATTRID))
            avpairs.append(("String", provider))

        sock._raw_sdp_record = sdp_make_data_element( "ElemSeq", avpairs )
#    pr = sdp_parse_raw_record( sock._raw_sdp_record )
#    for attrid, val in pr.items():
#        print "%5s: %s" % (attrid, val)
#    print binascii.hexlify(sock._raw_sdp_record)
#    print repr(sock._raw_sdp_record)

        sock._sdp_handle = bt.set_service_raw( sock._raw_sdp_record, True )

def stop_advertising( sock ):
    """
    Instructs the local SDP server to stop advertising the service associated
    with sock.  You should typically call this right before you close sock.
    """
    if sys.platform == "linux2":
        try:
            _bt.sdp_stop_advertising( sock._sock )
        except _bt.error, e:
            raise BluetoothError(str(e))
    elif sys.platform == "win32":
        if sock._raw_sdp_record is None:
            raise IOError("service isn't advertised, " \
                            "but trying to un-advertise")
        bt.set_service_raw( sock._raw_sdp_record, False, sock._sdp_handle )
        sock._raw_sdp_record = None
        sock._sdp_handle = None

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
    if sys.platform == "linux2":
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
                    s = _bt.SDPSession()
                    s.connect( addr )
                    matches = []
                    if uuid is not None:
                        matches = s.search(uuid)
                    else:
                        matches = s.browse()
                except _bt.error:
                    continue

                if name is not None:
                    matches = filter( lambda s: s.get( "name", "" ) == name, \
                            matches )

                for m in matches:
                    m["host"] = addr

                results.extend(matches)
        except _bt.error, e:
            raise BluetoothError(str(e))

        return results
    elif sys.platform == "win32":
        if address is not None:
            addresses = [ address ]
        else:
            addresses = discover_devices(lookup_names = False)

        results = []

        for addr in addresses:
            uuidstr = uuid or PUBLIC_BROWSE_GROUP
            if not is_valid_uuid( uuidstr ): raise ValueError("invalid UUID")

            uuidstr = to_full_uuid( uuidstr )

            dresults = bt.find_service( addr, uuidstr )

            for dict in dresults:
                raw = dict["rawrecord"]

                record = sdp_parse_raw_record( raw )

                if SERVICE_CLASS_ID_LIST_ATTRID in record:
                    svc_class_id_list = [ t[1] for t in \
                            record[SERVICE_CLASS_ID_LIST_ATTRID] ]
                    dict["service-classes"] = svc_class_id_list
                else:
                    dict["services-classes"] = []

                if BLUETOOTH_PROFILE_DESCRIPTOR_LIST_ATTRID in record:
                    pdl = []
                    for profile_desc in \
                            record[BLUETOOTH_PROFILE_DESCRIPTOR_LIST_ATTRID]:
                        uuidpair, versionpair = profile_desc[1]
                        pdl.append( (uuidpair[1], versionpair[1]) )
                    dict["profiles"] = pdl
                else:
                    dict["profiles"] = []

                dict["provider"] = record.get(PROVIDER_NAME_ATTRID, None)

                dict["service-id"] = record.get(SERVICE_ID_ATTRID, None)

                # XXX the C version is buggy (retrieves an extra byte or two),
                # so get the service name here even though it may have already
                # been set
                dict["name"] = record.get(SERVICE_NAME_ATTRID, None)

                dict["handle"] = record.get(SERVICE_RECORD_HANDLE_ATTRID, None)
            
#        if LANGUAGE_BASE_ATTRID_LIST_ATTRID in record:
#            for triple in record[LANGUAGE_BASE_ATTRID_LIST_ATTRID]:
#                code_ISO639, encoding, base_offset = triple
#
#        if SERVICE_DESCRIPTION_ATTRID in record:
#            service_description = record[SERVICE_DESCRIPTION_ATTRID]

            if name is None:
                results.extend(dresults)
            else:
                results.extend( [ d for d in dresults if d["name"] == name ] )
        return results


# ================ BlueZ internal methods ================
def _gethcisock( device_id = -1):
    try:
        sock = _bt.hci_open_dev( device_id )
    except:
        raise BluetoothError("error accessing bluetooth device")
    return sock

def _checkaddr(addr):
    if not is_valid_address(addr): 
        raise BluetoothError("%s is not a valid Bluetooth address" % addr)

def _get_acl_conn_handle(hci_sock, addr):
    hci_fd = hci_sock.fileno()
    reqstr = struct.pack( "6sB17s", _bt.str2ba(addr), 
            _bt.ACL_LINK, "\0" * 17)
    request = array.array( "c", reqstr )
    try:
        fcntl.ioctl( hci_fd, _bt.HCIGETCONNINFO, request, 1 )
    except IOError, e:
        raise BluetoothError("There is no ACL connection to %s" % addr)

    # XXX should this be "<8xH14x"?
    handle = struct.unpack("8xH14x", request.tostring())[0]
    return handle

def _write_flush_timeout( addr, timeout ):
    hci_sock = _bt.hci_open_dev()
    # get the ACL connection handle to the remote device
    handle = _get_acl_conn_handle(hci_sock, addr)
    # XXX should this be "<HH"
    pkt = struct.pack("HH", handle, _bt.htobs(timeout))
    response = _bt.hci_send_req(hci_sock, _bt.OGF_HOST_CTL, 
        0x0028, _bt.EVT_CMD_COMPLETE, 3, pkt)
    status = struct.unpack("B", response[0])[0]
    rhandle = struct.unpack("H", response[1:3])[0]
    assert rhandle == handle 
    assert status == 0

def _read_flush_timeout( addr ):
    hci_sock = _bt.hci_open_dev()
    # get the ACL connection handle to the remote device
    handle = _get_acl_conn_handle(hci_sock, addr)
    # XXX should this be "<H"?
    pkt = struct.pack("H", handle)
    response = _bt.hci_send_req(hci_sock, _bt.OGF_HOST_CTL, 
        0x0027, _bt.EVT_CMD_COMPLETE, 5, pkt)
    status = struct.unpack("B", response[0])[0]
    rhandle = struct.unpack("H", response[1:3])[0]
    assert rhandle == handle
    assert status == 0
    fto = struct.unpack("H", response[3:5])[0]
    return fto

# =============== DeviceDiscoverer ==================
class DeviceDiscoverer:
    """
    Skeleton class for finer control of the device discovery process.

    To implement asynchronous device discovery (e.g. if you want to do
    something *as soon as* a device is discovered), subclass
    DeviceDiscoverer and override device_discovered() and
    inquiry_complete()
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
        find_devices( lookup_names=True, service_name=None, 
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
        flt = _bt.hci_filter_new()
        _bt.hci_filter_all_events(flt)
        _bt.hci_filter_set_ptype(flt, _bt.HCI_EVENT_PKT)

        try:
            self.sock.setsockopt( _bt.SOL_HCI, _bt.HCI_FILTER, flt )
        except:
            raise BluetoothError("problem with local bluetooth device.")

        # send the inquiry command
        duration = 4
        max_responses = 255
        cmd_pkt = struct.pack("BBBBB", 0x33, 0x8b, 0x9e, \
                duration, max_responses)

        self.pre_inquiry()
        
        try:
            _bt.hci_send_cmd( self.sock, _bt.OGF_LINK_CTL, \
                    _bt.OCF_INQUIRY, cmd_pkt)
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
                _bt.hci_send_cmd( self.sock, _bt.OGF_LINK_CTL, \
                        _bt.OCF_INQUIRY_CANCEL )
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
        import socket

        if self.sock is None: return
        # voodoo magic!!!
        pkt = self.sock.recv(255)
        ptype, event, plen = struct.unpack("BBB", pkt[:3])
        pkt = pkt[3:]
        if event == _bt.EVT_INQUIRY_RESULT:
            nrsp = struct.unpack("B", pkt[0])[0]
            for i in range(nrsp):
                addr = _bt.ba2str( pkt[1+6*i:1+6*i+6] )
                psrm = pkt[ 1+6*nrsp+i ]
                pspm = pkt[ 1+7*nrsp+i ]
                devclass_raw = struct.unpack("BBB", 
                        pkt[1+9*nrsp+3*i:1+9*nrsp+3*i+3])
                devclass = (devclass_raw[2] << 16) | \
                        (devclass_raw[1] << 8) | \
                        devclass_raw[0]
                clockoff = pkt[1+12*nrsp+2*i:1+12*nrsp+2*i+2]

                self._device_discovered( addr, devclass, 
                        psrm, pspm, clockoff )
        elif event == _bt.EVT_INQUIRY_RESULT_WITH_RSSI:
            nrsp = struct.unpack("B", pkt[0])[0]
            for i in range(nrsp):
                addr = _bt.ba2str( pkt[1+6*i:1+6*i+6] )
                psrm = pkt[ 1+6*nrsp+i ]
                pspm = pkt[ 1+7*nrsp+i ]
#                devclass_raw = pkt[1+8*nrsp+3*i:1+8*nrsp+3*i+3]
#                devclass = struct.unpack("I", "%s\0" % devclass_raw)[0]
                devclass_raw = struct.unpack("BBB", 
                        pkt[1+9*nrsp+3*i:1+9*nrsp+3*i+3])
                devclass = (devclass_raw[2] << 16) | \
                        (devclass_raw[1] << 8) | \
                        devclass_raw[0]
                clockoff = pkt[1+11*nrsp+2*i:1+11*nrsp+2*i+2]
                rssi = struct.unpack("b", pkt[1+13*nrsp+i])[0]

                self._device_discovered( addr, devclass, 
                        psrm, pspm, clockoff )
        elif event == _bt.EVT_INQUIRY_COMPLETE:
            self.is_inquiring = False
            if len(self.names_to_find) == 0:
#                print "inquiry complete (evt_inquiry_complete)"
                self.sock.close()
                self.inquiry_complete()
            else:
                self._send_next_name_req()

        elif event == _bt.EVT_CMD_STATUS:
            # XXX shold this be "<BBH"
            status, ncmd, opcode = struct.unpack("BBH", pkt[:4])
            if status != 0:
                self.is_inquiring = False
                self.sock.close()
                
#                print "inquiry complete (bad status 0x%X 0x%X 0x%X)" % \
#                        (status, ncmd, opcode)
                self.names_to_find = {}
                self.inquiry_complete()
        elif event == _bt.EVT_REMOTE_NAME_REQ_COMPLETE:
            status = struct.unpack("B", pkt[0])[0]
            addr = _bt.ba2str( pkt[1:7] )
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

    def _device_discovered(self, address, device_class, 
            psrm, pspm, clockoff):
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
        bdaddr = _bt.str2ba( address )
        
        cmd_pkt = "%s%s\0%s" % (bdaddr, psrm, clockoff)

        try:
            _bt.hci_send_cmd( self.sock, _bt.OGF_LINK_CTL, \
                    _bt.OCF_REMOTE_NAME_REQ, cmd_pkt)
        except Exception, e:
            raise BluetoothError("error request name of %s - %s" % 
                    (address, str(e)))

    def fileno(self):
        if not self.sock: return None
        return self.sock.fileno()

    def pre_inquiry(self):
        """
        Called just after find_devices is invoked, but just before the
        inquiry is started.

        This method exists to be overriden
        """

    def device_discovered(self, address, device_class, name):
        """
        Called when a bluetooth device is discovered.

        address is the bluetooth address of the device

        device_class is the Class of Device, as specified in [1]
                     passed in as a 3-byte string

        name is the user-friendly name of the device if lookup_names was
        set when the inquiry was started.  otherwise None
        
        This method exists to be overriden.

        [1] https://www.bluetooth.org/foundry/assignnumb/document/baseband
        """
        if name:
            print "found: %s - %s (class 0x%X)" % \
                    (address, name, device_class)
        else:
            print "found: %s (class 0x%X)" % (address, device_class)

    def inquiry_complete(self):
        """
        Called when an inquiry started by find_devices has completed.
        """
        print "inquiry complete"

