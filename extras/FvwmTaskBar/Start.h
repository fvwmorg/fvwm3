/*
 * Start.c exported functions
 */

#ifndef _H_Start
#define _H_Start

void StartButtonInit(int height);
void StartButtonUpdate(char *title, int state);
void StartButtonDraw(int force);
int  MouseInStartButton(int x, int y);

#endif
