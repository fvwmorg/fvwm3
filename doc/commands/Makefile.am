# $Id$

docdir=@FVWM_DOCDIR@/commands
XSLTPROC=@XSLTPROC@

XSL_FILE=$(srcdir)/../fvwm.xsl
QUIET=@
COMMANDS=AddButtonStyle AddTitleStyle AddToDecor AddToFunc AddToMenu All AnimatedMove Any Asterix Beep BorderStyle Break BugOpts BusyCursor ButtonState ButtonStyle ChangeDecor ChangeMenuStyle CleanupColorsets ClickTime Close ColorLimit ColormapFocus Colorset CopyMenuStyle Current CursorMove CursorStyle DefaultColorset DefaultColors DefaultFont DefaultIcon DefaultLayers Delete Deschedule DesktopName DesktopSize Desk DestroyDecor DestroyFunc DestroyMenuStyle DestroyMenu DestroyModuleConfig DestroyStyle DestroyWindowStyle Destroy Direction Echo EdgeCommand EdgeLeaveCommand EdgeResistance EdgeScroll EdgeThickness Emulate EscapeFunc EwmhBaseStruts EWMHNumberOfDesktops ExecUseShell Exec FakeClick FakeKeypress FlipFocus FocusStyle Focus Function GlobalOpts GnomeButton GotoDeskAndPage GotoDesk GotoPage HideGeometryWindow HilightColorset HilightColor IconFont Iconify IconPath IgnoreModifiers ImagePath KeepRc Key KillModule Layer LocalePath Lower Maximize MenuStyle Menu ModuleListenOnly ModulePath ModuleSynchronous ModuleTimeout Module Mouse MoveThreshold MoveToDesk MoveToPage MoveToScreen Move Next None Nop NoWindow OpaqueMoveSize Pick PipeRead PixmapPath PlaceAgain Plus PointerKey PointerWindow Popup Prev PrintInfo QuitScreen QuitSession Quit RaiseLower Raise Read RecaptureWindow Recapture RefreshWindow Refresh Repeat ResizeMaximize ResizeMoveMaximize ResizeMove Resize RestackTransients Restart SaveQuitSession SaveSession ScanForWindow Schedule Scroll SendToModule SetAnimation SetEnv Silent SnapAttraction SnapGrid State StickAcrossDesks StickAcrossPages Stick StrokeFunc Stroke Style TearMenuOff TestRc Test ThisWindow TitleStyle Title UnsetEnv UpdateDecor UpdateStyles Wait WarpToWindow WindowFont WindowId WindowList WindowsDesk WindowShadeAnimate WindowShade WindowStyle XineramaPrimaryScreen XineramaSlsScreens XineramaSlsSize XineramaSls Xinerama XorPixmap XorValue XSynchronize XSync

HTML_FILES=$(patsubst %,%.html,$(COMMANDS))
XML_FILES=$(patsubst %,%.xml,$(COMMANDS))
EXTRA_DIST=$(XML_FILES)

if FVWM_BUILD_HTMLDOC
doc_DATA=$(HTML_FILES)
all: $(HTML_FILES)
endif

clean:
	rm -f $(HTML_FILES)

# All HTML output is generated from XML files.
%.html : $(srcdir)/%.xml
	$(QUIET) echo "Generating $@"
	$(QUIET) $(XSLTPROC) --path .. --xinclude --stringparam html.stylesheet "../style.css" -o $@ $(XSL_FILE) $<
