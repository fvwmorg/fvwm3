/*
   FvwmButtons v2.0.41-plural-Z-alpha, copyright 1996, Jarl Totland

 * This module, and the entire GoodStuff program, and the concept for
 * interfacing this module to the Window Manager, are all original work
 * by Robert Nation
 *
 * Copyright 1993, Robert Nation. No guarantees or warantees or anything
 * are provided or implied in any way whatsoever. Use this program at your
 * own risk. Permission to use this program for any purpose is given,
 * as long as the copyright is kept intact.

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

#define OLD_EXPOSE     /* Try this if resizing/exposes screw up */

/* -------------------------------- more  ---------------------------------- */

#include "../../libs/fvwmlib.h"

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

/* Flags for b->swallow */
#define b_Count       0x03 /* Init counter for swallowing */
#define b_NoHints     0x04 /* Ignore window hints from swallowed window */
#define b_NoClose     0x08 /* Don't close window when exiting, unswallow it */
#define b_Kill        0x10 /* Don't close window when exiting, kill it */
#define b_Respawn     0x20 /* Respawn if swallowed window dies */
#define b_UseOld      0x40 /* Try to capture old window, don't spawn it */
#define b_UseTitle    0x80 /* Allow window to write to b->title */

/* Flags for b->justify */
#define b_TitleHoriz  0x03 /* Mask for title x positioning info */
#define b_Horizontal  0x04 /* HACK: stack title and iconwin horizontally */

typedef struct panel_info_struct panel_info;
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
  int ButtonWidth;
  int ButtonHeight;
  int xpos,ypos;

  unsigned long flags;     /* Which data are set in this container? */
  byte justify;            /* b_Justify */
  byte justify_mask;       /* b_Justify */
  byte swallow;            /* b_Swallow */
  byte swallow_mask;       /* b_Swallow */
  byte xpad,ypad;          /* b_Padding */
  signed char framew;             /* b_Frame */
  XFontStruct *font;       /* b_Font */
  char *font_string;       /* b_Font */
  char *back;              /* b_Back && !b_IconBack */
  char *back_file;         /* b_Back && b_IconBack */
  char *fore;              /* b_Fore */
  Pixel fc;                /* b_Fore */
  Pixel bc,hc,sc;          /* b_Back && !b_IconBack */
  Picture *backicon;       /* b_Back && b_IconBack */
  ushort minx,miny;        /* b_Size */
};

struct button_info_struct
{
  /* required fields */
  unsigned long flags;
  int  BPosX,BPosY;        /* position in button units from top left */
  byte BWidth,BHeight;     /* width and height in button units  */
  button_info *parent;
  int n;                   /* number in parent */

  /* conditional fields */ /* applicable if these flags are set */
  XFontStruct *font;       /* b_Font */
  char *font_string;       /* b_Font */
  char *back;              /* b_Back */
  char *fore;              /* b_Fore */
  byte xpad,ypad;          /* b_Padding */
  signed char framew;             /* b_Frame */
  byte justify;            /* b_Justify */
  byte justify_mask;       /* b_Justify */
  container_info *c;       /* b_Container */
  char *title;             /* b_Title */
  char **action;           /* b_Action */
  char *icon_file;         /* b_Icon */
  char *hangon;            /* b_Hangon || b_Swallow */
  Window IconWin;          /* b_Icon || b_Swallow */
  Pixel fc;                /* b_Fore */
  Pixel bc,hc,sc;          /* b_Back && !b_IconBack */
  Picture *backicon;       /* b_Back && b_IconBack */
  ushort minx,miny;        /* b_Size */
  Picture *icon;           /* b_Icon */

  byte swallow;            /* b_Swallow */
  byte swallow_mask;       /* b_Swallow */
  int icon_w,icon_h;       /* b_Swallow */
  Window IconWinParent;    /* b_Swallow */
  XSizeHints *hints;       /* b_Swallow && !b_NoHints */
  char *spawn;             /* b_Swallow */
  int x,y;                 /* b_Swallow */
  ushort w,h,bw;           /* b_Swallow */
};

struct panel_info_struct
{ button_info  *uber;      /* panel */
  panel_info   *next;
};

#include "button.h"

/* -------------------------------- prototypes ----------------------------- */
void AddButtonAction(button_info*,int,char*);
void MakeContainer(button_info*);
#ifdef DEBUG
char *mymalloc(int);
#else
#define mymalloc(a) safemalloc(a)
#endif

/* ----------------------------- global variables -------------------------- */

extern Display *Dpy;
extern Window Root;
extern Window MyWindow;
extern char *MyName;
extern button_info *UberButton,*CurrentButton;
extern panel_info *MainPanel, *CurrentPanel;

extern char *iconPath;
extern char *pixmapPath;
extern int fd[];

extern int screen;
extern int d_depth;
extern int new_desk;
extern GC  NormalGC;
extern int x,y,xneg,yneg,w,h; /* Dirty... */

/* ---------------------------------- misc --------------------------------- */

