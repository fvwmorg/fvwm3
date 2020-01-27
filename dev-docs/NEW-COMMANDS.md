# New Commands in FVWM3

This document attempts to aggregate the functionality of FVWM3 across a common
and consistant command set.

There is a hierarchical organisation to identifing windows within FVWM3.

```
Screen --> Desktops --> Windows
```

# Screen

This is equivalent to a monitor attached to a computer.  Screens are numbered
automatically, although can also be named.  The name of a monitor is taken
from RandR.

RandR is used in identifing attached monitors, to get their rotation, size,
position and name.  FVWM3 maintains a runtime list of monitors.  Customised
behaviour about monitors which are attached/detached will be exposed to the
user.

# Desktop

A desktop is attached to a screen.  These are viewports into a monitor, of
which there could be many.   Desktops are containers which hold windows.
Desktops are separate from each atrached screen, although it would be possible
to have one "global" desktop which operated across all screens.

Desktos are linear in their arrangement.  There is no concept of pages within
a desktop (as there is with FVWM2).

Desktops can be named -- and since desktops are unique per screen, desktops
can have the same name across screens.

# Windows

Created by applications, and belonging to one or more desktops on a particular
screen.

Windows can have names, or user-defined labels.

# Hierarchy and Specification

The following diagram shows a typical hierarchy:

```
             Screen1                                       Screen2
+---------------------------------------+  +-----------------------------------+
|     Desktop1             Desktop2     |  |              Desktop1             |
| +---------------+   +---------------+ |  | +--------------------------------+|
| | +------+      |   |  +---------+  | |  | |                                ||
| | | W1   |      |   |  |         |  | |  | |                                ||
| | |      |      |   |  |   W3    |  | |  | |      +----------------------+  ||
| | +------+      |   |  |         |  | |  | |      |                      |  ||
| |   +---------+ |   |  |         |  | |  | |      |                      |  ||
| |   |    W2   | |   |  +---------+  | |  | |      |                      |  ||
| |   +---------+ |   |               | |  | |      |                      |  ||
| +---------------+   +---------------+ |  | |      |                      |  ||
|             Desktop3                  |  | |      |     W1               |  ||
| +-----------------------------------+ |  | |      |                      |  ||
| | +--------------------------------+| |  | |      |                      |  ||
| | |                                || |  | |      |                      |  ||
| | |             W4                 || |  | |      +----------------------+  ||
| | +--------------------------------+| |  | |                                ||
| +-----------------------------------+ |  | +--------------------------------+|
+---------------------------------------+  +-----------------------------------+
```

There's two screens.  Specifying how to access `W4` could be done like this:

```
Screen1:Desktop 4.W4
```

Hence in the general case, the following specifier format would be consistent:

```
<screen>:<desktop>.<window>
```

# Operational Commands

Commands tend to operate on either screens, desktops, or windows.  Sometimes,
there will be target states which affect screens, desktops, or windows.

## Target/Source States

* `-s` -- this specifies the source, either a `screen`, `desktop`, or `window`
* `-t` -- this specifies the target, either a `screen`, `desktop`, or `window`

### Examples

* To move Window W4 to screen 2 (current desktop):

```
move-window -s screen1:desktop3.W4 -tscreen2:
```

* To swap two windows (position/size) -- W4 and W1:

```
select-window -s screen1:desktop3.W4 -t screen1:desktop1.W1
```

# Commands

Things which move things...

* Windows
* Pointer

Bindings:

* Key
* Mouse

Contextual:

* Desktop
* Window
  * Styles:  decor, etc.
