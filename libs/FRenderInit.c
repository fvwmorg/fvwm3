/* -*-c-*- */
/* Copyright (C) 2002  Olivier Chapuis */
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

/* ---------------------------- included header files ---------------------- */

#include "config.h"

#include <stdio.h>

#include <X11/Xlib.h>

#include "fvwmlib.h"
#include "PictureBase.h"
#include "FRenderInit.h"

/* ---------------------------- local definitions -------------------------- */

/* ---------------------------- local macros ------------------------------- */

/* ---------------------------- imports ------------------------------------ */

/* ---------------------------- included code files ------------------------ */

/* ---------------------------- local types -------------------------------- */

/* ---------------------------- forward declarations ----------------------- */

/* ---------------------------- local variables ---------------------------- */

Bool FRenderExtensionSupported = False;
int FRenderErrorBase = -10000;
int FRenderMajorOpCode = -10000;
int FRenderAlphaDepth = 0;

/* ---------------------------- exported variables (globals) --------------- */

/* ---------------------------- local functions ---------------------------- */

void FRenderInit(Display *dpy)
{
	int event_basep;

	FRenderAlphaDepth = 8;
	if (!XRenderSupport || !(FRenderExtensionSupported = XQueryExtension(
		dpy, "RENDER", &FRenderMajorOpCode, &event_basep,
		&FRenderErrorBase)))
	{
		int *pmf = NULL;
		int i,n;
		int alpha_depth = 0;

		FRenderErrorBase = -10000;
		FRenderMajorOpCode = -10000;
		FRenderExtensionSupported = 0;
		pmf = XListDepths(dpy, DefaultScreen(dpy), &n);
		if (pmf)
		{
			i = 0;
			while(i < n)
			{
				if (pmf[i] == 8)
				{
					alpha_depth = 8;
					i = n-1;
				}
				else if (pmf[i] >= 8 &&
					 (pmf[i] < alpha_depth ||
					  alpha_depth == 0))
				{
					alpha_depth = pmf[i];
				}
				i++;
			}
			XFree(pmf);
		}
		FRenderAlphaDepth = alpha_depth;
	}
}


int FRenderGetErrorCodeBase(void)
{
	return FRenderErrorBase;
}

int FRenderGetMajorOpCode(void)
{
	return FRenderMajorOpCode;
}

Bool FRenderGetExtensionSupported(void)
{
	return FRenderExtensionSupported;
}

int FRenderGetAlphaDepth(void)
{
	return FRenderAlphaDepth;
}

Bool FRenderGetErrorText(int code, char *msg)
{

	if (XRenderSupport)
	{
		static char *error_names[] = {
			"BadPictFormat",
			"BadPicture",
			"BadPictOp",
			"BadGlyphSet",
			"BadGlyph"
		};

		if (code >= FRenderErrorBase &&
		    code <= FRenderErrorBase +
		    (sizeof(error_names) / sizeof(char *)) -1)
		{
			sprintf(msg, "%s\n", error_names[code - FRenderErrorBase]);
			return 1;
		}
	}
	return 0;
}
