/* This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef MISC_H
#define MISC_H

#include <ctype.h>
#include "defaults.h"
#include "functions.h"
#include <libs/fvwmlib.h>


/************************************************************************
 * ReapChildren - wait() for all dead child processes
 ************************************************************************/
#include <sys/types.h>
#include <sys/wait.h>

void ReapChildren(void);

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

extern XGCValues Globalgcv;
extern unsigned long Globalgcm;
extern Time lastTimestamp;
extern XEvent Event;

extern char NoName[];
extern char NoClass[];
extern char NoResource[];

/* Start of function prototype area.
   I wonder if there is any sequence to this stuff.

   Fvwm trivia: There were 97 commands in the fvwm command table
   when the F_CMD_ARGS macro was written.
   dje 12/19/98.
   */

/* Macro for args passed to fvwm commands... */
#define F_CMD_ARGS XEvent *eventp,Window w,FvwmWindow *tmp_win,\
unsigned long context,char *action, int *Module

void SetupFrame(FvwmWindow *,int,int,int,int,Bool,Bool);
void CreateGCs(void);
void InstallWindowColormaps(FvwmWindow *);
void InstallRootColormap(void);
void UninstallRootColormap(void);
void FetchWmProtocols(FvwmWindow *);
void FetchWmColormapWindows (FvwmWindow *tmp);
void SetTitleBar(FvwmWindow *, Bool,Bool);
void RestoreWithdrawnLocation(FvwmWindow *, Bool);
void Destroy(FvwmWindow *);
void GetGravityOffsets (FvwmWindow *, int *, int *);
FvwmWindow *AddWindow(Window w, FvwmWindow *ReuseWin);
int MappedNotOverride(Window w);
void GrabButtons(FvwmWindow *);
void GrabKeys(FvwmWindow *);
void GetWindowSizeHints(FvwmWindow *);
void SwitchPages(Bool,Bool);
void NextPage(void);
void PrevPage(void);

void Maximize(F_CMD_ARGS);
void WindowShade(F_CMD_ARGS);
void setShadeAnim(F_CMD_ARGS);

Bool GrabEm(int);
void UngrabEm(void);
void CaptureOneWindow(FvwmWindow *fw, Window window);
void CaptureAllWindows(void);
void SetTimer(int);
int flush_expose(Window w);

void do_windowList(F_CMD_ARGS);
void RaiseThisWindow(int);
void SetShape(FvwmWindow *, int);
void executeModule(F_CMD_ARGS);
void initModules(void);
int HandleModuleInput(Window w, int channel);
void match_string(struct config *, char *, char *, FILE *);
void no_popup(char *ptr);
void KillModule(int channel, int place);
void ClosePipes(void);
/*  RBW - 11/02/1998  */
int SmartPlacement(FvwmWindow *t, int width, int height,int *x,int *y,
			  int pdeltax, int pdeltay);
/**/
void usage(void);
void BroadcastPacket(unsigned long event_type, unsigned long num_datum, ...);
void SendPacket(int channel, unsigned long event_type,
                unsigned long num_datum, ...);
void BroadcastConfig(unsigned long event_type, const FvwmWindow *t);
void SendConfig(int Module, unsigned long event_type, const FvwmWindow *t);
void BroadcastName(unsigned long event_type, unsigned long data1,
		   unsigned long data2, unsigned long data3, const char *name);
void SendName(int channel, unsigned long event_type, unsigned long data1,
	      unsigned long data2, unsigned long data3, const char *name);
void SendStrToModule(F_CMD_ARGS);
RETSIGTYPE DeadPipe(int nonsense);
void GetMwmHints(FvwmWindow *t);
void GetOlHints(FvwmWindow *t);
void SelectDecor(FvwmWindow *, style_flags *, int,int);
void SetBorder (FvwmWindow *, Bool,Bool,Bool, Window);
void set_animation(F_CMD_ARGS);
void CreateIconWindow(FvwmWindow *, int, int);
void SetStickyProp(FvwmWindow *, int, int, int);
void SetClientProp(FvwmWindow *);
void show_panner(void);
void WaitForButtonsUp(void);
/*  RBW - 11/02/1998  */
Bool PlaceWindow(FvwmWindow *tmp_win, style_flags *sflag, int Desk, int PageX,
		 int PageY);
void free_window_names (FvwmWindow *tmp, Bool nukename, Bool nukeicon);

int check_if_function_allowed(int function, FvwmWindow *t,
			      Bool override_allowed, char *menu_string);
void ReInstallActiveColormap(void);

void SetOneStyle(char *text,FILE *,char **,int *);
void ParseStyle(char *text,FILE *,char **,int *);
void assign_string(char *text, FILE *fd, char **arg,int *);
void SetFlag(char *text, FILE *fd, char **arg,int *);
void SetCursor(char *text, FILE *fd, char  **arg,int *);
void SetInts(char *text, FILE *fd, char **arg,int *);
void SetBox(char *text, FILE *fd, char **arg,int *);
void set_func(char *, FILE *, char **,int *);
void copy_config(FILE **config_fd);
void SetEdgeScroll(F_CMD_ARGS);
void SetEdgeResistance(F_CMD_ARGS);
void CursorStyle(F_CMD_ARGS);
void ButtonStyle(F_CMD_ARGS);
#ifdef MULTISTYLE
void AddButtonStyle(F_CMD_ARGS);
#endif
#ifdef USEDECOR
void add_item_to_decor(F_CMD_ARGS);
void ChangeDecor(F_CMD_ARGS);
void DestroyDecor(F_CMD_ARGS);
#endif
void UpdateDecor(F_CMD_ARGS);
void SetColormapFocus(F_CMD_ARGS);
void SetColorLimit(F_CMD_ARGS);

#define UP 1
#define DOWN 0
void MapIt(FvwmWindow *t);
void UnmapIt(FvwmWindow *t);
void do_save(void);
Bool StashEventTime (XEvent *ev);
void FlushQueue(int Module);
void QuickRestart(void);
char *GetNextPtr(char *ptr);

void Bell(F_CMD_ARGS);
void movecursor(F_CMD_ARGS);
void PlaceAgain_func(F_CMD_ARGS);
void iconify_function(F_CMD_ARGS);
void raise_function(F_CMD_ARGS);
void lower_function(F_CMD_ARGS);
void destroy_function(F_CMD_ARGS);
void delete_function(F_CMD_ARGS);
void close_function(F_CMD_ARGS);
void restart_function(F_CMD_ARGS);
void exec_function(F_CMD_ARGS);
void exec_setup(F_CMD_ARGS);
void refresh_function(F_CMD_ARGS);
void refresh_win_function(F_CMD_ARGS);
void stick_function(F_CMD_ARGS);

/* --- virtual.c --- */
void setEdgeThickness(F_CMD_ARGS);
Bool HandlePaging(int, int, int *, int *, int *, int *,Bool,Bool);
void checkPanFrames(void);
void raisePanFrames(void);
void initPanFrames(void);
void MoveViewport(int newx, int newy,Bool);
void changeDesks_func(F_CMD_ARGS);
void changeDesks(int desk);
void do_move_window_to_desk(FvwmWindow *tmp_win, int desk);
void move_window_to_desk(F_CMD_ARGS);
void scroll(F_CMD_ARGS);
Bool get_page_arguments(char *action, int *page_x, int *page_y);
void goto_page_func(F_CMD_ARGS);
/* --- end of virtual.c --- */

int GetMoveArguments(char *action, int x, int y, int w, int h,
                     int *pfinalX, int *pfinalY, Bool *fWarp);
int GetTwoArguments(char *action, int *val1, int *val2, int *val1_unit,
		    int *val2_unit);
int GetTwoPercentArguments(char *action, int *val1, int *val2, int *val1_unit,
		    int *val2_unit);

void wait_func(F_CMD_ARGS);
void SendDataToModule(F_CMD_ARGS);
void SendLook(int module);
void BroadcastLook(void);
void send_list_func(F_CMD_ARGS);
void popup_func(F_CMD_ARGS);
void staysup_func(F_CMD_ARGS);
void quit_func(F_CMD_ARGS);
void quit_screen_func(F_CMD_ARGS);
void echo_func(F_CMD_ARGS);
void raiselower_func(F_CMD_ARGS);
void Nop_func(F_CMD_ARGS);
void SetGlobalOptions(F_CMD_ARGS);
void Emulate(F_CMD_ARGS);
void set_mask_function(F_CMD_ARGS);
Pixel GetColor(char *);
void FreeColors(Pixel *pixels, int n);
#ifdef GRADIENT_BUTTONS
Pixel *AllocLinearGradient(char *s_from, char *s_to, int npixels);
Pixel *AllocNonlinearGradient(char *s_colors[], int clen[],
			      int nsegs, int npixels);
#endif
void bad_binding(int num);
void nocolor(char *note, char *name);

void destroy_fvwmfunc(F_CMD_ARGS);
void ModuleConfig(F_CMD_ARGS);
void add_another_item(F_CMD_ARGS);
void add_item_to_func(F_CMD_ARGS);
void setModulePath(F_CMD_ARGS);
void imagePath_function(F_CMD_ARGS);
void iconPath_function(F_CMD_ARGS);
void pixmapPath_function(F_CMD_ARGS);
void SetHiColor(F_CMD_ARGS);
void SetDefaultColors(F_CMD_ARGS);
void SetDefaultIcon(F_CMD_ARGS);
void LoadDefaultFont(F_CMD_ARGS);
void LoadIconFont(F_CMD_ARGS);
void LoadWindowFont(F_CMD_ARGS);
#ifdef BORDERSTYLE
void SetBorderStyle(F_CMD_ARGS);
#endif
void SetTitleStyle(F_CMD_ARGS);
#ifdef MULTISTYLE
void AddTitleStyle(F_CMD_ARGS);
#endif
void SetDeskSize(F_CMD_ARGS);
void SetOpaque(F_CMD_ARGS);
void SetXOR(F_CMD_ARGS);
void SetXORPixmap(F_CMD_ARGS);
void SetClick(F_CMD_ARGS);
void SetSnapAttraction(F_CMD_ARGS);
void SetSnapGrid(F_CMD_ARGS);
void NextFunc(F_CMD_ARGS);
void PrevFunc(F_CMD_ARGS);
void NoneFunc(F_CMD_ARGS);
void CurrentFunc(F_CMD_ARGS);
void DirectionFunc(F_CMD_ARGS);
void WindowIdFunc(F_CMD_ARGS);
void PickFunc(F_CMD_ARGS);
void AllFunc(F_CMD_ARGS);
void ReadFile(F_CMD_ARGS);
void PipeRead(F_CMD_ARGS);
void module_zapper(F_CMD_ARGS);
void Recapture(F_CMD_ARGS);
void RecaptureWindow(F_CMD_ARGS);
void HandleHardFocus(FvwmWindow *t);
void DestroyModConfig(F_CMD_ARGS);
void AddModConfig(F_CMD_ARGS);
void SetEnv(F_CMD_ARGS);

void CoerceEnterNotifyOnCurrentWindow(void);

void change_layer(F_CMD_ARGS);
void SetDefaultLayers(F_CMD_ARGS);

/*
** message levels for fvwm_msg:
*/
#define DBG  -1
#define INFO 0
#define WARN 1
#define ERR  2
void fvwm_msg(int type,char *id,char *msg,...);

#ifdef GNOME
/* GNOME window manager hints support */

/* initalization */
void GNOME_Init(void);

/* client messages; setting hints on a window comes through this mechanism */
int  GNOME_ProcessClientMessage(FvwmWindow *fwin, XEvent *ev);

/* hook into .fvwm2rc functions */
void GNOME_ButtonFunc(
         XEvent *eventp,
	 Window w,
	 FvwmWindow *fwin,
	 unsigned long context,
	 char *action,
	 int *Module);

/* get hints on a window; sets parameters in a FvwmWindow */
void GNOME_GetHints(FvwmWindow *fwin);
/* get hints on a window, set parameters in style */
void GNOME_GetStyle(FvwmWindow *fwin, window_style *style);

/* set hints on a window from parameters in FvwmWindow */
void GNOME_SetHints(FvwmWindow *fwin);

void GNOME_SetLayer(FvwmWindow *fwin);
void GNOME_SetDesk(FvwmWindow *fwin);

/* update public window manager information */
void GNOME_SetAreaCount(void);
void GNOME_SetDeskCount(void);
void GNOME_SetCurrentArea(void);
void GNOME_SetCurrentDesk(void);
void GNOME_SetClientList(void);

#endif /* GNOME */


/* needed in misc.h */
typedef enum {
  ADDED_NONE = 0,
  ADDED_MENU,
#ifdef USEDECOR
  ADDED_DECOR,
#endif
  ADDED_FUNCTION
} last_added_item_type;

void set_last_added_item(last_added_item_type type, void *item);
void NewFontAndColor(Font newfont, Pixel color, Pixel backcolor);
Bool IsRectangleOnThisPage(rectangle *rec, int desk);

void Keyboard_shortcuts(XEvent *, FvwmWindow*, int);

#endif /* MISC_H */
