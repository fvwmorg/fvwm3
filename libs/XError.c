/* Copyright (C)",
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc.,",
 */

/*
 * This function should be used by all modules and fvwm when a X error
 * occurs and the module exits.
 */

#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>

static char *error_name(unsigned char code);
static char *request_name(unsigned char code);
static char unknown[32];

#define USE_GET_ERROT_TEXT 1
void PrintXErrorAndCoredump(Display *dpy, XErrorEvent *error, char *MyName)
{
#ifdef USE_GET_ERROR_TEXT
  char msg[256];

  msg[255] = 0;
  /* can't call this from within an error handler! */
  /* DV (21-Nov-2000): Well, actually we *can* call it in an error handler
   * since it does not trigger a protocol request. */
  XGetErrorText(dpy, error->error_code, msg, sizeof(msg));
  fprintf(stderr,"%s: Cause of next X Error.\n", MyName);
  fprintf(stderr, "   Error: %d (%s)\n", error->error_code, msg);
#else
  fprintf(stderr,"%s: Cause of next X Error.\n", MyName);
  fprintf(stderr, "   Error: %d (%s)\n",
	  error->error_code, error_name(error->error_code));
#endif
  fprintf(stderr, "   Major opcode of failed request:  %d (%s)\n",
	  error->request_code, request_name(error->request_code));
  fprintf(stderr, "   Minor opcode of failed request:  %d \n",
	  error->minor_code);
  /* error->resourceid may be uninitialised. This is no proble since we are
   * dumping core anyway. */
  fprintf(stderr, "   Resource id of failed request:  0x%lx \n",
	  error->resourceid);

  /* leave a coredump */
  fprintf(stderr, " Leaving a core dump now\n");
  abort();
  /* exit if this fails */
  exit(99);
}

#ifndef USE_GET_ERROR_TEXT
/* this comes out of X.h */
static char *error_names[] = {
  "BadRequest",
  "BadValue",
  "BadWindow",
  "BadPixmap",
  "BadAtom",
  "BadCursor",
  "BadFont",
  "BadMatch",
  "BadDrawable",
  "BadAccess",
  "BadAlloc",
  "BadColor",
  "BadGC",
  "BadIDChoice",
  "BadName",
  "BadLength",
  "BadImplementation"
};


static char *error_name(unsigned char code)
{
  if (code == 0 || code > (sizeof(error_names) / sizeof(char *)))
  {
    sprintf(unknown, "Unknown: %d", (int)code);
    return unknown;
  }
  return error_names[code - 1];
}
#endif

/* this comes out of Xproto.h */
static char *code_names[] = {
  "CreateWindow",
  "ChangeWindowAttributes",
  "GetWindowAttributes",
  "DestroyWindow",
  "DestroySubwindows",
  "ChangeSaveSet",
  "ReparentWindow",
  "MapWindow",
  "MapSubwindows",
  "UnmapWindow",
  "UnmapSubwindows",
  "ConfigureWindow",
  "CirculateWindow",
  "GetGeometry",
  "QueryTree",
  "InternAtom",
  "GetAtomName",
  "ChangeProperty",
  "DeleteProperty",
  "GetProperty",
  "ListProperties",
  "SetSelectionOwner",
  "GetSelectionOwner",
  "ConvertSelection",
  "SendEvent",
  "GrabPointer",
  "UngrabPointer",
  "GrabButton",
  "UngrabButton",
  "ChangeActivePointerGrab",
  "GrabKeyboard",
  "UngrabKeyboard",
  "GrabKey",
  "UngrabKey",
  "AllowEvents",
  "GrabServer",
  "UngrabServer",
  "QueryPointer",
  "GetMotionEvents",
  "TranslateCoords",
  "WarpPointer",
  "SetInputFocus",
  "GetInputFocus",
  "QueryKeymap",
  "OpenFont",
  "CloseFont",
  "QueryFont",
  "QueryTextExtents",
  "ListFonts",
  "ListFontsWithInfo",
  "SetFontPath",
  "GetFontPath",
  "CreatePixmap",
  "FreePixmap",
  "CreateGC",
  "ChangeGC",
  "CopyGC",
  "SetDashes",
  "SetClipRectangles",
  "FreeGC",
  "ClearArea",
  "CopyArea",
  "CopyPlane",
  "PolyPoint",
  "PolyLine",
  "PolySegment",
  "PolyRectangle",
  "PolyArc",
  "FillPoly",
  "PolyFillRectangle",
  "PolyFillArc",
  "PutImage",
  "GetImage",
  "PolyText",
  "PolyText1",
  "ImageText",
  "ImageText1",
  "CreateColormap",
  "FreeColormap",
  "CopyColormapAndFree",
  "InstallColormap",
  "UninstallColormap",
  "ListInstalledColormaps",
  "AllocColor",
  "AllocNamedColor",
  "AllocColorCells",
  "AllocColorPlanes",
  "FreeColors",
  "StoreColors",
  "StoreNamedColor",
  "QueryColors",
  "LookupColor",
  "CreateCursor",
  "CreateGlyphCursor",
  "FreeCursor",
  "RecolorCursor",
  "QueryBestSize",
  "QueryExtension",
  "ListExtensions",
  "ChangeKeyboardMapping",
  "GetKeyboardMapping",
  "ChangeKeyboardControl",
  "GetKeyboardControl",
  "Bell",
  "ChangePointerControl",
  "GetPointerControl",
  "SetScreenSaver",
  "GetScreenSaver",
  "ChangeHosts",
  "ListHosts",
  "SetAccessControl",
  "SetCloseDownMode",
  "KillClient",
  "RotateProperties",
  "ForceScreenSaver",
  "SetPointerMapping",
  "GetPointerMapping",
  "SetModifierMapping",
  "GetModifierMapping"
};


static char *request_name(unsigned char code)
{
  if (code == 0 || code > (sizeof(code_names) / sizeof(char *)))
  {
    sprintf(unknown, "Unknown: %d", (int)code);
    return unknown;
  }
  return code_names[code - 1];
}
