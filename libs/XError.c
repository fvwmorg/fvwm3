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

void PrintXErrorAndCoredump(Display *dpy, XErrorEvent *error, char *MyName)
{
#if 0
  /* can't call this from within an error handler! */
  XGetErrorText(dpy, error->error_code, msg, sizeof(msg));
#endif

  fprintf(stderr,"%s: Cause of next X Error.\n", MyName);
  fprintf(stderr, "   Error: %d (%s)\n",
	  error->error_code, error_name(error->error_code));
  fprintf(stderr, "   Major opcode of failed request:  %d (%s)\n",
	  error->request_code, request_name(error->request_code));
  fprintf(stderr, "   Minor opcode of failed request:  %d \n",
	  error->minor_code);
  fprintf(stderr, "   Resource id of failed request:  0x%lx \n",
	  error->resourceid);
  fprintf(stderr, " Leaving a core dump now\n");

  /* leave a coredump */
  abort();
  /* exit if this fails */
  exit(99);
}

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
  if (code > (sizeof(error_names) / sizeof(char *)))
    return "Unknown";
  return error_names[code - 1];
}

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
  if (code > (sizeof(code_names) / sizeof(char *)))
    return "Unknown";
  return code_names[code - 1];
}
