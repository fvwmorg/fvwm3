Note, the changes for the last STABLE release start with release
2.6.0.

-------------------------------------------------------------------
Changes in stable release 2.6.9 (UNRELEASED)

* Bug fixes:

  - Fix handling of configure's --enable-mandoc/--enable-htmldoc

* New fvwm features:

-------------------------------------------------------------------
Changes in stable release 2.6.8 (31-Mar-2018)

* Bug fixes:

  - Various DESTDIR fixes (especially around the default-config
    Makefile)
  - fvwm-perllib pod2man fixes.
  - FvwmIconMan no longer triggers a warning about bad size hints
    in the fvwm core.
  - VMware windows do not disapper when switching between
    fullscreen and normal state.
  - Fix (de)installation with the configure options
    --program-prefix, --program-suffix and
    --program-transform-name.
  - Remove further references to obsolete modules from man page(s).

* New fvwm features:
  
  - fvwm-menu-desktop(1) now requires python3 as an explicit
    dependency.
  - Add a 'fullscreen' option to the 'Maximize' command.

-------------------------------------------------------------------
Changes in stable release 2.6.7 (06-Mar-2016)

* New fvwm features:

  - A new default configuration which is available when fvwm
    doesn't detect a configuration file to load.
  - A new conitional command "Desk n" can restrict matching
    windows to a specific desk.
  - A new conditional command "Screen n" to restrict matching
    windows on a given Xinerama screen.
  - New expansion variable "w.screen" to ascertain the Xineram
    screen number a window is on.
  - New command "InfoStoreClear" to remove all items in the
    InfoStore.

* Removed features:

  - The old and unmaintained debian/and rpm/ directories have
    been remmoved; use the maintainers' copies where available.
  - VMS support has been removed.
  - GTK1.x support has been removed.
  - GNOME-specific window hints (pre-EWMH) have been removed.
  - Some fvwm modules have been removed:
      - FvwmDragWell   (no replacement)
      - FvwmGTK        (no replacement)
      - FvwmSave       (no replacement)
      - FvwmSaveDesk   (no replacement)
      - FvwmScroll     (no replacement)
      - FvwmTabs       (no replacement, never worked anyway)
      - FvwmTaskBar    (use FvwmButtons)
      - FvwmTheme      (in core of fvwm as colorsets)
      - FvwmWharf      (use FvwmButtons)
      - FvwmWinList    (use WindowList command)
      - FvwmWindowMenu (use WindowList command)
      - FvwmIconBox    (use the IconBox style instead)

* New module features:

  - FvwmButtons learned a new option "Colorset" to its
    ChangeButton command.

* fvwm-menu-desktop updated:

  - Renamed default menu to XDGMenu and changed the name
    of the FvwmForm to FvwmForm-XDGMenu-Config to not conflict
    with someone already using FvwmMenu.
  - fvwm-menu-desktop will now load defaults from the
    FvwmForm-XDGMenu-Config data file.
  - Improved dynamic menus by regenerating them on-the-fly.
  - Added new options: --regen-cmd, --dynamic, and more.

* Bug fixes:

  - A bug introduced in 2.6.6 could cause applications with
    negative coordinates to be placed at strange positions.  This
    affected for example acroread when switching to fullscreen
    mode.  This has been fixed.
  - FvwmButtons "Silent" option for dynamic ChangeButton commands
    no longer loops infinitely.

Changes in stable release 2.6.6 (15-Mar-2016)

* New fvwm features:

  - Support for Russian from Ivan Gayevskiy.
  - EnvMatch supports infostore variables.
  - The option "forget" to the Maximize command allows to
    unmaximize a window without changing its size and position.
  - Windows shaded to a corner (NW, NE, SW, SE) are reduced to a
    small square.
  - New option "!raise" to the WarpToWindow command.
  - The new extended variables $[wa.x], $[wa.y], $[wa.width],
    $[wa.height] can be used to get the geometry of the EWMH
    working area, and $[dwa.x], $[dwa.y], $[dwa.width],
    $[dwa.height] can be used to get the geometry of the EWMH
    dynamic working area.
  - The Resize commands accept "wa" or "da" as a suffix of the
    width or height value.  If present, the value is a percentage
    of the width or height of the EWMH working area or the EWMH
    dynamic working area.
  - Fvwm is much more resilient against applications that flood the
    window manager with repeated events.

* New module features:

  - FvwmForm supports separator lines.
  - New FvwmIconMan options:
      IconAndSelectButton
      IconAndSelectColorset

* Bug fixes:

  - Provide a wrapper for the deprecation of XKeycodeToKeysym and
    use XkbKeycodeToKeysym() where appropriate.
  - fvwm-menu-desktop is re-written and provides better support of
    the XDG menu specification.
  - Fix fvwm-menu-desktop keyError bug.  Use "others" if no desktop
    environment found.
  - FvwmIconMan had problems displaying the hilight colour on some
    systems (64 bit issue?).
  - Globally active windows cannot take the focus if the style
    forbids programs to take focus themselves (style
    !FPFocusByProgram).
  - Windows no longer jump from one position to the other which
    could happen in some cases with SnapAttraction.  Windows now
    snap to the closest window (or screen edge).
  - Removing bindings had several strange side effects that are
    fixed now (removing too many bindings; old bindings showing up
    again after another is removed; possibly other effects).
  - Windows sometimes did not get expose events (i.e. did not
    redraw properly) if they were uncovered by moving a window
    above them.  This has been fixed.
  - FvwmConsole now causes much less network traffic.
  - Suppress bogus events sent to the modules when a window is
    resized with the mouse.
  - Properly handle the has_ref_window_moved flag for ResizeMove and
    ResizeMoveMaximize.
  - Removed some unnecessary redraws in FvwmPager.
  - The option "NoDeskLimitY" option of the GotoPage command did
    not work.
  - Negative coordinates in the "rectangle" option to the Menu
    commend did not work correctly.  This has been fixed.
  - Removes a slight graphics problem whith the ResizeMaximize
    command being invoked from a window button menu.
  - When an attempt to reparent a client window (i.e. decorate
    it) fails, fvwm no longer throws away all events but only the
    events for that window.
  - The ChangeButton command of FvwmButtons used to strip
    whitespace from the beginning and end of button titles and
    image paths.  This is no longer done.

-------------------------------------------------------------------
Changes in stable release 2.6.5 (20-Apr-2012)

* New features:

  - FvwmPager now wraps window names if SmallFont is set.

* Bug fixes:

  - Made signals registered with FVWM unblocking.
  - The "UnderMouse" option to PositionPlacement now honors the EWMH
    working area by default, if it's in use.
  - FvwmButtons handles the deleted window correctly for sub-windows.

-------------------------------------------------------------------
Changes in stable release 2.6.4 (01-Feb-2012)

* New features:

  - FvwmIconMan no longer allows for itself to be transient by using:

    Module FvwmIconMan Transient

    This is too ambiguous with valid module alias names, hence a
    transient FvwmIconMan must be created with "-Transient" as in:

    Module FvwmIconMan -Transient
  - New command InfoStore -- to store key/value pairs of information
    to relieve the burden of the SetEnv command.
  - Speed-up improvements for fvwm-menu-desktop when generating XDG
    menus by removing unnecessary stat(2) calls.
  - BusyCursor and CursorStyle have been set to make the cursor look
    like a dot during Read/PipeRead commands, by default.

* Bug fixes:

  - FvwmRearrange now understands the "ewmhiwa" option when placing
    windows.
  - Client gravity for subwindows is honoured for reparenting
    windows, such as with XEmbed, when changing the parent's
    geometry.
  - Conditional command processing of !Layer n, as in:

        Next (Xteddy, !Layer 4) Echo cuddles

    has now been fixed.
  - Fixed handling of swallowed windows for transient FvwmButtons'.
  - fvwm-menu-desktop now looks in /usr/share/applications for KDE
    legacy mode.
  - fvwm-config no longer accepts "--is-stable", "--is-released",
    "--is-final".

-------------------------------------------------------------------
Changes in stable release 2.6.3 (30-Sep-2011)

* New features:

  - New Style commands:

        TitleFormat
        IconTitleFormat

        Used to specify a window's visible name.  These new options
        deprecate the older styles:

        IndexedWindowName
        IndexedIconName
        ExactWindowName
        ExactIconName

  - New Resize argument to direction:  "automatic" which works out,
    automatically, which direction a window should be resized in
    based on which quadrant the pointer is in on the window -- this
    is best observed by the grid formed from "ResizeOutline", as the
    shape of this grid is used when calculating this.

    Furthermore, the existing option of "warptoborder" can be used in
    conjunction with "direction automatic" to warp the pointer to the
    appropriate window edge.

  - Couple of changes to the Move command:
        - The EWMH working area is now honoured by default.
        - To move a window ignoring the working area, the option
          "ewmhiwa" can be used, similar to how the Maximize
          command works.

  - New expansion placeholder $[pointer.screen] to return the screen number
    the pointer is on.

* Bug fixes:

    - A few minor ones; nothing user-visible per se.

-------------------------------------------------------------------
Changes in stable release 2.6.2 (06-Aug-2011)

* New features:

  - New MenuStyle "UniqueHotkeyActivatesImmediate" and its
    negation "!UniqueHotkeyActivatesImmediate" to retain the menu
    until the user presses one of the menu keybindings to enact
    that change in the case where there's only one item defined in
    the menu at the specified hotkey.

* Bug fixes:

  - Fix infinite loop crash when FvwmButtons quits/restarts with
    RootTransparent colorsets.
  - Fix setting window states across FVWM restarts where those
    windows do not set WM_COMMAND.
  - Windows are now correctly restored when coming out of
    fullscreen mode.

-------------------------------------------------------------------
Changes in stable release 2.6.1 (16-Apr-2011)

* New features:

  None.

* Bug fixes:

  - Make the version and check of Fribidi >= 0.19.2
  - fvwm-menu-desktop:
        - Make XML::Parser check runtime as it's a non-standard
          module.
        - Fix perl version check.

-------------------------------------------------------------------
Changes in stable release 2.6.0 (15-Apr-2011)

* New features:

  - Support libpng 1.5.0's slightly newer API.

* Bug fixes:

  - Fix width of FvwmTaskBar to fit on screen properly by using
    the correct module information to determine the border size.
  - Fix resizing shaded windows with a shade direction that does
    not match the window's gravity.  Shaded windows might have
    jumped to strange positions when being resized by the
    application.
  - FvwmIconMan now accepts an optional module alias when running
    in transient mode.
  - Use of the NoUSPosition style is now properly reported by
    "BugOpts ExplainWindowPlacement on".
  - Fix the CursorMove command to no longer move the pointer beyond
    the boundary of a page, if EdgeScroll was set to disable page
    flipping.
  - Fix NeverFocus windows from taking focus when opening menus,
    etc.
  - Plus *MANY* other fixes, improvements, etc.
-------------------------------------------------------------------

-------------------------------------------------------------------

Changes in beta release 2.5.31 (09-Aug-2010)

* New features:

* Bug fixes:

  - fvwm-convert-2.6:  Don't double-up comma separated options to
    conditional commands if they're already delimited by commas.
  - Correctly report a window's height and width if the window has
    incomplete resize increment set.
  - Maintain any State hints on a window when used with
    WindowStyle.
  - FvwmIconMan now correctly handles sticky windows.

-------------------------------------------------------------------

Changes in beta release 2.5.30 (09-May-2010)

* New features:

  - Support libpng 1.4.0's slightly newer API.

* Bug fixes:

  - Don't lazy match AnyContext when printing out bindings and the
    contexts they apply to with "PrintInfo Bindings".

-------------------------------------------------------------------

Changes in beta release 2.5.29 (03-Apr-2010)

* New features:

   - Added new fvwm-convert-2.6 script to convert older fvwm 2.4.x
     config files.
   - New BugOpts option QtDragnDropWorkaround to work around an
     oddity in handling drag-n-drop events to Qt applications.

* Bug fixes:

   - Fixed the InitialMapCommand style from running when FVWM is
     restarting.
   - Fix rendering of FvwmForm windows when initially mapped.
   - Fix placement of windows when using MinoverlapPlacement and
     friends.
   - Fix segfault when tearing off menus using a Pixmap
     background.
   - Fix "window jump" bug when moving a window across page
     boundaries.
   - Flush property events for same type in applications which
     repeatedly set the same XAtom.  (Gnucash, Openoffice, etc.)
   - Fix opening of files using Read/PipeRead to accept paths in
     the form "./" to indicate CWD.  Fixes
       $ fvwm -f ./some-fvwm2rc
   - Fix further crash when copying menustyles with MenuFace
     involving pixmaps.
   - Make layer changes apply immediately via Style commands for
     any currently mapped windows.
   - Fix flickering/incorrect location of the GeometryWindow with
     Xinerama/TwinView when resizing windows.

-------------------------------------------------------------------

Changes in beta release 2.5.28 (20-Sep-2009)

* New features:

   - New differentiated options for SnapAttraction when snapping
     against screen edges:
       "None", "ScreenWindows", "ScreenIcons", "ScreenAll"
   - New option to the BugOpts command: TransliterateUtf8.

* Bug fixes:

   - Fixed non-visible Qt windows after a Qt deferred map (e.g.
     Skype profile windows).
   - Fixed the use of the X-resource "fvwmstyle".
   - Fixed segmentation fault in FvwmEvent when parsing an
     undefined event name, or an undefined environment variable
     to the RPlayHost option.
   - Fixed the events startup, shutdown and unknown in FvwmEvent.
   - Fvwm now retains utf8 window names when the WM_NAME changes,
     and the utf8 name converted to the default charset match
     the old WM_NAME.
   - Fixed the options RPlayVolume and RPlayPriority in FvwmEvent.
   - Fixed SnapAttraction: Option SameType/Icons/Windows did
     falsely not affect conditions of option "Screen" and option
     "SameType" snapped falsely icons and windows together.
   - Fixed a problem where modules would get incorrect stacking
     information if many windows were restacked at the same time.
   - Fixed BugOpts parsing of more than one option at a time and
     restoring of default value for the last option
     in the command line when omitted.
   - 64 bit fix for setting EWMH _NET_WM_ICON property on windows.

-------------------------------------------------------------------

Changes in beta release 2.5.27 (23-Feb-2009)

* New features:

   - New extended variable
       $[w.visiblename]

   - Style matching now honours the window's visible name which
     means it can be matched before the real name, hence:

        Style $[w.visiblename] Colorset 5

        is now honoured.  Useful with IndexedWindowName as a style
        option.

   - New style InitialMapCommand allows to execute any command
     when a window is mapped first.

   - New option to PrintInfo, "bindings" which prints out all of
     the Key, PointerKey, Mouse and Stroke bindings which fvwm
     knows about.

* Bug fixes:

   - Fixed compilation without XRender support.
   - Fixed handling of _NET_MOVERESIZE_WINDOWS requests.
   - Fixed a bug in automatic detection mode of the style
     MoveByProgramMethod.
   - Fixed png detection when cross compiling.
   - Fixed keeping fullscreen window mode over a restart.
   - The style PositionPlacement now honours StartsOnPage.
   - Reset signal handlers for executed modules and programs.
   - Try to handle a bug an old version of the EWMH spec by
     honouring both, the _NET_WM_STATE_MAXIMIZED_HORIZ hint and
     the _NET_WM_STATE_MAXIMIZED_HORZ hitn.
   - Fix the GNOME location prefix to application .desktop files.
   - 'PositionPlacement UnderMouse' now forces the window
     on-screen, just like the old UnderMousePlacement style option
     used to.

-------------------------------------------------------------------

Changes in beta release 2.5.26 (7-May-2008)

* New features:

   - New MenuStyle option VerticalMargins.

* New module features:

   - FvwmButtons: New button alignment option: top.

* Bug fixes:

   - Fixed crash in ARGB visual detection code.
   - Fixed compilation without XRender support.
   - Fixed drawing of background pictures in menu items and titles.
   - Fixed handling of shaped windows.
   - Fixed a 64-bit bug in the EWMH code.

-------------------------------------------------------------------

Changes in beta release 2.5.25 (26-Feb-2008)

* New features:

   - Handle the STATE_ADD command of the EWMH _NET_WM_STATE
     message from version 1.3 of the EWMH spec.
   - Support transparency in ARGB windows

* Bug Fixes:

   - Fixed problem with windows disappearing when created
     unless the style Unmanaged was used.
   - Edge move delay was used as resistance for the top edge.
   - Fixed a parsing problem of the screen argument of the
     SnapAttraction style.
   - Some html documentation files were not installed.
   - Fixed a memory leak in internationalized font handling.
   - Fixed a bug in MinOverlap placement.
   - Fixed the StickyAcrossPages style in the FvwmPager.
   - Fixed the determination of the X charset on UTF-8 systems.
   - Fixed a crash when certain EWMH messages were sent to
     unmanaged windows.
   - Fixed a memory leak in multibyte codepage code.
   - Ignore the EWMH staysontop and staysonbottom hints if the
     EWMHIgnoreStackingOrderHints style is used.
   - Fixed a sporadic crash when the root background set by gnome,
     fvwm-root, esetroot etc. changes and a root transparent
     colour set is used.
   - Fixed spradic crash in modules with root transparent
     background from colour sets.
   - Fixed a possible crash if the last active module fails.

-------------------------------------------------------------------

Changes in beta release 2.5.24 (24-Nov-2007)

* New features:

   - Disabled paging during interactive resize operations by
     default (see 2.5.20) as it is annoying to many people.
   - New style command options:
       EdgeMoveResistance
       EdgeMoveDelay
       EdgeResizeDelay
       SnapGrid
       SnapAttraction
     that replace the now obsolete commands EdgeResistance,
     SnapGrid and SnapAttraction.  The EdgeResistance command has
     a new syntax with only one argument.
   - New command MenuCloseAndExec for menu bindinngs that can be
     used to trigger certain commands from a menu without an
     associated item.  For example, with
       Key F1 MTI[]-_ A MenuCloseAndExec Menu RootMenu
     the RootMenu can be opened from any other menu by pressing
     F1.

* Bug Fixes:

   - Sometimes a window jumped by half the screen's size when
     moving with the mouse and hitting the border of the desktop.
   - Fixed the "screen w" argument of the Move and other commands.
   - Clicking on a menu title did not close the menu by default.
   - Temporary files in FvwmPerl overwrote each other.

-------------------------------------------------------------------

Changes in beta release 2.5.23 (1-Sep-2007)

* New features:

   - New Style command options:
       StartShaded

* Bug Fixes:

   - Fixed FvwmButton's button placement algorithm broken in
     2.5.22.

-------------------------------------------------------------------

Changes in beta release 2.5.22 (29-Aug-2007)

* New features:

   - New Style command options:
       UnderMousePlacementHonorsStartsOnPage
       UnderMousePlacementIgnoresStartsOnPage
       !MinOverlapPlacementPenalties
       !MinOverlapPercentPlacementPenalties
       MinWindowSize
   - SVG (scalable vector graphics) image loading support.
   - New extended variables
       $[w.iconfile.svgopts]
       $[w.miniiconfile.svgopts].
   - Added suffix 'w' to the arguments of the Move command and
     similar.  It is now possible to add multiple shifts to a
     window position, e.g. "50-50w 50-50w" for the center of the
     screen.
   - Removed UnderMousePlacement and CenterPlacement.  Use
     "PositionPlacement Center" and "PositionPlacement UnderMouse"
     instead.
   - Documentation in HTML format.
   - Replaced "UseListSkip" with "UseSkipList" & "OnlyListSkip" with
     "OnlySkipList" in WindowList command. (Old options deprecated.)
   - New subject ImageCache for PrintInfo command.
   - The new commad EchoFuncDefinition prints a function's
     definition to the console for debugging purposes.
   - The CursorStyle command can now load PNG and SVG images as
     mouse cursors.  New x and y arguments to specify the
     hot spot.  Also, it is now possible to load non-monochrome
     cursors and cursors with partial transparency.

* New module features:

   - FvwmScript: New instructions: ChangeWindowTitle and
     ChangeWindowTitleFromArg.

* Bug Fixes:

   - Windows with aspect ratio no longer maximize past the screen
     edges.
   - Fixed CenterPlacement with Xinerama screens.
   - Fixed CascadePlacement with title direction west and east
   - Windows no longer unstick when going to fullscreen mode.
   - Fixed crash when raising/lowering a destroyed window.
   - Fixed expansion of $[n-] and $[*], broken in 2.5.20.
   - Fixes for resizing shaded windows and windows with a gravity
     other than northwest.
   - Fixed CursorStyle POSITION, broken since 2.3.24.
   - The hi, sh and fgsh colors in colorsets are no longer replaced
     by computed values if not explicit set on the same line as the
     bg, or for fgsh fg, changes. (bug #3359)
   - FvwmButtons now redraws stretched button backgrounds correctly
     on partial expose.
   - Windows with circular transient for hints may no longer crash
     fvwm with StackTransientParent style.
   - FvwmPager now displays windows that are StickyAcrossPages
     correctly.
   - Fixed a possible crash with modules closing down.
   - Fixed broken demo script fvwm_make_browse_menu.sh.
   - The conditon following a comma separator without whitespace
     padding was previously ignored if the presiding condition was
     multi-worded.
   - Various FvwmButtons drawing problems.
   - Window movement or resizing triggered by an EWMH message now
     honours the FixedSize and FixedPosition window styles.
   - Properly generate leave_window and enter_window events for
     the root window in FvwmEvent.
   - Fixed crash in UTF8 code.
   - Fixed parsing of the PropertyChange command.
   - Fixed windowlist crash when combining CurrentAtEnd with
     IconifiedAtEnd and all windows are iconified.

-------------------------------------------------------------------

Changes in beta release 2.5.21 (20-Jan-2007)

* New features:

   - The command Scroll can now be used for interactive scrolling.

* Bug Fixes:

   - Fixed Tile...Placement styles (SmartPlacement) that were
     broken in 2.5.20.

-------------------------------------------------------------------

Changes in beta release 2.5.20 (15-Jan-2007)

* New features:

   - New Style options: StippledIconTitle, !StickyStippledTitle,
     and !StickyStippledIconTitle.
   - Full support for menu context (M) key and mouse bindings.  See
     the section Menu Bindings in the man page for details.
   - Hilighted menu backgrounds now use pixmaps gradients and
     transparency from their related colorset.
   - New window conditions: StickyIcon, StickyAcrossPagesIcon and
     StickyAcrossDesksIcon.

* Changed features:

   - "Mouse n M N" is no longer used to disable or remap the
      builtin tear off menu button. See the section Tear Off Menus
      for details on replacement commands.

* Bug Fixes:

   - FvwmWinList: fix problem with window name/button mixups during
     Init/Restart of fvwm. (bug #1393)
   - It is now possible to switch the viewport while resizing
     windows if "EdgeScroll 0 0" is set.
   - Fixed disappearing windows when aborting interactive resizing
     of maximized windows when unmaximizing them later.
   - Fixed disappearing windows when moving maximized windows and
     unmaximizing them later.
   - Fixed calculation of final location with MoveToPage and
     MoveToScreen with windows to the left or top of the viewport.
   - 64-bit architecture fix in FvwmProxy.
   - FvwmForm now work with balanced quoted command for Timeout.
   - FvwmPager correctly updates on window desk change.
   - FvwmIconBox: fixed problem with IconColorset's background
     color change not being applied immediately.
   - Allow FvwmConsole to run a terminal via rxvtc or urxvtc.
     FvwmConsole dies if no client connects within one minute.
   - Expansion of variables in FvwmTaskBar launch button commands
     fixed.
   - Fixed a race condition with applications raising their own
     transient windows in certain ways. (Apple Shake, kphotoalbum)
   - FvwmIdent reports the correct geometry if the window has its
     title at the left or right side.
   - Fixed an infinite loop when deiconifying windows in a group
     via a different window than the initially iconified.

-------------------------------------------------------------------

Changes in beta release 2.5.19 (9-Dec-2006)

* Bug Fixes:

   - FvwmCommand now reports "end windowlist" and "end configinfo".
   - FvwmCommand now prints config info split on lines.
   - FvwmTaskBar no longer gets lost with trailing whitespace after
     geometry specification.
   - Fixed a window size problem if the aspect ratio is set (e.g.
     mplayer).
   - Decorations now update when unmanaged windows take focus, and
     not FlickeringQtDialogsWorkaround is enabled.
   - FvwmPager again allows movement of windows added before a
     page change.
   - fvwm no longer crashes on 1 and 4 bit displays. (#1677)
   - EWMH desktops now correctly handles FPClickToFocus. (#1492)
   - Security fix in fvwm-menu-directory. (CVE-2006-5969)

-------------------------------------------------------------------

Changes in beta release 2.5.18 (11-Sep-2006)

* Bug Fixes:

   - If a window started fullscreen, leaving fullscreen state now
     properly unmaximizes and resizes the window.
   - Fixed the ForeColor/HilightFore styles that were broken in
     2.5.17.
   - FvwmPager can now move icons with the !IconTitle style.
   - Fixed drawing of icons that are moved to a different desk.
   - FvwmPager no longer tries to move non-movable windows.
   - FvwmPager now moves all windows as user requests.
   - FvwmPager no longer displaces windows with title and border
     sizes on moves.
   - TestRc now correctly matches Break, and $[cond.rc] is
     expanded for Break.
   - Fixed several 64-bit architecture problems with
     XGetWindowProperty().  Xine works much better on 64-bit
     machines.
   - Fixed handling of ClickToFocusPassesClick with the EWMH
     desktop (e.g. using nautilus).
   - Fixed handling of windows that are unmapped and mapped again
     too fast (e.g. fpga_editor).

-------------------------------------------------------------------

Changes in beta release 2.5.17 (19-Jul-2006)

* New features:

   - New MenuStyle options TitleFont, TitleColorset and
     HilightTitleBack.
   - New command PressButton in module FvwmButtons for being able
     to emulate button press via other means than the mouse.
   - New wrap options to EdgeScroll command for wrapping with pixel
     distances.
   - New Style option UnderMousePlacement.
   - Unused arguments to Style options generate warnings.
   - The name style names match against can be augmented by the
     X-resource "fvwmstyle".
   - New options, Reverse and UseStack, to All command.
   - WindowShade can now reshade windows using the Last direction.
   - Positional parameters to complex functions can now be expanded
     using $[n], $[n-m], $[n-] and $[*] expressions.
   - The width and height arguments of the Resize command now
     accept the prefix 'w' to allow resizing relative to the
     current window size.
   - New command ModuleListenOnly.
   - New "Periodic" option added to Schedule command.

* Bug Fixes:

   - Fixed detection of running non-ICCCM2 wm (bug #3151).
   - Fixed drawing of menus with the sidepic on the right.
   - EdgeScroll no longer divides pixel distances >1000 by 1000.
     (bug #3162)
   - The configure script can now cope with four-part version
     numbers when detecting some libraries.
   - The WarpToWindow command followed by Move in a complex
     function now uses the correct pointer position.
   - The menu style TitleWarp does no longer warp the pointer for
     root menus (as it is documented).
   - Fixed detection of safe system version of mkstemp.
   - Fixed the conditions Iconifiable, Fixed, FixedSize,
     Maximizable and Closable.
   - Fixed problem with window outline and placement position
     running out of sync.
   - FvwmConsole no longer conflicts with Cygwin stdio (bug #3772).
   - FvwmGtk now configures correctly on Cygwin (bug #3772).
   - Fixed tempfile vulnerabilities in FvwmCommand.

-------------------------------------------------------------------

Changes in beta release 2.5.16 (20-Jan-2006)

* New features:

   - If the pointer can not be grabbed in functions, a message is
     printed to the console instead of beeping.

* Bug Fixes:

   - Fixed a couple of build problems introduced in 2.5.15.

-------------------------------------------------------------------

Changes in beta release 2.5.15 (14-Jan-2006)

* New features:

   - Variables can be nested, like $[desk.name$[desk.n]].
   - Obsolete one-letter variables work, but generate warnings now.
   - Windows can be placed by any button (now also >3).
   - It is now possible to redefine the buttons usable to finish
     window movement and manual placement.
   - New window condition PlacedByButton.
   - MenuStyle pairs can be negated by prefixing '!'.
   - New generic tabbing module - FvwmTabs.
   - New Style option: EWMHIgnoreWindowType.
   - New MenuStyle options: MouseWheel, ScrollOffPage and
     TrianglesUseFore.
   - To compile from CVS, autoconf-2.53 or above is now required.
     This does not affect compiling the released tarballs.
   - New option "screen" to Move and ResizeMove commands to allow
     specifying the target Xinerama screen.

* Bug Fixes:

   - Supported a new fribidi version 0.10.5 in addition to 0.10.4.
   - Better look for windows with "BorderStyle TiledPixmap".
   - Some EWMH-related 64-bit fixes.
   - Fixed segmentation fault when replacing title of title only
     menus (Bug #1121).
   - Fixes for resizing of shaded windows and resizing/moving
     windows with complex functions.

-------------------------------------------------------------------

Changes in beta release 2.5.14 (24-Aug-2005)

* New features:

   - Fvwm now officially supports 64-bit architectures.
   - New Test conditions EnvIsSet, EnvMatch, EdgeHasPointer and
     EdgeIsActive.
   - New window condition FixedPosition.

* New module features:

   - FvwmPerl module supports window context when preprocessing.
   - FvwmPerl module accepts new --export option that by default
     defines two fvwm functions "Eval" and ".", to be used like:

       FvwmPerl -x
       Eval $a = $[desk.n] - 2; cmd("GotoDesk 0 $a") if $a >= 0
       . Exec xmessage %{2 + cos(0)}%    # embedded calculator

   - New FvwmProxy option ProxyIconified.
   - New FvwmTaskBar option Pad to control the gap between
     buttons.

* Bug Fixes:

   - Fixed a Solaris compiler error introduced in 2.5.13.
   - Fixed a hang with layers set by applications (e.g. AbiWord).
   - GotoDesk with a relative page argument now wraps around at
     the end of the given range as documented. (Bug #1396).
   - PopupDelayed menu style option was not copied on
     CopyMenuStyle.
   - Transparent Animated menus with non-transparent popup were
     not animated correctly.
   - Supported euc-jp class of encodings.
   - A window's default layer is no longer set to 0 during a
     restart.
   - Fixed an annoying MouseFocus/SloppyFocus problem in
     conjunction with EdgeResistance + EdgeScroll (sometimes a
     window did not get the focus as it should have). This
     problem first occured in 2.5.11.

-------------------------------------------------------------------

Changes in beta release 2.5.13 (16-Jul-2005)

* Bug Fixes:

   - The MoveToPage command did not work without arguments in
     2.5.11 and 2.5.12.
   - Mouse/Key command no args possible core dump.
   - Direction with no args possible core dump.
   - FvwmScript periodic tasks run too often.
   - Perl modules did not work on 64 machines.
   - FvwmDebug did not report any extended messages.
   - fvwm-menu-desktop supports mandriva.
   - fvwm-menu-desktop when verifying executable, allow full path.

* New module features:

   - FvwmIconMan: MaxButtonWidth and MaxButtonWidthByColumns
     options.
   - FvwmIconMan: added tool tips with Tips, TipsDelays, TipsFont,
     TipsColorset, TipsFormat, TipsBorderWidth, TipsPlacement,
     TipsJustification and TipsOffsets options.
   - FvwmButtons: PressColorset & ActiveColorset options for
     _individual_ buttons.

-------------------------------------------------------------------

Changes in alpha release 2.5.12 (6-Oct-2004)

* New commands:

   - EdgeLeaveCommand

* New module features:

   - FvwmIconMan: ShowOnlyFocused option.

-------------------------------------------------------------------

Changes in alpha release 2.5.11 (30-Sep-2004)

* Multiple window names can be specified in conditions.

* Window-specific key/mouse bindings. (Bindings no longer have to
  be global.)

* The default fvwm configuration files are now: ~/.fvwm/config and
  $FVWM_DATADIR/config. Five previously used config file locations
  are still searched as usual for backward compatibility.

* New extended variables $[w.desk] and $[w.layer].

* New options GrowOnWindowLayer and GrowOnlayers to the Maximize
  command.

* New Style option "State".

* New Style option "CenterPlacement".

* New option to FvwmIconMan: ShowNoIcons.

* New WindowList tracker and other enhancements in Perl library.

* New option to fvwm-menu-directory: --func-name.

* Improved FvwmWindowMenu module.

* Fluxbox-like Alt-Button3 resizing with the new Resize options
  Direction, WarpToBorder and FixedDirection

* Enhanced "Test (Version >= x.y.z)" option to allow version
  comparisons.

* New FvwmButtons options: ActiveColorset, ActiveIcon, ActiveTitle,
  PressColorset, PressIcon and PressTitle.

* New FvwmButtons swallow option: SwallowNew.

* The option CurrentGlobalPageAnyDesk was accidentally named
  CurrentGlobbalPageAnyDesk before.

* New conditions AnyScreen and Overlapped.

* The Read and PipeRead commands return 1 if the file or command
  could be read or executed and -1 otherwise.

* New menu option TearOffImmediately.

* Added support for Solaris' Xinerama.

* New option MailDir in FvwmTaskBar.

* MoveToPage command:

    New options wrapx, wrapy, nodesklimitx and nodesklimity.
    New suffix 'w' to allow for window relative movement.

-------------------------------------------------------------------

Changes in alpha release 2.5.10 (19-Mar-2004)

* New command FakeKeyPress.

* New BugOpts option ExplainWindowPlacement.

* Adjustable button reliefs in FvwmIconMan (option ReliefThickness).

* Security patch in fvwm-bug.
  See http://securitytracker.com/alerts/2004/Jan/1008781.html

* Security fixes in:

    fvwm-menu-directory (BugTraq id 9161)
    fvwm_make_directory_menu.sh
    fvwm_make_browse_menu.sh

-------------------------------------------------------------------

Changes in alpha release 2.5.9 (2-Mar-2004)

* New MenuStyle options PopupIgnore and PopupClose.

* New configure option --disable-iconv to disable iconv support.

* New extended variables $[w.iconfile] and $[w.miniiconfile].

* New Style option Unmanaged.  Such windows are not managed by
  fvwm.

* New binding context 'U' for unmanaged windows, similar to 'R'oot.

* New option DisplayNewWindowNames to the BugOpts command.

* Security fix for fvwm-menu-directory.
  See BugTraq id 9161.

-------------------------------------------------------------------

Changes in development release 2.5.8 (31-Oct-2003)

* New prefix command KeepRc.

* Renamed the Cond command to TestRc, and the On command to Test.
  Removed the CondCase command.  Use "KeepRc TestRc" instead.

* The Break command can be told the number of nested function
  levels to break out of.  Break now has a return code of -2
  ("Break").

* Directions can be abbreviated with -, _, [, ], <, >, v or ^ like
  in key or mouse bindings.

* New extended variable $[func.context].

* New Style option MoveByProgramMethod.  Tries to autodetect
  whether application windows are moved honouring the ICCCM or not
  (default).  The method can be overridden manually if the
  detection does not work.

* fvwm supports tear off menus.  See the "Tear Off Menus" section
  in the man page or press Backspace on any menu to try them out.

* fvwm now handles what Unicode calls "combining characters" (i.e.
  marks drawn on top of other characters).

* New commands WindowStyle and DestroyWindowStyle for individual
  (per window) styles.

* The conditions !Current... and !Layer now work as expected.

* Added a nice autohide script to the FAQ.

* FvwmAnimate now supports dynamical commands "pause", "play",
  "push", "pop" and "reset" to manipulate the playing state.

-------------------------------------------------------------------

Changes in development release 2.5.7 (30-May-2003)

* The commands Cond and CondCase now support checking for
  inequality by prefixing the return code with '!'.

* Schedule and Deschedule support hexadecimal and octal command
  ids.

* In FvwmIconMan, windows can move from one manager to another
  according to the managers' Resolution options.

* In order to fix a problem with StartsOnScreen and applications
  that set a user specified position hint, the StartsOnScreen style
  no longer works for the following modules:  FvwmBanner,
  FvwmButtons, FvwmDragWell, FvwmIconBox, FvwmIconMan, FvwmIdent,
  FvwmPager, FvwmScroll, FvwmTaskBar, FvwmWharf, FvwmWinList.  Use
  the '@<screen>' bit in the module geometry specification where
  applicable.

* Documented variable $[gt.any_string] and LocalePath command
  (new in 2.5.5).

* Added gettext support to FvwmScript. New head instruction
  UseGettext and WindowLocaleTitle. New widget instruction
  LocaleTitle. New instruction ChangeLocaleTitle and new function
  Gettext.

* WindowListFunc is executed now within a window context, so
  a prefix "WindowId $0" is not needed in its definition anymore,
  and it is advised to remove it in user configs.

* FvwmEvent now executes all window related events within a window
  context, so PassId is not needed anymore, and all prefixes
  "WindowId $0" may be removed in user event handlers.

* New FvwmTaskBar option NoDefaultStartButton.

-------------------------------------------------------------------

Changes in development release 2.5.6 (28-Feb-2003)

* Fix button 3 drag in FvwmPager so that drag follows the mouse.

* Fix for gmplayer launched by fvwm.  Close stdin on Exec so the
  exec'd process knows it's not running interactively.

* Improvement in MultiPixmap.  In particular Colorset and Solid
  color can be specified.

* New ButtonStyle and TitleStyle style options AdjustedPixmap,
  StretchedPixmap and ShrunkPixmap.

* Use the MIT Shared Memory Extension for XImage.

* The TitleStyle decor of a vertical window Title is rotated.
  This is controllable using a new style option:

    !UseTitleDecorRotation / UseTitleDecorRotation

* New style options IconBackgroundColorset, IconTitleColorset,
  HilightIconTitleColorset, IconTitleRelief, IconBackgroundRelief
  and IconBackgroundPadding.

* Minor incompatible improvements to the Perl library API.

* Renamed FvwmWindowLister to FvwmWindowMenu.

* New option to IconSize style: Adjusted, Stretched, Shrunk.

* New shortcuts for button states: AllActiveUp, AllActiveDown,
  AllInactiveUp, AllInactiveDown.

* New Style options:

    Closable, Iconifiable, Maximizable, AllowMaximizeFixedSize

* New conditions for matching windows:

    Closable, Iconifiable, Maximizable, FixedSize and HasHandles

* New conditional command On for non-window related conditions.

* Removed --disable-gnome-hints and --disable-ewmh configure
  options.

* All single letter variables are deprecated now; new variables:

    $[w.id], $[w.name], $[w.iconname], $[w.class], $[w.resource],
    $[desk.n], $[version.num], $[version.info], $[version.line],
    $[desk.pagesx], $[desk.pagesy]

-------------------------------------------------------------------

Changes in development release 2.5.5 (2-Dec-2002)

* Added support for joining and shaping in bi-directional
  languages that need this.

* ButtonStyle and TitleStyle new style type Colorset.

* New experimental module FvwmProxy.

* New command RestackTransients.

* Added a pixel offset to vector button definitions.

* New command FocusStyle as a shorthand for setting the FP...
  styles with the Style command.

* New option Locale to PrintInfo command.

* The Next and Prev commands start looking for a matching window at
  the context window if there is any.

* The MoveToPage command does nothing with sticky windows.

* New module FvwmWindowLister, a WindowList substitute.

* Sticky windows can be sticky across pages, across desks or both.
  Related to this are:
   - New commands StickAcrossPages and StickAcrossDesks.
   - New Style options StickyAcrossPages and StickyAcrossDesks.
   - New conditional command options StickyAcrossPages and
     StickyAcrossDesks.
   - New WindowList options NoStickyAcrossPages,
     NoStickyAcrossDesks, StickyAcrossPages, StickyAcrossDesks,
     OnlyStickyAcrossPages, OnlyStickyAcrossDesks.
   - New FvwmRearrange options -sp and -sd.

* Fixed flickering in FvwmTaskBar, FvwmWinList, FvwmIconBox,
  FvwmForm, FvwmScript and FvwmScroll when xft fonts or icons with
  an alpha channel are used.

* New Colorset option RootTransparent

* The Transparent Colorset option can be tinted under certain
  conditions

* New MinHeight option to TitleStyle

* Added gettext support. New command LocalePath and new variable
  $[gt.string]

-------------------------------------------------------------------

Changes in alpha release 2.5.4 (20-Oct-2002)

* FvwmTaskBar may now include mini launch buttons using the Button
  command.  Also has new options for spacing the buttons:
  WindowButtonsLeftMargin, WindowButtonsRightMargin, and
  StartButtonRightMargin.  See man page for details.

* Style switches can be prefixed with '!' to inverse their meaning.
  For example, "Style * Sticky" is the same as "Style * !Slippery".
  This works *only* for pairs of styles that take no arguments and
  for the Button and NoButton styles.

* New button property Id in FvwmButtons.  FvwmButtons now accepts
  dynamical actions using SendToModule, see the man page:

    ChangeButton <button-id> <properties-to-change>
    ExpandButtonVars <button-id> <command-to-expand>

* New Colorset options bgTint, fgTint (which complete Tint), Alpha,
  IconTint and IconAlpha.

* Alpha blending rendering is supported even without XRender
  support.

* Enhanced commands [Add]ButtonStyle, [Add]TitleStyle. ButtonState.
  Titles and buttons now have 4 main states instead of 3: ActiveUp,
  ActiveDown, InactiveUp and InactiveDown, plus 4 Toggled variants.
  Several shortcuts added: Active means ActiveUp + ActiveDown,
  Inactive means InactiveUp + InactiveDown, similarly for shortcuts
  ToggledActive and ToggledInactive.

* More shortcuts for button states added: AllNormal, AllToggled,
  AllActive, AllInactive, AllUp, AllDown.  These six shortcuts are
  actually different masks for 4 individual states (from 8 total).

* FPClickToFocus and FPClickToRaise work with any modifier by
  default.

* Perl library API regarding event handlers is changed, so personal
  modules in Perl should be adjusted.

* Allow the use of mouse buttons other than the first in
  FvwmWinList when invoked transient.

* ImagePath now supports directories in form "/some/directory;.png"
  (where semicolon delimits a file extension that files in
  /some/directory have.  This file extension replaces the original
  image extension (if any) or it is added if there is no extension.
  For example with: Style XTerm MiniIcon mini/xterm.xpm
  the file /some/directory/mini/xterm.png is searched.

* New graphical debugger module FvwmGtkDebug.

* New command NoWindow for removing the window context.

* New FvwmIconMan option ShowTransient.

* All conditions have a negation.

* New FvwmBacker option RetainPixmap.

* Fixed flickering in menus, icon title, FvwmButtons, FvwmIconMan
  and FvwmPager when xft fonts or icons with an alpha channel is
  used.

* FvwmButtons redrawing improvement: colorsets are now usable with
  containers.

* FvwmIconMan options PlainColorset, IconColorset, FocusColorset
  and SelectColorset are now strictly respected.

* The HilightBack and ActiveFore menu styles are independent of
  each other.  HilightBack without using ActiveFore does no longer
  hilight the item text.

* New WindowList option SortByResource; the previously added
  SortClassName option is renamed to SortByClass.

* New command PrintInfo for debugging.

-------------------------------------------------------------------

Changes in alpha release 2.5.3 (25-Aug-2002)

* TitleStyle MultiPixmap now works once again.

* Removed the old module interface for ConfigureWindow
  packets. External modules relying on this interface no longer
  work.

* Fixed interaction bug between CascadePlacement and StartsOnPage:
  if the target page was at a negative x or y page displacement
  from the current viewport, the window would be placed on the
  wrong page.

* New Style option IconSize for restricting icon sizes.

* New WindowList options SortClassName, MaxLabelWidth, NoLayer,
  ShowScreen, ShowPage, ShowPageX and ShowPageY.

* Restored old way of handling clicks in windows with ClickToFocus
  and ClickToFocusPassesClickOff.  This fixes a problem with
  click+drag in an unfocused rxvt or aterm window.

* Fixed wrong warp coordinates when WarpToWindow was used with two
  arguments on an unmanaged window.

* Vastly improved FvwmEvent performance.

* FvwmAuto can operate on Enter and Leave events too.  This makes
  it possible to have auto raising with ClickToFocus and
  NeverFocus. See -menter and -menterleave options and examples in
  the FvwmAuto man page.

* The "hangon" strings in FvwmButtons support wild cards.

* New option "Icon" to PlaceAgain command.

* New option "FromPointer" and direction "Center" to the Direction
  command.

* The styles ClickToFocusRaises(Off) and
  MouseFocusClickRaises(Off) are now different names for the same
  style.  Configurations that used

    Style * ClickToFocus, ClickToFocusRaises
    Style * MouseFocusClickRaisesOff

  or vice versa no longer work as like before.  Remove the second
  line to fix the problem.  ClickToFocusRaises now works only on
  the client part of a window, not on the decorations as it did
  before.

* New color limit method for screens that only display 256 colors
  (or less).

* In depth less or equal to 16 image and gradient can be
  dithered. This can be enabled/disabled by using the new
  dither/nodither options to the Colorset command.

* Removed the styles MouseFocusClickIgnoreMotion and
  MouseFocusClickIgnoreMotionOff again.

* Many new focus policy styles "FP..." and "!FP...".

-------------------------------------------------------------------

Changes in alpha release 2.5.2 (24-Jun-2002)

* Fonts can have shadow effects, see the FONT SHADOW EFFECTS
  section of fvwm manual page.

* New Colorset options: fgsh for shadow text and fg_alpha for
  merging text with the background.

* New module FvwmPerl adds perl scripting ability to fvwm
  commands.

* Provided powerful perl library for creating fvwm modules in
  perl.

* New WindowList option IconifiedtAtEnd.

* Always display the current desk number in the FvwmPager window
  title.

* Allow to bind a function to the focus click and pass it to the
  application at the same time.

* New styles !Borders and Borders to enable or disable window
  borders.

* Removed the --enable-multibyte configure option.  Multibyte
  support is now compiled in unconditionally.

* New FvwmButtons option ActionOnPress enables execution of action
  on press rather than on release for any specific button.

* Improved CascadePlacement, the last used position is reused if
  the last placed window does not exist any more.

* FvwmIconMan may now change the resolution dynamically, just
  issue "*FvwmIconMan: resolution page" while FvwmIconMan is
  running.

* New command XineramaSlsScreens.

* MoveToPage and MoveToDesk no longer unstick windows.

* It is possible to specify a string encoding in a font name, see
  the "FONT AND STRING ENCODINGS" section of fvwm manual page.

-------------------------------------------------------------------

Changes in alpha release 2.5.1 (26-Apr-2002)

* Changed the executable and the man page names from fvwm2 to
  fvwm. The old names are still supported by symlinking.

* All fvwm utilities are renamed to conform to the "fvwm-"
  scheme. fvwm-bug, fvwm-root (was xpmroot), fvwm-config,
  fvwm-convert-2.4.

* Added a full support for the side window titles. New Style
  options TitleAtLeft and TitleAtRight added to TitleAtTop and
  TitleAtBottom.

* A title text may now be optionally rotated for both vertical and
  horizontal title directions, search for "Rotated" in the man
  page.

* Added support for loading images in PNG (including alpha
  blending) and XBM formats anywhere.

* New commands Schedule and Deschedule.

* New command State.  New conditions State and !State.

* New commands XSync and XSynchronize for debugging purposes.

* In interactive move/placement when Alt/Meta modifier is pressed,
  snap attraction (SnapAttraction, SnapGrid and EdgeResistance) is
  ignored.

* New flags (No)FvwmModule to FvwmButtons Swallow option

* The I18N_MB patch (--enable-multibyte) has been
  rewritten. MULTIBYTE is used to identify what is was I18N_MB. A
  set of locale functions (locale initialization, font loading,
  text drawing, ...etc.) is created in libs/Flocale.{c,h} and used
  in the entire fvwm code (but FvwmWharf).  Font loading and
  memory management is improved in the multibyte case.

* Better support of non ISO-8859-1 window and icon titles. See the
  --disable-compound-text option in INSTALL.fvwm for more
  details.

* Added anti-aliased text rendering support using Xft (use
  --enable-xft to enable it). Recent versions of XFree and
  freetype2 are needed (see the FONT NAMES AND FONT LOADING
  section of the fvwm manual page).

* Conditional commands now have a return code (Match, NoMatch or
  Error).  This return code can be checked and tied to an action
  with the new commands Cond and CondCase.

* Bindings can be made to the separate parts of the window border
  with the new contexts '[', ']', '-', '_', '<', '^', '>' and
  'v'.

* New parameters for FvwmRearange: -maximize and -animate.

* New option StartCommand for FvwmTaskBar to allow placing the
  StartMenu correctly.

    *FvwmTaskBar: StartCommand Popup RootMenu \
        rectangle $widthx$height+$left+$top 0 -100m

  Please refer to the FvwmTaskBar man page for details.

* New extended variables pointer.x, pointer.y, pointer.wx,
  pointer.wy, pointer.cx and pointer.cy.

* The Current command does not select a random window when no
  window has the focus.

* New color '@4' (transparent) in button vectors.

* New option "Frame" for the Resize and ResizeMove commands.

* Added direction options to WindowShade command.  Windows can be
  shaded in any of the eight compass directions.

* Styles WindowShadeLazy (default), WindowShadeBusy and
  WindowShadeAlwaysLazy to optimize performance of the WindowShade
  command.

* The DO_START_ICONIC flag is no longer supported in the module
  interface.

* Added bi-directional text support for Asian charsets.

* New option MinimalLayer for FvwmIdent to control window layer.

* New FvwmIconMan configuration syntax now conforms to the syntax
  of other modules, see the man page.

* FvwmForm can automatically run commands after a timeout
  interval.

* Fvwm commands can be invoked when the edge of the screen is hit
  with the mouse.

* New commands Colorset, ReadWriteColors and CleanupColorsets
  allow the colorset functionality previously available using
  FvwmTheme.

* FvwmTheme is obsolete now, but still supported for some time.

* New options Tint, TintMask and NoTint to colorsets.

* New WindowList option CurrentAtEnd.

* New weighted sorting in FvwmIconMan.

* New conditional command ThisWindow.

-------------------------------------------------------------------

Changes in alpha release 2.5.0 (27-Jan-2002)

* New commands ResizeMaximize and ResizeMoveMaximize that modify
  the maximized geometry of windows and maximize them as
  necessary.  Very useful to make a window larger manually and
  then get back the old geometry with a click.

* New command ResizeMoveMaximize similarly.

* New styles FixedPPosition, FixedUSPosition, FixedSize,
  FixedUSSize, FixedPSize, VariablePPosition, VariableUSPosition,
  VariableSize, VariableUSSize, and VariablePSize.

* Actions can be bound to windows swallowed in FvwmButtons.  To
  disable this feature for a specific button, the new option
  ActionIgnoresClientWindow can be used.

* Fvwm respect the extended window manager hints
  specification. This allows fvwm to work with KDE version >= 2
  and GNOME version 2. This support is configurable using a bunch
  of new commands and style options, search for "EWMH" in the man
  page.

* New DateFormat option in FvwmTaskBar to change the date format
  in the clock's popup tip.

* Window titlebars may now be designed using a new powerful
  TitleStyle option MultiPixmap, that enables to specify separate
  pixmaps for different parts of the titlebar: Main, LeftEnd,
  LeftOfText, UnderText, RightOfText, RightEnd and more.

* New styles MinOverlapPlacementPenalties and
  MinOverlapPercentPlacementPenalties.

* New styles IndexedWindowName / ExactWindowName and
  IndexedIconName / ExactIconName.

* New command "DesktopName desk name" to define the name of a
  desktop for the FvwmPager, the WindowList and EWMH compliant
  pagers. New expanding variables $[desk.name<n>] for the desktop
  names.

* New window list options NoDeskNum, NoCurrentDeskTitle,
  TitleForAllDesks, NoNumInDeskTitle.

* New emacs style bindings in menus.

* New Maximize global flag "layer" which causes the various grow
  methods to ignore the windows with a layer less than or equal to
  the layer on the window which is maximized.

* Better support of the Transparent colorset in the modules and in
  animated menus.

* Amelioration of the WindowShade animation.

* New ButtonStyle and TitleStyle option MwmDecorLayer.

* New style BackingStoreWindowDefault which is the default
  now. Fvwm no longer disables backing store on windows by
  default.

* New styles MouseFocusClickIgnoreMotion and
  MouseFocusClickIgnoreMotionOff.

* The module interface now supports up to 62 message types.

* New module messages MX_ENTER_WINDOW and MX_LEAVE_WINDOW.

* New events "enter_window" and "leave_window" in FvwmEvent.

* New MenuStyle PopupActiveArea.

* New command line option -passid to FvwmAuto.

* New conditional commands Any and PointerWindow.

* New conditions Focused, !Focused, HasPointer and !HasPointer.

* New commands EWMHBaseStruts and EWMHNumberOfDesktops.

-------------------------------------------------------------------

Changes in stable release 2.4.20 (6-Dec-2006)

* The configure script now correctly appends executable file
  extensions to conditionally built binaries. Fixes building on
  Cygwin.

* FvwmConsole no longer conflicts with getline of Cygwin's stdio.

* Fixed parsing of For loops in FvwmScript.

* Fixed a possible endless loop when de-iconifying a transient
  window.

* Reject some invalid GNOME hints.

* Fixed a loop when xterm changes its "active icon" size.

* The configure script can now cope with four-part version numbers
  when detecting some libraries.

* Security fixes in

     fvwm-menu-directory. (CVE-2006-5969)
     FvwmCommand

-------------------------------------------------------------------

Changes in stable release 2.4.19 (30-Sep-2004)

* Fixed BackingStore style option.

* Fixed MoveToDesk commend with a single argument.

* Allow whitespace in menu names.

* Fixed a hang when restarting FvwmCommand or FvwmConsole.

* A double click no longer occurs when two different mouse
  buttons are pressed.

* Fixed a relief drawing problem in FvwmWinList.

* Fixed travelling windows on restart if a window used non NorthWest
  gravity and changed that before the restart.

* Fixed installation of FvwmGtk.1 for debian (with DESTDIR set).

* The clock in FvwmTaskBar is redrawn immediately when its colour
  changes.

* The option CurrentGlobalPageAnyDesk was accidentally named
  CurrentGlobbalPageAnyDesk before.

* Fixed a problem with fvwm startup and shutdown when the pointer
  was grabbed by another application.

* Fixed parsing of the Pointer option to the Move command.

* Fixed handling of MWM hints on 64 bit machines.

-------------------------------------------------------------------

Changes in stable release 2.4.18 (19-Mar-2004)

* Corrected rebooting the machine in FvwmScript-Quit.

* Fixed the FlickeringMoveWorkaround option to the BugOpts command.

* Security patch in fvwmbug.sh.
  See http://securitytracker.com/alerts/2004/Jan/1008781.html

* Security fixes in
    fvwm-menu-directory (BugTraq id 9161)
    fvwm_make_directory_menu.sh
    fvwm_make_browse_menu.sh

-------------------------------------------------------------------

Changes in stable release 2.4.17 (10-Oct-2003)

* Fixed error message for incorrect --with-SOME-library argument.

* It is now possible to suppress title action or title completely
  in menus created by fvwm-menu-directory.

* Fixed a compile problem on QNX 4.25.

* New configure switch --disable-gtk.

* FvwmGtk.1 is not installed if FvwmGtk is not built.

* The "Visible" condition does no longer select windows on
  different desks.

* With the styles StartsOnPage, SkipMapping and UsePPosition,
  windows that request a specific position are still placed on the
  given page.

* Fixed sending M_NEW_PAGE packets to the modules if the page did
  not change.

* Added support for new BBC headlines in fvwm-menu-headlines, this
  replaces the old BBC-Worlds and BBC-SciTech headlines.

-------------------------------------------------------------------

Changes in stable release 2.4.16 (30-May-2003)

* Fixed a transparency problem in FvwmButtons.

* The PageOnly option in FvwmTaskBar now works after a desk change
  too.

* Fixed a possible core dump when more than 256 windows are on
  the desktop.

* Initial drawing and final undrawing of wire frame no longer
  toggles the pixel in the top left corner of the screen.

* Fixed parsing of button geometries in FvwmButtons when the
  geometry specification appeared after another option with a comma
  at the end.

* FvwmCommand works too when invoked without the DISPLAY variable
  set.

* Fixed displaying iconified windows without an icon in
  FvwmIconMan.

* All single letter variables are deprecated now; new variables:

    $[w.id], $[w.name], $[w.iconname], $[w.class], $[w.resource],
    $[desk.n], $[version.num], $[version.info], $[version.line],
    $[desk.pagesx], $[desk.pagesy]

* Fixed a bug with aspect pixmaps in colorsets.

* Iconified windows without an icon do not receive focus.

* Fixed a bug in FvwmPager that displayed iconified windows without
  icon title or picture as not iconified.

* Fixed parsing of '@<screen>' Xinerama specification in the
  ButtonGeometry option of FvwmButtons.

* The NoWarp menu position hint option works with root menus too.

* Fixed a problem with styles not being properly applied after a
  DestroyStyle command.

* Fixed a bug introduced in 2.4.14.  The pointer was sometimes
  warped to another screen during a restart and command execution.

* Fixed a crash when a window was destroyed twice in a complex
  function.

* Fixed corruption of the window list when windows died at the
  wrong time.

* Fixed problem with empty frame windows if X recycled the window
  id of a window that died recently.

* Fixed loading of application supplied pixmap on 8/24 depth screen.

* WindowListFunc is executed now within a window context, so
  a prefix "WindowId $0" is not needed in its definition anymore
  and it is advised to remove it in user configs.

* FvwmEvent now executes all window related events within a window
  context, so PassId is not needed anymore, and all prefixes
  "WindowId $0" may be removed in user event handlers.

* Fixed "GotoDeskAndPage prev" on desks larger than 2x2.

* Expansion of variables like $[w.name] or $[EMPTY_STRING] that are
  empty works.

-------------------------------------------------------------------

Changes in stable release 2.4.15 (24-Jan-2003)

* Fix for gmplayer launched by fvwm.  Close stdin on Exec so the
  exec'd process knows its not running interactively.

* Windows using the WindowListSkip style do not appear in the
  FvwmTaskBar at random.

* Fixed a memory leak in ChangeIcon, ChangeForeColor and
  ChangeBackColor FvwmScript Instruction.

* Fixed a core dump in the parsing of FvwmAuto arguments.

* Fixed screwed calculation of icon picture size when application
  specifies it explicitly.

* The option ShowOnlyIcons now works as described in the
  FvwmIconMan man page.  It was accidentally named ShowOnlyIconic
  before.

-------------------------------------------------------------------

Changes in stable release 2.4.14 (29-Nov-2002)

* Modules do not crash anymore when more than 126 windows are on
  the desktop.

* FvwmIconMan displays windows correctly that were iconified and
  then moved to another page.

* Application provided icon windows no longer appear at 0 0 when
  restarting.

* The built-in session management can handle window names, classes
  etc. beginning with whitespace (textedit).

* Removed the flawed "A"ny context key binding patch from 2.4.13.

* The default EdgeScroll (if not specified) was incorrectly
  assumed to be 100 pixels instead of 100 percents.

* Icons no longer appear on top of all other windows after a
  restart.

-------------------------------------------------------------------

Changes in stable release 2.4.13 (1-Nov-2002)

* Icon titles for windows with an icon position hint no longer
  appear at random places.

* Fvwm no longer displays two icon pictures when switching from
  NoIconOverride to IconOverride with windows that provide their
  own icon window.

* The Current, All, Pick, ThisWindow and PointerWindow commands
  work on shaded windows too.

* Fixed a problem stacking iconified transients.

* No more flickering when raising transients.

* Fixed a number of problems with window stacking, some new in
  2.4.10 or later, some older problems that have been around for a
  long time.

* Windows starting lowered or on any layer other than the default
  layer are displayed at the right place in FvwmPager.

* Bindings with the "A"ny context can not be overridden by Gnome
  panel or OpenOffice.

-------------------------------------------------------------------

Changes in stable release 2.4.12 (10-Oct-2002)

* Fixed drawing problems with TiledPixmap and Solid MenuFaces which
  appeared in 2.4.10, replacing the same problem with ?Gradient
  MenuFaces in 2.4.9.

* Fixed accidental menu animation with certain menu position hints.

* Increased maximum allowed key symbol name length to 200
  characters.

* Allow quotes in conditional command conditions.

* Fixed starting Move at random position when pointer is on a
  different screen.

* Transient windows do not appear in FvwmWinList after they have
  been moved on the desktop.

-------------------------------------------------------------------

Changes in stable release 2.4.11 (20-Sep-2002)

* Allow the use of mouse buttons other than the first in
  FvwmWinList when invoked transient.

* Fixed a crash with ssh-askpass introduced in 2.4.10.

-------------------------------------------------------------------

Changes in stable release 2.4.10 (15-Sep-2002)

* The commands Maximize, Resize and ResizeMove can be used on icons
  as it was in 2.2.x.

* Fixed hilighting of menu items with HGradient and VGradient
  MenuFace.  Reduced flickering with these options.

* Fixed a minor problem with entering submenus via keyboard.

* Fixed race conditions in FvwmTaskBar with AutoStick that caused
  it to hang.

* Fixed drawing of pager balloons with BalloonBack option.

* Fixed drawing of SidePic menu background with B/D gradients.

* Fixed drawing of menu item reliefs with gradient menu faces.

* Fixed key bindings on window corners.

* Fixed FvwmTaskBar i18n font loading

* Fixed StackTransientParent style without RaiseTransient or
  LowerTransient on the parent window.

* StackTransientParent works only on parent window if it is on the
  same layer.

* Fixed handling of window group hint with the (De)Iconify
  command.

* No more flickering when a transient overlapping its parent window
  is lowered.

* Fixed hilighting of unfocused windows.

-------------------------------------------------------------------

Changes in stable release 2.4.9 (11-Aug-2002)

* Fixed interaction bug between CascadePlacement and StartsOnPage -
  if the target page was at a negative x or y page displacement
  from the current viewport, the window would be placed on the
  wrong page.

* Fixed a problem with colormap transition when a transient window
  died.

* Fixed a FvwmScript crash with Swallow widget and very long window
  names.

* Restored old way of handling clicks in windows with ClickToFocus
  and ClickToFocusPassesClickOff.  This fixes a problem with
  click+drag in an unfocused rxvt or aterm window.

* Fixed problems with $fg and $bg variables in FvwmButtons when the
  UseOld option was used.

* Fixed wrong warp coordinates when WarpToWindow was used with two
  arguments on an unmanaged window.

* Added a workaround for popup menus in TK applications that appear
  on some random position.

* Fixed problems with wish scripts creating windows that start
  iconic.

* Fixed the NoClose option with unmapped panels in FvwmButtons.

* A number of drawing fixes in FvwmPager.

* Fixed a slight bug when waiting until all buttons are released,
  for example after executing a complex function.

* Fixed potentially harmful change in module interface.

* Fixed displaying menu items with icons when using the MenuStyle
  SubmenusLeft.

* Fixed problems with the pointer moving off screen in a multi
  head setup.

* Fixed a potential crash with windows being destroyed during a
  recapture operation.

* Fixed a memory leak in some modules when not using glibc.

* Applications using Mwm hints can now enforce that a window can
  not be moved.

* Fixed negative arguments of WarpToWindow when used on an
  unmanaged window.

* DESTDIR may be fully used again (only useful for distributors).

* Fixed a key binding problem with key symbols that are generated
  by several keys.

* Fixed a possible crash when a window was recaptured and the
  focus could not be transfered to another window.

* Fixed a minor problem with clicks on focused windows being
  ignored.

* fvwm-menu-headlines: added support for CNN and BBC headlines.

* Fixed a performance problem with large numbers of mouse binding
  commands.

-------------------------------------------------------------------

Changes in stable release 2.4.8 (11-Jun-2002)

* A fix for switching between czech and us keyboard layout.

* Remember the icon position when an icon is moved
  non-interactively.

* Setup "fvwm" and "fvwm-root" name symlinks for the executable and
  the man page when installing, see INSTALL.fvwm.

* Fixed another problem with the DeskOnly option and sticky icons
  in FvwmTaskBar.

* New FvwmIconMan configuration syntax now conforms to the syntax
  of other modules, see the man page.

* New WindowList option CurrentAtEnd.

* Fixed maximal length of a named module packet

* Fixed a crash on a config with a new 2.5.x Colorset command.

* Always display the current desk number in the FvwmPager window
  title.

* Allow to bind a function to the focus click and pass it to the
  application at the same time.

* Fixed a problem with fvwm not accepting keyboard input when the
  application with the focus vanished at the start of a session.

* A small security patch regarding TMPDIR.

* Fixed a problem with colormap transition when a transient window
  died.

* Fixed calculation of average bg colour in colour sets with large
  pictures.

* Fixed some minor problems regarding the multibyte patch.

* Fixed selection in FvwmScript List widget.

* fvwm-menu-headlines: updated the site data, added a configurable
  timeout on socket reading (20 sec) to avoid fvwm hanging, new
  --icon-error option.

* Fixed a problem with ClickToFocus + ClickToFocusRaisesOff and
  windows that are below others.

* Fixed the ClickToFocusPassesClick style.

* Fixed CascadePlacement for huge windows, so that the top-left
  corner is always visible.

* Fixed parsing of SendToModule with the first parameter quoted.

* Fonts in double quotes now should work in module configurations.

* Fixed copying PopupOffset values in CopyMenuStyle.

-------------------------------------------------------------------

Changes in stable release 2.4.7 (11-Apr-2002)

* Fixed parsing of WindowList with conditions and a position at
  the same time that was broken in 2.4.6.

* Fixed some problems with the DeskOnly option of FvwmTaskBar
  (windows were duplicated when moving to a different Desk; the
  StickyIcon style was ignored).

* Fixed config.h warnings with some compilers introduced in 2.4.6.

* Fixed icon titles being raised when they should not be.

* Fixed initial drawing of the internals of the FvwmPager window.

* Fixed the FvwmAudio compatible mode in FvwmEvent when external
  audio player is used.

* Fixed execution on QNX.

-------------------------------------------------------------------

Changes in stable release 2.4.6 (10-Mar-2002)

* Better support of non ISO-8859-1 window and icon titles. See the
  --disable-compound-text option in INSTALL.fvwm for more details.

* Improved speed of opaque window movement/resizing.

* Fixed a bug that caused windows not being raised and lowered
  properly.

* Suppress error message when using XBM icons.

* Fixed a read descriptor problem in FvwmTaskBar

* Fixed a minor colour update bug in the pager.

* Fixed an fvwm crash when a module died at the wrong moment;
  specifically a transient FvwmPager or FvwmIconMan.

* Fixed placement of WindowList on wrong Xinerama screen when
  called without any options on a screen other than the primary
  screen.

* Fixed a problem with root bindings and xfishtank.

* Fixed moving windows with the keyboard over the edge of the
  screen when the pointer remained of the previous page.

* Do not hilight windows after ResizeMove.

* New conditional command ThisWindow.

* Some fixes in the configure script that caused some rare
  problems detecting gnome and ncurses.

* Fixed a memory leak in the Pick command.

* Allow to choose windows with CirculateSkip with the Pick command.

* Fixed an FvwmScript compile problem on dec-osf5.

* The window handles are now resizes as they should when the
  HandleWidth style changes.

* The Current command does not select a random window when no
  window has the focus.

* Fixed a rare menu placement problem with Xinerama.

-------------------------------------------------------------------

Changes in stable release 2.4.5 (27-Jan-2002)

* Fixed minor problems in popping sub menus up and down.

* Fixed moving windows between pages with the keyboard.

* Fixed the size of the geometry window that was broken sometimes.

* Fixed problem with pointer warping to another screen on a dual
  head setup.

* Fixed a problem with the focus in internal Ddd and Netscape
  windows.

* Reduced the time in which fvwm attempts to grab the pointer.

* Fixed unmanaged window when window was mapped/unmapped/mapped too
  fast.

* Fixed MiniScroll's auto repeating in FvwmScript.

* Fixed a crash with the UseStyle style in combination with
  HilightBack.

* Fixed excessive redraws of the windows under a window being
  shaded.

* Fixed a minor memory leak in the Style command.

* Fixed pixmap background of FvwmButtons behind buttons with only
  text.

* Fixed a crash in FvwmIconBox when the application provided an
  illegal icon.

* Fixed a configure problem with libstroke-0.5.1.

* New style BackingStoreWindowDefault which is the default
  now. Fvwm no longer disables backing store on windows by
  default.

* Fixed bug that sometimes caused unnecessary redraws when a style
  was changed.

* Fixed crash when something like "$[$v]" appeared in a command.

* Fixed parsing of conditions with more than one comma.

-------------------------------------------------------------------

Changes in stable release 2.4.4 (16-Dec-2001)

* Minor title drawing fixes.

* Fixed manual placement with Xinerama.

* Minor button 3 handling fix in FvwmPager.

* Fixed *FvwmIconMan*shaped option with empty managers.

* Fixed ClickToFocusClickRaises style.

* FvwmForm: Customize pointers, support ISO_Tab key, buttons can
  activate on press or release, special pointer during grab, arrow
  keys useful in form with one input field.

* New OpaqueMoveSize argument "unlimited".

* Fixed binding keys with and without "Shift" modifier under some
  circumstances.

* Fixed binding actions to the client window with ClickToFocus.

* Mouse bindings are activated without a recapture.

* FvwmScript: new keyboard bindings. New flags NoFocus and Left,
  Center, and Right for text position. Amelioration of the Menu
  and PopupMenu Widgets. New functions GetPid, Parse,
  SendMsgAndGet and LastString. New instruction Key for key
  bindings. New command SendToModule ScriptName SendString.

* Command "Silent" when precedes "Key", "Mouse" and "PointerKey"
  disables warning messages.

* Restored the default Alt-Tab behaviour from 2.4.0.

-------------------------------------------------------------------

Changes in stable release 2.4.3 (08-Oct-2001)

* Fixed activation of shape extension.

* Fixed problems with overriding key bindings.

* Single letter key names are allowed in upper and lower case in
  key bindings as before 2.4.0.

* Fixed WindowList placement with Xinerama.

* Fixed flickering icon titles.

* New X resource fvwmscreen to select the Xinerama screen on which
  to place new windows.

* Coordinates of a window during motion are show relative to the
  Xinerama screen.

* Some icon placement improvements with Xinerama.

-------------------------------------------------------------------

Changes in stable release 2.4.2 (16-Sep-2001)

* Desk and page can be given as X resources in .Xdefaults, for
  example:
    xterm.desk: 1
    xterm.page: 1 2 3

* Several Shape compilation problems fixed.

-------------------------------------------------------------------

Changes in stable release 2.4.1 (15-Sep-2001)

* Added Xinerama and SingleLogicalScreen support.

* New commands Xinerama, XineramaPrimaryScreen, XineramaSls,
  XineramaSlsSize and MoveToScreen.

* New context rectangle option XineramaRoot for the menu commands.

* New conditions CurrentGlobalPage, CurrentGlobalPageAnyDesk and
  AcceptsFocus for conditional commands.

* The DestroyStyle command takes effect immediately.

* New style option StartsOnScreen.

* New style options NoUSPosition, UseUSPosition,
  NoTransientPPosition, UseTransientPPosition,
  NoTransientUSPosition, and UseTransientUSPosition.  These work
  similar to the old styles NoPPosition and UsePPosition.

* New option "screen" for Maximize command.

* New option ReverseOrder for WindowList command.

* The default Alt-Tab binding works more intuitive.

* New condition "PlacedByFvwm"

* New Geometry option for FvwmForm.

* New Screen resolution and ShowOnlyIcons options for FvwmIconMan.

* FvwmIconMan can be closed with Delete or Close too.

* New options PageOnly and ScreenOnly for FvwmTaskBar.

* FvwmIconBox, FvwmTaskBar and FvwmWinList support aliases.

* Enhancements in fvwm-menu-headlines and support for 10 more
  sites.

* Color enhancements in button vectors: @2 is bg color, @3 is fg
  color.

* Improved detection of the Shape library.

* Fixed FvwmButtons button titles not being erased for swallowed
  windows that showed up on certain setups.

* Fixed bug that caused transient windows to be buried below their
  parents with the "BugOpts RaiseOverUnmanaged on".  This occured
  with the system.fvwm2rc-sample-95 configuration.

* The modules FvwmPager, FvwmIconMan, FvwmWinList and FvwmButtos
  set the transient_for hint when started with the "transient"
  option.

* Fixed FvwmIconMan with the transient option when mapped off
  screen.

* Fixed ClickToFocus focus policy when iconifying the focused
  window.

* Fixed some focus problems in conjunction with unclutter vs
  xv/xmms and Open Look applications.

* Fixed a problem that could cause windows to be lost off screen
  with interactive window motion.

* Fixed some FvwmTaskBar autohide problem.

* Fixed a display string problem in FvwmForm.

* Fixed a problem with FvwmTheme shadow colours.

* Fixed the CirculateSkipIcon and CirculateSkipShaded options in
  conditional commands.

* Fixed a formatting problem of the man page on AIX, Solaris, and
  some other UNIX variants.

* Fixed a problems with FvwmIconBox exiting on 64 bit platforms.

* Fixed FvwmIconBox crashes with MaxIconSize dimensions 0.

* Fixed parameters of fvwm24_convert.

* Fixed a number of building problems related to old vendor unices,
  libstroke-0.5, autoconf-2.50, bogus gnome-config and imlib-config.

* Fixed drawing of title bar buttons with MWMDecorStick.

* Fixed missing button or key events over the pan frames.

* Fixed placement of the FvwmDragWell, FvwmButtons and FvwmForm
  modules.

* Fixed parsing double quotes in FvwmPager's Font and SmallFont
  options. Fixed FvwmPager crash with certain font strings.

* Fixed drawing of the grid lines in an iconified FvwmPager window.

* Fixed button grabbing problem for buttons > 3 in FvwmTaskBar.

* Fixed some exotic problems with window gravity and resizing
  windows.

* Fixed a problem with maximizing windows with the viewport not
  starting on a page boundary.

* Fixed handling of parentheses in FvwmButtons button actions.

* Work around a key binding problem with keys that generate the
  same symbol with more than one key code (e.g. Shift-F1 = F11).

* The Desk option of FvwmBacker is compatible to earlier
  version. Desk or Page coordinates can be omitted to indicate
  that desk or page changes trigger no action at all.

* Fixed double updating of background with FvwmBacker sometimes
  leading to the wrong background.

* Fixed several escaping errors in fvwm-menu-directory, so files
  and directories containing special chars and spaces should
  work.

* Fixed PlacedByButton3 condition.

* Fixed vanishing windows when mapping/unmapping too fast.

* Fixed prev option of the GotoDeskAndPage command.

* Fixed calculations of X_RESOLUTION and Y_RESOLUTION for screen
  dimensions larger than 2147.

* Fixed compatibility of the FvwmM4 modules on platforms that have
  a System V implementation of m4 (Solaris 2.6).

* The SetEnv command without a value for a variable is the same as
  UnsetEnv.

* Fixed shading/unshading shaped windows and windows without title
  and border.

-------------------------------------------------------------------

Changes in stable release 2.4.0 (03-Jul-2001)

* Finally released. :)
-------------------------------------------------------------------

For older NEWS, read the ONEWS file.
