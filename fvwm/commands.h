/* -*-c-*- */

#ifndef COMMANDS_H
#define COMMANDS_H

/* ---------------------------- included header files ---------------------- */

/* ---------------------------- global definitions ------------------------- */

/* ---------------------------- global macros ------------------------------ */

/* ---------------------------- type definitions --------------------------- */

enum
{
	F_UNDEFINED = -1,

	/* functions that need no window */
	F_NOP = 0,
	F_ADDFUNC,
	F_ADDMENU,
	F_ADDMENU2,
	F_ALL,
	F_ANY,
	F_BEEP,
	F_BREAK,
	F_BUG_OPTS,
	F_BUSY_CURSOR,
	F_BUTTON_STATE,
	F_BUTTON_STYLE,
	F_CHANGE_MENUSTYLE,
	F_CIRCULATE_DOWN,
	F_CIRCULATE_UP,
	F_CLICK,
	F_CLOSE,
	F_COLORMAP_FOCUS,
	F_COND,
	F_CONDCASE,
	F_CONFIG_LIST,
	F_COPY_MENU_STYLE,
	F_CURRENT,
	F_CURSOR_STYLE,
	F_DESCHEDULE,
	F_DESKTOP_NAME,
	F_DESTROY_FUNCTION,
	F_DESTROY_MENU,
	F_DESTROY_MENUSTYLE,
	F_DESTROY_STYLE,
	F_DFLT_COLORS,
	F_DFLT_COLORSET,
	F_DFLT_FONT,
	F_DFLT_ICON,
	F_DFLT_LAYERS,
	F_DIRECTION,
	F_EDGE_COMMAND,
	F_EDGE_LEAVE_COMMAND,
	F_EDGE_RES,
	F_EDGE_SCROLL,
	F_EMULATE,
	F_ESCAPE_FUNC,
	F_EWMH_BASE_STRUTS,
	F_EWMH_NUMBER_OF_DESKTOPS,
	F_EXEC,
	F_EXEC_SETUP,
	F_FAKE_CLICK,
	F_FAKE_KEYPRESS,
	F_FOCUSSTYLE,
	F_FUNCTION,
	F_GLOBAL_OPTS,
	F_GOTO_DESK,
	F_GOTO_PAGE,
	F_HICOLOR,
	F_HICOLORSET,
	F_HIDEGEOMWINDOW,
	F_ICONFONT,
	F_ICON_PATH,
	F_IGNORE_MODIFIERS,
	F_IMAGE_PATH,
	F_KEEPRC,
	F_KEY,
	F_KILL_MODULE,
	F_LAYER,
	F_LOCALE_PATH,
	F_MENUSTYLE,
	F_MODULE,
	F_MODULE_PATH,
	F_MODULE_SYNC,
	F_MOUSE,
	F_MOVECURSOR,
	F_MOVE_TO_DESK,
	F_NEXT,
	F_NONE,
	F_OPAQUE,
	F_PICK,
	F_PIXMAP_PATH,
	F_POINTERKEY,
	F_POINTERWINDOW,
	F_POPUP,
	F_PREV,
	F_PRINTINFO,
	F_QUIT,
	F_QUIT_SESSION,
	F_QUIT_SCREEN,
	F_READ,
	F_RECAPTURE,
	F_RECAPTURE_WINDOW,
	F_REFRESH,
	F_REPEAT,
	F_RESTART,
	F_SAVE_SESSION,
	F_SAVE_QUIT_SESSION,
	F_SCANFORWINDOW,
	F_SCHEDULE,
	F_SCROLL,
	F_SETDESK,
	F_SETENV,
	F_SET_ANIMATION,
	F_SET_MASK,
	F_SET_NOGRAB_MASK,
	F_SET_SYNC_MASK,
	F_SHADE_ANIMATE,
	F_SILENT,
	F_SNAP_ATT,
	F_SNAP_GRID,
	F_STAYSUP,
	STROKE_ARG(F_STROKE)
	STROKE_ARG(F_STROKE_FUNC)
	F_STYLE,
	F_TEARMENUOFF,
	F_TEST_,
	F_TESTRC,
	F_THISWINDOW,
	F_TITLE,
	F_TITLESTYLE,
	F_TOGGLE_PAGE,
	F_UPDATE_STYLES,
	F_WAIT,
	F_WINDOWFONT,
	F_WINDOWLIST,
	F_XINERAMA,
	F_XINERAMAPRIMARYSCREEN,
	F_XINERAMASLS,
	F_XINERAMASLSSCREENS,
	F_XINERAMASLSSIZE,
	F_XOR,
	F_XSYNC,
	F_XSYNCHRONIZE,

	/* functions that need a window to operate on */
	F_ADD_BUTTON_STYLE,
	F_ADD_DECOR,
	F_ADD_TITLE_STYLE,
	F_ANIMATED_MOVE,
	F_BORDERSTYLE,
	F_CHANGE_DECOR,
	F_COLOR_LIMIT,
	F_DELETE,
	F_DESTROY,
	F_DESTROY_DECOR,
	F_DESTROY_MOD,
	F_DESTROY_WINDOW_STYLE,
	F_ECHO,
	F_FLIP_FOCUS,
	F_FOCUS,
	F_ICONIFY,
	F_LOWER,
	F_MAXIMIZE,
	F_MOVE,
	F_MOVE_THRESHOLD,
	F_MOVE_TO_PAGE,
	F_MOVE_TO_SCREEN,
	F_PLACEAGAIN,
	F_RAISE,
	F_RAISELOWER,
	F_RESIZE,
	F_RESIZE_MAXIMIZE,
	F_RESIZEMOVE,
	F_RESIZEMOVE_MAXIMIZE,
	F_RESTACKTRANSIENTS,
	F_SEND_STRING,
	F_STATE,
	F_STICK,
	F_STICKACROSSDESKS,
	F_STICKACROSSPAGES,
	F_UPDATE_DECOR,
	F_WARP,
	F_WINDOWID,
	F_WINDOW_SHADE,
	F_WINDOW_STYLE,

	F_END_OF_LIST = 999,

	/* Functions for use by modules only! */
	F_SEND_WINDOW_LIST = 1000
};

/* ---------------------------- exported variables (globals) --------------- */

/* ---------------------------- interface functions ------------------------ */

#ifdef P
#undef P
#endif
#define P(n) void CMD_ ## n(F_CMD_ARGS)
/* This file contains all command prototypes. */
P(Plus);
P(AddButtonStyle);
P(AddTitleStyle);
#ifdef USEDECOR
P(AddToDecor);
#endif /* USEDECOR */
P(AddToFunc);
P(AddToMenu);
P(Alias);
P(All);
P(AnimatedMove);
P(Any);
P(Beep);
P(Break);
P(BorderStyle);
P(BugOpts);
P(BusyCursor);
P(ButtonState);
P(ButtonStyle);
#ifdef USEDECOR
P(ChangeDecor);
#endif /* USEDECOR */
P(ChangeMenuStyle);
P(CleanupColorsets);
P(ClickTime);
P(Close);
P(ColorLimit);
P(ColormapFocus);
P(Colorset);
P(CopyMenuStyle);
P(Current);
P(CursorMove);
P(CursorStyle);
P(DefaultColors);
P(DefaultColorset);
P(DefaultFont);
P(DefaultIcon);
P(DefaultLayers);
P(Delete);
P(Deschedule);
P(Desk);
P(DesktopName);
P(DesktopSize);
P(Destroy);
#ifdef USEDECOR
P(DestroyDecor);
#endif /* USEDECOR */
P(DestroyFunc);
P(DestroyMenu);
P(DestroyMenuStyle);
P(DestroyModuleConfig);
P(DestroyStyle);
P(DestroyWindowStyle);
P(Direction);
P(Dummy);
P(Echo);
P(EdgeCommand);
P(EdgeLeaveCommand);
P(EdgeResistance);
P(EdgeScroll);
P(EdgeThickness);
P(Emulate);
P(EscapeFunc);
P(EwmhBaseStruts);
P(EwmhNumberOfDesktops);
P(Exec);
P(ExecUseShell);
P(FakeClick);
P(FakeKeypress);
P(FlipFocus);
P(Focus);
P(FocusStyle);
P(GlobalOpts);
P(GnomeButton);
P(GnomeShowDesks);
P(GotoDesk);
P(GotoDeskAndPage);
P(GotoPage);
P(HideGeometryWindow);
P(HilightColor);
P(HilightColorset);
P(IconFont);
P(Iconify);
P(IconPath);
P(IgnoreModifiers);
P(ImagePath);
P(Key);
P(KillModule);
P(Layer);
P(LocalePath);
P(Lower);
P(Maximize);
P(Menu);
P(MenuStyle);
P(Module);
P(ModulePath);
P(ModuleSynchronous);
P(ModuleTimeout);
P(Mouse);
P(Move);
P(MoveThreshold);
P(MoveToDesk);
P(MoveToPage);
P(MoveToScreen);
P(Next);
P(None);
P(Nop);
P(NoWindow);
P(OpaqueMoveSize);
P(Pick);
P(PipeRead);
P(PixmapPath);
P(PlaceAgain);
P(PointerKey);
P(PointerWindow);
P(Popup);
P(Prev);
P(PrintInfo);
P(PropertyChange);
P(Quit);
P(QuitScreen);
P(QuitSession);
P(Raise);
P(RaiseLower);
P(Read);
P(ReadWriteColors);
P(Recapture);
P(RecaptureWindow);
P(Refresh);
P(RefreshWindow);
P(Repeat);
P(Resize);
P(ResizeMaximize);
P(ResizeMove);
P(ResizeMoveMaximize);
P(RestackTransients);
P(Restart);
P(SaveQuitSession);
P(SaveSession);
P(ScanForWindow);
P(Schedule);
P(Scroll);
P(Send_ConfigInfo);
P(Send_WindowList);
P(SendToModule);
P(set_mask);
P(set_nograb_mask);
P(set_sync_mask);
P(SetAnimation);
P(SetEnv);
P(SnapAttraction);
P(SnapGrid);
P(State);
P(Stick);
P(StickAcrossDesks);
P(StickAcrossPages);
#ifdef HAVE_STROKE
P(Stroke);
P(StrokeFunc);
#endif /* HAVE_STROKE */
P(Style);
P(Test);
P(TestRc);
P(ThisWindow);
P(TitleStyle);
P(Unalias);
P(UnsetEnv);
P(UpdateDecor);
P(UpdateStyles);
P(Wait);
P(WarpToWindow);
P(WindowFont);
P(WindowId);
P(WindowList);
P(WindowShade);
P(WindowShadeAnimate);
P(WindowStyle);
P(Xinerama);
P(XineramaPrimaryScreen);
P(XineramaSls);
P(XineramaSlsScreens);
P(XineramaSlsSize);
P(XorPixmap);
P(XorValue);
P(XSync);
P(XSynchronize);
#undef P

#endif /* COMMANDS_H */
