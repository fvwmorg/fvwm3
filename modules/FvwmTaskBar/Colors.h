/* Part of the FvwmTaskBar Module for Fvwm.
 *
 * The functions in this header file were originally part of the GoodStuff
 * and FvwmIdent modules for Fvwm, so there copyrights are listed:
 *
 * Copyright 1994, Robert Nation and Nobutaka Suzuki.
 * No guarantees or warantees or anything
 * are provided or implied in any way whatsoever. Use this program at your
 * own risk. Permission to use this program for any purpose is given,
 * as long as the copyright is kept intact. */

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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <X11/Intrinsic.h>

/* Function Prototypes */
Pixel GetColor(char *name);
Pixel GetHilite(Pixel background);
Pixel GetShadow(Pixel background);
void nocolor(char *a, char *b);

