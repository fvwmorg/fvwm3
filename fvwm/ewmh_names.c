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

#include "libs/fvwmlib.h"
#include "libs/Flocale.h"
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

#define UTF8_CHARSET_NAME "UTF-8"
#define CONVERSION_MAX_NUMBER_OF_WARNING 10

extern ScreenInfo Scr;
extern char *Fcharset;

/***********************************************************************
 * conversion between charsets
 ***********************************************************************/
static
char *convert_charsets(const char *in_charset, const char *out_charset,
		       const unsigned char *in, unsigned int in_size)
{
  static int error_count = 0;
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
    if (error_count > CONVERSION_MAX_NUMBER_OF_WARNING)
      return NULL;
      error_count++;
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

  /* in maybe a none terminate string */
  len = in_size;

  outbuf_size = len + 1;
  outbytes_remaining = outbuf_size - 1;
  insize = len;
  outp = dest = safemalloc(outbuf_size);

  inptr = in;

  for (is_finished = 0; is_finished == 0; )
  {
#ifdef ICONV_ARG_USE_CONST
    nconv =
      iconv(cd, (const char **)&inptr, &insize, &outp, &outbytes_remaining);
#else
    nconv =
      iconv(cd, (char **)&inptr,&insize, &outp, &outbytes_remaining);
#endif
    is_finished = 1;
    if (nconv == (size_t) - 1)
    {
      switch (errno)
      {
      case EINVAL:
        /* Incomplete text, do not report an error */
        break;
      case E2BIG:
      {
        size_t used = outp - dest;

        outbuf_size *= 2;
        dest = realloc (dest, outbuf_size);

        outp = dest + used;
        outbytes_remaining = outbuf_size - used - 1; /* -1 for nul */
	is_finished = 0;
        break;
      }
      case EILSEQ:
	    /* Something went wrong.  */
	if (error_count <= CONVERSION_MAX_NUMBER_OF_WARNING)
	  fvwm_msg(ERR, "convert_charsets",
		   "Invalid byte sequence during conversion from %s to %s\n",
		   in_charset,out_charset);
        have_error = 1;
	break;
      default:
	if (error_count <= CONVERSION_MAX_NUMBER_OF_WARNING)
	  fvwm_msg(ERR, "convert_charsets",
		   "Error during conversion from %s to %s\n",
		   in_charset,out_charset);
        have_error = 1;
        break;
      }
    }
  }


  /* Terminate the output string  */
  *outp = '\0';

  if (iconv_close (cd) != 0)
    fvwm_msg(ERR, "convert_charsets","iconv_close fail\n",
	     in_charset,out_charset);

  if (have_error)
  {
    error_count++;
    free (dest);
    return NULL;
  }
  else
    return dest;
}

/***********************************************************************
 * conversion from UTF8 to the current charset
 ***********************************************************************/
static
char *utf8_to_charset(const char *in, unsigned int in_size)
{
  char *out = NULL;
  const char *charset;

  charset = FlocaleGetCharset();
  if (charset != NULL)
    out = convert_charsets(UTF8_CHARSET_NAME, charset, in, in_size);

  return out;
}

/***********************************************************************
 * conversion from the current charset to UTF8
 ***********************************************************************/
static
char *charset_to_utf8(const char *in, unsigned int in_size)
{
  char *out = NULL;
  const char *charset;

  charset = FlocaleGetCharset();
  if (charset != NULL)
    out = convert_charsets(charset,UTF8_CHARSET_NAME, in, in_size);

  return out;
}

/***********************************************************************
 * set the visibale window name and icon name
 ***********************************************************************/
void EWMH_SetVisibleName(FvwmWindow *fwin, Bool is_icon_name)
{
  unsigned char *val;
  char *tmp_str;

  /* set the ewmh visible name only if it is != wm name */
  if (is_icon_name)
  {
    if ((fwin->icon_name_count == 0 || !USE_INDEXED_ICON_NAME(fwin)) &&
	!HAS_EWMH_WM_ICON_NAME(fwin) && !HAS_EWMH_WM_NAME(fwin))
    {
      ewmh_DeleteProperty(
	FW_W(fwin), "_NET_WM_ICON_VISIBLE_NAME", EWMH_ATOM_LIST_FVWM_WIN);
      return;
    }
    tmp_str = fwin->visible_icon_name;
  }
  else
  {
    if ((fwin->name_count == 0 || !USE_INDEXED_WINDOW_NAME(fwin)) &&
	!HAS_EWMH_WM_NAME(fwin) && !HAS_EWMH_WM_ICON_NAME(fwin))
    {
      ewmh_DeleteProperty(
	FW_W(fwin), "_NET_WM_VISIBLE_NAME", EWMH_ATOM_LIST_FVWM_WIN);
      return;
    }
    tmp_str = fwin->visible_name;
  }

  if (tmp_str == NULL)
    return; /* should never happen */

  val = (unsigned char *)charset_to_utf8(tmp_str, strlen(tmp_str));

  if (val == NULL)
    return;

  if (is_icon_name)
  {
    ewmh_ChangeProperty(FW_W(fwin), "_NET_WM_ICON_VISIBLE_NAME",
			EWMH_ATOM_LIST_FVWM_WIN,
			(unsigned char *)val, strlen(val));
  }
  else
  {
    ewmh_ChangeProperty(FW_W(fwin), "_NET_WM_VISIBLE_NAME",
			EWMH_ATOM_LIST_FVWM_WIN,
			(unsigned char *)val, strlen(val));
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

  val = ewmh_AtomGetByName(FW_W(fwin), "_NET_WM_ICON_NAME",
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
  {
    /* client message */
    free_window_names(fwin, False, True);
  }

  fwin->icon_name.name = tmp_str;

  SET_HAS_EWMH_WM_ICON_NAME(fwin, 1);
  SET_WAS_ICON_NAME_PROVIDED(fwin, 1);

  if (ev == NULL)
  {
    /* return now for setup */
    return 1;
  }

  if (fwin->icon_name.name && strlen(fwin->icon_name.name) > MAX_ICON_NAME_LEN)
    (fwin->icon_name.name)[MAX_ICON_NAME_LEN] = 0;

  setup_visible_name(fwin, True);
  EWMH_SetVisibleName(fwin, True);
  BroadcastWindowIconNames(fwin, False, True);
  RedoIconName(fwin);
  return 1;
}

int EWMH_WMName(EWMH_CMD_ARGS)
{
  unsigned int size = 0;
  CARD32 *val;
  char *tmp_str;

  val = ewmh_AtomGetByName(FW_W(fwin), "_NET_WM_NAME",
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

  fwin->name.name = tmp_str;
  SET_HAS_EWMH_WM_NAME(fwin, 1);

  if (ev == NULL)
  {
    return 1;
  }

  if (fwin->name.name && strlen(fwin->name.name) > MAX_WINDOW_NAME_LEN)
      (fwin->name.name[MAX_WINDOW_NAME_LEN]) = 0;

  setup_visible_name(fwin, False);
  SET_NAME_CHANGED(fwin, 1);

  EWMH_SetVisibleName(fwin, False);
  BroadcastWindowIconNames(fwin, True, False);

  /* fix the name in the title bar */
  if(!IS_ICONIFIED(fwin))
    DrawDecorations(fwin, PART_TITLE, (Scr.Hilite == fwin),
		    True, None, CLEAR_ALL);

  if (!WAS_ICON_NAME_PROVIDED(fwin))
  {
    fwin->icon_name = fwin->name;
    setup_visible_name(fwin, True);
    BroadcastWindowIconNames(fwin, False, True);
    EWMH_SetVisibleName(fwin, True);
    RedoIconName(fwin);
  }
  return 0;
}

#define MAX(A,B) ((A)>(B)? (A):(B))
/***********************************************************************
 * set the desktop name
 ***********************************************************************/
void EWMH_SetDesktopNames(void)
{
  int nbr = 0;
  int len = 0;
  int i;
  int j = 0;
  DesktopsInfo *d,*s;
  unsigned char **names;
  unsigned char *val;

  d = Scr.Desktops->next;
  /* skip negative desk */
  while (d != NULL && d->desk < 0)
    d = d->next;
  s = d;
  while (d != NULL && d->name != NULL && d->desk == nbr)
  {
    nbr++;
    len += strlen(d->name) + 1;
    d = d->next;
  }
  if (nbr == 0)
    return;

  val = (unsigned char *)safemalloc(len);
  names = (void *)safemalloc(sizeof(*names)*nbr);
  for (i = 0; i < nbr; i++)
  {
    names[i] =
	(unsigned char *)charset_to_utf8(s->name, strlen(s->name));
    s = s->next;
  }
  for (i = 0; i < nbr; i++)
  {
    memcpy(&val[j], names[i], strlen(names[i])+1);
    j += strlen(names[i]) + 1;
    free(names[i]);
  }
  free(names);
  ewmh_ChangeProperty(Scr.Root, "_NET_DESKTOP_NAMES",
		      EWMH_ATOM_LIST_CLIENT_ROOT,
		      (unsigned char *)val, len);
  free(val);
}
#endif /* defined(HAVE_EWMH) && defined(HAVE_ICONV) */
