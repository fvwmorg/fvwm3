/* -*-c-*- */

#ifndef FVWMSCRIPT_TYPES_H
#define FVWMSCRIPT_TYPES_H

#include "libs/fvwmlib.h"
#include "libs/Module.h"
#include "libs/Picture.h"
#include "libs/PictureUtils.h"
#include "libs/PictureGraphics.h"
#include "libs/Colorset.h"
#include "libs/Flocale.h"
#include "libs/Rectangles.h"
#include "libs/fsm.h"

#include <stdio.h>
#include <signal.h>
#include <ctype.h>

#include <unistd.h>
#include <fcntl.h>
#include "libs/ftime.h"

#define XK_MISCELLANY
#include <sys/types.h>

#if HAVE_SYS_BSDTYPES_H
#include <sys/bsdtypes.h> /* Saul */
#endif

/* flags */
#define IS_HIDDEN(x) (x->flags[0])
#define HAS_NO_RELIEF_STRING(x) (x->flags[1])
#define HAS_NO_FOCUS(x) (x->flags[2])
#define TEXT_POS_DEFAULT 0
#define TEXT_POS_CENTER 1
#define TEXT_POS_LEFT 2
#define TEXT_POS_RIGHT 3
#define IS_TEXT_POS_DEFAULT(x) (x->flags[3] == TEXT_POS_DEFAULT)
#define GET_TEXT_POS(x) (x->flags[3])
#define IS_TEXT_CENTER(x) (x->flags[3] == TEXT_POS_CENTER)
#define IS_TEXT_LEFT(x) (x->flags[3] == TEXT_POS_LEFT)
#define IS_TEXT_RIGHT(x) (x->flags[3] == TEXT_POS_RIGHT)

#include <X11/keysymdef.h>
#include <X11/keysym.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <X11/cursorfont.h>
#include <X11/Xatom.h>

/* Prototype for yylex().  There must be a way to get this automatically from
   yacc/bison.  */
extern int yylex(void);

/* Color constants */
#define back 0
#define fore 1
#define shad 2
#define hili 3

/* Widget Type constants */
#define PushButton 1
#define RadioButton 2
#define ItemDraw 3
#define CheckBox 4
#define TextField 5
#define HScrollBar 6
#define VScrollBar 7
#define PopupMenu 8
#define Rectangle 9
#define MiniScroll 10
#define SwallowExec 11
#define HDipstick 12
#define VDipstick 13
#define List 14
#define Menu 15

/* Structure that groups command and conditional loop */
typedef struct {
	int   Type;   /* comman type */
	long *TabArg; /* array of arguments: id var,fnc,const,adr block... */
	int   NbArg;
} Instr;

/* Structure for instruction block */
typedef struct {
	Instr *TabInstr; /* array of instructions */
	int    NbInstr;	 /* number of instructions */
} Bloc;

/* Structure to store script header: default value */
typedef struct /* X management type */
{
	int   x, y;	     /* Window origin */
	int   height, width; /* Height, Width */
	char *titlewin;	     /* Title */
	char *forecolor;     /* Foreground color */
	char *backcolor;     /* Background color */
	char *shadcolor;     /* Shaded color */
	char *hilicolor;     /* Hilighted color */
	int   colorset;
	char *font;	     /* Font */
	char *icon;	     /* Icon for iconnized app */
	Bool  usegettext;    /* Is gettext used? */
	char *localepath;    /* path for gettext */
	Bloc *periodictasks; /* Periodic tasks array */
	Bloc *initbloc;	     /* Initalization block */
	Bloc *quitfunc;	     /* Block executed at exit */
} ScriptProp;

typedef struct {
	int  NbCase;
	int *LstCase; /* Array of case values to be checked */
} CaseObj;

/* Structure to store graphical objects */
typedef struct /* Buttons Type */
{
	int   id;
	char *type;
	int   x, y;	     /* Origin */
	int   height, width; /* Height, Width */
	char *title;	     /* Title */
	char *swallow;	     /* swallowexec */
	char *icon;	     /* Icon */
	char *forecolor;     /* Forground color */
	char *backcolor;     /* Backgournd color */
	char *shadcolor;
	char *hilicolor;
	int   colorset;
	char *font;	/* Font */
	int   flags[5]; /* Button state: hidden, unchecked, checked */
	int   value;
	int   value2;
	int   value3;
} MyObject;

typedef MyObject TabObj[1000];

typedef struct {
	Window	   root;
	Window	   win;
	GC	   gc;
	Pixel	   TabColor[4];
	XSizeHints size;
	char	  *backcolor;
	char	  *forecolor;
	char	  *shadcolor;
	char	  *hilicolor;
	int	   colorset;
	char	  *font;
	char	  *cursor;
	char	  *icon;
	char	  *title;
	Bloc	  *periodictasks;
	Bloc	  *quitfunc;
	int	   HaveXSelection;
	char	  *TabScriptId[999];
	int	   NbChild;
	time_t	   BeginTime;
	char	  *TabArg[20];
	Bool	   swallowed;
	Window	   swallower_win;
} X11base;

typedef struct {
	char *Msg;
	char *R;
} TypeName;

typedef struct {
	int	 NbMsg;
	TypeName TabMsg[40];
} TypeBuffSend;

struct XObj {
	int	     TypeWidget;
	Window	     win;	/* Window containing the oject */
	Window	    *ParentWin; /* Parent window */
	Pixel	     TabColor[4];
	GC	     gc;	    /* gc used for requests: 4 bytes */
	int	     id;	    /* ID */
	int	     x, y;	    /* Origin */
	int	     height, width; /* Height, Width */
	char	    *title;	    /* Title */
	char	    *swallow;	    /* SwallowExec */
	char	    *icon;	    /* Icon */
	char	    *forecolor;	    /* Foreground color */
	char	    *backcolor;	    /* Background color */
	char	    *shadcolor;	    /* Shaded color */
	char	    *hilicolor;	    /* Hilighted color */
	int	     colorset;
	char	    *font;	       /* Font */
	Pixmap	     iconPixmap;       /* Loaded icon */
	Pixmap	     icon_maskPixmap;  /* Icon mask */
	Pixmap	     icon_alphaPixmap; /* alpha channel */
	Pixel	    *alloc_pixels;
	int	     nalloc_pixels;
	int	     icon_w, icon_h; /* Icon Width and Height */
	FlocaleFont *Ffont;
	int	     value;  /* Current value */
	int	     value2; /* Min value */
	int	     value3; /* Max value */
	int	     flags[5];
	void (*InitObj)(struct XObj *xobj);    /* Object Initializer */
	void (*DestroyObj)(struct XObj *xobj); /* Object Destructor */
	void (*DrawObj)(struct XObj *xobj, XEvent *evp); /* Object Drawing */
	void (*EvtMouse)(struct XObj *xobj, XButtonEvent *EvtButton);
	void (*EvtKey)(struct XObj *xobj, XKeyEvent *EvtKey);
	void (*ProcessMsg)(struct XObj *xobj, unsigned long type,
	    unsigned long *body);
	void *UserPtr;
};

void ChooseFunction(struct XObj *xobj, char *type);

void SendMsg(struct XObj *xobj, int TypeMsg);

void ExecBloc(Bloc *bloc);

void UnselectAllTextField(struct XObj **xobj);

void Quit(int NbArg, long *TabArg) __attribute__((noreturn));

void SendMsgToScript(XEvent event);

#endif /* FVWMSCRIPT_TYPES_H */
