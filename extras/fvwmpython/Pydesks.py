#! /usr/bin/env python

"""A CTWM desktop pager emulator.

Requires Python 1.4 and Fvwm 2.x.

Copyright 1996 Barry A. Warsaw <bwarsaw@python.org>

See <http://www.python.org/~bwarsaw/pyware/> for details.

"""
__version__ = '1.0'

import os
import sys
import string
from Tkinter import *
import Fvwm

_APPNAME = 'Pydesks'



class View:
    def __init__(self, master, controller, tracker, geom):
	self.__controller = controller
	self.__tracker = tracker
	self.__frame = Toplevel(master)
	if geom:
	    try: self.__frame.geometry(geom)
	    except TclError, msg:
		Fvwm.error('%s: %s' % (sys.exc_type, msg))
	self.__frame.bind('<Alt-q>', controller.quit)
	self.__frame.bind('<Alt-Q>', controller.quit)
	self.__deskno_var = IntVar()

    def __select_desk(self, event=None):
	deskno = self.__deskno_var.get()
	self.__controller.send('Desk 0 %d' % deskno)

    def add_desk(self, deskno, deskname):
	btn = Radiobutton(
	    self.__frame,
	    variable=self.__deskno_var,
	    text=deskname,
	    value=deskno,
	    anchor=W,
	    command=self.__select_desk)
	btn.pack(fill=X, expand=1)

    def update(self):
	self.__deskno_var.set(self.__tracker.desk)



class Controller(Fvwm.FvwmModule):
    __autogrow = 0
    __pagerkludge = 0

    def __init__(self, argv):
	Fvwm.FvwmModule.__init__(self, argv)
	self.__desknamebyno = {}
	self.__desknobyname = {}
	self.__tracker = self.get_windowlist()
	geom = self.__readconfig()
	#
	# get the desk to start on.
	#
	starton_no = None
	try:
	    starton_name = self.args[0]
	    starton_no = self.__desknobyname[starton_name]
	except IndexError:
	    pass
	except KeyError:
	    Fvwm.error('Unknown start on desk name was specified: %s' %
		       starton_name)
	    pass
	#
	# create the GUI
	#
	master = Tk(className=_APPNAME)
	master.protocol('WM_DELETE_WINDOW', self.quit)
	master.title(_APPNAME)
	master.iconname(_APPNAME)
	master.withdraw()
	self.__view = View(master, self, self.__tracker, geom)
	#
	# populate the desks
	#
	for deskno, deskname in self.__desknamebyno.items():
	    self.__view.add_desk(deskno, deskname)
	if starton_no is not None:
	    self.send('Desk 0 %d' % starton_no)
	self.set_mask()			# the Fvwm module's event mask

    def __readconfig(self):
	geom = None
	for config in self.get_configinfo().get_infolines(_APPNAME):
	    try:
		words = string.split(config)
		keyword = words[0]
		if keyword == '*PydesksDesk':
		    # *PydesksDesk Name Number
		    name = words[1]
		    number = string.atoi(words[2])
		    if self.__desknobyname.has_key(name):
			Fvwm.error('Ignoring duplicate desk name: %s' % name)
		    else:
			self.__desknamebyno[number] = name
			self.__desknobyname[name] = number
		elif keyword == '*PydesksAutoGrow':
		    self.__autogrow = 1
		elif keyword == '*PydesksFvwmPagerKludge':
		    self.__pagerkludge = 1
		elif keyword == '*PydesksGeometry':
		    geom = words[1]
		else:
		    Fvwm.error('Bad config line: %s' % config)
	    except (IndexError, ValueError):
		Fvwm.error('%s: %s' % (sys.exc_type, config))
	return geom

    #
    # interface for views and menus
    #

    def quit(self, event=None):
	sys.exit(0)

    #
    # packet handlers
    #

    def NewDesk(self, pkt):
	# TBD: FvwmPager has a very weird behavior in that it bogusly
	# sends `Desk 0 10000' messages.  So far, I think these are
	# unnecessary, but still, we have to work around this
	# behavior.
	deskno = pkt.desk
	if deskno == 10000 and self.__pagerkludge:
	    return
	if not self.__desknamebyno.has_key(deskno):
	    if self.__autogrow:
		deskname = 'Desk %d' % deskno
		self.__desknamebyno[deskno] = deskname
		self.__desknobyname[deskname] = deskno
		self.__view.add_desk(deskno, deskname)
	    else:
		return
	self.__do_updates(pkt)

    def NewPage(self, pkt):
	# work around for FvwmPager race condition kludge
	if pkt.desk == 10000 and self.__pagerkludge:
	    del pkt.desk
	self.__do_updates(pkt)

    def __do_updates(self, pkt):
	self.__tracker.feed_pkt(pkt)
	self.__view.update()

    # shut up debugging
    def unhandled_packet(self, pkt): pass



if __name__ == '__main__':
    Controller(sys.argv).start()
