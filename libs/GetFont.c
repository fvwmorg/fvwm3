#include "config.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "fvwmlib.h"

/*
** loads font or "fixed" on failure
*/
XFontStruct *GetFontOrFixed(Display *disp, char *fontname)
{
  XFontStruct *fnt;

  if ((fnt = XLoadQueryFont(disp,fontname))==NULL)
  {
    fprintf(stderr,
            "[GetFontOrFixed]: WARNING -- can't get font %s, trying 'fixed'",
            fontname);
    /* fixed should always be avail, so try that */
    if ((fnt = XLoadQueryFont(disp,"fixed"))==NULL) 
    {
      fprintf(stderr,"[GetFontOrFixed]: ERROR -- can't get font 'fixed'");
    }
  }
  return fnt;
}

