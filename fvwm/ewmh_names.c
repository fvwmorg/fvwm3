/* Copyright (C) 1999  Tom Tromey
 * Copyright (C) 2000  Red Hat, Inc.
 * Copyright (C) 2001  Olivier Chapuis
 *
 * This program is free software; you can redistribute it and/or modify
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

/* Some code (charset conversion) from the glib-2 (gutf8.c) copyrighted
 * by Tom Tromey & Red Hat, Inc. */

#include "config.h"

#if defined(HAVE_EWMH) && defined(HAVE_ICONV)
#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xmd.h>

#include <iconv.h>
#include <errno.h>
#ifdef HAVE_CODESET
#include <locale.h>
#include <langinfo.h>
#endif

#include "libs/fvwmlib.h"
#include "fvwm.h"
#include "window_flags.h"
#include "cursor.h"
#include "functions.h"
#include "misc.h"
#include "screen.h"
#include "module_interface.h"
#include "borders.h"
#include "add_window.h"
#include "icons.h"
#include "ewmh.h"
#include "ewmh_intern.h"

#if defined(USE_LIBICONV) && !defined (_LIBICONV_H)
#error libiconv in use but included iconv.h not from libiconv
#endif
#if !defined(USE_LIBICONV) && defined (_LIBICONV_H)
#error libiconv not in use but included iconv.h is from libiconv
#endif

extern ScreenInfo Scr;

static char *charset = NULL;
static Bool need_to_get_charset = 1;

/***********************************************************************
 * get the charset unsing nl_langinfo
 ***********************************************************************/
void get_charset(void)
{
  charset = getenv("CHARSET");

  if (charset)
      return;

#ifdef HAVE_CODESET
  /* Get the name of the current locale.  */
  if ((setlocale(LC_CTYPE, getenv("LC_CTYPE"))) == NULL)
      fprintf(stderr,
	      "[FVWM]: Can't set locale. Check your $LC_CTYPE or $LANG.\n");

  charset = nl_langinfo(CODESET);
#endif
}


/***********************************************************************
 * conversion between charsets
 ***********************************************************************/
char *convert_charsets(const char *in_charset, const char *out_charset,
		       const unsigned char *in, unsigned int in_size)
{
  iconv_t cd;
  int have_error = 0;
  int is_finished = 0;
  size_t nconv;
  size_t insize,outbuf_size,outbytes_remaining,len;
  const char *inptr;
  char *dest;
  char *outp;

  if (in == NULL)
    return NULL;

  cd = iconv_open (out_charset, in_charset);
  if (cd == (iconv_t) -1)
  {
    /* Something went wrong.  */
    if (errno == EINVAL)
      fvwm_msg(WARN, "convert_charsets",
	       "conversion from `%s' to `%s' not available\n",
	       in_charset,out_charset);
    else
      fvwm_msg(WARN, "convert_charsets",
	       "conversion from `%s' to `%s' fail (init)\n",
	       in_charset,out_charset);

    /* Terminate the output string.  */
    return NULL;
  }

  /* in maybe a none terminate string (thanks to kde2) */
  len = in_size;

  outbuf_size = len + 1;
  outbytes_remaining = outbuf_size - 1;
  insize = len;
  outp = dest = safemalloc(outbuf_size);

  inptr = in;

  for (is_finished = 0; is_finished == 0; )
  {
    nconv = iconv(cd,(char **)&inptr,&insize,&outp,&outbytes_remaining);
    if (nconv == (size_t) -1)
    {
      switch (errno)
      {
      case EINVAL:
        /* Incomplete text, do not report an error */
        is_finished = 1;
        break;
      case E2BIG:
      {
        size_t used = outp - dest;

        outbuf_size *= 2;
        dest = realloc (dest, outbuf_size);

        outp = dest + used;
        outbytes_remaining = outbuf_size - used - 1; /* -1 for nul */

        break;
      }
      default:
        fvwm_msg(ERR, "convert_charsets",
                 "Error during conversion from %s to %s\n",
                 in_charset,out_charset);
        have_error = 1;
        is_finished = 1;
        break;
      }
    }
  }


  /* Terminate the output string.  */
  *outp = '\0';

  if (iconv_close (cd) != 0)
    fvwm_msg(ERR, "convert_charsets","iconv_close fail\n",
	     in_charset,out_charset);

  if (have_error)
  {
    free (dest);
    return NULL;
  }
  else
    return dest;
}

/***********************************************************************
 * conversion from UTF8 to the current charset
 ***********************************************************************/
char *utf8_to_charset(const char *in, unsigned int in_size)
{
  char *out = NULL;
  if (need_to_get_charset)
    get_charset();
  need_to_get_charset = False;

  if (charset != NULL)
    out = convert_charsets("UTF8", charset, in, in_size);

  return out;
}

/***********************************************************************
 * conversion from the current charset to UTF8
 ***********************************************************************/
char *charset_to_utf8(const char *in, unsigned int in_size)
{
  char *out = NULL;
  if (need_to_get_charset)
    get_charset();
  need_to_get_charset = False;

  if (charset != NULL)
    out = convert_charsets(charset,"UTF8", in, in_size);

  return out;
}
/***********************************************************************
 * set the visibale window name and icon name
 ***********************************************************************/
void EWMH_SetVisibleName(FvwmWindow *fwin, Bool is_icon_name)
{
  unsigned char *val;
  char *tmp_str;

  if (is_icon_name)
  {
    tmp_str = fwin->icon_name;
  }
  else
  {
    tmp_str = fwin->name;
  }

  if (tmp_str == NULL)
    return;

  val = (unsigned char *)charset_to_utf8(tmp_str, strlen(tmp_str));

  if (val == NULL)
    return;

  if (is_icon_name)
  {
    ewmh_ChangeProperty(fwin->w, "_NET_WM_ICON_VISIBLE_NAME",
			EWMH_ATOM_LIST_FVWM_WIN,
			(unsigned char *)val, strlen(val) + 1);
  }
  else
  {
    ewmh_ChangeProperty(fwin->w, "_NET_WM_VISIBLE_NAME",
			EWMH_ATOM_LIST_FVWM_WIN,
			(unsigned char *)val, strlen(val) + 1);
  }
  free(val);
}

/***********************************************************************
 * setup and property notify
 ***********************************************************************/
int EWMH_WMIconName(EWMH_CMD_ARGS)
{
  unsigned int size = 0;
  CARD32 *val;
  char *tmp_str;

  val = ewmh_AtomGetByName(fwin->w, "_NET_WM_ICON_NAME",
			   EWMH_ATOM_LIST_PROPERTY_NOTIFY, &size);

  if (val == NULL)
  {
    SET_HAS_EWMH_WM_ICON_NAME(fwin,0);
    return 0;
  }

  tmp_str = (char *)utf8_to_charset((const char *) val, size);
  free(val);
  if (tmp_str == NULL)
  {
    SET_HAS_EWMH_WM_ICON_NAME(fwin,0);
    return 0;
  }

  if (ev != NULL)
    free_window_names(fwin, False, True);

  fwin->icon_name = tmp_str;

  if (ev == NULL)
  {
    /* return now for setup */
    SET_HAS_EWMH_WM_NAME(fwin, 1);
    return 1;
  }

  if (fwin->icon_name && strlen(fwin->icon_name) > MAX_ICON_NAME_LEN)
      fwin->icon_name[MAX_ICON_NAME_LEN] = 0;
  SET_WAS_ICON_NAME_PROVIDED(fwin, 1);

  EWMH_SetVisibleName(fwin, True);
  BroadcastName(M_ICON_NAME,fwin->w,fwin->frame,
		(unsigned long)fwin,fwin->icon_name);
  RedoIconName(fwin);
  return 1;
}

int EWMH_WMName(EWMH_CMD_ARGS)
{
  unsigned int size = 0;
  CARD32 *val;
  char *tmp_str;

  val = ewmh_AtomGetByName(fwin->w, "_NET_WM_NAME",
			   EWMH_ATOM_LIST_PROPERTY_NOTIFY, &size);

  if (val == NULL)
  {
    SET_HAS_EWMH_WM_NAME(fwin,0);
    return 0;
  }

  tmp_str = (char *)utf8_to_charset((const char *) val, size);
  free(val);
  if (tmp_str == NULL)
  {
    SET_HAS_EWMH_WM_NAME(fwin,0);
    return 0;
  }

  if (ev != NULL)
    free_window_names(fwin, True, False);

  fwin->name = tmp_str;

  if (ev == NULL)
  {
    SET_HAS_EWMH_WM_NAME(fwin, 1);
    return 1;
  }

  if (fwin->name && strlen(fwin->name) > MAX_WINDOW_NAME_LEN)
      fwin->icon_name[MAX_WINDOW_NAME_LEN] = 0;
  SET_NAME_CHANGED(fwin, 1);

  EWMH_SetVisibleName(fwin, False);
  BroadcastName(M_WINDOW_NAME,fwin->w,fwin->frame,
		(unsigned long)fwin,fwin->name);
  /* fix the name in the title bar */
  if(!IS_ICONIFIED(fwin))
    DrawDecorations(fwin, DRAW_TITLE, (Scr.Hilite == fwin), True, None);

  if (!WAS_ICON_NAME_PROVIDED(fwin))
  {
    fwin->icon_name = fwin->name;
    BroadcastName(M_ICON_NAME,fwin->w,fwin->frame,
		  (unsigned long)fwin,fwin->icon_name);
    EWMH_SetVisibleName(fwin, True);
    RedoIconName(fwin);
  }
  return 0;
}

#endif /* defined(HAVE_EWMH) && defined(HAVE_ICONV) */
