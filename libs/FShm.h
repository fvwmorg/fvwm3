/* -*-c-*- */

#ifndef FVWMLIB_FSHM_H
#define FVWMLIB_FSHM_H

/* ---------------------------- included header files ---------------------- */

#include "config.h"
#include "fvwm_x11.h"

/* ---------------------------- global definitions ------------------------- */

/* ---------------------------- global macros ------------------------------ */

/* ---------------------------- type definitions --------------------------- */

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
#define FShmGetImage(a, b, c, d, e, f)                0
#define FShmCreateImage(a, b, c, d, e, f, g, h)       NULL

/* shm */
#define Fshmget(a, b, c) 0
#define Fshmat(a, b, c)  NULL
#define Fshmdt(a)
#define Fshmctl(a, b, c)

#endif

#endif /* FVWMLIB_FSHM_H */
