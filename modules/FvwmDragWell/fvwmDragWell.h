#ifndef FVWMDRAGWELL_H
#define FVWMDRAGWELL_H

#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#define MW_EVENTS (ExposureMask | ButtonReleaseMask | ButtonPressMask | \
		   ButtonMotionMask)


#define MODULE_TO_FVWM       0   /*fd[] entries*/
#define FVWM_TO_MODULE       1

#define FOUND_XEVENT           1 /*return types of myXNextEvent*/
#define FOUND_FVWM_MESSAGE     -1
#define FOUND_FVWM_NON_MESSAGE 0

#define DEFAULT_ANIMATION      1 /*xg.animationType*/

/*This is a collection of globals that deal with X*/
typedef struct XGlobals_struct {
  Window root;     /* root window of screen*/
  Window win;      /* the button/drag window*/
  Display *dpy;    /* Display */
  int xfd;         /*X file descriptor, used to check for events */
  int fdWidth;     /*fd_set_size_t fdWidth;*/
  int animationType; /*how we animate upon receipt of a drag item*/
  int screen;      /* Screen*/
  int dpyHeight;   /*height  of screen*/
  int dpyWidth;
  int dpyDepth;    /* display depth*/
  int colorset;    /*colorset*/
  Pixel fore;      /* colors..*/
  Pixel back;   
  Pixel shadow; /*colors of buttons, relief on menus*/
  Pixel hilite;
  Pixmap icon;
  char *foreColorStr; /*strings for colors*/
  char *backColorStr;
  char *shadowColorStr;
  char *hiliteColorStr;
  GC hiliteGC;
  GC shadowGC;
  GC buttonGC;
  FILE *log;       /* debugging log*/
  Atom wmAtomDelWin; /*seems to be used for closing the window*/
  char *appName;   /* name of the application */
  int x,y;     /* screen location*/
  unsigned w,h; /*width of window*/
  int dbx,dby; /* the x,y pos of drop box*/
  unsigned dbw,dbh; /*the width and height of the drop box*/
  int xneg,yneg,usposition; /*window size stuff*/;
  XSizeHints sizehints;
} XGlobals;



/*
 *Button stuff
 */
typedef struct DragWellButton_struct DragWellButton;
struct DragWellButton_struct {
  Window win;                      /*window the button is on*/
  int wx,wy;                       /*x,y position on window*/
  int w,h;                         /*width, height*/
  int state;                       /*state of button*/
  int (*drawButFace)(DragWellButton *); /*func ptr to draw something on face of button*/
};

/* The two possible states of the DragWellButton */
#define DRAGWELL_BUTTON_NOT_PUSHED     0
#define DRAGWELL_BUTTON_PUSHED         1


/*Maximum length of qfsPath */
#define MAX_PATH_LEN             256

/* Menu entry types, used by entryType[] */
#define DRAGWELLMENUITEM_DIR          1
#define DRAGWELLMENUITEM_FILE         2

/*Total time it takes to animate a sucessful selection*/
#define TOTAL_ANIMATION_TIME 500000

#define DRAG_DATA_TYPE_PATH      0
#define DRAG_DATA_TYPE_NONPATH   1
/*
 *Global stuff common to all menus
 */
typedef struct DragWellMenuGlobal_struct {
  short exportAsURL;  /* export as "file://" or "/" */
  Pixel fore,back;    /* colors...*/

  char dragData[MAX_PATH_LEN]; /*the path*/
  char dragTypeStr[MAX_PATH_LEN]; /*the path*/
  int dragDataType;
  int dragState;      /* Do we have an item to drag*/
  Atom action;        /* the XDND action we will use*/
  Atom *typelist;     /* the drag types we support, Null terminated */
  Atom textUriAtom;
  /*UNUSED*/
  char *fontname;     /* name of the font used in the menus*/
  XFontStruct *font;  /* font structure */
  int fontHeight;     /* height of the font */
  /*end UNUSED*/
} DragWellMenuGlobals;

/*UNUSED*/

/*Do we have a draggable item, used by dragState*/
#define DRAGWELLMENU_NO_DRAG_ITEM    0 
#define DRAGWELLMENU_HAVE_DRAG_ITEM  1 

#endif

