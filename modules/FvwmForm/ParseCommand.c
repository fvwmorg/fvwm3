/* -*-c-*- */
/*
 * ParseComand.c was originally part of FvwmForm.c.
 *
 * Turned into separate file 02/27/99, Dan Espen.
 *
 * FvwmForm is original work of Thomas Zuwei Feng.
 *
 * Copyright Feb 1995, Thomas Zuwei Feng.  No guarantees or warantees are
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
#include "libs/fvwmlib.h"
#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <FvwmForm.h>                   /* common FvwmForm stuff */

static char *buf;
static int N = 8;

/* Macro used in following functions */
#define AddChar(chr) \
 { if (dn >= N) {\
     N *= 2;\
     buf = xrealloc(buf, N, sizeof(buf));\
   }\
   buf[dn++] = (chr);\
 }

/* do var substitution for command string */
char * ParseCommand (int dn, char *sp, char end, int *dn1, char **sp1)
{
  static char var[256];
  char c, x, *cp, *vp;
  int j, dn2;
  int added_sel;
  Item *item;

  if (buf == 0) {                       /* if no buffer yet */
    buf = (char *)malloc(N);
  }
  while (1) {
    c = *(sp++);
    if (c == '\0' || c == end) {  /* end of substitution */
      *dn1 = dn;
      *sp1 = sp;
      if (end == 0) {                   /* if end of command reached */
	AddChar('\0');                  /* make sure theres a null */
      }
      return(buf);
    }
    if (c == '$') {  /* variable */
      if (*sp != '(')
	goto normal_char;
      ++sp;
      vp = var;
      while (1) {
	x = *(sp++);
	if (x == '\\') {
	  *(vp++) = '\\';
	  *(vp++) = *(sp++);
	}
	else if (x == ')' || x == '?' || x == '!') {
	  *(vp++) = '\0';
	  break;
	}
	else if (!isspace(x))
	  *(vp++) = x;
      }
      for (item = root_item_ptr; item != 0;
	   item = item->header.next) {/* all items */
	if (strcmp(var, item->header.name) == 0) {
	  switch (item->type) {
	  case I_INPUT:
	    if (x == ')') {
	      for (cp = item->input.value; *cp != '\0'; cp++) {
		AddChar(*cp);
	      }
	    } else {
	      ParseCommand(dn, sp, ')', &dn2, &sp);
	      if ((x == '?' && strlen(item->input.value) > 0) ||
		  (x == '!' && strlen(item->input.value) == 0))
		dn = dn2;
	    }
	    break;
	  case I_CHOICE:
	    if (x == ')') {
	      for (cp = item->choice.value; *cp != '\0'; cp++)
		AddChar(*cp);
	    } else {
	      ParseCommand(dn, sp, ')', &dn2, &sp);
	      if ((x == '?' && item->choice.on) ||
		  (x == '!' && !item->choice.on))
		dn = dn2;
	    }
	    break;
	  case I_SELECT:
	    if (x != ')')
	      ParseCommand(dn, sp, ')', &dn2, &sp);
	    added_sel=0;
	    for (j = 0; j < item->selection.n; j++) {
	      if (item->selection.choices[j]->choice.on) {
		if (added_sel) {        /* if not first sel added */
		  AddChar(' ');         /* insert space before next value */
		}
		added_sel=1;
		for (cp = item->selection.choices[j]->choice.value;
		     *cp != '\0'; cp++) {
		  AddChar(*cp);
		}
	      }
	    }
	    break;
	  }
	  goto next_loop;
	}
      }
      goto next_loop;
    } /* end char is $, not followed by ( */
    /* if char is \, followed by ), want to pass thru the paren literally */
    if (c == '\\' && *sp == ')') {
      c = *(sp++);                      /* skip to the paren */
    }
  normal_char:
    AddChar(c);
  next_loop:
      ;
  }
}
