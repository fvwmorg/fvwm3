#! /usr/bin/env python

"""A quit verifier.

Written in Python using Fvwm.py.  This module brings up a window which
verifies whether you really want to quit the window manager or not.
This doesn't work as well as I'd like, since it seems that Tk's
tk_dialog script (of which Dialog.Dialog is just a thin veneer)
doesn't give you any way to do a global server grab.  I think you need
a global server grab to make this module truly effective.  Sigh.

Requires Python 1.4 and Fvwm 2.x.

Copyright 1996 Barry A. Warsaw <bwarsaw@python.org>

"""
__version__ = '1.0'

import os
import sys
import tempfile
import Fvwm
import Dialog
from Tkinter import *



if __name__ == '__main__':
    fvwm = Fvwm.FvwmModule(sys.argv)
    # the Dialog object doesn't do a proper global server grab, so we
    # need to use this kludge instead.  Yes, this is bogus, the
    # tempfile module doesn't exactly have the interface we want. :-(
    tempdir = os.path.split(tempfile.mktemp())[0]
    file = os.path.join(tempdir, 'Pyquiter-' + os.environ['DISPLAY'])
    if os.path.exists(file):
	fvwm.send('Beep', cont=0)
	sys.exit(0)
    #
    # create the lock file
    #
    try:
	fp = open(file, 'w')
	master = Tk(className='Pyquiter')
	master.withdraw()
	d = Dialog.Dialog(
	    master,
	    title='Confirm Fvwm Quit',
	    text='Do you really want to quit Fvwm?',
	    bitmap=Dialog.DIALOG_ICON,
	    default=0,
	    strings=('No (cancel)', 'Yes (quit)'))
	if d.num == 1:
	    fvwm.send('Quit', cont=0)
    finally:
	fp.close()
	os.unlink(file)
