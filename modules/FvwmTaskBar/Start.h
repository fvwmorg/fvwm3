/*
 * Start.c exported functions
 */

#ifndef START_H
#define START_H

extern void StartButtonParseConfig(char *tline, char *Module);

extern void StartButtonInit(int height);
extern int StartButtonUpdate(char *title, int state);
extern void StartButtonDraw(int force);
extern int  MouseInStartButton(int x, int y);

#endif
