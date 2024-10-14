MONITOR HANDLING
================

Some random notes on the internal monitor and RandR handling in
Fvwm3.

Note that Fvwm3 identifies monitors by their output name.

Structure `monitor`
-------------------

Gets allocated by function `monitor_new`.

Important (and Less Important) Variables
----------------------------------------

-   `struct monitors monitors_q`

    RB-tree root of monitors, ordered by increasing (y, x)
    coordinates through function `monitor_compare`.  This tree
    keeps all monitors ever seen during Fvwm3's runtime.  Which
    means that it only ever grows in size.

    (As an RB-tree this structure does not allow for duplicate
    monitors with identical (x, y) coordinates.  One of the
    reasons why Fvwm3 does not fully support mirrored
    (`--same-as`) monitors.)

-   `struct monitorsold monitorsold_q`

    Tail queue of changed monitors.  Subset of the monitors in
    RB-tree `monitors`.

-   `int randr_event`

    RandR event base, to be substracted from X event types to
    determine the RandR event type

These variables get initialized in function
`libs/FScreen.c:FScreenInit`.  That function exits hard with
error code 101 on any errors during RandR (and general)
initialization.

-   `static struct monitor *monitor_global`

    Dummy monitor structure covering the complete display width
    and height.  It is not really relevant in terms of RandR
    processing.

Important (and Less Important) Functions
----------------------------------------

-   `fvwm/events.c:dispatch_event`

    Handles RandR events as follows:

    ```
    XRRUpdateConfiguration(e);
    monitor_output_change(e->xany.display, NULL);
    monitor_update_ewmh();
    monitor_emit_broadcast();
    initPanFrames(NULL);
    ```

    At least on some systems RandR fires each event twice, which
    this function attempts to filter.

    The call to `XRRUpdateConfiguration` is required to notify
    Xlib itself about the configuration change.

-   `libs/FScreen.c:scan_screens` (wrapped by
    `libs/FScreen.c:monitor_output_change`)

    Calls `XRRGetMonitors` to get the current *active* monitors
    and initializes or updates the monitors in `monitors_q`.

    Updates for each monitor from `monitors_q` its flags
    according to the monitor configuration:

    +   adds `MONITOR_NEW` for truly new monitors, which have not
        been seen before in `monitors_q`
    +   adds `MONITOR_CHANGED` for monitors that are already
        present in `monitors_q`, but with different position or
        dimension
    +   above two flags are mutually exclusive
    +   adds or removes `MONITOR_DISABLED` depending on whether
        the monitor is disabled or not
