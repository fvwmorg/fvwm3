#ifndef MISC_H
#define MISC_H

#include <ctype.h>
#include "menus.h"
#include "../libs/fvwmlib.h"

/************************************************************************
 * ReapChildren - wait() for all dead child processes
 ************************************************************************/
#include <sys/types.h>
#include <sys/wait.h>

#if HAVE_WAITPID
#define ReapChildren()  while ((waitpid(-1, NULL, WNOHANG)) > 0);
#elif HAVE_WAIT3
#define ReapChildren()  while ((wait3(NULL, WNOHANG, NULL)) > 0);
#else
#error One of waitpid or wait3 is needed.
#endif

typedef struct name_list_struct
{
  struct name_list_struct *next;   /* pointer to the next name */
  char *name;		  	   /* the name of the window */
  char *value;                     /* icon name */
#ifdef MINI_ICONS
  char *mini_value;                /* mini icon name */
#endif
#ifdef USEDECOR
  char *Decor;
#endif
  int Desk;                        /* Desktop number */
/* RBW - 11/02/1998 - page x,y numbers */
  int PageX;
  int PageY;
/**/
  unsigned long on_flags; 
  unsigned long off_flags; 
  int border_width;
  int resize_width;
  char *ForeColor;
  char *BackColor;
  icon_boxes *IconBoxes;                /* pointer to iconbox(s) */
  unsigned long on_buttons;
  unsigned long off_buttons;

} name_list;

/* used for parsing configuration */
struct config
{
  char *keyword;
#ifdef __STDC__
  void (*action)(char *, FILE *, char **, int *);
#else
  void (*action)();
#endif
  char **arg;
  int *arg2;
};

/* used for parsing commands*/
struct functions
{
  char *keyword;
#ifdef __STDC__
  void (*action)(XEvent *,Window,FvwmWindow *, unsigned long,char *, int *);
#else
  void (*action)();
#endif
  int code;
  int type;
};

/* values for name_list flags */
/* The first 13 items are mapped directly into the FvwmWindow structures
 * flag value, so they MUST correspond to the first 13 entries in fvwm.h */
#define START_ICONIC_FLAG    (1<<0)
#define STAYSONTOP_FLAG      (1<<1)
#define STICKY_FLAG          (1<<2)
#define LISTSKIP_FLAG        (1<<3)
#define SUPPRESSICON_FLAG    (1<<4)
#define NOICON_TITLE_FLAG    (1<<5)
#define LENIENCE_FLAG        (1<<6)
#define STICKY_ICON_FLAG     (1<<7)
#define CIRCULATE_SKIP_ICON_FLAG  (1<<8)
#define CIRCULATESKIP_FLAG   (1<<9)
#define CLICK_FOCUS_FLAG     (1<<10)
#define SLOPPY_FOCUS_FLAG    (1<<11)
#define SHOW_MAPPING         (1<<12)

#define NOTITLE_FLAG         (1<<13)
#define NOBORDER_FLAG        (1<<14)
#define ICON_FLAG            (1<<15)
#define STARTSONDESK_FLAG    (1<<16)
#define BW_FLAG              (1<<17)
#define NOBW_FLAG            (1<<18)
#define FORE_COLOR_FLAG      (1<<19)
#define BACK_COLOR_FLAG      (1<<20)
#define RANDOM_PLACE_FLAG    (1<<21)
#define SMART_PLACE_FLAG     (1<<22)
#define MWM_BUTTON_FLAG      (1<<23)
#define MWM_DECOR_FLAG       (1<<24)
#define MWM_FUNCTIONS_FLAG   (1<<25)
#define MWM_OVERRIDE_FLAG    (1<<26)
#define MWM_BORDER_FLAG      (1<<27)
#define DECORATE_TRANSIENT_FLAG (1<<28)
#define NO_PPOSITION_FLAG    (1<<29)
#define OL_DECOR_FLAG        (1<<30)

#ifdef MINI_ICONS
#define MINIICON_FLAG        (1<<31)
#endif

/* some fancy font handling stuff */
#define NewFontAndColor(newfont,color,backcolor) {\
   Globalgcv.font = newfont;\
   Globalgcv.foreground = color;\
   Globalgcv.background = backcolor;\
   Globalgcm = GCFont | GCForeground | GCBackground; \
   XChangeGC(dpy,Scr.ScratchGC3,Globalgcm,&Globalgcv); \
}

#ifdef NO_ICONS
#define ICON_HEIGHT 1
#else
#define ICON_HEIGHT (Scr.IconFont.height+6)
#endif

extern XGCValues Globalgcv;
extern unsigned long Globalgcm;
extern Time lastTimestamp;
extern XEvent Event;

extern char NoName[];
extern char NoClass[];
extern char NoResource[];

extern void       LookInList(FvwmWindow *, name_list *);
extern void       MoveOutline(Window, int,int,int,int);
extern void       AnimatedMoveOfWindow(Window w,int startX,int startY,int endX, int endY,
				       Bool fWarpPointerToo, int cusDelay, float *ppctMovement);
extern void       DoResize(int, int, FvwmWindow *);
extern void       DisplaySize(FvwmWindow *, int, int, Bool);
extern void       DisplayPosition(FvwmWindow *, int, int,Bool);
extern void       SetupFrame(FvwmWindow *,int,int,int,int,Bool);
extern void       CreateGCs(void);
extern void       InstallWindowColormaps(FvwmWindow *);
extern void       InstallRootColormap(void);
extern void       UninstallRootColormap(void);
extern void       FetchWmProtocols(FvwmWindow *);
extern void       FetchWmColormapWindows (FvwmWindow *tmp);
extern void       InitEventHandlerJumpTable(void);
extern void       DispatchEvent(void);
extern void       HandleEvents(void);
extern void       HandleExpose(void);
extern void       HandleFocusIn(void);
extern void       HandleFocusOut(void);
extern void       HandleDestroyNotify(void);
extern void       HandleMapRequest(void);
extern void       HandleMapRequestKeepRaised(Window keepraised);
extern void       HandleMapNotify(void);
extern void       HandleUnmapNotify(void);
extern void       HandleMotionNotify(void);
extern void       HandleButtonRelease(void);
extern void       HandleButtonPress(void);
extern void       HandleEnterNotify(void);
extern void       HandleLeaveNotify(void);
extern void       HandleConfigureRequest(void);
extern void       HandleClientMessage(void);
extern void       HandlePropertyNotify(void);
extern void       HandleKeyPress(void);
extern void       HandleVisibilityNotify(void);
extern void       HandleColormapNotify(void);
extern void       SetTitleBar(FvwmWindow *, Bool,Bool);
extern void       RestoreWithdrawnLocation(FvwmWindow *, Bool);
extern void       Destroy(FvwmWindow *);
extern void       GetGravityOffsets (FvwmWindow *, int *, int *);
extern void       MoveViewport(int newx, int newy,Bool);
extern FvwmWindow *AddWindow(Window w);
extern int        MappedNotOverride(Window w);
extern void       GrabButtons(FvwmWindow *);
extern void       GrabKeys(FvwmWindow *);
extern void       GetWindowSizeHints(FvwmWindow *);
extern void       SwitchPages(Bool,Bool);
extern void       NextPage(void);
extern void       PrevPage(void);
extern void       moveLoop(FvwmWindow *, int, int, int,int, int *, int *,Bool,Bool, XEvent *);

extern void       Keyboard_shortcuts(XEvent *, int);
extern void       RedoIconName(FvwmWindow *);
extern void       DrawIconWindow(FvwmWindow *);
extern void       CreateIconWindow(FvwmWindow *tmp_win, int def_x, int def_y);


extern inline void RelieveWindow(FvwmWindow *, Window, 
				      int, int, int, int, GC, GC, int);
extern inline void RelieveWindowHH(FvwmWindow *,Window,
					int,int,int,int, GC,GC, int, int);
void RelieveParts(FvwmWindow *t,int i,GC hor, GC vert);
#define NO_HILITE     0x0000
#define TOP_HILITE    0x0001
#define RIGHT_HILITE  0x0002
#define BOTTOM_HILITE 0x0004
#define LEFT_HILITE   0x0008
#define FULL_HILITE   0x000F
#define HH_HILITE     0x0010

void Maximize(XEvent *eventp,Window w,FvwmWindow *tmp_win,
	      unsigned long context, char *action, int *Module);
#ifdef  WINDOWSHADE
void WindowShade(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		 unsigned long context, char *action, int *Module);
#endif
extern void       RaiseWindow(FvwmWindow *t);
extern void       LowerWindow(FvwmWindow *t);
extern Bool       GrabEm(int);
extern void       UngrabEm(void);
extern MenuRoot   *NewMenuRoot(char *name, int function_or_popup);
extern void       AddToMenu(MenuRoot *, char *, char *, Bool, Bool);
extern void       MakeMenu(MenuRoot *);
extern void       CaptureAllWindows(void);
extern void       SetTimer(int);
extern int        flush_expose(Window w);
void ExecuteFunction(char *Action, FvwmWindow *tmp_win, XEvent *eventp,
		     unsigned long context, int Module);
void do_windowList(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		unsigned long context, char *action,int *Module);
extern void       RaiseThisWindow(int);
extern int        GetContext(FvwmWindow *, XEvent *, Window *dummy);
extern void       ConstrainSize (FvwmWindow *, int *, int *);
extern void       HandlePaging(int, int, int *, int *, int *, int *,Bool);
extern void       SetShape(FvwmWindow *, int);
extern void       AutoPlace(FvwmWindow *);
void executeModule(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		     unsigned long context, char *action,int* Module);
extern void       SetFocus(Window,FvwmWindow *, Bool FocusByMouse);
extern void       CheckAndSetFocus(void);
extern void       initModules(void);
extern int        HandleModuleInput(Window w, int channel);
extern void       match_string(struct config *, char *, char *, FILE *);
extern void       no_popup(char *ptr);
extern void       KillModule(int channel, int place);
extern void       ClosePipes(void);
extern char       *findIconFile(char *icon, char *pathlist, int mode);
int find_func_type(char *action);
extern void       GetBitmapFile(FvwmWindow *tmp_win);
extern void       GetXPMFile(FvwmWindow *tmp_win);
extern void       GetIconWindow(FvwmWindow *tmp_win);
extern void       GetIconBitmap(FvwmWindow *tmp_win);
/*  RBW - 11/02/1998  */
extern int SmartPlacement(FvwmWindow *t, int width, int height,int *x,int *y, int pdeltax, int pdeltay);
/**/
extern void usage(void);
void Broadcast_v(unsigned long event_type, unsigned long num_datum,...);
void Broadcast(unsigned long event_type, unsigned long num_datum,
	       unsigned long data1, unsigned long data2, 
	       unsigned long data3, unsigned long data4,
	       unsigned long data5, unsigned long data6, 
	       unsigned long data7);
void BroadcastConfig(unsigned long event_type, FvwmWindow *t);
void SendPacket(int channel, unsigned long event_type, unsigned long num_datum,
		unsigned long data1, unsigned long data2, 
		unsigned long data3, unsigned long data4,
		unsigned long data5, unsigned long data6, 
		unsigned long data7);
void SendConfig(int Module, unsigned long event_type, FvwmWindow *t);
void BroadcastName(unsigned long event_type, unsigned long data1,
		   unsigned long data2, unsigned long data3, char *name);
void SendName(int channel, unsigned long event_type,unsigned long data1,
	      unsigned long data2, unsigned long data3, char *name);
void SendStrToModule(XEvent *eventp,Window junk,FvwmWindow *tmp_win,
                     unsigned long context, char *action,int* Module);
void DeadPipe(int nonsense);
void GetMwmHints(FvwmWindow *t);
void GetOlHints(FvwmWindow *t);
void SelectDecor(FvwmWindow *, unsigned long, int,int);
extern Bool PopUpMenu(MenuRoot *, int, int);
void ComplexFunction(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		     unsigned long context, char *action, int *Module);
extern int DeferExecution(XEvent *, Window *,FvwmWindow **, unsigned long *, int, int);
void SetBorder (FvwmWindow *, Bool,Bool,Bool, Window);
void move_window(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		 unsigned long context,char *action, int *Module);
void animated_move_window(XEvent *eventp,Window w,FvwmWindow *tmp_win,
			  unsigned long context,char *action, int *Module);
void move_window_to_page(XEvent *eventp,Window w,FvwmWindow *tmp_win,
			 unsigned long context,char *action, int *Module);
void set_animation(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		   unsigned long context, char *action,int* Module);
void set_menudelay(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		   unsigned long context, char *action,int* Module);
void resize_window(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		 unsigned long context,char *action, int *Module);
void CreateIconWindow(FvwmWindow *, int, int);
void SetMapStateProp(FvwmWindow *, int);
void SetStickyProp(FvwmWindow *, int, int, int);
void SetClientProp(FvwmWindow *);
void Iconify(FvwmWindow *, int, int);
void DeIconify(FvwmWindow *);
void KeepOnTop(void);
void show_panner(void);
void WaitForButtonsUp(void);
void FocusOn(FvwmWindow *t,int DeIconifyOnly);
void WarpOn(FvwmWindow *t,int warp_x, int x_unit, int warp_y, int y_unit);
/*  RBW - 11/02/1998  */
Bool PlaceWindow(FvwmWindow *tmp_win, unsigned long flags,
                 int Desk, int PageX, int PageY);
void free_window_names (FvwmWindow *tmp, Bool nukename, Bool nukeicon);

MenuStatus do_menu (MenuRoot *menu,MenuRoot *menuPrior,
		    MenuItem **pmiExecuteAction, int cmenuDeep, Bool fSticks,
		    XEvent *eventp, MenuOptions *pops);
int check_allowed_function(MenuItem *mi);
int check_allowed_function2(int function, FvwmWindow *t);
void ReInstallActiveColormap(void);
void ParsePopupEntry(char *,FILE *, char **, int *);
void ParseMouseEntry(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		unsigned long context, char *tline,int* Module);
void ParseKeyEntry(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		unsigned long context, char *tline,int* Module);
void SetOneStyle(char *text,FILE *,char **,int *);
void ParseStyle(char *text,FILE *,char **,int *);
void assign_string(char *text, FILE *fd, char **arg,int *);
void SetFlag(char *text, FILE *fd, char **arg,int *);
void SetCursor(char *text, FILE *fd, char  **arg,int *);
void SetInts(char *text, FILE *fd, char **arg,int *);
void SetBox(char *text, FILE *fd, char **arg,int *);
void set_func(char *, FILE *, char **,int *);
void copy_config(FILE **config_fd);
void SetEdgeScroll(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		     unsigned long context, char *action,int *Module);
void SetEdgeResistance(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		     unsigned long context, char *action,int *Module);
void CursorStyle(XEvent *eventp,Window junk,FvwmWindow *tmp_win,
                 unsigned long context, char *action,int* Module);
void ButtonStyle(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		 unsigned long context, char *action,int *Module);
#ifdef MULTISTYLE
void AddButtonStyle(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		 unsigned long context, char *action,int *Module);
#endif
#ifdef USEDECOR
void add_item_to_decor(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		      unsigned long context, char *action,int *Module);
void ChangeDecor(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		 unsigned long context, char *action,int *Module);
void DestroyDecor(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		  unsigned long context, char *action,int *Module);
#endif
void UpdateDecor(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		unsigned long context, char *action,int *Module);
void SetColormapFocus(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		      unsigned long context, char *action,int* Module);
void SetColorLimit(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		      unsigned long context, char *action,int* Module);

Pixel    GetShadow(Pixel);
Pixel    GetHilite(Pixel);


#define UP 1
#define DOWN 0
void MapIt(FvwmWindow *t);
void UnmapIt(FvwmWindow *t);
void do_save(void);
void checkPanFrames(void);
void raisePanFrames(void);
void initPanFrames(void);
Bool StashEventTime (XEvent *ev);
int My_XNextEvent(Display *dpy, XEvent *event);
void FlushQueue(int Module);
void QuickRestart(void);
void     AddFuncKey (char *, int, int, int, char *, int, int, MenuRoot *,
		     char , char);
char *GetNextPtr(char *ptr);

void InteractiveMove(Window *w, FvwmWindow *tmp_win, int *FinalX, int *FinalY,
		     XEvent *eventp);

MenuRoot *FindPopup(char *action);

void Bell(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		      unsigned long context,char *action, int *Module);
void scroll(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		      unsigned long context,char *action, int *Module);
void movecursor(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		      unsigned long context,char *action, int *Module);
void iconify_function(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		      unsigned long context,char *action, int *Module);
void raise_function(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		    unsigned long context, char *action, int *Module);
void lower_function(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		    unsigned long context, char *action, int *Module);
void destroy_function(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		      unsigned long context,char *action, int *Module);
void delete_function(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		     unsigned long context,char *action, int *Module);
void close_function(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		    unsigned long context,char *action, int *Module);
void restart_function(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		      unsigned long context, char *action, int *Module);
void exec_function(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		   unsigned long context, char *action, int *Module);
void exec_setup(XEvent *eventp,Window w,FvwmWindow *tmp_win,
                unsigned long context, char *action, int *Module);
void refresh_function(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		      unsigned long context, char *action, int *Module);
void refresh_win_function(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		      unsigned long context, char *action, int *Module);
void stick_function(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		    unsigned long context, char *action, int *Module);

void changeDesks_func(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		      unsigned long context,char *action, int *Module);
void changeDesks(int desk);
void changeWindowsDesk(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		       unsigned long context, char *action, int *Module);

int GetPositionArguments(char *action, int x, int y, int w, int h, int *pfinalX, int *pfinalY);
char *GetMenuOptions(char *action, Window w, FvwmWindow *tmp_win,
		     MenuItem *mi, MenuOptions *pops);
int GetTwoArguments(char *action, int *val1, int *val2, int *val1_unit, int *val2_unit);
int GetOneArgument(char *action, int *val1, int *val1_unit);
void goto_page_func(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		    unsigned long context, char *action, int *Module);

void wait_func(XEvent *eventp,Window w,FvwmWindow *tmp_win,
	       unsigned long context,char *action, int *Module);
void flip_focus_func(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		unsigned long context, char *action, int *Module);
void focus_func(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		unsigned long context, char *action, int *Module);
void warp_func(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		unsigned long context, char *action, int *Module);
void SendDataToModule(XEvent *eventp,Window w,FvwmWindow *tmp_win,
			unsigned long context, char *action,int *Module);
void send_list_func(XEvent *eventp,Window w,FvwmWindow *tmp_win,
			unsigned long context, char *action,int *Module);
void popup_func(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		unsigned long context, char *action,int *Module);
void staysup_func(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		unsigned long context, char *action,int *Module);
void quit_func(XEvent *eventp,Window w,FvwmWindow *tmp_win,
	       unsigned long context, char *action,int *Module);
void quit_screen_func(XEvent *eventp,Window w,FvwmWindow *tmp_win,
	       unsigned long context, char *action,int *Module);
void echo_func(XEvent *eventp,Window w,FvwmWindow *tmp_win,
	       unsigned long context, char *action,int *Module);
void raiselower_func(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		     unsigned long context, char *action,int *Module);
void Nop_func(XEvent *eventp,Window w,FvwmWindow *tmp_win,unsigned long context,
	      char *action, int *Module);
void SetGlobalOptions(XEvent *eventp,Window junk,FvwmWindow *tmp_win,
                      unsigned long context, char *action,int* Module);
void set_mask_function(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		     unsigned long context, char *action,int* Module);
void DestroyMenu(MenuRoot *mr);
void     GetColors(void);
Pixel    GetColor(char *);
#ifdef GRADIENT_BUTTONS
Pixel *AllocLinearGradient(char *s_from, char *s_to, int npixels);
Pixel *AllocNonlinearGradient(char *s_colors[], int clen[],
			      int nsegs, int npixels);
#endif
void     bad_binding(int num);
void     nocolor(char *note, char *name);
void     MakeMenus(void);
void GetMenuXPMFile(char *name, MenuItem *it);
void GetMenuBitmapFile(char *name, MenuItem *it);
void add_item_to_menu(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		      unsigned long context,
		      char *action, int *Module);
void destroy_menu(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		      unsigned long context,
		      char *action, int *Module);
void ModuleConfig(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		      unsigned long context,
		      char *action, int *Module);
void add_another_item(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		      unsigned long context,
		      char *action, int *Module);
void add_item_to_func(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		      unsigned long context,
		      char *action, int *Module);
void setModulePath(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		     unsigned long context, char *action,int* Module);
void setIconPath(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		     unsigned long context, char *action,int* Module);
void setPixmapPath(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		     unsigned long context, char *action,int* Module);

void ProcessNewStyle(XEvent *eventp,Window w,FvwmWindow *tmp_win,unsigned long context,
	  char *action, int *Module);
void SetHiColor(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		unsigned long context, char *action,int* Module);
void SetMenuColor(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		unsigned long context, char *action,int* Module);
void LoadIconFont(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		unsigned long context, char *action,int* Module);
void LoadWindowFont(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		unsigned long context, char *action,int* Module);
#ifdef BORDERSTYLE
void SetBorderStyle(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		    unsigned long context, char *action,int* Module);
#endif
void SetMenuStyle(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		unsigned long context, char *action,int* Module);
void SetTitleStyle(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		   unsigned long context, char *action,int* Module);
#ifdef MULTISTYLE
void AddTitleStyle(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		   unsigned long context, char *action,int* Module);
#endif
void SetDeskSize(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		     unsigned long context, char *action,int* Module);
void SetOpaque(XEvent *eventp,Window w,FvwmWindow *tmp_win,
	       unsigned long context, char *action,int* Module);
void SetXOR(XEvent *eventp,Window w,FvwmWindow *tmp_win,
	       unsigned long context, char *action,int* Module);
void SetClick(XEvent *eventp,Window w,FvwmWindow *tmp_win,
	       unsigned long context, char *action,int* Module);
void NextFunc(XEvent *eventp,Window w,FvwmWindow *tmp_win,
	      unsigned long context, char *action,int* Module);
void PrevFunc(XEvent *eventp,Window w,FvwmWindow *tmp_win,
	      unsigned long context, char *action,int* Module);
void NoneFunc(XEvent *eventp,Window w,FvwmWindow *tmp_win,
	      unsigned long context, char *action,int* Module);
void CurrentFunc(XEvent *eventp,Window w,FvwmWindow *tmp_win,
 	      unsigned long context, char *action,int* Module);
void WindowIdFunc(XEvent *eventp,Window w,FvwmWindow *tmp_win,
                  unsigned long context, char *action,int* Module);
void ReadFile(XEvent *eventp,Window junk,FvwmWindow *tmp_win,
		unsigned long context, char *action,int* Module);
void PipeRead(XEvent *eventp,Window junk,FvwmWindow *tmp_win,
		unsigned long context, char *action,int* Module);
void module_zapper(XEvent *eventp,Window junk,FvwmWindow *tmp_win,
		unsigned long context, char *action,int* Module);
char *expand(char *input, char *arguments[], FvwmWindow *tmp_win);
void Recapture(XEvent *eventp,Window junk,FvwmWindow *tmp_win,
		unsigned long context, char *action,int* Module);
void HandleHardFocus(FvwmWindow *t);
void DestroyModConfig(XEvent *eventp,Window junk,FvwmWindow *tmp_win,
                      unsigned long context, char *action,int* Module);
void AddModConfig(XEvent *eventp,Window junk,FvwmWindow *tmp_win,
                  unsigned long context, char *action,int* Module);
void SetEnv(XEvent *eventp,Window junk,FvwmWindow *tmp_win,
	    unsigned long context, char *action,int* Module);

void CoerceEnterNotifyOnCurrentWindow();

/*
** message levels for fvwm_msg:
*/
#define DBG  -1
#define INFO 0
#define WARN 1
#define ERR  2
void fvwm_msg(int type,char *id,char *msg,...);

#endif /* MISC_H */

