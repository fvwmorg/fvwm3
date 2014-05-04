/* -*-c-*- */
/*
 * FvwmButtons, copyright 1996, Jarl Totland
 *
 * This module, and the entire GoodStuff program, and the concept for
 * interfacing this module to the Window Manager, are all original work
 * by Robert Nation
 */

/* -------------------------------- DEBUG ---------------------------------- */

/* Uncomment some of these defines to get (lots of) debug output. If you can't
   get it started, DEBUG_LOADDATA and DEBUG_INIT might be handy. The default
   DEBUG only generates output on warnings, and adds the functions DumpButtons
   and SaveButtons */
#if 0
#define DEBUG
#define DEBUG_LOADDATA /* Get updates on loading */
#define DEBUG_INIT     /* Get startup progress */
#define DEBUG_HANGON   /* Debug hangon, swallow and unswallow */
/* #define DEBUG_EVENTS */  /* Get much info on events */
#define DEBUG_FVWM     /* Debug communciation with fvwm */
/* #define DEBUG_X       */
#endif

/* ---------------------------- compatibility ------------------------------ */

/*#define OLD_EXPOSE*/     /* Try this if resizing/exposes screw up */

/* -------------------------------- more  ---------------------------------- */

#include "libs/fvwmlib.h"
#include "libs/Picture.h"
#include "libs/Flocale.h"

/* ------------------------------- structs --------------------------------- */

/* flags for b->flags */
typedef struct
{
	unsigned b_Container  : 1;     /* Contains several buttons */
	unsigned b_Font       : 1;     /* Has personal font data */
	unsigned b_Fore       : 1;     /* Has personal text color */
	unsigned b_Back       : 1;     /* Has personal background color (or "none") */
	unsigned b_Padding    : 1;     /* Has personal padding data */
	unsigned b_Frame      : 1;     /* Has personal framewidth */
	unsigned b_Title      : 1;     /* Contains title */
	unsigned b_Icon       : 1;     /* Contains icon */
	unsigned b_Swallow    : 1;     /* Contains swallowed window */
	unsigned b_Action     : 1;     /* Fvwm action when clicked on */
	unsigned b_Hangon     : 1;     /* Is waiting for a window before
	                                  turning active */
	unsigned b_Justify    : 1;     /* Has justification info */
	unsigned b_Size       : 1;     /* Has a minimum size, don't guess */
	unsigned b_IconBack   : 1;     /* Has an icon as background */
	unsigned b_IconParent : 1;     /* Parent has an icon as background */
	unsigned b_TransBack  : 1;     /* Transparent background */
	unsigned b_Left       : 1;     /* Button is left-aligned */
	unsigned b_Right      : 1;     /* Button is right-aligned */
	unsigned b_Top        : 1;     /* Button is aligned at the top */
	unsigned b_SizeFixed  : 1;     /* User provided rows/columns may not be
	                                  altered */
	unsigned b_PosFixed   : 1;     /* User provided button position */
	unsigned b_SizeSmart  : 1;     /* Improved button box sizing */
	unsigned b_Colorset   : 1;     /* use colorset instead of fore/back */
	unsigned b_ColorsetParent : 1; /* Parent has a colorset background */
	unsigned b_Panel      : 1;     /* like swallow but drawn differently */
	unsigned b_ActionIgnoresClientWindow : 1;
	                               /* Actions work only on the background
	                                  of a button with a swallowed app. */
	unsigned b_ActionOnPress  : 1; /* By default this only done on Popup */
	unsigned b_Id             : 1; /* Has user-defined id for referencing */
	unsigned b_ActiveIcon     : 1; /* Use alternate Icon on hover */
	unsigned b_ActiveColorset : 1; /* Use alternate colorset on hover */
	unsigned b_ActiveTitle    : 1; /* Use alternate Title text on hover */
	unsigned b_PressIcon      : 1; /* Use alternate Icon on press */
	unsigned b_PressColorset  : 1; /* Use alternate Colorset on press */
	unsigned b_PressTitle     : 1; /* Use alternate Title text on press */
} flags_type;

/* Flags for b->swallow */
#define b_Count       0x0003 /* Init counter for swallowing */
#define b_NoHints     0x0004 /* Ignore window hints from swallowed window */
#define b_NoClose     0x0008 /* Don't close window when exiting, unswallow it */
#define b_Kill        0x0010 /* Don't close window when exiting, kill it */
#define b_Respawn     0x0020 /* Respawn if swallowed window dies */
#define b_UseOld      0x0040 /* Try to capture old window, don't spawn it */
#define b_UseTitle    0x0080 /* Allow window to write to b->title */
#define b_FvwmModule  0x0100 /* consider the swallowed app as module */

/* Flags for b->justify */
#define b_TitleHoriz  0x03 /* Mask for title x positioning info */
#define b_Horizontal  0x04 /* HACK: stack title and iconwin horizontally */

typedef struct button_info_struct button_info;
typedef struct container_info_struct container_info;
#define ushort unsigned int
#define byte unsigned char

/* This structure contains data that the parents give their children */
struct container_info_struct
{
	button_info **buttons;     /* Required fields */
	int allocated_buttons;
	int num_buttons;
	int num_columns;
	int num_rows;
	int xpos;
	int ypos;
	int width;
	int height;

	flags_type flags;          /* Which data is set in this container? */
	byte justify;              /* b_Justify */
	byte justify_mask;         /* b_Justify */
	unsigned int swallow;      /* b_Swallow */
	unsigned int swallow_mask; /* b_Swallow */
	byte xpad, ypad;           /* b_Padding */
	signed char framew;        /* b_Frame */
	FlocaleFont *Ffont;        /* b_Font */
	char *font_string;         /* b_Font */
	char *back;                /* b_Back && !b_IconBack */
	char *back_file;           /* b_Back && b_IconBack */
	char *fore;                /* b_Fore */
	int colorset;              /* b_Colorset */
	int activeColorset;        /* b_ActiveColorset */
	int pressColorset;         /* b_PressColorset */
	Pixel fc;                  /* b_Fore */
	Pixel bc, hc, sc;          /* b_Back && !b_IconBack */
	FvwmPicture *backicon;     /* b_Back && b_IconBack */
	ushort minx, miny;         /* b_Size */
};

typedef struct
{
	unsigned smooth : 1;
	unsigned ignore_lrborder : 1;
	unsigned ignore_tbborder : 1;
	unsigned buttons_lrborder : 1;
	unsigned buttons_tbborder : 1;
	unsigned relative_x_pixel : 1;
	unsigned relative_y_pixel : 1;
	unsigned panel_indicator : 1; /* b_Panel */
} panel_flags_type;

struct button_info_struct
{
	/* required fields */
	flags_type flags;
	int  BPosX,BPosY;          /* position in button units from top left */
	unsigned int BWidth,BHeight;    /* width and height in button units  */
	button_info *parent;
	int n;                   /* number in parent */

	/* conditional fields */ /* applicable if these flags are set */
	char *id;                /* b_Id */
	FlocaleFont *Ffont;      /* b_Font */
	char *font_string;       /* b_Font */
	char *back;              /* b_Back */
	char *fore;              /* b_Fore */
	int colorset;            /* b_Colorset */
	byte xpad, ypad;         /* b_Padding */
	signed char framew;      /* b_Frame */
	byte justify;            /* b_Justify */
	byte justify_mask;       /* b_Justify */
	container_info *c;       /* b_Container */
	char *title;             /* b_Title */
	char *activeTitle;       /* b_ActiveTitle */
	char *pressTitle;        /* b_PressTitle */
	char **action;           /* b_Action */
	char *icon_file;         /* b_Icon */
	char *active_icon_file;  /* b_ActiveIcon */
	char *press_icon_file;   /* b_PressIcon */
	char *hangon;            /* b_Hangon || b_Swallow */
	Pixel fc;                /* b_Fore */
	Pixel bc, hc, sc;        /* b_Back && !b_IconBack */
	ushort minx, miny;       /* b_Size */
	FvwmPicture *icon;       /* b_Icon */
	FvwmPicture *backicon;   /* b_Back && b_IconBack */
	FvwmPicture *activeicon; /* b_ActiveIcon */
	FvwmPicture *pressicon;  /* b_PressIcon */
	int activeColorset;      /* b_ActiveColorset */
	int pressColorset;       /* b_PressColorset */
	Window IconWin;          /* b_Swallow */
	Window PanelWin;         /* b_Panel */
	Window BackIconWin;      /* b_Back && b_IconBack */

	unsigned int swallow;      /* b_Swallow */
	unsigned int swallow_mask; /* b_Swallow */
	int icon_w, icon_h;        /* b_Swallow */
	Window IconWinParent;      /* b_Swallow */
	XSizeHints *hints;         /* b_Swallow && !b_NoHints */
	char *spawn;               /* b_Swallow */
	int x, y;                  /* b_Swallow */
	ushort w, h, bw;           /* b_Swallow */

	/* The newflags struct can probably be merged in with flags_type struct. */
	struct
	{
		unsigned is_panel : 1;        /* b_Panel */
		unsigned panel_mapped : 1;    /* b_Panel */
		unsigned panel_iconified : 1; /* b_Panel */
		unsigned panel_shaded : 1;    /* b_Panel */
		unsigned do_swallow_new : 1;
	} newflags;

#define SLIDE_UP 'u'
#define SLIDE_DOWN 'd'
#define SLIDE_LEFT 'l'
#define SLIDE_RIGHT 'r'
#define SLIDE_GEOMETRY 'g'
	char slide_direction;        /* b_Panel */
#define SLIDE_POSITION_CENTER 'c'
#define SLIDE_POSITION_LEFT_TOP 'l'
#define SLIDE_POSITION_RIGHT_BOTTOM 'r'
	char  slide_position;        /* b_Panel */
#define SLIDE_CONTEXT_PB 'b'
#define SLIDE_CONTEXT_MODULE 'm'
#define SLIDE_CONTEXT_ROOT 'r'
	char  slide_context;         /* b_Panel */
	int relative_x;              /* b_Panel */
	int relative_y;              /* b_Panel */
	int slide_steps;             /* b_Panel */
	int slide_delay_ms;          /* b_Panel */
	int indicator_size;          /* b_Panel */
	panel_flags_type panel_flags;
};

#include "button.h"

/* -------------------------------- prototypes ----------------------------- */
void AddButtonAction(button_info*, int, char*);
void MakeContainer(button_info*);
void change_swallowed_window_colorset(button_info *b, Bool do_clear);
void handle_xinerama_string(char *args);
int LoadIconFile(const char *s, FvwmPicture **p, int cset);
void SetTransparentBackground(button_info *ub, int w, int h);
void exec_swallow(char *action, button_info *b);

char *GetButtonAction(button_info*,int);
void ButtonPressProcess(button_info *b, char **act);

/* ----------------------------- global variables -------------------------- */

extern Display *Dpy;
extern Window Root;
extern Window MyWindow;
extern char *MyName;
extern button_info *UberButton, *CurrentButton, *ActiveButton;
extern Bool is_pointer_in_current_button;

extern char *imagePath;
extern int fd[];

extern int new_desk;
extern GC NormalGC;
extern GC ShadowGC;
extern FlocaleWinString *FwinString;

/* ---------------------------------- misc --------------------------------- */

