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

/*
 * Goodies.c structures and exported functions
 */

#ifndef _H_Goodies
#define _H_Goodies

#ifdef __FreeBSD__
#define DEFAULT_MAIL_PATH  "/var/mail/"
#else
#define DEFAULT_MAIL_PATH  "/var/spool/mail/"
#endif
#define DEFAULT_BELL_VOLUME 20

/* Tip window types */
#define NO_TIP    (-1)
#define DATE_TIP  (-2)
#define MAIL_TIP  (-3)
#define START_TIP (-4)

typedef struct {
  int  x, y, w, h, tw, th, open, type;
  char *text;
  Window win;
} TipStruct;

Bool GoodiesParseConfig(char *tline);
void LoadGoodiesFont(void);
void InitGoodies(void);
void DrawGoodies(void);
int MouseInClock(int x, int y);
int MouseInMail(int x, int y);
void CreateDateWindow(void);
void CreateMailTipWindow(void);
void PopupTipWindow(int px, int py, const char *text);
void CreateTipWindow(int x, int y, int w, int h);
void RedrawTipWindow(void);
void DestroyTipWindow(void);
void ShowTipWindow(int open);
void HandleMouseClick(XEvent event);
void HandleMailClick(XEvent event);
Bool change_goody_colorset(int cset, Bool force);

#endif
