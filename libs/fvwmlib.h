#ifndef FVWMLIB_H
#define FVWMLIB_H

#include "config.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>
#include <X11/Intrinsic.h>              /* needed for xpm.h and Pixel defn */
#include <ctype.h>

/* Allow GCC extensions to work, if you have GCC */

#ifndef __attribute__
/* This feature is available in gcc versions 2.5 and later.  */
# if __GNUC__ < 2 || (__GNUC__ == 2 && __GNUC_MINOR__ < 5) || __STRICT_ANSI__
#  define __attribute__(x)
# endif
/* The __-protected variants of `format' and `printf' attributes
   are accepted by gcc versions 2.6.4 (effectively 2.7) and later.  */
# if __GNUC__ < 2 || (__GNUC__ == 2 && __GNUC_MINOR__ < 7)
#  define __format__ format
#  define __printf__ printf
# endif
#endif

/***********************************************************************
 * Generic debugging
 ***********************************************************************/

#ifndef DEBUG
# define DB(_x)
#else
# ifndef __FILE__
#  define __FILE__ "?"
#  define __LINE__ 0
# endif
# define DB(_x) do{f_db_info.filenm=__FILE__;f_db_info.lineno=__LINE__;\
                   f_db_print _x;}while(0)
struct f_db_info { const char *filenm; unsigned long lineno; };
extern struct f_db_info f_db_info;
extern void f_db_print(const char *fmt, ...)
                       __attribute__ ((__format__ (__printf__, 1, 2)));
#endif


/***********************************************************************
 * Replacements for missing system calls.
 ***********************************************************************/

#ifndef HAVE_ATEXIT
int atexit( void(*func)() );
#endif

#ifndef HAVE_GETHOSTNAME
int gethostname( char* name, int len );
#endif

#ifndef HAVE_STRCASECMP
int strcasecmp( char* s1, char* s2 );
#endif

#ifndef HAVE_STRNCASECMP
int strncasecmp( char* s1, char* s2, int len );
#endif

#ifndef HAVE_STRERROR
char* strerror( int errNum );
#endif

#ifndef HAVE_USLEEP
int usleep( unsigned long usec );
#endif



/***********************************************************************
 * Routines for dealing with strings
 ***********************************************************************/

char *CatString3(char *a, char *b, char *c);
void CopyString(char **dest, char *source);
char *stripcpy( const char *source );
int StrEquals(char *s1,char *s2);

int  envExpand(char *s, int maxstrlen);
char *envDupExpand(const char *s, int extra);

int matchWildcards(char *pattern, char *string);

/***********************************************************************
 * Stuff for consistent parsing
 ***********************************************************************/
#define EatWS(s) do { while ((s) && (isspace(*(s)) || *(s) == ',')) (s)++; } while (0)
#define IsQuote(c) ((c) == '"' || (c) == '\'' || (c) =='`')
#define IsBlockStart(c) ((c) == '[' || (c) == '{' || (c) == '(')
#define IsBlockEnd(c,cs) (((c) == ']' && (cs) == '[') || ((c) == '}' && (cs) == '{') || ((c) == ')' && (cs) == '('))
#define MAX_TOKEN_LENGTH 255

char *SkipQuote(char *s, const char *qlong, const char *qstart,
		const char *qend);
char *GetQuotedString(char *sin, char **sout, const char *delims,
		      const char *qlong, const char *qstart, const char *qend);
char *DoPeekToken(char *indata, char **token, char *spaces, char *delims,
		  char *out_delim);
char *PeekToken(char *indata, char **token);
int MatchToken(char *pstr,char *tok);
void NukeToken(char **pstr);

/* old style parse routine: */
char *DoGetNextToken(char *indata,char **token, char *spaces, char *delims,
		     char *out_delim);
char *GetNextToken(char *indata,char **token);
char *GetNextOption(char *indata,char **token);
char *SkipNTokens(char *indata, unsigned int n);
char *GetModuleResource(char *indata, char **resource, char *module_name);
int GetSuffixedIntegerArguments(char *action, char **ret_action, int retvals[],
				int num, char *suffixlist,
				int ret_suffixnum[]);
int GetIntegerArguments(char *action, char**ret_action, int retvals[],int num);
int GetTokenIndex(char *token, char *list[], int len, char **next);
char *GetNextTokenIndex(char *action, char *list[], int len, int *index);
int GetRectangleArguments(char *action, int *width, int *height);
int GetOnePercentArgument(char *action, int *value, int *unit_io);
int GetTwoPercentArguments(char *action, int *val1, int *val2, int *val1_unit,
			   int *val2_unit);
int ParseToggleArgument(char *action, char **ret_action, int default_ret,
			char no_toggle);


/***********************************************************************
 * Various system related utils
 ***********************************************************************/

int GetFdWidth(void);
int getostype(char *buf, int max);
char *safemalloc(int);

/**
 * Set a colon-separated path, with environment variable expansions.
 * Expand '+' to be the value of the previous path.  
 * 
 * Parameters:
 * p_path          pointer to the path variable
 * newpath         new value for *p_path
 * free_old_path   set true if we should free the memory pointed to
 *                 by p_path on entry
 *
 * The value of newpath is copied into a newly-allocated place, to which 
 * '*p_path' will point to upon return.  The memory region pointed to by
 * '*p_path' upon entry will be freed if 'free_old_path' is true.
 * 
 **/
void setPath( char** p_path, const char* newpath, int free_old_path );

/**
 * Search along colon-separated path for filename, with optional suffix.
 *
 * Parameters:
 * path          colon-separated path of directory names
 * filename      basename of file to search for
 * suffix        if non-NULL, filename may have this suffix
 * type          mode sought for file
 *
 * For each DIR in the path, search for DIR/filename then
 * DIR/<filename><suffix> (if suffix is non-NULL).  Return the full path of
 * the first found.
 *
 * The parameter type is a mask consisting of one or more of R_OK, W_OK, X_OK
 * and F_OK.  R_OK, W_OK and X_OK request checking whether the file exists and
 * has read, write and execute permissions, respectively.  F_OK just requests
 * checking for the existence of the file.
 *
 **/
char* searchPath( const char* path, const char* filename, 
		  const char* suffix, int type );


/***********************************************************************
 * Stuff for modules to communicate with fvwm
 ***********************************************************************/
int ReadFvwmPacket(int fd, unsigned long *header, unsigned long **body);
void SendText(int *fd,char *message,unsigned long window);
#define SendInfo SendText
void GetConfigLine(int *fd, char **tline);
void SetMessageMask(int *fd, unsigned long mask);

/***********************************************************************
 * Stuff for dealing w/ bitmaps & pixmaps:
 ***********************************************************************/

Pixel    GetShadow(Pixel);              /* 3d.c */
Pixel    GetHilite(Pixel);              /* 3d.c */


/***********************************************************************
 * Wrappers around various X11 routines
 ***********************************************************************/

XFontStruct *GetFontOrFixed(Display *disp, char *fontname);

void MyXGrabServer(Display *disp);
void MyXUngrabServer(Display *disp);

void send_clientmessage (Display *disp, Window w, Atom a, Time timestamp);

/***********************************************************************
 * Wrappers around Xrm routines (XResources.c)
 ***********************************************************************/
void MergeXResources(Display *dpy, XrmDatabase *pdb, Bool override);
void MergeCmdLineResources(XrmDatabase *pdb, XrmOptionDescList opts,
			   int num_opts, char *name, int *pargc, char **argv,
			   Bool fNoDefaults);
Bool MergeConfigLineResource(XrmDatabase *pdb, char *line, char *prefix,
			     char *bindstr);
Bool GetResourceString(XrmDatabase db, const char *resource,
		       const char *prefix, char **val);


/* things in Graphics.c */
void RelieveRectangle(Display *dpy, Window win, int x,int y,int w,int h,
		      GC ReliefGC, GC ShadowGC, int line_width);


/***********************************************************************
 * Key and mouse bindings
 ***********************************************************************/

/* contexts for button presses */
#define C_NO_CONTEXT	0x00
#define C_WINDOW	0x01
#define C_TITLE		0x02
#define C_ICON		0x04
#define C_ROOT		0x08
#define C_FRAME		0x10
#define C_SIDEBAR       0x20
#define C_L1            0x40
#define C_L2            0x80
#define C_L3           0x100
#define C_L4           0x200
#define C_L5           0x400
#define C_R1           0x800
#define C_R2          0x1000
#define C_R3          0x2000
#define C_R4          0x4000
#define C_R5          0x8000
#define C_RALL       (C_R1|C_R2|C_R3|C_R4|C_R5)
#define C_LALL       (C_L1|C_L2|C_L3|C_L4|C_L5)
#define C_ALL   (C_WINDOW|C_TITLE|C_ICON|C_ROOT|C_FRAME|C_SIDEBAR|\
                 C_L1|C_L2|C_L3|C_L4|C_L5|C_R1|C_R2|C_R3|C_R4|C_R5)

#define ALL_MODIFIERS (ShiftMask|LockMask|ControlMask|Mod1Mask|Mod2Mask|\
                       Mod3Mask|Mod4Mask|Mod5Mask)

/* Possible binding types */
#define BindingType     Bool
#define KEY_BINDING     True
#define MOUSE_BINDING   False

typedef struct Binding
{
  Bool type;              /* Is it a mouse or key binding 1= mouse; */
  int Button_Key;         /* Mouse Button number of Keycode */
  char *key_name;         /* In case of keycode, give the key_name too */
  int Context;            /* Contex is Fvwm context, ie titlebar, frame, etc */
  int Modifier;           /* Modifiers for keyboard state */
  void *Action;           /* What to do? */
  void *Action2;          /* This one can be used too */
  struct Binding *NextBinding;
} Binding;

Bool ParseContext(char *in_context, int *out_context_mask);
Bool ParseModifiers(char *in_modifiers, int *out_modifier_mask);
Binding *AddBinding(Display *dpy, Binding **pblist, BindingType type,
		    int button, KeySym keysym, char *key_name, int modifiers,
		    int contexts, void *action, void *action2);
void RemoveBinding(Display *dpy, Binding **pblist, BindingType type,
		   int button, KeySym keysym, int modifiers, int contexts);
Binding *ParseBinding(Display *dpy, Binding ** pblist, char *tline,
		      BindingType type, int *nr_left_buttons,
		      int *nr_right_buttons, unsigned char *buttons_grabbed);
void *CheckBinding(Binding *blist, int button_keycode, unsigned int modifier,
		   unsigned int dead_modifiers, int Context, BindingType type);
void GrabWindowKey(Display *dpy, Window w, Binding *binding,
		   unsigned int contexts, unsigned int dead_modifiers,
		   Bool fGrab);
void GrabAllWindowKeys(Display *dpy, Window w, Binding *blist,
		       unsigned int contexts, unsigned int dead_modifiers,
		       Bool fGrab);


#endif
