#! /usr/bin/env python

"""
Copyright 1996, Barry A. Warsaw <bwarsaw@python.org>

FVWM version 2 module interface framework for Python.
Requires Python 1.4 <http://www.python.org/>

Many thanks to Jonathan Kelley <jkelley@ugcs.caltech.edu> for his
original inspiration, fvwmmod.py which worked with FVWM version 1.

"""

__version__ = '1.0'

import struct
import os
import sys
import string
import regex
from types import *

_APPNAME = ''



# useful contants from fvwm/module.h -- not all are duplicated here

SynchronizationError = 'SynchronizationError'
PipeReadError = 'PipeReadError'

HEADER_SIZE = 4
DATASIZE = struct.calcsize('l')
START = 0xffffffff
WINDOW_NONE = 0

# convert from a C unsigned long to a Python long int
def ulong(x): return long(x) % 0x100000000L



Contexts = {'C_NO_CONTEXT': 0,
	    'C_WINDOW':     1,
	    'C_TITLE':      2,
	    'C_ICON':       4,
	    'C_ROOT':       8,
	    'C_FRAME':     16,
	    'C_SIDEBAR':   32,
	    'C_L1':        64,
	    'C_L2':       128,
	    'C_L3':       256,
	    'C_L4':       512,
	    'C_L5':      1024,
	    'C_R1':      2048,
	    'C_R2':      4096,
	    'C_R3':      8192,
	    'C_R4':     16384,
	    'C_R5':     32768,
	    }
for k, v in Contexts.items()[:]:
    Contexts[v] = k



Types = {'M_NEW_PAGE':            1,
	 'M_NEW_DESK':         1<<1,
	 'M_ADD_WINDOW':       1<<2,
	 'M_RAISE_WINDOW':     1<<3,
	 'M_LOWER_WINDOW':     1<<4,
	 'M_CONFIGURE_WINDOW': 1<<5,
	 'M_FOCUS_CHANGE':     1<<6,
	 'M_DESTROY_WINDOW':   1<<7,
	 'M_ICONIFY':          1<<8,
	 'M_DEICONIFY':        1<<9,
	 'M_WINDOW_NAME':      1<<10,
	 'M_ICON_NAME':        1<<11,
	 'M_RES_CLASS':        1<<12,
	 'M_RES_NAME':         1<<13,
	 'M_END_WINDOWLIST':   1<<14,
	 'M_ICON_LOCATION':    1<<15,
	 'M_MAP':              1<<16,
	 'M_ERROR':            1<<17,
	 'M_CONFIG_INFO':      1<<18,
	 'M_END_CONFIG_INFO':  1<<19,
	 'M_ICON_FILE':        1<<20,
	 'M_DEFAULTICON':      1<<21,
	 'M_STRING':           1<<22,
	 }
for k, v in Types.items():
    Types[v] = k



# Window flags.  See fvwm.h
Flags = {'STARTICONIC':       (1<<0),
	 'ONTOP':             (1<<1),	# does window stay on top
	 'STICKY':            (1<<2),	# Does window stick to glass?
	 'WINDOWLISTSKIP':    (1<<3),
	 'SUPPRESSICON':      (1<<4),
	 'NOICON_TITLE':      (1<<5),
	 'Lenience':          (1<<6),
	 'StickyIcon':        (1<<7),
	 'CirculateSkipIcon': (1<<8),
	 'CirculateSkip':     (1<<9),
	 'ClickToFocus':      (1<<10),
	 'SloppyFocus':       (1<<11),
	 'SHOW_ON_MAP':       (1<<12),
	 'BORDER':            (1<<13),	# Is this decorated with border
	 'TITLE':             (1<<14),	# Is this decorated with title
	 'MAPPED':            (1<<15),	# is it mapped?
	 'ICONIFIED':         (1<<16),  # is it an icon now?
	 'TRANSIENT':         (1<<17),	# is it a transient window?
	 'RAISED':            (1<<18),	# if a sticky window, needs raising?
	 'VISIBLE':           (1<<19),  # is the window fully visible
	 'ICON_OURS':         (1<<20),  # is the icon win supplied by the app?
	 'PIXMAP_OURS':       (1<<21),  # is the icon pixmap ours to free?
	 'SHAPED_ICON':       (1<<22),  # is the icon shaped?
	 'MAXIMIZED':         (1<<23),  # is the window maximized?
	 'DoesWmTakeFocus':   (1<<24),
	 'DoesWmDeleteWindow': (1<<25),
	 'ICON_MOVED':        (1<<26),  # has the icon been moved by the user?
	 'ICON_UNMAPPED':     (1<<27),	# was the icon unmapped, even though
					# the window is still
					# iconified (Transients)
	 'MAP_PENDING':       (1<<28),  # Sent an XMapWindow, but didn't
					# receive a MapNotify yet.
	 'HintOverride':      (1<<29),
	 'MWMButtons':        (1<<30),
	 'MWMBorders':        (1<<31),
	 }

for k, v in Flags.items():
    Flags[v] = k

ALL_COMMON_FLAGS = (Flags['STARTICONIC']    | Flags['ONTOP'] | 
		    Flags['STICKY']         | Flags['WINDOWLISTSKIP'] | 
		    Flags['SUPPRESSICON']   | Flags['NOICON_TITLE'] |
		    Flags['Lenience']       |
		    Flags['StickyIcon']     | Flags['CirculateSkipIcon'] |
		    Flags['CirculateSkip']  | Flags['ClickToFocus'] |
		    Flags['SloppyFocus']    | Flags['SHOW_ON_MAP'])



# Bit gravity.  See X11/X.h
Gravity = {'Forget':    0,
           'NorthWest': 1,
           'North':     2,
           'NorthEast': 3,
           'West':      4,
           'Center':    5,
           'East':      6,
           'SouthWest': 7,
           'South':     8,
           'SouthEast': 9,
           'Static':    10,
           }

for k, v in Gravity.items():
    Gravity[v] = k


class PacketReader:
    """Reads a packet from the given file-like object.

    Public functions: read_packet(fp)
    """
    def read_packet(self, fp):
	"""Reads a packet from `fp' a file-like object.

	Returns an instance of a class derived from Packet.  Raises
	PipeReadError if a valid packet could not be read.  Returns
	None if no corresponding Packet-derived class could be
	instantiated.

	"""
	data = fp.read(DATASIZE * 2)
	try:
	    start, typenum = struct.unpack('2l', data)
	    typenum = ulong(typenum)
	except struct.error:
	    raise PipeReadError
	if start <> START:
	    raise SynchronizationError
	try:
	    msgtype = Types[typenum]
	except KeyError:
	    print 'unknown FVWM message type:', typenum, '(ignored)'
	    return None
	classname = string.joinfields(
	    map(string.capitalize,
		string.splitfields(msgtype, '_')[1:]) + ['Packet'],
	    '')
	# create an instance of the class named by the packet type
	try:
	    return getattr(PacketReader, classname)(typenum, fp)
	except AttributeError:
	    raise RuntimeError, 'no packet class named: %s' % classname

    class _Packet:
	def __init__(self, typenum, fp):
	    # start of packet and type have already been read
	    data = fp.read(DATASIZE * 2)
	    self.pktlen, self.timestamp = struct.unpack('2l', data)
	    self.pktlen = ulong(self.pktlen)
	    self.timestamp = ulong(self.timestamp)
	    self.msgtype = typenum
	    self.pkthandler = string.joinfields(
		map(string.capitalize,
		    string.splitfields(Types[typenum], '_')[1:]),
		'')
    
	def _read(self, fp, *attrs):
	    attrcnt = len(attrs)
	    fmt = '%dl' % attrcnt
	    data = fp.read(DATASIZE * attrcnt)
	    attrvals = struct.unpack(fmt, data)
	    i = 0
	    for a in attrs:
		setattr(self, a, attrvals[i])
		i = i+1
    
    class NewPagePacket(_Packet):
	def __init__(self, type, fp):
	    PacketReader._Packet.__init__(self, type, fp)
	    self._read(fp, 'x', 'y', 'desk', 'max_x', 'max_y')
    
    class NewDeskPacket(_Packet):
	def __init__(self, type, fp):
	    PacketReader._Packet.__init__(self, type, fp)
	    self._read(fp, 'desk')
    
    class AddWindowPacket(_Packet):
	def __init__(self, type, fp):
	    PacketReader._Packet.__init__(self, type, fp)
	    self._read(fp, 'top_id', 'frame_id', 'db_entry', 'x', 'y',
		       'width', 'height', 'desk', 'flags',
		       'title_height', 'border_width',
		       'base_width', 'base_height',
		       'resize_width_incr', 'resize_height_incr',
		       'min_width', 'min_height',
		       'max_width', 'max_height',
		       'icon_label_id', 'icon_pixmap_id',
		       'gravity',
		       'text_color', 'border_color')
    
    class ConfigureWindowPacket(AddWindowPacket):
	pass
    
    class LowerWindowPacket(_Packet):
	def __init__(self, type, fp):
	    PacketReader._Packet.__init__(self, type, fp)
	    self._read(fp, 'top_id', 'frame_id', 'db_entry')
    
    class RaiseWindowPacket(LowerWindowPacket):
	pass
    
    class DestroyWindowPacket(LowerWindowPacket):
	pass
    
    class FocusChangePacket(LowerWindowPacket):
	def __init__(self, type, fp):
	    PacketReader.LowerWindowPacket.__init__(self, type, fp)
	    self._read(fp, 'text_color', 'border_color')
    
    class IconifyPacket(LowerWindowPacket):
	def __init__(self, type, fp):
	    PacketReader.LowerWindowPacket.__init__(self, type, fp)
	    self._read(fp, 'x', 'y', 'width', 'height')
    
    class IconLocationPacket(IconifyPacket):
	pass
    
    class DeiconifyPacket(LowerWindowPacket):
	pass
    
    class MapPacket(LowerWindowPacket):
	pass
    
    class _VariableLenStringPacket:
	"""Can only be used for multiple inheritance with Packet"""
	def __init__(self, fp, extraskip=3):
	    fieldlen = self.pktlen - HEADER_SIZE - extraskip
	    data = fp.read(DATASIZE * fieldlen)
	    index = string.find(data, '\000')
	    if index >= 0:
		data = data[:index]
	    self.data = string.strip(data)
    
    class WindowNamePacket(LowerWindowPacket, _VariableLenStringPacket):
	def __init__(self, type, fp):
	    PacketReader.LowerWindowPacket.__init__(self, type, fp)
	    PacketReader._VariableLenStringPacket.__init__(self, fp)
	    self.name = self.data
    
    class IconNamePacket(WindowNamePacket):
	pass
    
    class ResClassPacket(WindowNamePacket):
	pass
    
    class ResNamePacket(WindowNamePacket):
	pass
    
    class EndWindowlistPacket(_Packet):
	pass
    
    class StringPacket(_Packet, _VariableLenStringPacket):
	def __init__(self, type, fp):
	    PacketReader._Packet.__init__(self, type, fp)
	    dummy = fp.read(DATASIZE * 3)       # three zero fields
	    PacketReader._VariableLenStringPacket.__init__(self, fp)

    class ErrorPacket(StringPacket):
	def __init__(self, type, fp):
	    PacketReader.StringPacket.__init__(self, type, fp)
	    self.errmsg = self.data
    
    class ConfigInfoPacket(StringPacket):
	pass
    
    class EndConfigInfoPacket(_Packet):
	pass

    class IconFilePacket(WindowNamePacket):
	pass

    class DefaulticonPacket(StringPacket):
	pass



class FvwmModule:
    __usetk = None
    __done = 0

    def __init__(self, argv):
	if len(argv) < 6:
	    raise UsageError, \
		  'Usage: %s ofd ifd configfile appcontext wincontext' % \
		  argv[0]
	try:
	    self.__tofvwm   = os.fdopen(string.atoi(argv[1]), 'wb', 0)
	    self.__fromfvwm = os.fdopen(string.atoi(argv[2]), 'rb', 0)
	except ValueError:
	    raise UsageError, \
		  'Usage: %s ofd ifd configfile appcontext wincontext' % \
		  argv[0]
	global _APPNAME
	self.program = _APPNAME = argv[0]
	self.configfile = argv[3]
	self.appcontext = argv[4]
	self.wincontext = argv[5]
	self.args = argv[6:][:]
	self.__pktreader = PacketReader()
	# initialize dispatch table
	self.__dispatch = {}

    def __close(self):
	self.__tofvwm.close()
	self.__fromfvwm.close()

    def __del__(self):
	self.__close()

    def __do_dispatch(self, *args):
	"""Dispatch on the read packet type.

	Any register()'d callbacks take precedence over instance methods.

	"""
	try:
	    pkt = self.__pktreader.read_packet(self.__fromfvwm)
	except PipeReadError:
	    self.__close()
	    sys.exit(1)
	# dispatch
	if pkt:
## 	    debug('dispatching: %s' % pkt.pkthandler)
	    if self.__dispatch.has_key(pkt.pkthandler):
		self.__dispatch[pkt.pkthandler](self, pkt)
	    elif hasattr(self, pkt.pkthandler):
		method = getattr(self, pkt.pkthandler)
		method(pkt)
	    else:
		self.unhandled_packet(pkt)

    #
    # alternative callback mechanism (used before method invocation)
    #
    def register(self, pktname, callback):
	"""Register a callback for a packet type.

	PKTNAME is the name of the packet to dispatch on.  Packet
	names are strings as described in the Fvwm Module Interface
	documentation.  PKTNAME can also be the numeric equivalent of
	the packet type.

	CALLBACK is a method that takes two arguments.  The first
	argument is the FvwmModule instance receiving the packet, and
	the second argument is the packet instance.

	Callbacks are not nestable.  If a callback has been previously
	registered for the packet, the old callback is returned.

	"""

	if type(pktname) == IntType:
	    pktname = Types[pktname]
	# raise KeyError if it is an invalid packet name
	Types[pktname]
	pkthandler = string.joinfields(
	    map(string.capitalize,
		string.splitfields(pktname, '_')[1:]),
	    '')
	try:
	    cb = self.__dispatch[pkthandler]
	except KeyError:
	    cb = None
	self.__dispatch[pkthandler] = callback
	return cb

    def unregister(self, pktname):
	"""Unregister any callbacks for the named packet.

	Any registered callback is returned.

	"""
	if type(pktname) == IntType:
	    pktname = Types[pktname]
	# raise KeyError if it is an invalid packet name
	Types[pktname]
	pkthandler = string.joinfields(
	    map(string.capitalize,
		string.splitfields(pktname, '_')[1:]),
	    '')
	try:
	    cb = self.__dispatch[pkthandler]
	    del self.__dispatch[pkthandler]
	except KeyError:
	    cb = None
	return cb

    #
    # starting and stopping the main loop
    #
    def start(self):
	# use a Tkinter mainloop only if Tkinter has been imported and
	# a Tk widget has been created.  Poking on
	# Tkinter._default_root is a kludge.
	if not self.__usetk:
	    try:
		t = sys.modules['Tkinter']
		self.__usetk = t._default_root
		if self.__usetk:
		    t.tkinter.createfilehandler(
			self.__fromfvwm,
			t.tkinter.READABLE,
			self.__do_dispatch)
	    except KeyError:
		pass
	if self.__usetk:
	    self.__usetk.mainloop()
	else:
	    self.__done = 0
	    while not self.__done:
		self.__do_dispatch()

    def stop(self):
	if self.__usetk:
	    self.__usetk.quit()
	else:
	    self.__done = 1

    #
    # misc
    #

    def send(self, command, window=WINDOW_NONE, cont=1):
	if command:
	    self.__tofvwm.write(struct.pack('l', window))
	    self.__tofvwm.write(struct.pack('i', len(command)))
	    self.__tofvwm.write(command)
	    self.__tofvwm.write(struct.pack('i', cont))
	    self.__tofvwm.flush()

    def set_mask(self, mask=None):
	if mask is None:
	    mask = 0
	    for name, flag in Types.items():
		if type(name) <> StringType:
		    continue
		methodname = string.joinfields(
		    map(string.capitalize,
			string.splitfields(name, '_')[1:]),
		    '')
		if hasattr(self, methodname) or \
		   self.__dispatch.has_key(methodname):
		    mask = mask | flag
	self.send('Set_Mask ' + `mask`)

    def unhandled_packet(self, pkt):
	debug('Unhandled packet type: %s' % Types[pkt.msgtype])

    #
    # useful shortcuts
    #
    def __override(self, callback):
	"""Returns a `callback cache' object for __restore()"""
	cache = {}
	for pktname in Types.keys():
	    if type(pktname) == StringType:
		cache[pktname] = self.register(pktname, callback)
	return cache

    def __restore(self, cache):
	for pktname, callback in cache.items():
	    if callback:
		self.register(pktname, callback)
	    else:
		self.unregister(pktname)

    def get_configinfo(self):
	"""Returns Fvwm module configuration line object."""
	info = []
	def callback(self, pkt, info=info):
	    pktname = Types[pkt.msgtype]
	    if pktname == 'M_CONFIG_INFO':
		info.append(string.strip(pkt.data))
	    elif pktname == 'M_END_CONFIG_INFO':
		self.stop()
	    else:
		error('Unexpected packet type: %s' % pktname)

	cache = self.__override(callback)
	self.send('Send_ConfigInfo')
	self.start()
	self.__restore(cache)
	return ConfigInfo(info)

    def get_windowlist(self):
	"""Returns an Fvwm window list dump as an instance."""
	import Tracker
	t = Tracker.Tracker()
	def callback(self, pkt, tracker=t):
	    if Types[pkt.msgtype] == 'M_END_WINDOWLIST':
		self.stop()
	    else:
		tracker.feed_pkt(pkt)

	cache = self.__override(callback)
	self.send('Send_WindowList')
	self.start()
	self.__restore(cache)
	return t



class ConfigInfo:
    """Class encapsulating the FVWM configuration lines.

    Public instance variables:

        iconpath   -- the value of IconPath variable
	pixmappath -- the value of PixmapPath variable
	clicktime  -- the value of ClickTime variable

    Public methods:

	get_infolines(re) -- returns all lines.  If optional RE is given, only
	                     lines that start with `*<RE>' are
	                     returned (the initial star should not be
	                     included).

    """

    def __init__(self, lines):
	self.__lines = lines[:]
	self.iconpath = self.__get_predefineds('iconpath')
	self.pixmappath = self.__get_predefineds('pixmappath')
	self.clicktime = self.__get_predefineds('clicktime')

    def __get_predefineds(self, varname):
	for line in self.__lines:
	    parts = string.split(line)
	    if string.lower(parts[0]) == varname:
		return parts[1]

    def get_infolines(self, re=None):
	pred = None
	if re:
	    cre = regex.compile('\*' + re)
	    pred = lambda l, cre=cre: cre.match(l) >= 0
	return filter(pred, self.__lines)



def error(msg, exit=None, flag='ERROR'):
    sys.stderr.write('[%s] <<%s>> %s\n' % (_APPNAME, flag, msg))
    if exit is not None:
	sys.exit(exit)

def debug(msg):
    error(msg, flag='DEBUG')



# testing

if __name__ == '__main__':
    def printlist(list):
	for item in list:
	    print "`%s'" % item

    m = FvwmModule(sys.argv)
    info = m.get_configinfo()
    print 'IconPath   ==', info.iconpath
    print 'PixmapPath ==', info.pixmappath
    print 'ClickTime  ==', info.clicktime
    print '1 =========='
    printlist(info.get_infolines())
    print '2 =========='
    printlist(info.get_infolines('FvwmIdent'))
