/* -*-c-*- */

#ifndef FVWM_MISC_H
#define FVWM_MISC_H

#include "fvwm.h"

/* ---------------------------- included header files ---------------------- */

/* ---------------------------- global definitions ------------------------- */

enum
{
	GRAB_ALL      = 0,       /* sum of all grabs */
	GRAB_STARTUP  = 1,       /* Startup busy cursor */
	GRAB_NORMAL   = 2,       /* DeferExecution, Move, Resize, ... */
	GRAB_MENU     = 3,       /* a menus.c grabing */
	GRAB_BUSY     = 4,       /* BusyCursor stuff */
	GRAB_BUSYMENU = 5,       /* Allows menus.c to regrab the cursor */
	GRAB_PASSIVE  = 6,       /* Override of passive grab, only prevents grab
			          * to be released too early */
	GRAB_FREEZE_CURSOR = 7,  /* Freeze the cursor shape if a window is
				  * pressed. */
	GRAB_MAXVAL              /* last GRAB macro + 1 */
};

/* ---------------------------- global macros ------------------------------ */

/* ---------------------------- type definitions --------------------------- */

typedef enum
{
	ADDED_NONE = 0,
	ADDED_MENU,
	ADDED_DECOR,
	ADDED_FUNCTION
} last_added_item_t;

/* ---------------------------- forward declarations ----------------------- */

/* ---------------------------- exported variables (globals) --------------- */

/* ---------------------------- interface functions ------------------------ */

Bool GrabEm(
	int cursor, int grab_context);
Bool UngrabEm(
	int ungrab_context);
int GetTwoArguments(
	char *action, int *val1, int *val2, int *val1_unit, int *val2_unit);
void NewFontAndColor(
	FlocaleFont *flf, Pixel color, Pixel backcolor);
void Keyboard_shortcuts(
	XEvent *ev, FvwmWindow *fw, int *x_defect, int *y_defect,
	int ReturnEvent);
Bool check_if_fvwm_window_exists(
	FvwmWindow *fw);
int truncate_to_multiple(
	int x, int m);
Bool IsRectangleOnThisPage(struct monitor *, const rectangle *rec, int desk);
FvwmWindow *get_pointer_fvwm_window(void);
Time get_server_time(void);
void fvwm_msg_report_app(void);
void fvwm_msg_report_app_and_workers(void);
void set_last_added_item(last_added_item_t type, void *item);
void print_g(char *text, rectangle *g);

#endif /* FVWM_MISC_H */
