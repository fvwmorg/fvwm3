#! /usr/bin/env python

"""A replacement for FvwmIdentify.

Written in Python using Fvwm.py.  There are two ways to use this
module: if you run it as a script, it will prompt the user to select a
window to identify by changing the cursor to a crosshatch.  Once a
window is selected, a window will be displayed with identifying
information.

Alternatively, you can just create instances of IdWindow by passing in
a Tk master widget and a Tracker.Window instance.

"""

__version__ = '1.0'

import Fvwm
from Tkinter import *



class IdWindow:
    def __init__(self, master, w):
	try: gravity = Fvwm.Gravity[w.gravity]
	except KeyError: gravity = 'Unknown'
	self.__master = master
	if w.flags & Fvwm.Flags['ICONIFIED']:
	    labels = [
		('Class:', w.res_class, None),
		('Resource:', w.res_name, None),
		('Window ID:', hex(w.top_id), None),
		('Desk:', w.desk, None),
		('Icon Name:', w.icon_name, None),
		('Icon X:', w.icon_x, None),
		('Icon Y:', w.icon_y, None),
		('Icon Width:', w.icon_width, None),
		('Icon Height:', w.icon_height, None),
		('Sticky:', w.flags, 'STICKY'),
		('On Top:', w.flags, 'ONTOP'),
		('Has Title:', w.flags, 'TITLE'),
		('Iconified:', w.flags, 'ICONIFIED'),
		('Gravity:', gravity, None),
## 		('Geometry:', w.geometry),
		]
	else:
	    labels = [
		('Name:', w.name, None),
		('Class:', w.res_class, None),
		('Resource:', w.res_name, None),
		('Window ID:', hex(w.top_id), None),
		('Desk:', w.desk, None),
		('X:', w.x, None),
		('Y:', w.y, None),
		('Width:', w.width, None),
		('Height:', w.height, None),
		('Boundary Width:', w.border_width, None),
		('Sticky:', w.flags, 'STICKY'),
		('On Top:', w.flags, 'ONTOP'),
		('Has Title:', w.flags, 'TITLE'),
		('Iconified:', w.flags, 'ICONIFIED'),
		('Gravity:', gravity, None),
## 		('Geometry:', w.geometry),
		]
	textw = valuew = 0
	massaged = []
	for l in labels:
	    text = l[0]
	    value = l[1]
	    flag = l[2]
	    if flag:
		if value & Fvwm.Flags[flag]: value = 'Yes'
		else: value = 'No'
	    if value is None:
		value = ''
	    if type(value) == IntType:
		value = `value`
	    textw = max(textw, len(text))
	    valuew = max(valuew, len(value))
	    massaged.append((text, value))
	for text, value in massaged:
	    frame = Frame(master)
	    frame.pack()
	    label = Label(frame, text=text, anchor=E, width=textw)
	    label.pack(side=LEFT)
	    value = Label(frame, text=value, anchor=W, width=valuew)
	    value.pack(side=LEFT)

    def show(self): self.__master.deiconify()
    def hide(self): self.__master.withdraw()



if __name__ == '__main__':
    import sys
    raise RuntimeError, 'Sorry.  Running %s as a script is not yet supported' \
	  % sys.argv[0]
