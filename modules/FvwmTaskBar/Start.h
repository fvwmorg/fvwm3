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
 * Start.c exported functions
 */

#ifndef START_H
#define START_H

extern Bool StartButtonParseConfig(char *tline);
extern void StartButtonInit(int height);
extern int StartButtonUpdate(const char *title, int state);
extern void StartButtonDraw(int force);
extern int  MouseInStartButton(int x, int y);

#endif
