/* -*-c-*- */
/* Copyright (C) 2002  fvwm workers
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

#ifndef FSHM_H
#define FSHM_H

/* ---------------------------- included header files ----------------------- */

#include "config.h"

#ifdef HAVE_XSHM
#define XShmSupport 1
#else
#define XShmSupport 0
#endif

#if XShmSupport
#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/extensions/XShm.h>
#endif

/* ---------------------------- global definitions -------------------------- */

/* ---------------------------- global macros ------------------------------- */

/* ---------------------------- type definitions ---------------------------- */

#if XShmSupport

/* XShm */
typedef ShmSeg FShmSeg;
typedef XShmSegmentInfo FShmSegmentInfo;

#define FShmAttach XShmAttach
#define FShmDetach XShmDetach
#define FShmPutImage XShmPutImage
#define FShmGetImage XShmGetImage
#define FShmCreateImage XShmCreateImage

/* shm */
#define Fshmget shmget
#define Fshmat shmat
#define Fshmdt shmdt
#define Fshmctl shmctl
#define Fshmget shmget

#else

/* XShm */
typedef unsigned long FhmSeg;
typedef struct {
	FhmSeg shmseg;
	int shmid;
	char *shmaddr;
	Bool readOnly;
} FShmSegmentInfo;

#define FShmAttach(a, b)                              0
#define FShmDetach(a, b)                              0
#define FShmPutImage(a, b, c, d, e, f, g, h, i, j, k) 0
#define FShmGetImage(a, b, c, d, e, f, g)             0
#define FShmCreateImage(a, b, c, d, e, f, g, h)       NULL

/* shm */
#define Fshmget(a, b, c) 0
#define Fshmat(a, b, c)  NULL;
#define Fshmdt(a)
#define Fshmctl(a, b, c) 

#endif

#endif /* FSHM_H */
