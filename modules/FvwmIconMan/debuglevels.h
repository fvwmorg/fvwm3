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

/* int CORE = 1; */
/* int FUNCTIONS = 1; */
/* int X11 = 1; */
/* int FVWM = 1; */
/* int CONFIG = 1; */
/* int WINLIST = 1; */
/* int MEM = 1; */

/* I'm finding lots of the debugging is dereferencing pointers
   to zero.  I fixed some of them, until I grew tired of the game.
   If you want to turn these back on, be perpared for lots of core
   dumps.  dje 11/15/98. */
int CORE = 0;
int FUNCTIONS = 0;
int X11 = 0;
int FVWM = 0;
int CONFIG = 0;
int WINLIST = 0;
int MEM = 0;
