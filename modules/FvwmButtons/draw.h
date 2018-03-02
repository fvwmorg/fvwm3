/* -*-c-*- */
/*
 * FvwmButtons, copyright 1996, Jarl Totland
 *
 * This module, and the entire GoodStuff program, and the concept for
 * interfacing this module to the Window Manager, are all original work
 * by Robert Nation
 */

#define DRAW_RELIEF      0
#define DRAW_CLEAN       1
#define DRAW_ALL         2
#define DRAW_FORCE       3
#define DRAW_DESK_RELIEF 4

void RelieveButton(Window, int, int, int, int, int, Pixel, Pixel, int);
void MakeButton(button_info *);
void RedrawButton(button_info *, int draw, XEvent *pev);
void DrawTitle(button_info *b, Window win, GC gc, XEvent *pev);

