#ifndef FVWMLIB_H
#define FVWMLIB_H

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>
#include <ctype.h>

/***********************************************************************
 * Routines for dealing with strings
 ***********************************************************************/
int mystrcasecmp(char *a, char *b);
int mystrncasecmp(char *a, char *b,int n);
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

char *PeekToken(const char *pstr);
char *GetToken(char **pstr);
int CmpToken(const char *pstr,char *tok);
int MatchToken(const char *pstr,char *tok);
void NukeToken(char **pstr);

/* old style parse routine: */
char *DoGetNextToken(char *indata,char **token, char *spaces, char *delims);
char *GetNextToken(char *indata,char **token);
char *GetNextOption(char *indata,char **token);
char *GetModuleResource(char *indata, char **resource, char *module_name);
int GetIntegerArguments(char *action, char**ret_action, int retvals[],int num);
int GetTokenIndex(char *token, char *list[], int len, char **next);
char *GetNextTokenIndex(char *action, char *list[], int len, int *index);

/***********************************************************************
 * Various system related utils
 ***********************************************************************/
int mygethostname(char *client, int namelen);
int GetFdWidth(void);

char *safemalloc(int);

void sleep_a_little(int n);

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
#include <X11/Intrinsic.h>              /* needed for xpm.h */
#include <X11/xpm.h>                    /* needed for next prototype */
void color_reduce_pixmap(XpmImage *, int);
#endif

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
