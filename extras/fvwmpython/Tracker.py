"""A module that tracks window and desktop information.

You keep this class up-to-date by feeding it useful packets, such as
M_NEW_PAGE and M_CONFIGURE_WINDOW, etc.  It collates this information
and makes it available via its public methods.

"""
__version__ = '1.0'

import Fvwm



class Window:
    """Attributes of a window.

    These are kept consistent by the Tracker class.  You should
    consider these attributes read-only.
    """
    name = ''				# Window's name
    icon_name = ''			# Icon's name
    icon_file = ''			# Icon's file
    res_class = ''			# Resource class
    res_name = ''			# Resource name
    top_id = None			# App's window ID
    frame_id = None			# Fvwm frame window ID
    db_entry = None			# Fvwm DB entry
    x = 0				# X origin
    y = 0				# Y origin
    width = 0				# width
    height = 0				# height
    desk = 0				# desk
    flags = 0				# flags
    title_height = 0			# title height
    border_width = 0			# border width
    base_width = 0			# base width
    base_height = 0			# base height
    resize_width_incr = 0		# resize increment
    resize_height_incr = 0		# resize increment
    min_width = 0			# minimum width
    min_height = 0			# minimum height
    max_width = 0			# maximum width
    max_height = 0			# maximum height
    icon_x = 0				# icon's X origin
    icon_y = 0				# icon's Y origin
    icon_width = 0			# icon's width
    icon_height = 0			# icon's height
    icon_label_id = 0			# icon label window ID
    icon_pixmap_id = 0			# icon pixmap window ID
    gravity = 0				# gravity
    text_color = 0			# text color 
    border_color = 0			# border color



_ALWAYSFILTER = ('msgtype', 'pkthandler', 'pktlen', 'timestamp')

def _insert(self, pkt, **translate):
    for fromattr in dir(pkt):
	if fromattr[:1] == '_' or fromattr in _ALWAYSFILTER:
	    continue
	if translate.has_key(fromattr):
	    toattr = translate[fromattr]
	    if toattr is None:
		continue
	else:
	    toattr = fromattr
	setattr(self, toattr, getattr(pkt, fromattr))



class Tracker:
    """Attributes of the desktop.

    The following attributes describe information about the desktop as
    a whole.  They should be considered read-only.  Information about
    individual windows are accessible through the public methods:

    window_count()  -- returns the number of windows
    get_windows()   -- return a list all known windows
    get_window(dbe) -- return window with db_entry `dbe' or None

    """
    x = 0				# Viewport X
    y = 0				# Viewport Y
    desk = 0				# Current desk
    max_x = 0				# Viewport Max X
    max_y = 0				# Viewport Max Y

    def __init__(self):
	self.__winsbydb = {}

    def __getwin(self, index):
	if self.__winsbydb.has_key(index):
	    w = self.__winsbydb[index]
	else:
	    w = Window()
	    self.__winsbydb[index] = w
	return w

    def feed_pkt(self, pkt):
	pktname = Fvwm.Types[pkt.msgtype]
	if pktname in ('M_NEW_PAGE', 'M_NEW_DESK'):
	    _insert(self, pkt)
	elif pktname in ('M_ADD_WINDOW', 'M_CONFIGURE_WINDOW',
			 'M_FOCUS_CHANGE', 'M_WINDOW_NAME'):
	    _insert(self.__getwin(pkt.db_entry), pkt)
	elif pktname in ('M_LOWER_WINDOW', 'M_RAISE_WINDOW', 'M_DEICONIFY',
			 'M_MAP', 'M_END_WINDOWLIST', 'M_ERROR',
			 'M_CONFIG_INFO', 'M_END_CONFIG_INFO',
			 'M_DEFAULTICON'):
	    pass
	elif pktname == 'M_DESTROY_WINDOW':
	    del self.__winsbydb[pkt.db_entry]
	elif pktname in ('M_ICONIFY', 'M_ICON_LOCATION'):
	    _insert(self.__getwin(pkt.db_entry), pkt,
		    x='icon_x', y='icon_y',
		    width='icon_width', height='icon_height')
	elif pktname == 'M_ICON_NAME':
	    _insert(self.__getwin(pkt.db_entry), pkt, name='icon_name')
	elif pktname == 'M_ICON_FILE':
	    _insert(self.__getwin(pkt.db_entry), pkt, name='icon_file')
	elif pktname == 'M_RES_CLASS':
	    _insert(self.__getwin(pkt.db_entry), pkt, name='res_class')
	elif pktname == 'M_RES_NAME':
	    _insert(self.__getwin(pkt.db_entry), pkt, name='res_name')
	else:
	    Fvwm.error('Unknown packet type: %s' % pktname)

    def window_count(self):
	return len(self.__winsbydb)

    def get_windows(self):
	return self.__winsbydb.values()

    def get_window(self, dbe):
	try:
	    return self.__winsbydb[dbe]
	except KeyError:
	    return None
