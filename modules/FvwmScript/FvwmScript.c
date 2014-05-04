/* -*-c-*- */
/* Copyright May 1996, Frederic Cordier.  No guarantees or warantees are
 * provided or implied in any way whatsoever.  Use this program at your own
 * risk.  Permission to use, modify, and redistribute this program is hereby
 * given, provided that this copyright is kept intact.
 */

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
 * along with this program; if not, see: <http://www.gnu.org/licenses/>
 */

#include "config.h"

#include "types.h"
#include "libs/fvwmlib.h"
#include "libs/FScreen.h"
#include "libs/FShape.h"
#include "libs/fvwmsignal.h"
#include "libs/Picture.h"
#include "libs/PictureGraphics.h"
#include "libs/FRender.h"
#include "libs/FRenderInit.h"
#include "libs/FGettext.h"
#include "libs/Bindings.h"
#include "libs/charmap.h"
#include "libs/wcontext.h"
#include "libs/ColorUtils.h"
#include "libs/Graphics.h"
#include "libs/Parse.h"
#include "libs/Strings.h"
#include "libs/XError.h"

#ifdef MEMDEBUG                 /* For debugging */
#include <unchecked.h>
#endif

/* Variables utilise par l'analyseur syntaxique */
extern ScriptProp *scriptprop;
extern int nbobj;                       /* Nombre d'objets */
extern int numligne;                    /* Numero de ligne */
extern TabObj *tabobj;                  /* Tableau d'objets, limite=1000 */
extern char **TabVVar;                  /* Tableau des variables du sript */
extern int TabIdObj[1001];
extern Bloc **TabIObj;
extern CaseObj *TabCObj;
#ifdef MEMDEBUG
extern int __bounds_debug_no_checking;
#endif

/* Constante de couleurs utilise dans le tableau TabColor */
#define back 0
#define fore 1
#define shad 2
#define hili 3


/* Variables globales */
char *ScriptName;       /* Nom du fichier contenat le script decrivant le GUI */
char *ScriptBaseName;
char *ScriptPath = "";
ModuleArgs *module;
int fd[2];                      /* pipe pair */
int fd_err;
int x_fd;                       /* fd for X */
Window ref;
FlocaleWinString *FwinString;

extern int yyparse(void);
extern void (*TabCom[25]) (int NbArg,long *TabArg);

Display *dpy;
int screen;
Window  Root;
X11base *x11base;               /* Pour le serveur X */
TypeBuffSend BuffSend;          /* Pour les communication entre script */
int grab_server = 0;
struct XObj *tabxobj[1000];
char *Scrapt;
Atom propriete,type;
static Atom wm_del_win;
char *imagePath = NULL;
static Bool is_dead_pipe = False;
KeySym shift_tab_ks;  /* shift-Tab keysym */
char *LastString = NULL;  /* last string send by a SendString ot Key cmd */
Binding *BindingsList;

extern void InitCom(void);
static void TryToFind(char *filename);


/* Exit procedure - called whenever we call exit(), or when main() ends */
static void
ShutdownX(void)
{
  int i;
  static XEvent event;
  fd_set in_fdset;
  extern int x_fd;
  Atom MyAtom;
  int NbEssai=0;
  struct timeval tv;

#ifdef DEBUG                    /* For debugging */
  XSync(dpy,0);
#endif

  /* execute the QuitFunc */
  if (x11base->quitfunc != NULL)
    ExecBloc(x11base->quitfunc);

  if (is_dead_pipe)
    return;

  /* On cache la fenetre */
  XUnmapWindow(dpy,x11base->win);
  XFlush(dpy);

  /* Le script ne possede plus la propriete */
  MyAtom = XInternAtom(dpy, x11base->TabScriptId[1], False);
  XSetSelectionOwner(dpy, MyAtom, x11base->root, CurrentTime);

  /* On verifie si tous les messages ont ete envoyes */
  while(!isTerminated && (BuffSend.NbMsg > 0) && (NbEssai < 10000) )
  {
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    FD_ZERO(&in_fdset);
    FD_SET(x_fd,&in_fdset);
    if ( fvwmSelect(x_fd+1, &in_fdset, NULL, NULL, &tv) > 0 )
    {
      if (FD_ISSET(x_fd, &in_fdset))
      {
	if (FCheckTypedEvent(dpy, SelectionRequest, &event))
	  SendMsgToScript(event);
	else
	  NbEssai++;
      }
    }
  }
  XFlush(dpy);

  /* Attente de deux secondes afin d'etre sur que tous */
  /* les messages soient arrives a destination         */
  /* On quitte proprement le serveur X */
  for (i=0; i<nbobj; i++)
    tabxobj[i]->DestroyObj(tabxobj[i]);
  XFlush(dpy);
  fsm_close();
  sleep(2);
  XCloseDisplay(dpy);
}


void Debug(void)
{
  int i,j;

  for (j=1;j<=nbobj;j++)
    for (i=0; i<=TabCObj[TabIdObj[j]].NbCase; i++)
    {
      /* Execution du bloc d'instruction */
      fprintf(stderr,"Id de l'objet %d\n",TabIdObj[j]);
      fprintf(stderr,"Nb Instruction %d\n",TabIObj[TabIdObj[j]][i].NbInstr);
    }

}

/* Lecture du fichier contenant le script */
void ReadConfig (char *ScriptName)
{
  extern FILE *yyin;
  char s[FILENAME_MAX];

  /* This is legacy support.  The first try is the command line file name
     with the scriptfile directory.  If the scriptfile directory wasn't set
     it ends up trying the command line file name undecorated, which
     could be a full path, or it could be relative to the current directory.
     Not very pretty, dje 12/26/99 */
  sprintf(s,"%s%s%s",ScriptPath,(!*ScriptPath ? "" : "/"),ScriptName);
  yyin = fopen(s,"r");
  if (yyin == NULL) {                   /* file not found yet, */
    TryToFind(ScriptName);                   /* look in some other places */
  }
  if (yyin == NULL)
  {
    fprintf(stderr,"[%s][ReadConfig]: <<ERROR>> Can't open the script %s\n",
	    module->name,s);
    exit(1);
  }
  /* On ne redefini pas yyout qui est la sortie standard */

  /* Application de l'analyseur syntaxique et lexical */
  yyparse();
  /* Fermeture du script */

  fclose(yyin);
}

/* Original method for finding the config file didn't work,
   try using the same strategy fvwm uses in its read command.
   1. If the file name starts with a slash, just try it.
   2. Look in the home directory, using FVWM_USERDIR.
   3. If that doesn't work, look in FVWM_DATADIR.
   The extern "yyin" indicates success or failure.
*/
static void TryToFind(char *filename) {
  extern FILE *yyin;
  char path[FILENAME_MAX];

  if (filename[0] == '/') {             /* if absolute path */
    yyin = fopen(filename,"r");
    return;
  }
  sprintf(path,"%s/%s",getenv("FVWM_USERDIR"),filename);
  yyin = fopen( path, "r" );
  if ( yyin == NULL ) {
    sprintf(path,"%s/%s",FVWM_DATADIR, filename );
    yyin = fopen( path, "r" );
  }
  return;
}

/* Quitter par l'option Delete du bouton de la fenetre */
RETSIGTYPE DeadPipe(int nonsense)
{
  is_dead_pipe = True;
  Quit (0,NULL);
  SIGNAL_RETURN;
}

/* Lecture du fichier system.fvwmrc ou .fvwmrc */
void ParseOptions(void)
{
  char *tline;

  InitGetConfigLine(fd,"*FvwmScript");
  GetConfigLine(fd,&tline);
  while(tline != (char *)0) {
    if (strlen(tline) > 1) {
      if (strncasecmp(tline,"ImagePath",9) == 0)
	CopyString(&imagePath,&tline[9]);
      else if (strncasecmp(tline,"*FvwmScriptPath",15) == 0)
	CopyString(&ScriptPath,&tline[15]);
      else if (strncasecmp(tline,"*FvwmScriptDefaultColorset",26) == 0)
      {
	LoadColorset(&tline[26]);
	x11base->colorset = atoi(&tline[26]);
      }
      else if (strncasecmp(tline,"*FvwmScriptDefaultBack",22) == 0)
      {
	x11base->colorset = -1;
	CopyString(&x11base->backcolor,&tline[22]);
      }
      else if (strncasecmp(tline,"*FvwmScriptDefaultFore",22) == 0)
      {
	x11base->colorset = -1;
	CopyString(&x11base->forecolor,&tline[22]);
      }
      else if (strncasecmp(tline,"*FvwmScriptDefaultHilight",25) == 0)
      {
	x11base->colorset = -1;
	CopyString(&x11base->hilicolor,&tline[25]);
      }
      else if (strncasecmp(tline,"*FvwmScriptDefaultShadow",24) == 0)
      {
	x11base->colorset = -1;
	CopyString(&x11base->shadcolor,&tline[24]);
      }
      else if (strncasecmp(tline,"*FvwmScriptDefaultFont",22) == 0)
	CopyStringWithQuotes(&x11base->font,&tline[22]);
      else if (strncasecmp(tline,"Colorset",8) == 0)
	LoadColorset(&tline[8]);
      else if (strncasecmp(tline, XINERAMA_CONFIG_STRING,
			   sizeof(XINERAMA_CONFIG_STRING) - 1) == 0)
	FScreenConfigureModule(
	  tline + sizeof(XINERAMA_CONFIG_STRING) - 1);
    }

    GetConfigLine(fd,&tline);
  }
}

static int myErrorHandler(Display *dpy, XErrorEvent *event)
{
  /* some errors are acceptable, mostly they're caused by
   * trying to use a deleted pixmap */
  if((event->error_code == BadDrawable) || (event->error_code == BadPixmap) ||
     (event->error_code == BadWindow) ||
     (event->error_code == FRenderGetErrorCodeBase() + FRenderBadPicture))
    return 0;

  PrintXErrorAndCoredump(dpy, event, x11base->title);

  /* return (*oldErrorHandler)(dpy,event); */
  return 0;
}

/* Procedure d'initialisation du serveur X et des variables globales*/
void Xinit(int IsFather)
{
  char *name;
  Atom myatom;
  int i = 16;

  /* Connextion au serveur X */
#ifdef MEMDEBUG
  __bounds_debug_no_checking=True;
#endif

  dpy=XOpenDisplay(NULL);
  if (dpy == NULL)
  {
    fprintf(stderr,"[%s]: <<ERROR>> Can't open display %s",
	    ScriptName, XDisplayName(NULL));
    exit(1);
  }
  screen = DefaultScreen(dpy);
  Root = RootWindow(dpy,screen);
  flib_init_graphics(dpy);
  FlocaleAllocateWinString(&FwinString);
  XSetErrorHandler(myErrorHandler);

#ifdef MEMDEBUG
  __bounds_debug_no_checking=False;
#endif

  if (IsFather)
  {
    name = xcalloc(sizeof(char), strlen("FvwmScript") + 5);
    do
    {
      sprintf(name,"%c%xFvwmScript",161,i);
      i++;
      myatom = XInternAtom(dpy, name, False);
    }
    while (XGetSelectionOwner(dpy,myatom) != None)
      ;
    x11base->TabScriptId[1] = name;
    x11base->TabScriptId[0] = NULL;
  }
  x11base->NbChild = 0;
  x11base->root = RootWindow(dpy,screen);
  x_fd = XConnectionNumber(dpy);

  /* install exit procedure to close X down again */
  atexit(ShutdownX);
}

/*
 * Lecture d'un icone
 */
void LoadIcon(struct XObj *xobj)
{
	char *path;
	int depth;
	FvwmPictureAttributes fpa;

	fpa.mask = 0;
	if (xobj->colorset >= 0 && Colorset[xobj->colorset].do_dither_icon)
	{
		fpa.mask |= FPAM_DITHER;
	}
	if (xobj->iconPixmap != None)
	{
		XFreePixmap(dpy, xobj->iconPixmap);
		xobj->iconPixmap = None;
	}
	if (xobj->icon_maskPixmap != None)
	{
		XFreePixmap(dpy, xobj->icon_maskPixmap);
		xobj->icon_maskPixmap = None;
	}
	if (xobj->icon_alphaPixmap != None)
	{
		XFreePixmap(dpy, xobj->icon_alphaPixmap);
		xobj->icon_alphaPixmap = None;
	}
	if (xobj->alloc_pixels != NULL && xobj->nalloc_pixels > 0)
	{
		PictureFreeColors(
			dpy, Pcmap, xobj->alloc_pixels, xobj->nalloc_pixels, 0,
			False);
		xobj->alloc_pixels = NULL;
		xobj->nalloc_pixels = 0;
	}
	if ((xobj->icon) == NULL)
		return;
	if ((path = PictureFindImageFile(xobj->icon,imagePath,R_OK)) == NULL)
	{
		return;
	}
	if (!PImageLoadPixmapFromFile(
		dpy, x11base->win, path, &xobj->iconPixmap,
		&xobj->icon_maskPixmap, &xobj->icon_alphaPixmap, &xobj->icon_w,
		&xobj->icon_h, &depth, &xobj->nalloc_pixels,
		&xobj->alloc_pixels, 0, fpa))
	{
		fprintf(stderr,"[%s][LoadIcon]: <<WARNING>> Unable to "
			"load pixmap %s\n",
			ScriptName,xobj->icon);
		free(path);
		return;
	}
	if (depth <= 1)
	{
		XGCValues gcv;
		GC m_gc;
		Pixmap temp;

		gcv.background= WhitePixel(dpy, screen);
		gcv.foreground= BlackPixel(dpy, screen);
		m_gc = fvwmlib_XCreateGC(dpy, x11base->win,
				       GCForeground | GCBackground, &gcv);
		temp = XCreatePixmap(dpy, x11base->win,
				     xobj->icon_w, xobj->icon_h, Pdepth);
		XCopyPlane(dpy, xobj->iconPixmap, temp, m_gc,
			   0,0,xobj->icon_w, xobj->icon_h, 0,0,1);
		XFreePixmap(dpy, xobj->iconPixmap);
		XFreeGC(dpy,m_gc);
		xobj->iconPixmap = temp;
	}
	free(path);
}


/* Ouvre une fenetre pour l'affichage du GUI */
void OpenWindow (void)
{
  XTextProperty Name;
  XWMHints *IndicWM;
  XSizeHints *IndicNorm;
  unsigned long mask;
  XClassHint classHints;
  XSetWindowAttributes Attr;

  /* Allocation des couleurs */
  if (x11base->colorset >= 0) {
    x11base->TabColor[fore] = Colorset[x11base->colorset].fg;
    x11base->TabColor[back] = Colorset[x11base->colorset].bg;
    x11base->TabColor[shad] = Colorset[x11base->colorset].shadow;
    x11base->TabColor[hili] = Colorset[x11base->colorset].hilite;
  } else {
    x11base->TabColor[fore] = GetColor(x11base->forecolor);
    x11base->TabColor[back] = GetColor(x11base->backcolor);
    x11base->TabColor[shad] = GetColor(x11base->shadcolor);
    x11base->TabColor[hili] = GetColor(x11base->hilicolor);
  }

  /* Definition des caracteristiques de la fentre */
  mask = CWBackPixel | CWBorderPixel | CWColormap;
  Attr.background_pixel = x11base->TabColor[back];
  Attr.border_pixel = 0;
  Attr.colormap = Pcmap;

  x11base->win=XCreateWindow(
    dpy, x11base->root, x11base->size.x,x11base->size.y, x11base->size.width,
    x11base->size.height, 0, Pdepth, InputOutput, Pvisual, mask, &Attr);
  x11base->gc=fvwmlib_XCreateGC(dpy,x11base->win,0,NULL);
  if (x11base->colorset >= 0)
    SetWindowBackground(
      dpy, x11base->win, x11base->size.width, x11base->size.height,
      &Colorset[x11base->colorset], Pdepth, x11base->gc, True);

  /* Choix des evts recus par la fenetre */
  XSelectInput(dpy,x11base->win, KeyPressMask|ButtonPressMask|
	       ExposureMask|ButtonReleaseMask|EnterWindowMask|LeaveWindowMask|
	       ButtonMotionMask|StructureNotifyMask);
  XSelectInput(dpy, x11base->root, PropertyChangeMask);

  /* Specification des parametres utilises par le gestionnaire de fenetre */
  if (XStringListToTextProperty(&x11base->title,1,&Name) == 0)
    fprintf(stderr,"[%s][OpenWindow]: <<WARNING>> Can't use icon name\n",
	    ScriptName);
  IndicNorm = XAllocSizeHints();
  if (x11base->size.x!=-1)
  {
    IndicNorm->x = x11base->size.x;
    IndicNorm->y = x11base->size.y;
    IndicNorm->flags = PSize|PMinSize|PMaxSize|PBaseSize|PPosition;
  }
  else
    IndicNorm->flags = PSize|PMinSize|PMaxSize|PBaseSize;
  IndicNorm->width = x11base->size.width;
  IndicNorm->height = x11base->size.height;
  IndicNorm->min_width = x11base->size.width;
  IndicNorm->min_height = x11base->size.height;
  IndicNorm->max_width = x11base->size.width;
  IndicNorm->max_height = x11base->size.height;
  IndicWM = XAllocWMHints();
  IndicWM->input = True;
  IndicWM->initial_state = NormalState;
  IndicWM->flags = InputHint|StateHint;

  classHints.res_name = xstrdup(ScriptBaseName);
  classHints.res_class = xstrdup(module->name);

  XSetWMProperties(dpy, x11base->win, &Name,
		   &Name, NULL, 0, IndicNorm, IndicWM, &classHints);
  Scrapt = xcalloc(sizeof(char), 1);

  /* Construction des atomes pour la communication inter-application */
  propriete = XInternAtom(dpy,"Prop_selection",False);
  wm_del_win = XInternAtom(dpy,"WM_DELETE_WINDOW",False);
  XSetWMProtocols(dpy,x11base->win,&wm_del_win,1);

  free(classHints.res_class);
  free(classHints.res_name);
}

/*
 * Execution d'une sequence d'instruction
 */
void ExecBloc(Bloc *bloc)
{
  int i;

  for (i = 0; i <= bloc->NbInstr; i++)
  {
    TabCom[bloc->TabInstr[i].Type](bloc->TabInstr[i].NbArg,
				   bloc->TabInstr[i].TabArg);
  }
}


/* Construction de l'interface graphique */
void BuildGUI(int IsFather)
{
  int i;

  if (scriptprop->font != NULL)
  {
    /* leak! */
    x11base->font = scriptprop->font;
  }

  if (scriptprop->forecolor != NULL)
  {
    x11base->colorset = -1;
    /* leak! */
    x11base->forecolor = scriptprop->forecolor;
  }
  if (scriptprop->backcolor != NULL)
  {
    x11base->colorset = -1;
    /* leak! */
    x11base->backcolor = scriptprop->backcolor;
  }
  if (scriptprop->shadcolor != NULL)
  {
    x11base->colorset=-1;
    /* leak! */
    x11base->shadcolor = scriptprop->shadcolor;
  }
  if (scriptprop->hilicolor != NULL)
  {
    x11base->colorset=-1;
    /* leak! */
    x11base->hilicolor = scriptprop->hilicolor;
  }
  if (scriptprop->colorset != -1)
    x11base->colorset = scriptprop->colorset;

  x11base->icon = scriptprop->icon;

  {
    int sx;
    int sy;

    FScreenGetScrRect(NULL, FSCREEN_CURRENT, &sx, &sy, NULL, NULL);
    x11base->size.x = scriptprop->x + sx;
    x11base->size.y = scriptprop->y + sy;
  }
  x11base->size.width = scriptprop->width;
  x11base->size.height = scriptprop->height;
  x11base->title = scriptprop->titlewin;

  /* Initialisation de la fenetre */
  OpenWindow();

  /* Parcour de tous les objets graphiques */
  nbobj++;
  for (i=0; i<nbobj; i++)
  {
    tabxobj[i] = xcalloc(1, sizeof(struct XObj));
    tabxobj[i]->id = (*tabobj)[i].id;
    tabxobj[i]->x = (*tabobj)[i].x;
    tabxobj[i]->y = (*tabobj)[i].y;
    tabxobj[i]->width = (*tabobj)[i].width;
    tabxobj[i]->height = (*tabobj)[i].height;
    if (tabxobj[i]->width == 0)
      tabxobj[i]->width = 1;
    if (tabxobj[i]->height == 0)
      tabxobj[i]->height = 1;
    tabxobj[i]->value = (*tabobj)[i].value;
    tabxobj[i]->value2 = (*tabobj)[i].value2;
    tabxobj[i]->value3 = (*tabobj)[i].value3;
    tabxobj[i]->flags[0] = (*tabobj)[i].flags[0];
    tabxobj[i]->flags[1] = (*tabobj)[i].flags[1];
    tabxobj[i]->flags[2] = (*tabobj)[i].flags[2];
    tabxobj[i]->flags[3] = (*tabobj)[i].flags[3];
    tabxobj[i]->icon = (*tabobj)[i].icon;
    tabxobj[i]->swallow = (*tabobj)[i].swallow;

    if ((*tabobj)[i].title == NULL)
      tabxobj[i]->title = xcalloc(1, 200);
    else
      tabxobj[i]->title = (*tabobj)[i].title;

    if ((*tabobj)[i].font == NULL && x11base->font != NULL)
      tabxobj[i]->font = xstrdup(x11base->font);
    else
      tabxobj[i]->font = (*tabobj)[i].font;

    if ((*tabobj)[i].forecolor == NULL)
      tabxobj[i]->forecolor = xstrdup(x11base->forecolor);
    else
      tabxobj[i]->forecolor = (*tabobj)[i].forecolor;

    if ((*tabobj)[i].backcolor == NULL)
      tabxobj[i]->backcolor = xstrdup(x11base->backcolor);
    else
      tabxobj[i]->backcolor = (*tabobj)[i].backcolor;

    if ((*tabobj)[i].shadcolor == NULL)
      tabxobj[i]->shadcolor = xstrdup(x11base->shadcolor);
    else
      tabxobj[i]->shadcolor = (*tabobj)[i].shadcolor;

    if ((*tabobj)[i].hilicolor == NULL)
      tabxobj[i]->hilicolor = xstrdup(x11base->hilicolor);
    else
      tabxobj[i]->hilicolor = (*tabobj)[i].hilicolor;

    if ((*tabobj)[i].colorset >= 0)
      tabxobj[i]->colorset=(*tabobj)[i].colorset;
    else if ((*tabobj)[i].backcolor == NULL && (*tabobj)[i].forecolor == NULL &&
	     (*tabobj)[i].shadcolor == NULL && (*tabobj)[i].hilicolor == NULL)
      tabxobj[i]->colorset = x11base->colorset;
    else tabxobj[i]->colorset = -1;


    ChooseFunction(tabxobj[i],(*tabobj)[i].type);
    tabxobj[i]->gc = x11base->gc;
    tabxobj[i]->ParentWin = &(x11base->win);
    tabxobj[i]->iconPixmap = None;
    tabxobj[i]->icon_maskPixmap = None;

    LoadIcon(tabxobj[i]);                  /* Chargement de l'icone du widget */

    tabxobj[i]->InitObj(tabxobj[i]);
  }

  /* Enregistrement du bloc de taches periodic */
  x11base->periodictasks = scriptprop->periodictasks;
  /* the QuitFunc */
  x11base->quitfunc = scriptprop->quitfunc;

  /*Si un bloc d'initialisation du script existe, on l'execute ici */
  if (scriptprop->initbloc != NULL)
  {
    ExecBloc(scriptprop->initbloc);
    free(scriptprop->initbloc->TabInstr);
    free(scriptprop->initbloc);
  }

  free(tabobj);
  free(scriptprop);
  XMapRaised(dpy,x11base->win);
  for (i=0; i<nbobj; i++)
    if (tabxobj[i]->flags[0] != True)
      XMapWindow(dpy,tabxobj[i]->win);
}

/*
 * Fonction de traitement des msg entre objets
 */
void SendMsg(struct XObj *xobj,int TypeMsg)
{
  int i;

  for (i=0; i <= TabCObj[TabIdObj[xobj->id]].NbCase; i++)
    if (TabCObj[TabIdObj[xobj->id]].LstCase[i] == TypeMsg)
    {
      /* Execution du bloc d'instruction */
      ExecBloc(&TabIObj[TabIdObj[xobj->id]][i]);
    }
}

/*
 * Fonction de traitement des msg entre objets
 */
void SendMsgAndString(char *action, char *type)
{
  int val[2];
  char *token = NULL;
  char *arg1 = NULL;
  char *arg2 = NULL;
  char *windowName;
  int i;

  if(StrEquals(type,"SendString") || StrEquals(type,"CheckBinding")){
    if (GetIntegerArguments(action, NULL, val, 2) == 2){
      if (val[0] > 1000 || val[0] < 1){
	fprintf(stderr,"[%s][%s]: <<WARNING>> Widget id out of range: %i\n",
		ScriptName,type,val[0]);
	return;
      }

      i = TabIdObj[val[0]];
      if (i != -1) {
	/* skip the integer argument */
	action = GetNextToken(action, &token);
	action = GetNextToken(action, &token);
	if (LastString != NULL) {
	  free(LastString);
	  LastString = NULL;
	}
	if (action != NULL && strlen(action) > 0)
	  CopyString(&LastString,action);

	SendMsg(tabxobj[i],val[1]);
      }else{
	fprintf(stderr,"[%s][%s]: <<WARNING>> no Widget %i\n",
		ScriptName,type,val[0]);
      }
      if (token)
	free(token);
    }else{
      fprintf(stderr,"[%s][%s]: <<WARNING>> Syntax Error: %s\n",
	      ScriptName,type,action);
    }
  }else{

    if(StrEquals(type,"ChangeWindowTitle")){
      action=GetNextToken(action, &arg1);
      action=GetNextToken(action, &arg2);

      if(arg2 == NULL){

	 XChangeProperty(
		 dpy, x11base->win, XA_WM_NAME, XA_STRING, 8, PropModeReplace,
		 (unsigned char*)arg1, strlen(arg1));

      }else{
	if(XFetchName(dpy, x11base->win, &windowName) == 0){
		fprintf(
			stderr,"[%s][SendMsgAndString]: <<ERROR>> "
			"Can't find the title of a window\n", module->name);
	}else{
	  if(StrEquals(arg2,windowName)){
	    XChangeProperty(
		    dpy, x11base->win, XA_WM_NAME, XA_STRING, 8,
		    PropModeReplace, (unsigned char*)arg1, strlen(arg1));
	  }
	}
      }
    }
    else
    {
	    fprintf(
		    stderr,"[%s][SendMsgAndString]: <<ERROR>> "
		    "Unknown SendToModule command: %s\n", module->name, type);
    }

    if(arg1) free(arg1);
    if(arg2) free(arg2);

  }
}

/*
 * Appeler lors d'une demande de selection
 */
void SendMsgToScript(XEvent event)
{
  Atom Sender,Receiver=None;
  static XEvent evnt_sel;
  int i;

  Sender = XInternAtom(dpy,x11base->TabScriptId[1],True);

  if (event.xselectionrequest.selection == Sender)
  {
    i=0;
    while ((i < BuffSend.NbMsg) && (event.xselectionrequest.target != Receiver))
    {
      Receiver = XInternAtom(dpy,BuffSend.TabMsg[i].R,True);
      i++;
    }
    i--;

    evnt_sel.type = SelectionNotify;
    evnt_sel.xselection.requestor = event.xselectionrequest.requestor;
    evnt_sel.xselection.selection = event.xselectionrequest.selection;
    evnt_sel.xselection.target = Receiver;
    evnt_sel.xselection.time = event.xselectionrequest.time;
    /* On a trouve le recepteur */
    if (event.xselectionrequest.target == Receiver)
    {
      evnt_sel.xselection.property = event.xselectionrequest.property;
      XChangeProperty(dpy,
		      evnt_sel.xselection.requestor,
		      evnt_sel.xselection.property,
		      evnt_sel.xselection.target,
		      8,
		      PropModeReplace,
		      (unsigned char *)(BuffSend.TabMsg[i].Msg),
		      strlen(BuffSend.TabMsg[i].Msg)+1);
      BuffSend.NbMsg--;
      free(BuffSend.TabMsg[i].Msg);
      if (BuffSend.NbMsg > 0)
      {
	memmove(&BuffSend.TabMsg[i],
		&BuffSend.TabMsg[i+1],(BuffSend.NbMsg-i)*sizeof(TypeName));
      }
    }
    else
    {
      /* Cas ou le recepteur demande un message et qu'il n'y en a pas */
      evnt_sel.xselection.property = None;
    }
    FSendEvent(dpy,evnt_sel.xselection.requestor, False, 0, &evnt_sel);
  }
}

static
void UpdateRootTransparency(Bool pr_only)
{
	int i;

	if (CSET_IS_TRANSPARENT(x11base->colorset))
	{
		if (CSET_IS_TRANSPARENT_PR_PURE(x11base->colorset))
		{
			XClearArea(dpy, x11base->win, 0, 0, 0, 0, True);
		}
		else if (!pr_only || CSET_IS_TRANSPARENT_PR(x11base->colorset))
		{
			SetWindowBackground(
				dpy, x11base->win, x11base->size.width,
				x11base->size.height,
				&Colorset[x11base->colorset], Pdepth,
				x11base->gc, True);
		}
	}
	for (i=0; i<nbobj; i++)
	{
		if (Rectangle == tabxobj[i]->TypeWidget ||
		    SwallowExec == tabxobj[i]->TypeWidget)
		{
			continue;
		}
		if (!CSET_IS_TRANSPARENT(tabxobj[i]->colorset) ||
		    (CSET_IS_TRANSPARENT_PR(tabxobj[i]->colorset) &&
		     !CSET_IS_TRANSPARENT(x11base->colorset)))
		{
			continue;
		}
		if (CSET_IS_TRANSPARENT_PR_PURE(tabxobj[i]->colorset))
		{
			XClearArea(dpy, tabxobj[i]->win, 0, 0, 0, 0, True);
		}
		else if (!pr_only ||
			 CSET_IS_TRANSPARENT_PR(tabxobj[i]->colorset))
		{
			SetWindowBackground(
				dpy, tabxobj[i]->win, tabxobj[i]->width,
				tabxobj[i]->height,
				&Colorset[tabxobj[i]->colorset], Pdepth,
				tabxobj[i]->gc, True);
		}
	}
}


/* read an X event */
void ReadXServer (void)
{
  static XEvent event,evnt_sel;
  int i;
  int isTab = 0;
  char *octet;
  KeySym ks;
  static unsigned char buf[10];         /* unsigned for international */
  Bool find = False;
  char *action;
  int ex, ey, ex2, ey2;
  XClassHint tmp;

  while (FEventsQueued(dpy, QueuedAfterReading))
  {
    FNextEvent(dpy, &event);
    switch (event.type)
    {
    case Expose:
      ex = event.xexpose.x;
      ey = event.xexpose.y;
      ex2 = event.xexpose.x + event.xexpose.width;
      ey2 = event.xexpose.y + event.xexpose.height;
      while (FCheckTypedWindowEvent(dpy, event.xany.window, Expose, &event))
      {
	      ex = min(ex, event.xexpose.x);
	      ey = min(ey, event.xexpose.y);
	      ex2 = max(ex2, event.xexpose.x + event.xexpose.width);
	      ey2 = max(ey2 , event.xexpose.y + event.xexpose.height);
      }
      event.xexpose.x = ex;
      event.xexpose.y = ey;
      event.xexpose.width = ex2 - ex;
      event.xexpose.height = ey2 - ey;
      for (i=0; i<nbobj; i++)
      {
	      if (event.xexpose.window == tabxobj[i]->win)
	      {
		      tabxobj[i]->DrawObj(tabxobj[i], &event);
		      break;
	      }
	      /* handle exposes on x11base that need an object to render */
	      if (event.xexpose.window == x11base->win)
	      {
		      /* redraw first menu item to get the 3d menubar */
		      for (i=0; i<nbobj; i++)
		      {
			      if (Menu == tabxobj[i]->TypeWidget)
			      {
				      tabxobj[i]->DrawObj(tabxobj[i], &event);
				      break;
			      }
		      }
		      /* redraw all Rectangles and SwallowExec's */
		      for (i=0; i<nbobj; i++)
		      {
			      if ((Rectangle == tabxobj[i]->TypeWidget)
				  ||(SwallowExec == tabxobj[i]->TypeWidget))
			      {
				      tabxobj[i]->DrawObj(tabxobj[i], &event);
			      }
		      }
	      }
      }
      break;
    case ConfigureNotify:
    {
      Bool moved = False;

      fev_sanitise_configure_notify(&event.xconfigure);
      moved = event.xconfigure.send_event;
      while (FCheckTypedEvent(dpy, ConfigureNotify, &event))
      {
	fev_sanitise_configure_notify(&event.xconfigure);
	/* check for movement */
	if (event.xconfigure.send_event)
	  moved = True;
      }
      if (moved)
      {
	UpdateRootTransparency(False);
      }
    }
    break;
    case KeyPress:
      /* Touche presse dans un objet / Key press in an object */
      isTab = 0;
      find = False;
      XLookupString(&event.xkey, (char *)buf, sizeof(buf), &ks, NULL);
      event.xkey.keycode =
	XKeysymToKeycode(dpy,fvwm_KeycodeToKeysym(dpy,event.xkey.keycode,0,0));

      /* check for bindings defined by the Key instruction */
	  tmp.res_class = tmp.res_name = "root";
      if ((action = CheckBinding(
		   BindingsList, STROKE_ARG(0) event.xkey.keycode,
		   event.xkey.state, LockMask, C_WINDOW, BIND_KEYPRESS,
		   &tmp, tmp.res_class)) !=
	  NULL)
      {
	SendMsgAndString(action, "CheckBinding");
	break;
      }

      if (ks == XK_Tab) {
	isTab = 1;
	if (event.xkey.state & ShiftMask) {
	  /* does not work in general see AddBinding */
	  isTab = 2;
	}
      }
      else if (ks == shift_tab_ks)
	isTab = 2;
      if (event.xkey.subwindow!=0)
      {
	/* Envoi de l'evt à l'objet */
	for (i=0; i<nbobj; i++)
	{
	  if (tabxobj[i]->win == event.xkey.subwindow)
	  {
	    find = True;
	    if (isTab == 0 && !HAS_NO_FOCUS(tabxobj[i])) {
	      tabxobj[i]->EvtKey(tabxobj[i],&event.xkey);
	    }
	    break;
	  }
	}
      }
      if (isTab > 0) {
	Bool loop = False;
	if (!find)
	  i = (isTab == 1)? -1:nbobj;
	find = False;
	while(!find) {
	  i = (isTab == 1)? i+1 : i-1;
	  if (i == nbobj || i == -1) {
	    i = (isTab == 1)? 0 : nbobj-1;
	    if (loop) {
	      i = -1;
	      break;
	    }
	    loop = True;
	  }
	  if (!IS_HIDDEN(tabxobj[i]) && !HAS_NO_FOCUS(tabxobj[i]) &&
	      tabxobj[i]->TypeWidget != Rectangle &&
	      tabxobj[i]->TypeWidget != HDipstick &&
	      tabxobj[i]->TypeWidget != VDipstick)
	    find = True;
	}
	if (find)
	  FWarpPointer(dpy, x11base->win, tabxobj[i]->win, 0, 0, 0, 0,
		       tabxobj[i]->width/2, 10);
      }
      break;
    case ButtonPress:
      /* Clique dans quel fenetre? */
      if (event.xbutton.subwindow != 0)
	for (i=0; i<nbobj; i++)
	  if (tabxobj[i]->win == event.xbutton.subwindow) {
	    tabxobj[i]->EvtMouse(tabxobj[i],&event.xbutton);
	    break;
	  }
      break;
    case ButtonRelease:
      break;
    case EnterNotify:
      break;
    case LeaveNotify:
      break;
    case MotionNotify:
      break;
    case MappingNotify:
      XRefreshKeyboardMapping((XMappingEvent*)&event);
      break;
    case SelectionRequest:
      if (event.xselectionrequest.selection == XA_PRIMARY)
      {
	evnt_sel.type = SelectionNotify;
	evnt_sel.xselection.requestor = event.xselectionrequest.requestor;
	evnt_sel.xselection.selection = event.xselectionrequest.selection;
	evnt_sel.xselection.target = event.xselectionrequest.target;
	evnt_sel.xselection.time = event.xselectionrequest.time;
	evnt_sel.xselection.property = event.xselectionrequest.property;
	switch (event.xselectionrequest.target)
	{
	case XA_STRING:
	  XChangeProperty(dpy,
			  evnt_sel.xselection.requestor,
			  evnt_sel.xselection.property,
			  evnt_sel.xselection.target,
			  8,
			  PropModeReplace,
			  (unsigned char *)(Scrapt),
			  strlen(Scrapt)+1);
	  break;
	default:evnt_sel.xselection.property = None;
	}
	FSendEvent(dpy,evnt_sel.xselection.requestor,
		   False,0,&evnt_sel);
      }
      else
	SendMsgToScript(event);
      break;
    case SelectionClear:
      if (event.xselectionclear.selection == XA_PRIMARY)
	UnselectAllTextField(tabxobj);
      break;
    case ClientMessage:
      if ((event.xclient.format == 32) &&
	  (event.xclient.data.l[0] == wm_del_win))
	DeadPipe(1);
      break;
    case PropertyNotify:
      if (event.xproperty.atom == XA_CUT_BUFFER0)
	octet = XFetchBuffer(dpy,&i,0);
      else if (event.xproperty.atom == XA_CUT_BUFFER1)
	octet = XFetchBuffer(dpy,&i,1);
      else if (event.xproperty.atom == XA_CUT_BUFFER2)
	octet = XFetchBuffer(dpy,&i,2);
      else if (event.xproperty.atom == XA_CUT_BUFFER3)
	octet = XFetchBuffer(dpy,&i,3);
      else if (event.xproperty.atom == XA_CUT_BUFFER4)
	octet = XFetchBuffer(dpy,&i,4);
      else if (event.xproperty.atom == XA_CUT_BUFFER5)
	octet = XFetchBuffer(dpy,&i,5);
      else if (event.xproperty.atom == XA_CUT_BUFFER6)
	octet = XFetchBuffer(dpy,&i,6);
      else if (event.xproperty.atom == XA_CUT_BUFFER7)
	octet = XFetchBuffer(dpy,&i,7);
      else break;
      if (i > 0)
      {
	Scrapt = xrealloc((void*)Scrapt,sizeof(char)*(i+1), sizeof(Scrapt));
	Scrapt = strcpy(Scrapt,octet);
      }
      break;
    }
  }
}


/* main event loop */
void MainLoop (void)
{
  fd_set in_fdset;
  int i;
  struct timeval tv;
  struct timeval *ptv;
  struct timeval now, last_periodic;
  long delta;
  fd_set_size_t fd_width = fd[1];
  Bool fsm_pending = False;

  if (x_fd > fd_width) fd_width = x_fd;
  ++fd_width;

  tv.tv_sec = 1;
  tv.tv_usec = 0;
  ptv = NULL;
  if (x11base->periodictasks != NULL)
  {
    gettimeofday(&last_periodic, NULL);
    ptv = &tv;
  }

  while ( !isTerminated )
  {
    while (FPending(dpy))
      ReadXServer();

    FD_ZERO(&in_fdset);
    FD_SET(x_fd,&in_fdset);
    FD_SET(fd[1],&in_fdset);
    fsm_fdset(&in_fdset);

    gettimeofday(&now, NULL);
    delta = (now.tv_sec - last_periodic.tv_sec) * 1000000;
    delta += now.tv_usec - last_periodic.tv_usec;

    if (fsm_pending)
    {
	    tv.tv_sec  = 0;
	    tv.tv_usec = 10000; /* 10 ms */
    }
    else
    {
	    tv.tv_usec = 1000000 - delta;
	    if (tv.tv_usec < 0)
	      tv.tv_usec = 0;
	    tv.tv_sec = delta ? 0 : 1;
    }

    if (fvwmSelect(fd_width, &in_fdset, NULL, NULL, ptv) > 0)
    {
      if (FD_ISSET(x_fd, &in_fdset))
	ReadXServer();

      if(FD_ISSET(fd[1], &in_fdset))
      {
	FvwmPacket* packet = ReadFvwmPacket(fd[1]);
	if ( packet == NULL )
	  exit(0);
	if (packet->type == M_CONFIG_INFO) {
	  char *line, *token;
	  int n;

	  line = (char*)&(packet->body[3]);
	  line = GetNextToken(line, &token);
	  if (StrEquals(token, "Colorset")) {
	    /* track all colorset changes and update display if necessary */
	    n = LoadColorset(line);
	    for (i=0; i<nbobj; i++) {
	      if (n == tabxobj[i]->colorset) {
		tabxobj[i]->TabColor[fore] = Colorset[n].fg;
		tabxobj[i]->TabColor[back] = Colorset[n].bg;
		tabxobj[i]->TabColor[shad] = Colorset[n].shadow;
		tabxobj[i]->TabColor[hili] = Colorset[n].hilite;
		if (tabxobj[i]->TypeWidget != SwallowExec) {
		  SetWindowBackground(dpy, tabxobj[i]->win, tabxobj[i]->width,
				      tabxobj[i]->height, &Colorset[n], Pdepth,
				      tabxobj[i]->gc, False);
		  XClearWindow(dpy, tabxobj[i]->win);
		}
		tabxobj[i]->DrawObj(tabxobj[i],NULL);
	      }
	    }
	    if (n == x11base->colorset) {
	      x11base->TabColor[fore] = Colorset[n].fg;
	      x11base->TabColor[back] = Colorset[n].bg;
	      x11base->TabColor[shad] = Colorset[n].shadow;
	      x11base->TabColor[hili] = Colorset[n].hilite;
	      SetWindowBackground(dpy, x11base->win, x11base->size.width,
				  x11base->size.height, &Colorset[n], Pdepth,
				  x11base->gc, True);
	      for (i=0; i<nbobj; i++)
	      {
		if (Rectangle != tabxobj[i]->TypeWidget &&
		    SwallowExec != tabxobj[i]->TypeWidget &&
		    CSET_IS_TRANSPARENT_PR(tabxobj[i]->colorset))
		{
		  XClearArea(dpy, tabxobj[i]->win, 0, 0, 0, 0, True);
		}
	      }
	    }
	  }
	  else if (StrEquals(token, XINERAMA_CONFIG_STRING)) {
	    FScreenConfigureModule(line);
	  }
	  if (token)
	    free(token);
	}
	else if (packet->type == MX_PROPERTY_CHANGE)
	{
	  if (packet->body[0] == MX_PROPERTY_CHANGE_BACKGROUND &&
	      ((!x11base->swallowed && packet->body[2] == 0) ||
	       (x11base->swallowed && packet->body[2] == x11base->win)))
	  {
	    UpdateRootTransparency(True);
	  }
	  else if  (packet->body[0] == MX_PROPERTY_CHANGE_SWALLOW &&
		    packet->body[2] == x11base->win)
	  {
		  char *str;
		  unsigned long u;
		  Window s;
		  char cmd[256];

		  x11base->swallowed = packet->body[1];
		  str = (char *)&(packet->body[3]);
		  if (x11base->swallowed && str && sscanf(str,"%lu",&u) == 1)
		  {
			  x11base->swallower_win = (Window)u;
		  }
		  else
		  {
			  x11base->swallower_win = 0;
		  }
		  /* update the swallower */
		  s = ((x11base->swallower_win && x11base->swallowed)?
		       x11base->swallower_win:x11base->win);
		  for (i=0; i<nbobj; i++)
		  {
			  if (tabxobj[i]->TypeWidget == SwallowExec &&
			      tabxobj[i]->win != None)
			  {
				  sprintf(cmd,"PropertyChange %u %u %lu %lu",
					  MX_PROPERTY_CHANGE_SWALLOW, 1,
					  tabxobj[i]->win, s);
				  SendText(fd,cmd,0);
			  }
		  }
		  UpdateRootTransparency(False);
	  }
	}
	else if (packet->type == M_STRING) {
	  char *action, *token;
	  action = (char*)&(packet->body[3]);
	  action = GetNextToken(action, &token);

          SendMsgAndString(action, token);

	  if (token)
	    free(token);
	}
	else
	  for (i=0; i<nbobj; i++)
	    tabxobj[i]->ProcessMsg(tabxobj[i], packet->type, packet->body);
      }

      fsm_pending = fsm_process(&in_fdset);
    }

    if (!isTerminated && x11base->periodictasks!=NULL && delta >= 1000000)
    {
      /* Execution des taches periodics */
      last_periodic.tv_sec = now.tv_sec;
      last_periodic.tv_usec = now.tv_usec;
      ExecBloc(x11base->periodictasks);
      usleep(10000);		/* slight delay, not the whole second */
    }
  }
}

void ReadFvwmScriptArg(int argc, char **argv,int IsFather)
{
  int i;
  Atom myatom;

  BuffSend.NbMsg=0;                     /* Aucun message dans le buffer */

  for (i=2; i<98; i++)
    x11base->TabScriptId[i]=NULL;

  if (IsFather)                 /* Cas du pere */
  {
    myatom = XInternAtom(dpy,x11base->TabScriptId[1],True);
    XSetSelectionOwner(dpy, myatom, x11base->win, CurrentTime);
  }
  else
  {                             /* Cas du fils */
    x11base->TabScriptId[0] = xcalloc(sizeof(char), strlen(argv[7]));
    x11base->TabScriptId[0] = strncpy(x11base->TabScriptId[0],argv[7],
				    strlen(argv[7])-2);
    x11base->TabScriptId[1] = argv[7];
    myatom=XInternAtom(dpy, x11base->TabScriptId[1], True);
    XSetSelectionOwner(dpy, myatom, x11base->win, CurrentTime);
  }
}


/* signal handler to close down the module */
static RETSIGTYPE
TerminateHandler(int sig)
{
  fvwmSetTerminate(sig);
  SIGNAL_RETURN;
}


/* main procedure */
int main (int argc, char **argv)
{
  int IsFather;
  int i;

  FlocaleInit(LC_CTYPE, "", "", "FvwmScript");

  module = ParseModuleArgs(argc,argv,0); /* no alias */
  if (module == NULL)
  {
    fprintf(stderr,"FvwmScript must be started by Fvwm.\n");
    exit(1);
  }

  if (module->user_argc == 0)
  {
    fprintf(stderr,"FvwmScript requires the script's name or path.\n");
    exit(1);
  }

  /* On determine si le script a un pere */
  if (module->user_argc >= 2)
    IsFather = (module->user_argv[1][0] != (char)161);
  else
    IsFather = 1;

  ScriptName = module->user_argv[0];
  ScriptBaseName = GetFileNameFromPath(ScriptName);
  ref = strtol(argv[4], NULL, 16);
  if (ref == 0) ref = None;
  fd[0] = module->to_fvwm;
  fd[1] = module->from_fvwm;
  SetMessageMask(fd, M_NEW_DESK | M_END_WINDOWLIST| M_STRING |
		 M_MAP|  M_RES_NAME| M_RES_CLASS| M_CONFIG_INFO|
		 M_END_CONFIG_INFO| M_WINDOW_NAME | M_SENDCONFIG);
  SetMessageMask(fd, MX_PROPERTY_CHANGE);
  /* Enregistrement des arguments du script */
  x11base = xcalloc(1, sizeof(X11base));
  x11base->TabArg[0] = module->name;
  for (i=2-IsFather; i< module->user_argc; i++)
    x11base->TabArg[i-1+IsFather] = module->user_argv[i];
  /* Couleurs et fontes par defaut */
  x11base->font = NULL;
  x11base->forecolor = xstrdup("black");
  x11base->backcolor = xstrdup("grey85");
  x11base->shadcolor = xstrdup("grey55");
  x11base->hilicolor = xstrdup("grey100");
  x11base->colorset = -1;
  x11base->swallowed = False;

  /* Initialisation du serveur X et de la fenetre */
  Xinit(IsFather);

  ParseOptions();

  SendText(fd,"Send_WindowList",0);

  ReadConfig(ScriptName);       /* Lecture et analyse du script */

  /* Fonction d'initialisation de TabCom et TabFunc   */
  InitCom();

#ifdef HAVE_SIGACTION
  {
    struct sigaction  sigact;

    sigemptyset(&sigact.sa_mask);
    sigaddset(&sigact.sa_mask, SIGPIPE);
    sigaddset(&sigact.sa_mask, SIGINT);
    sigaddset(&sigact.sa_mask, SIGHUP);
    sigaddset(&sigact.sa_mask, SIGTERM);
    sigaddset(&sigact.sa_mask, SIGQUIT);
#ifdef SA_INTERRUPT
    sigact.sa_flags = SA_INTERRUPT;
#else
    sigact.sa_flags = 0;
#endif
    sigact.sa_handler = TerminateHandler;
    sigaction(SIGPIPE, &sigact, NULL);
    sigaction(SIGINT,  &sigact, NULL);
    sigaction(SIGHUP,  &sigact, NULL);
    sigaction(SIGTERM, &sigact, NULL);
    sigaction(SIGQUIT, &sigact, NULL);
  }
#else
#ifdef USE_BSD_SIGNALS
  fvwmSetSignalMask( sigmask(SIGPIPE) |
		     sigmask(SIGINT) |
		     sigmask(SIGHUP) |
		     sigmask(SIGTERM) |
		     sigmask(SIGQUIT) );
#endif
  signal (SIGPIPE, TerminateHandler);
  signal (SIGINT, TerminateHandler);  /* cleanup on other ways of closing too */
  signal (SIGHUP, TerminateHandler);
  signal (SIGQUIT, TerminateHandler);
  signal (SIGTERM, TerminateHandler);
#ifdef HAVE_SIGINTERRUPT
  siginterrupt(SIGPIPE, 1);
  siginterrupt(SIGINT,  1);
  siginterrupt(SIGHUP,  1);
  siginterrupt(SIGQUIT, 1);
  siginterrupt(SIGTERM, 1);
#endif
#endif

  /* Compute the Shift-Tab keysym */
  {
    KeyCode tab = XKeysymToKeycode(dpy, XK_Tab);
    shift_tab_ks = fvwm_KeycodeToKeysym(dpy, tab, ShiftMask, 0);
  }

  /* Construction des boutons et de la fenetre */
  BuildGUI(IsFather);

  ReadFvwmScriptArg(argc,argv,IsFather);

  /* tell fvwm we're running */
  SendFinishedStartupNotification(fd);

  MainLoop();
  return 0;
}
