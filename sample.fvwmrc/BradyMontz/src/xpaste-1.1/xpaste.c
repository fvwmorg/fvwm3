/* 
 * Xpaste - grab the current primary selection, and pop up a window containing
 *          that selection. 
 *        - print button added by Stewart A. Levin 6/20/91
 *
 */

#ifndef lint
static char rcsid[] = "$Id$";
#endif

#include <stdio.h>
#include <X11/StringDefs.h>
#include <X11/Xlib.h>
#include <X11/Intrinsic.h>
#include <X11/Xatom.h>
#include <X11/Xaw/Text.h>
#include <X11/Xaw/AsciiText.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Command.h>
#include <varargs.h>
#include "patchlevel.h"

#ifndef WIDTHPADDING
#define WIDTHPADDING 6
#endif /* WIDTHPADDING */
#ifndef HEIGHTPADDING
#define HEIGHTPADDING 6
#endif /* HEIGHTPADDING */

void Xit();
void Printit();
void GetDimensions();
int GetPrimarySelection();

char *progname;
char **av;
int ac;

static XtCallbackRec done[] = {
	{Xit, NULL},
	{NULL,NULL},
};
static XtCallbackRec print[] = {
	{Printit, NULL},
	{NULL,NULL},
};

Widget CreateWidget();
void ConfigureWidget();

static char *buff;
static Widget root, text, fm, button[3];

static struct xpaste_res {
    XFontStruct *font;
    char *printcmd;
    } appres;

typedef struct xpaste_res *xpres;
extern char *getenv();

main(argc, argv)
int argc;
char *argv[];
{

	Dimension height, width;

	progname=argv[0];
	av=argv;
	ac=argc;
	root = XtInitialize(argv[0], "XPaste", NULL, 0, &argc, argv);

	(void)GetPrimarySelection(XtDisplay(root), &buff);

	/* 
	 * If there is no current selection, exit quietly. 
	 */
	if(NULL==buff) (void)exit(0);
	
	/*
	 * Compute the extents of the text. The eventual width of the text 
	 * widget will be the length (in pixels) of the longest line. Variable
	 * width fonts are handled correctly. The eventual height of the text
	 * widget will be the sum of the heights of the lines in the selection.
	 * Yes, computing the height for a single line and multiplying by the
	 * number of lines would be more efficient. But there's no server
	 * roundtrip required by XTextExtents, so the cost isn't high. And who
	 * knows: maybe we'll see variable height fonts?  
	 */

	{
		/* Get the default font */
		static XtResource resources[] = {
			{"font", XtCFont, XtRFontStruct, sizeof(XFontStruct *),
			XtOffset(xpres,font), XtRString, "fixed"},
			{"printcmd", XtCString, XtRString, sizeof(char *),
			XtOffset(xpres,printcmd), XtRString, "lpr"},
		};
		XtGetApplicationResources( root, (caddr_t)(&appres), 
				resources, XtNumber(resources), 
				(ArgList)NULL, (Cardinal)0      );
	}
	GetDimensions(buff, appres.font, &width, &height);

	/* 
	 * now create the text widget, using the computed width and height and 
	 * the selection string.
	 */
	fm = CreateWidget("form", formWidgetClass, root, 
					   XtNdefaultDistance,  0,
					   NULL);
	button[0] = CreateWidget("done", commandWidgetClass, fm,
					   XtNcallback,         done,
					   XtNresizable,        False,
					   NULL);
	button[1] = CreateWidget("print", commandWidgetClass,fm,
					   XtNcallback,         print,
					   XtNresizable,        False,
					   XtNfromHoriz,        button[0],
					   NULL);
	text = CreateWidget("text", asciiTextWidgetClass, fm,
					   XtNwidth,            width,
					   XtNheight,           height,
					   XtNstring,           buff,
					   XtNfont,             appres.font,
					   XtNfromVert,         button[0],
					   XtNborderWidth,      0,
					   XtNresizable,        True,
					   XtNdisplayCaret,	False,
					   NULL); 

	XtRealizeWidget(root);
	XtMainLoop();
}

void Xit(w, closure, call_data)
Widget w;
caddr_t closure;
caddr_t call_data;
{
	/* exit program */
	(void)exit(0);
}

void Printit(w, closure, call_data)
Widget w;
caddr_t closure;
caddr_t call_data;
{
	FILE *syscmd;
	if(NULL==(syscmd=popen(appres.printcmd,"w"))) {
		(void)XtWarning("Can't create pipe\n");
	} else {
		(void) fprintf(syscmd,"%s\n",buff);
		(void) pclose(syscmd);
	}
}

void GetDimensions(string, font, width, height)
char *string;
XFontStruct *font;
Dimension *width, *height;
{
	/* Compute the width and height of the given string when    */
	/* displayed using the specified font. It does the right    */
	/* thing for both variable width and height fonts, and      */
	/* multiple-line (newline) strings. TABS are not supported. */
	char *p, *b;
	int dir, ascent, descent;
	XCharStruct all;

	p=b=string;
	*width=*height=0;

	while(True){
		if(!*p || '\n'==*p){
			Dimension tmp;

			/* height computation */
			XTextExtents(font,b,p-b,&dir,&ascent,&descent,&all);
			*height+=(Dimension)(ascent+descent);

			/* width computation */
			tmp=(Dimension)XTextWidth(font, b, p-b);
			if(tmp>*width) *width=tmp;

			b=p+1;
			if(!*p) break;
		}
		p++;
	}
	*width+=WIDTHPADDING;
	*height+=HEIGHTPADDING;
}

int GetPrimarySelection(display, string)
Display *display;
char **string;
{
	/* 
	 * Get and return the primary selection. This clobbers cut buffer 0.
	 * The return value is the number of bytes.
	 */

	int bytes;
	Window root;

	/* 
	 * Need to convert PRIMARY selection to CUT_BUFFER0 because 
	 * XFetchBytes only knows how to get contents of cut buffer 0. I 
	 * could have used the Xt selection functions to grab the PRIMARY
	 * selection directly, but they're unnecessarily complicated.
	 * 
	 * Note: gnuemacs doesn't use the primary selection, so this won't
	 *       work in an emacs window. 
	 */
	
	root=XDefaultRootWindow(display);
	XConvertSelection(display, XA_PRIMARY, XA_CUT_BUFFER0, None, root,
			  CurrentTime);
	(*string)=XFetchBytes(display, &bytes);
	return(bytes);
}
