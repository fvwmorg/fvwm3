#include "config.h"

#include "../../fvwm/module.h"
#include "fvwmlib.h"

#include <stdio.h>
#include <signal.h>
#include <ctype.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>

#define XK_MISCELLANY
#include <sys/types.h>
#include <sys/time.h>

#if HAVE_SYS_BSDTYPES_H
#include <sys/bsdtypes.h> /* Saul */
#endif

#if HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

#include <X11/keysymdef.h>
#include <X11/keysym.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <X11/cursorfont.h>
#include <X11/Xatom.h>
#include <X11/xpm.h>
#include <X11/X.h>


/* Constante de couleurs */
#define black 0
#define white 1
#define back 2
#define fore 3
#define shad 4
#define li 5

/* Constante vrai ou faux */
#define True 1
#define False 0

/* Constante type de widget */
#define PushButton	1
#define RadioButton	2
#define ItemDraw	3
#define CheckBox	4
#define TextField	5
#define HScrollBar	6
#define VScrollBar	7
#define PopupMenu	8
#define Rectangle	9
#define MiniScroll	10
#define SwallowExec	11
#define HDipstick	12
#define VDipstick	13
#define List		14
#define Menu		15

/* Structure qui regroupe commande et boucle conditionnelle */
typedef struct
{
 int Type;		/* Type de la commande */
 long *TabArg;		/* Tableau d'argument: id var,fnc,const,adr bloc... */
 int NbArg;
} Instr;


/* Structure bloc d'instruction */
typedef struct
{
 Instr *TabInstr;		/* Tableau d'instruction */
 int NbInstr;			/* Nombre d'instruction */
} Bloc;


/* Structure pour enregistrer l'entete du script: valeur par defaut */
typedef struct			/* Type pour la gestion X */
{
  int x,y ;			/* Origine de la fenetre */
  int height,width;		/* Hauteur et largeur */
  char *titlewin;		/* Titre */
  char *forecolor;		/* Couleur des lignes */
  char *backcolor;		/* Couleur de fond */
  char *shadcolor;		/* Couleur des lignes */
  char *licolor;		/* Couleur des lignes */
  char *font;			/* Police utilisee */
  char *icon;			/* Icone pour l'application iconisee */
  Bloc *periodictasks;		/* Tableau de taches periodiques */
  Bloc *initbloc;		/* Bloc d'initalisation */
} ScriptProp;


typedef struct
{
 int NbCase;
 int *LstCase;		/* Tableau des valeurs case a verifier */
} CaseObj;

/* Structure pour enregistrer les caracteristiques des objets graphiques */
typedef struct			/* Type pour les boutons */
{
  int id;
  char *type;
  int x,y ;			/* Origine du bouton */
  int height,width;		/* Hauteur et largeur */
  char *title;			/* Titre */
  char *swallow;		/* swallowexec */
  char *icon;			/* Icon */
  char *forecolor;		/* Couleur des lignes */
  char *backcolor;		/* Couleur de fond */
  char *shadcolor;
  char *licolor;
  char *font;			/* Police utilisé */
  int flags[5];			/* Etat du bouton:invisible, inactif et actif */
  int value;
  int value2;
  int value3;
} MyObject;

typedef MyObject TabObj[100];


typedef struct
{
  Display *display ;
  int screen ;
  Window root ;
  Window win ;
  Colormap colormap ;
  GC gc ;
  int depth ;
  unsigned long WhitePix ;
  unsigned long BlackPix ;
  XColor TabColor[6];
  XSizeHints size;
  char *backcolor;
  char *forecolor;
  char *shadcolor;
  char *licolor;
  char *font;
  char *icon;
  char *title;
  Bloc *periodictasks;
  int HaveXSelection;
  char* TabScriptId[99];
  int NbChild;
  time_t BeginTime;
  char *TabArg[20];
} X11base ;

typedef struct
{
 char *Msg;
 char* R;
} TypeName;

typedef struct
{
 int NbMsg;
 TypeName TabMsg[40];
} TypeBuffSend ;

struct XObj
{
  int TypeWidget;
  Window win;		/* Fenetre contenant l'objet */
  Window *ParentWin;		/* Fenetre parent */
  Display *display;
  int Screen;
  Colormap *colormap;
  XColor TabColor[6];
  GC gc;			/* gc utilise pour les requetes: 4 octets */
  int id;			/* Numero d'id */
  int x,y;			/* Origine du bouton */
  int height,width;		/* Hauteur et largeur */
  char *title;			/* Titre */
  char *swallow;		/* SwallowExec */
  char *icon;			/* Icon */
  char *forecolor;		/* Couleur des lignes */
  char *backcolor;		/* Couleur de fond */
  char *shadcolor;		/* Couleur des lignes */
  char *licolor;		/* Couleur de fond */
  char *font;			/* Police utilisee */
  Pixmap iconPixmap;		/* Icone charge */
  Pixmap icon_maskPixmap;	/* Icone masque */
  int icon_w,icon_h;		/* Largeur et hauteur de l'icone */
  XFontStruct *xfont;
  int value;			/* Valeur courante */
  int value2;			/* Valeur minimale */
  int value3;			/* Valeur maximale */
  int flags[5];
  void (*InitObj) (struct XObj *xobj);	/* Initialisation de l'objet */
  void (*DestroyObj) (struct XObj *xobj);	/* Destruction objet */
  void (*DrawObj) (struct XObj *xobj);	/* Dessin de l'objet */
  void (*EvtMouse) (struct XObj *xobj,XButtonEvent *EvtButton);
  void (*EvtKey) (struct XObj *xobj,XKeyEvent *EvtKey);
  void (*ProcessMsg) (struct XObj *xobj,unsigned long type,unsigned long *body);
  void *UserPtr;
};

void ChooseFunction(struct XObj *xobj,char *type);

void SendMsg(struct XObj *xobj,int TypeMsg);

void ExecBloc(Bloc *bloc);

void UnselectAllTextField(struct XObj **xobj);

int MyAllocNamedColor(Display *display,Colormap colormap,char* colorname,XColor* color);

void Quit (int NbArg,long *TabArg);

void SendMsgToScript(XEvent event);
