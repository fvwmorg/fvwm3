#ifndef FVWMLIB_H
#define FVWMLIB_H

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
 * Routines for dealing with strings
 ***********************************************************************/

char *CatString3(char *a, char *b, char *c);
void CopyString(char **dest, char *source);
char *stripcpy(char *source);
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
char *PeekToken(const char *pstr);
char *GetToken(char **pstr);
int CmpToken(const char *pstr,char *tok);
int MatchToken(const char *pstr,char *tok);
void NukeToken(char **pstr);

/* old style parse routine: */
char *DoGetNextToken(char *indata,char **token, char *spaces, char *delims,
		     char *out_delim);
char *GetNextToken(char *indata,char **token);
char *GetNextOption(char *indata,char **token);
char *SkipNTokens(char *indata, unsigned int n);
char *GetModuleResource(char *indata, char **resource, char *module_name);
int GetIntegerArguments(char *action, char**ret_action, int retvals[],int num);
int GetTokenIndex(char *token, char *list[], int len, char **next);
char *GetNextTokenIndex(char *action, char *list[], int len, int *index);
int GetRectangleArguments(char *action, int *width, int *height);
int GetOnePercentArgument(char *action, int *value, int *unit_io);
int GetTwoPercentArguments(char *action, int *val1, int *val2, int *val1_unit,
			   int *val2_unit);


/***********************************************************************
 * Various system related utils
 ***********************************************************************/

int GetFdWidth(void);
int getostype(char *buf, int max);
char *safemalloc(int);

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
typedef struct PictureThing
{
  struct PictureThing *next;
  char *name;
  Pixmap picture;
  Pixmap mask;
  unsigned int depth;
  unsigned int width;
  unsigned int height;
  unsigned int count;
} Picture;

void InitPictureCMap(Display*,Window);
Picture *GetPicture(Display* dpy, Window Root, char* IconPath,
		    char* PixmapPath, char* name, int color_limit);
Picture *CachePicture(Display*,Window,char *iconpath,
                      char *pixmappath,char*,int);
void DestroyPicture(Display*,Picture*);

char *findIconFile(char *icon, char *pathlist, int type);
#ifdef XPM
#include <X11/xpm.h>                    /* needed for next prototype */
void color_reduce_pixmap(XpmImage *, int);
#endif

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


#endif
