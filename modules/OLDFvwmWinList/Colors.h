/* Part of the FvwmWinList Module for Fvwm. 
 *
 * The functions in this header file were originally part of the GoodStuff
 * and FvwmIdent modules for Fvwm, so there copyrights are listed:
 *
 * Copyright 1994, Robert Nation and Nobutaka Suzuki.
 * No guarantees or warantees or anything
 * are provided or implied in any way whatsoever. Use this program at your
 * own risk. Permission to use this program for any purpose is given,
 * as long as the copyright is kept intact. */

#include <X11/Intrinsic.h>

/* Function Prototypes */
Pixel GetColor(char *name);
Pixel GetHilite(Pixel background);
Pixel GetShadow(Pixel background);
void nocolor(char *a, char *b);

