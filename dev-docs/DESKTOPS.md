Desktop Thoughts
================

Fvwm3 has two modes for handling desktops:

* `global` -- all connected monitors share the same desktop
* `per-monitor` -- all desktops defined by `DesktopName` are separate from one
  another.

A third mode is suggested:  `shared`.


Shared would operate much like spectrwm does now:

So for example, let's say you had the following number of desktops:

```
0     1      2      3     4
```

... and let's say that you have two monitors.  You might have this:

```
[0]     1      2      <3>     4
```

Where `[]` is monitor1 and `<>` is monitor2.

You could move monitor1 (`[]`) all the way along to desktop 2, without changing where monitor2 (`<>`) is, as in:

```
0     1     [2]      <3>     4
```

If you were to then continue to move monitor1 (`[]`) to desktop 3, what would happen is this:

```
0     1     <2>      [3]     4
```

So that now, the desktops have switched over from each monitor.

FVWM3 Changes
=============

In order to provide this feature, fvwm3 needs to try and structure things such
that existing `DesktopConfiguration` modes are not broken by this
implementation.  Unlike with other modes, this `shared` mode is not a
structure suited to isolation per-monitor, thus this breaks the conceptual
model.

In order for `fvwm3` to sustain all three models, the following should be
true:

* When setting `shared`:

1. Initialise the shared desktop environment.
2. Existing windows per desktop should be migrated to the share env.
3. All monitors should be scanned.  They need to be placed on different desktops.
4. On a switch to a new desktop (regardless of page) if a desktop:
   * Is mapped:
     * Swap the desktop:
       * Mark the windows on the current monitor
       * Move the windows on the *other* monitor to this monitor's desk
       * Switch desktops
       * Move the marked windows from this monitor to the other monitor.
