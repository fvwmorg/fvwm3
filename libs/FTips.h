/* -*-c-*- */
/* Copyright (C) 2004  Olivier Chapuis */

#ifndef FVWMLIB_FTIPS_H
#define FVWMLIB_FTIPS_H

/* ---------------------------- included header files ---------------------- */

/* ---------------------------- global definitions ------------------------- */

/* ---------------------------- global macros ------------------------------ */

/* ---------------------------- type definitions --------------------------- */

typedef enum
{
	FTIPS_PLACEMENT_UP,
	FTIPS_PLACEMENT_DOWN,
	FTIPS_PLACEMENT_LEFT,
	FTIPS_PLACEMENT_RIGHT,
	FTIPS_PLACEMENT_AUTO_UPDOWN,
	FTIPS_PLACEMENT_AUTO_LEFTRIGHT
} ftips_placement_t;

typedef enum
{
	FTIPS_JUSTIFICATION_CENTER,
	FTIPS_JUSTIFICATION_LEFT_UP,
	FTIPS_JUSTIFICATION_RIGHT_DOWN
} ftips_position_t;

typedef struct
{
	int colorset;
	Pixel fg;
	Pixel bg;
	Pixel border_pixel;
	int border_width;
	FlocaleFont *Ffont;
	ftips_placement_t placement;
	ftips_position_t justification;
	unsigned int placement_offset; /* pixel */
	unsigned int justification_offset; /* pixel */
	unsigned long delay; /* ms */
	unsigned long mapped_delay; /* ms */
} ftips_config;

/* ---------------------------- exported variables (globals) --------------- */

/* ---------------------------- interface functions ------------------------ */

Bool FTipsInit(Display *dpy);

ftips_config *FTipsNewConfig(void);

void FTipsOn(
	Display *dpy, Window win_f, ftips_config *fc, void *id, char *str,
	int x, int y, int w, int h);

void FTipsCancel(Display *dpy);

unsigned long FTipsCheck(Display *dpy);

Bool FTipsExpose(Display *dpy, XEvent *ev);

Bool FTipsHandleEvents(Display *dpy, XEvent *ev);

void FTipsUpdateLabel(Display *dpy, char *str);

void FTipsColorsetChanged(Display *dpy, int cs);

#endif
