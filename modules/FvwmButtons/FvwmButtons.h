/* -*-c-*- */
/*
 * FvwmButtons, copyright 1996, Jarl Totland
 *
 * This module, and the entire GoodStuff program, and the concept for
 * interfacing this module to the Window Manager, are all original work
 * by Robert Nation
 *
 * Copyright 1993, Robert Nation. No guarantees or warantees or anything
 * are provided or implied in any way whatsoever. Use this program at your
 * own risk. Permission to use this program for any purpose is given,
 * as long as the copyright is kept intact.
 *
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
#define b_Container  0x00000001 /* Contains several buttons */
#define b_Font       0x00000002 /* Has personal font data */
#define b_Fore       0x00000004 /* Has personal text color */
#define b_Back       0x00000008 /* Has personal background color (or "none")*/
#define b_Padding    0x00000010 /* Has personal padding data */
#define b_Frame      0x00000020 /* Has personal framewidth */
#define b_Title      0x00000040 /* Contains title */
#define b_Icon       0x00000080 /* Contains icon */
#define b_Swallow    0x00000100 /* Contains swallowed window */
#define b_Action     0x00000200 /* Fvwm action when clicked on */
#define b_Hangon     0x00000400 /* Is waiting for a window before turning
				 * active */
#define b_Justify    0x00000800 /* Has justification info */
#define b_Size       0x00001000 /* Has a minimum size, don't guess */
#define b_IconBack   0x00002000 /* Has an icon as background */
#define b_IconParent 0x00004000 /* Parent button has an icon as background */
#define b_TransBack  0x00008000 /* Transparent background */
#define b_Left       0x00010000 /* Button is left-aligned */
#define b_Right      0x00020000 /* Button is right-aligned */
#define b_SizeFixed  0x00040000 /* User provided rows/columns may not be
				 * altered */
#define b_PosFixed   0x00080000 /* User provided button position */
#define b_SizeSmart  0x00100000 /* Improved button box sizing */
#define b_Colorset   0x00200000 /* use colorset instead of fore/back colours */
#define b_ColorsetParent 0x00400000 /* Parent has a colorset background */
#define b_Panel      0x00800000 /* similar to swallow, but drawn differently */
#define b_ActionIgnoresClientWindow \
		     0x01000000 /* Actions work only on the background of a
				 * button with a swallowed app. */
#define b_ActionOnPress \
		     0x02000000 /* By default this only done on Popup */
#define b_Id            0x04000000 /* Has a user defined id for referencing */
#define b_HoverIcon     0x08000000 /* Use alternate Icon on hover */
#define b_HoverColorset 0x10000000 /* Use alternate colorset on hover */
#define b_HoverTitle    0x20000000 /* Use alternate Title text on hover */
#define b_PressIcon     0x40000000 /* Use alternate Icon on press */
#define b_PressColorset 0x80000000 /* Use alternate Colorset on press */
/* FIXME: We're out of bits!
   Nasty hack: b_PressColorset is used by UberButton & it would never use
   b_PressTitle (& vice-versa) so they have the same bit-value. */
#define b_PressTitle    0x80000000 /* Use alternate Title text on press */

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
  button_info **buttons;   /* Required fields */
  int allocated_buttons;
  int num_buttons;
  int num_columns;
  int num_rows;
  int xpos;
  int ypos;
  int width;
  int height;

  unsigned long flags;       /* Which data are set in this container? */
  byte justify;              /* b_Justify */
  byte justify_mask;         /* b_Justify */
  unsigned int swallow;      /* b_Swallow */
  unsigned int swallow_mask; /* b_Swallow */
  byte xpad,ypad;            /* b_Padding */
  signed char framew;        /* b_Frame */
  FlocaleFont *Ffont;        /* b_Font */
  char *font_string;       /* b_Font */
  char *back;              /* b_Back && !b_IconBack */
  char *back_file;         /* b_Back && b_IconBack */
  char *fore;              /* b_Fore */
  int colorset;            /* b_Colorset */
  int hoverColorset;       /* b_HoverColorset */
  int pressColorset;       /* b_PressColorset */
  Pixel fc;                /* b_Fore */
  Pixel bc,hc,sc;          /* b_Back && !b_IconBack */
  FvwmPicture *backicon;   /* b_Back && b_IconBack */
  ushort minx,miny;        /* b_Size */
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
  unsigned long flags;
  int  BPosX,BPosY;               /* position in button units from top left */
  unsigned int BWidth,BHeight;     /* width and height in button units  */
  button_info *parent;
  int n;                   /* number in parent */

  /* conditional fields */ /* applicable if these flags are set */
  char *id;                /* b_Id */
  FlocaleFont *Ffont;      /* b_Font */
  char *font_string;       /* b_Font */
  char *back;              /* b_Back */
  char *fore;              /* b_Fore */
  int colorset;            /* b_Colorset */
  byte xpad,ypad;          /* b_Padding */
  signed char framew;      /* b_Frame */
  byte justify;            /* b_Justify */
  byte justify_mask;       /* b_Justify */
  container_info *c;       /* b_Container */
  char *title;             /* b_Title */
  char *hoverTitle;        /* b_HoverTitle */
  char *pressTitle;        /* b_PressTitle */
  char **action;           /* b_Action */
  char *icon_file;         /* b_Icon */
  char *hover_icon_file;   /* b_HoverIcon */
  char *press_icon_file;   /* b_PressIcon */
  char *hangon;            /* b_Hangon || b_Swallow */
  Pixel fc;                /* b_Fore */
  Pixel bc,hc,sc;          /* b_Back && !b_IconBack */
  ushort minx,miny;        /* b_Size */
  FvwmPicture *icon;       /* b_Icon */
  FvwmPicture *backicon;   /* b_Back && b_IconBack */
  FvwmPicture *hovericon;  /* b_HoverIcon */
  FvwmPicture *pressicon;  /* b_PressIcon */
  Window IconWin;          /* b_Swallow */
  Window PanelWin;         /* b_Panel */
  Window BackIconWin;      /* b_Back && b_IconBack */

  unsigned int swallow;       /* b_Swallow */
  unsigned int swallow_mask;  /* b_Swallow */
  int icon_w,icon_h;          /* b_Swallow */
  Window IconWinParent;       /* b_Swallow */
  XSizeHints *hints;          /* b_Swallow && !b_NoHints */
  char *spawn;                /* b_Swallow */
  int x,y;                    /* b_Swallow */
  ushort w,h,bw;              /* b_Swallow */

  struct
  {
    unsigned is_panel : 1;        /* b_Panel */
    unsigned panel_mapped : 1;    /* b_Panel */
    unsigned panel_iconified : 1; /* b_Panel */
    unsigned panel_shaded : 1;    /* b_Panel */
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
void AddButtonAction(button_info*,int,char*);
void MakeContainer(button_info*);
void change_swallowed_window_colorset(button_info *b, Bool do_clear);
#ifdef DEBUG
char *mymalloc(int);
#else
#define mymalloc(a) safemalloc(a)
#endif
void handle_xinerama_string(char *args);
int LoadIconFile(const char *s, FvwmPicture **p, int cset);
void SetTransparentBackground(button_info *ub,int w,int h);
void exec_swallow(char *action, button_info *b);

/* ----------------------------- global variables -------------------------- */

extern Display *Dpy;
extern Window Root;
extern Window MyWindow;
extern char *MyName;
extern button_info *UberButton, *CurrentButton, *HoverButton;
extern Bool is_pointer_in_current_button;

extern char *imagePath;
extern int fd[];

extern int new_desk;
extern GC  NormalGC;
extern GC  ShadowGC;
extern FlocaleWinString *FwinString;

/* ---------------------------------- misc --------------------------------- */

