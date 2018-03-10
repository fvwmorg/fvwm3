/* -*-c-*- */
/*
 * FvwmButtons, copyright 1996, Jarl Totland
 *
 * This module, and the entire GoodStuff program, and the concept for
 * interfacing this module to the Window Manager, are all original work
 * by Robert Nation
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

#include "misc.h"
#include <stdio.h>

/**
*** ConstrainSize()
*** Adjust a given width and height to account for the constraints imposed by
*** size hints.
*** The general algorithm, especially the aspect ratio stuff, is borrowed from
*** uwm's CheckConsistency routine.
**/
void ConstrainSize (XSizeHints *hints, int *widthp, int *heightp)
{
#define makemult(a,b) ((b == 1) ? (a) : (((int)((a) / (b))) * (b)))

	int minWidth, minHeight, maxWidth, maxHeight, xinc, yinc, delta;
	int baseWidth, baseHeight;
	int dwidth = *widthp, dheight = *heightp;

	if (hints->flags & PMinSize)
	{
		minWidth = hints->min_width;
		minHeight = hints->min_height;
		if (hints->flags & PBaseSize)
		{
			baseWidth = hints->base_width;
			baseHeight = hints->base_height;
		}
		else
		{
			baseWidth = hints->min_width;
			baseHeight = hints->min_height;
		}
	}
	else if (hints->flags & PBaseSize)
	{
		minWidth = hints->base_width;
		minHeight = hints->base_height;
		baseWidth = hints->base_width;
		baseHeight = hints->base_height;
	}
	else
	{
		minWidth = 1;
		minHeight = 1;
		baseWidth = 1;
		baseHeight = 1;
	}

	if (hints->flags & PMaxSize)
	{
		maxWidth = hints->max_width;
		maxHeight = hints->max_height;
	}
	else
	{
		maxWidth = 32767;
		maxHeight = 32767;
	}
	if (hints->flags & PResizeInc)
	{
		xinc = hints->width_inc;
		yinc = hints->height_inc;
	}
	else
	{
		xinc = 1;
		yinc = 1;
	}

	/*
	 * First, clamp to min and max values
	 */
	if (dwidth < minWidth)
	{
		dwidth = minWidth;
	}
	if (dheight < minHeight)
	{
		dheight = minHeight;
	}

	if (dwidth > maxWidth)
	{
		dwidth = maxWidth;
	}
	if (dheight > maxHeight)
	{
		dheight = maxHeight;
	}


	/*
	 * Second, fit to base + N * inc
	 */
	dwidth = ((dwidth - baseWidth) / xinc * xinc) + baseWidth;
	dheight = ((dheight - baseHeight) / yinc * yinc) + baseHeight;


	/*
	 * Third, adjust for aspect ratio
	 */
#define maxAspectX hints->max_aspect.x
#define maxAspectY hints->max_aspect.y
#define minAspectX hints->min_aspect.x
#define minAspectY hints->min_aspect.y
	/*
	 * The math looks like this:
	 *
	 * minAspectX    dwidth     maxAspectX
	 * ---------- <= ------- <= ----------
	 * minAspectY    dheight    maxAspectY
	 *
	 * If that is multiplied out, then the width and height are
	 * invalid in the following situations:
	 *
	 * minAspectX * dheight > minAspectY * dwidth
	 * maxAspectX * dheight < maxAspectY * dwidth
	 *
	 */

	if (hints->flags & PAspect)
	{
		if (minAspectX * dheight > minAspectY * dwidth)
		{
			delta = makemult(
				minAspectX * dheight / minAspectY - dwidth,
				xinc);
			if (dwidth + delta <= maxWidth)
			{
				dwidth += delta;
			}
			else
			{
				delta = makemult(
					dheight - minAspectY*dwidth/minAspectX,
					yinc);
				if (dheight - delta >= minHeight)
				{
					dheight -= delta;
				}
			}
		}

		if (maxAspectX * dheight < maxAspectY * dwidth)
		{
			delta = makemult(
				dwidth * maxAspectY / maxAspectX - dheight,
				yinc);
			if (dheight + delta <= maxHeight)
			{
				dheight += delta;
			}
			else
			{
				delta = makemult(
					dwidth - maxAspectX*dheight/maxAspectY,
					xinc);
				if (dwidth - delta >= minWidth)
				{
					dwidth -= delta;
				}
			}
		}
	}

	*widthp = dwidth;
	*heightp = dheight;
	return;
#undef makemult
#undef maxAspectX
#undef maxAspectY
#undef minAspectX
#undef minAspectY
}
