TODO
====

Welcome to the braindump for current ideas in FVWM.  These items are not
ordered in temrs of their priority; rather they represent the key areas that
FVWM could benefit from.

If you're thinking of working on any of these items, it might be worth
dropping @ThomasAdam an email for discussion.

Items
=====

* Backend:
	* Wayland? -- maybe, but it's new and a moving target.  No decorations (a
	  la Xlib).
	* XCB

* Parsing:
    * [ ] Print where an error was found whilst reading the config file
      (including line numbers).
        * [ ] What would modules do here?  Think:  *FvwmModuleAlias: foo bar

* Code auditing/security:
    * [ ] Remove SAFEFREE() macro;
    * [ ] Add xasprintf() to libs/safemalloc.[ch]
    * [ ] Audit xmalloc() use and consider xasprintf() wrapper

* Code cleanup:
    * [ ] Don't use typedefs for structs:
        * Opaqueness is not useful here!
            * 'struct foo' versus 'foo_t' or 'Foo' as the type.
            * Will allow for reducing the number of structs used as the main
              API, and will allow for better streamlining and accessing.
        * There's a lot of macro #defines for accessing struct members,
          presumably to hide the type; that's usually OK, but I'd like to
          see these go away.  FW and Decor are good examples of this!  I'd
          rather see more exposed functions to replace these.  The API will
          need discussion.
          dv: Many of these macros were introduced to hide the layout from the
              underlying structures from its users.  So, when the structures
              change - as they have many times - one would only have to change
              the macro definition and not hundreds of users.  Especially with
              a future rewrite of the style system in mind, this is still very
              important.
    * [ ] What's in libs/ -- the static linking of libfvwm.a useful anymore?
      There's a lot of code being ripped out of libs/ and I don't see much
      more being added in.
      dv: Static linkage was good when the library was small.  Nowadays its
          just a burden.
    * [X] I'm not keen on commands.h:P() macro, I'd rather have the prototypes
      explicitly stated.
    * [X] Remove the ewmh_intern.h:CMD_EWMH_ARGS macro, I'd rather have the
      prototypes explicitly stated.
    * [ ] Consider using doxygen for API-specific documentation?  This would
      help justify/cleanup functions in the longer-term by identifying
      similar functions, etc.
      dv: In my experience using doxygen automatically leads to mostly
          unuseable function documentation.  I'm against using it since it does
          not help to understand how functions are related to each other.

* Messages:
    * More debug output to help the user pinning down why a window does not
      "behave".  One problem that comes up all the time is "why does my window
      appear at position X Y with size WIDTHxHEIGHT".  Although there is some
      output, when a window first pops up, there is none after the initial
      mapping when the really interesing things happen (messages from the client
      application).
    * Look over all existing messages, add a proper severity (debug, info,
      warning, deprecated, error etc.) and feature number and make the messages
      switchable through the config file.
    * Allow to disable all messages through configure to allow for smaller
      builds.

* String handling:
    * [ ] libs/String.[ch] and friends have idioms like CatString3, and
      other means to manipulating strings.  Get rid of these.
    * [ ]  Audit the code for strcpy()/strcat(), and take out back and shoot
      them!  Specifically:
        * [ ] Check whether the original string is being malloc()d and just use
          xasprintf()
        * [ ]  If it's on the stack, use strlcpy(), and carefully consider
          whether the return value should be checked.  If it's not
          necessary, add (void) at the start of the call.
    * [ ]  Each command shouldn't be doing its own string parsing; the items
      should be tokenised before-hand and sent along to the relevant CMD_*()
      functions.
        - This means rethinking how commands/lines/etc., are parsed.

* RandR:
    * [ ] Configuration:
        * [ ] Enumerate outputs without requiring config file changes.  See:
            http://www.mail-archive.com/fvwm-workers@fvwm.org/msg03649.html
    * [ ] Separate desktops per monitor:
	    * [ ] PanWindows per-monitor inhibit moving windows across screen
	      boundaries.  Have a key-binding to stop panning?

* Clients:
    * [ ] Have an "undo" list of geometries which can be reverted to.
    * [ ] Per-page geometries for windows, makes handling sticky windows easier.
    * [ ] No special-casing of maximised state, it's just another geometry set
      which is added on to a list, and popped when no longer used in that
      state.
      dv: Unfortunately its not that easy because being "maximised", "shaded",
          "iconified" or "fullscreen" is announced to the world through the EWMH
          interface.  The list approach sounds promising, but one has to keep in
          mind that the geometries are not independent of each other.   For
          example, a window is halfway between two pages, maximised on the
          current page, then the user switches to the other page and
          unmaximises the window.  Now, where dos he expect the window to
          appear?

* Commands:
    * There's far too many commands in FVWM:
        * [ ] Too many duplicates; merge.
        * [ ] Think about a common way of referring to pages/windows/etc -- much
          like with tmux's "-t session:window.pane" syntax, can we not do
          something similar here for referring to pages/windows?
	* Handling of command options:
		* [ ] Commands don't do their own parsing; instead, command
		tokens are parsed with getopt(), and a tree of key/values are
		built; that tree is passed in, and referenced
		arg_has()/arg_get(), etc.

* Functions / Conditionals / Exec (PipeRead) / Etc.:
        * Next/Prev/etc., are all very scripting-like.  Do we just want hook
          support?  Is it worth expanding FvwmEvent to do this?

* Menus:
	* [ ] Typeahead/autocomplete inside of menus.  See 'rofi' for example of
	  functionality.

* Modules:
    * [ ] The module interface (FVWM <-> Module) is a mess; consider:
	* DBus
	* imsg
	* Msgpack-C:
	  (https://github.com/msgpack/msgpack-c/blob/master/QUICKSTART-C.md)
	* Cap'n Proto (bloody stupid name!): https://capnproto.org
    * [ ] Use libevent to replace the hand-rolled (and often broken) select/poll
	  mechanism.
    * [ ] What about third-party scripting languages?  How do we handle that
      without requiring linking against the specific language in question?
    * [ ] Amalgamation of module(s) is a possibility:
	* Functionality for things like buttons, taskbars, iconman, pager,
	  etc., can be replaced with a single module:
		* Combines all the functionality of the obsolete modules.
		* Uses colorsets consistently, to allow configuring all GUI
		  elements.
		* Can be reconfigured at runtime through user interactions (e.g.
		  dragging elements to a different position on the window)
		* Initially support the older module interface.
		* Can replace all the old modules with symlinks.

* perllib (FvwmPerl):
	* Transition away from 5.004 as the minimum version;
	* Update the code to use 5.14 at least:
        - [ ] Do away with subroutine prototypes;
        - [ ] "use vars" is deprecated;
        - [ ] "use parent", rather than manipulating @ISA/Exporter directly;
        - [ ] "use warnings/strictures" everywhere;
	* [ ] Don't rely on the crappy command generation stuff from FVWM releases to feed
	  the perl API ""abstraction"" as we have it now (deprecate create-constants
	  and create-commands).

* Decors:
        dv: I think decorations should be dropped in favour of styles.  The
            whole concept was invented because stlyes were unable to do it.
	* Think carefully about the syntax of the decor replacement!
	* [ ] Remove {Border,Title,Button}Style and decors, just one simple look.
	* [ ] New style `DecoratedByFvwm`/`DecoratedByModule`.
	* [ ] New module `FvwmDecor` to do per-window decoarations:
		* [ ] How does resizing work with this module?

* Styles:
        dv: I have much more radical plans for styles:  Being able to nest
            styles, making them conditional with any condition that can be used
            anywhere in fvwm at any time, defining window specific preferences
            of style order.
	* [ ] Expand Style command to support name=pattern syntax, for instance:
	  `Style (Name "File *", Class XPaint, Resource *browser) NoIcon`
	* [ ] Add id-pattern to Style command.
	* [ ] Split styles into five lists (resource, class, icon, name, id) which are
	  then applied in that order.
	* [ ] Split Style into run time control (`Style`) and startup style
	  (`InitialStyle`).
	* [ ] Remove the ability to toggle specific parts of EWMH; it's either on
	  or it isn't.
          dv: I don't think this should be done.  EWMH is annoying in many ways,
              and in order to tame EWMH "enabled" applications, detailed
              control over the various aspects.  It was a single switch
              originally, and that was not good enough.
