/* File: session.h
 *
 * Description:
 *       exports from session.c shall go into this file
 *
 * Created:
 *       4 April 1999 - Steve Robbins <steve@nyongwa.montreal.qc.ca>
 */

#ifndef SESSION_H
#define SESSION_H


/*
**  Load and save the state of FVWM into a file.
*/
void LoadGlobalState(char *filename);
int SaveGlobalState(FILE *f);


/*
**  Load and save window states.  I don't know how this differs from 
** the global state.
*/
void LoadWindowStates(char *filename);
int SaveWindowStates(FILE *f);



#endif
