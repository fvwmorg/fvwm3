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

#ifndef _COMMONSTUFF_H
#define _COMMONSTUFF_H
/* atoms used by Xdnd, copied and modified from Paul Sheer's xdndv2 library.*/
typedef struct XdndAtomsStruct {
  Atom xdndAware;
  Atom xdndEnter;
  Atom xdndLeave;
  Atom xdndStatus;
  Atom xdndSelection;
  Atom xdndPosition;
  Atom xdndDrop;
  Atom xdndFinished;
  Atom xdndActionCopy;
  Atom xdndActionMove;
  Atom xdndActionLink;
  Atom xdndActionAsk;
  Atom xdndActionPrivate;
  Atom xdndTypeList;
  Atom xdndActionList;
  Atom xdndActionDescription;
} XdndAtoms;

#define XDND_SETBIT(datum,bit,val) ( ((datum) & (~(0x1UL<<bit))) | (val<<bit))
#define XDND_GETBIT(datum,bit) ((datum) & (0x1UL<<bit))

#endif
