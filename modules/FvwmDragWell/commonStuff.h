/* -*-c-*- */
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
