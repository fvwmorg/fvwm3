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
     buf = (char *)realloc(buf, N);\
   }\
   buf[dn++] = (chr);\
 }

/* do var substitution for command string */
char * ParseCommand (int dn, char *sp, char end, int *dn1, char **sp1)
{
  static char var[256];
  char c, x, *wp, *cp, *vp;
  int j, dn2;
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
    } if (c == '\\') {  /* escape char */
      AddChar('\\');
      AddChar(*(sp++));
      goto next_loop;
    }
    if (c == '$') {  /* variable */
      if (*sp != '(')
	goto normal_char;
      wp = ++sp;
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
		if (*cp == '\"' || *cp == '\'' || *cp == '\\')
		  AddChar('\\');
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
	    AddChar(' ');
	    for (j = 0; j < item->selection.n; j++) {
	      if (item->selection.choices[j]->choice.on) {
		for (cp = item->selection.choices[j]->choice.value;
		     *cp != '\0'; cp++)
		  AddChar(*cp);
		AddChar(' ');
	      }
	    }
	    break;
	  }
	  goto next_loop;
	}
      }
      goto next_loop;
    }
  normal_char:
    AddChar(c);
  next_loop:
      ;
  }
}
