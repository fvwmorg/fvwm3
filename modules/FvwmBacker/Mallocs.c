/* FvwmWinList Module for Fvwm. 
 *
 *  Copyright 1994,  Mike Finger (mfinger@mermaid.micro.umn.edu or
 *                               Mike_Finger@atk.com)
 *
 * The author makes not guarantees or warantees, either express or
 * implied.  Feel free to use any contained here for any purpose, as long
 * and this and any other applicible copyrights are kept intact.

 * The functions in this source file that are based on part of the FvwmIdent
 * module for Fvwm are noted by a small copyright atop that function, all others
 * are copyrighted by Mike Finger.  For those functions modified/used, here is
 * the full, original copyright:
 *
 * Copyright 1994, Robert Nation and Nobutaka Suzuki.
 * No guarantees or warantees or anything
 * are provided or implied in any way whatsoever. Use this program at your
 * own risk. Permission to use this program for any purpose is given,
 * as long as the copyright is kept intact. */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <sys/time.h>
#include "../../libs/fvwmlib.h"

extern char *Module;

/******************************************************************************
  saferealloc - safely reallocate memory or exit if fails. (Doesn't work right)
******************************************************************************/
char *saferealloc(char *ptr, int length)
{
char *newptr;

  if(length <=0) length=1;

  newptr=realloc(ptr,length);
    if (ptr == (char *)0) {
      fprintf(stderr,"%s:realloc failed",Module);
      exit(1);
    }
  return ptr;
}

void UpdateString(char **string,char *value)
{
  if (value==NULL) return;
  if (*string==NULL) *string=safemalloc(strlen(value)+1);
  else *string=(char *)realloc(*string,strlen(value)+1);
  strcpy(*string,value);
}

