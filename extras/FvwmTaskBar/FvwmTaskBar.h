/* FvwmTaskBar Module for Fvwm.
 *
 *  Copyright 1994,  Mike Finger (mfinger@mermaid.micro.umn.edu or
 *                               Mike_Finger@atk.com)
 *
 * The author makes not guarantees or warantees, either express or
 * implied.  Feel free to use any contained here for any purpose, as long
 * and this and any other applicible copyrights are kept intact.

 * The functions in this header file that are based on part of the FvwmIdent
 * module for Fvwm are noted by a small copyright atop that function, all others
 * are copyrighted by Mike Finger.  For those functions modified/used, here is
 * the full, original copyright:
 *
 * Copyright 1994, Robert Nation and Nobutaka Suzuki.
 * No guarantees or warantees or anything
 * are provided or implied in any way whatsoever. Use this program at your
 * own risk. Permission to use this program for any purpose is given,
 * as long as the copyright is kept intact. */

#ifndef _H_FvwmTaskBar
#define _H_FvwmTaskBar

#include "config.h"
#include "fvwm/fvwm.h"

#define F_SWALLOWED      1
#define F_NOT_SWALLOWED  2

/* Motif  window hints */
typedef struct
{
    CARD32      flags;
    CARD32      functions;
    CARD32      decorations;
    INT32       inputMode;
} PropMotifWmHints;

typedef PropMotifWmHints        PropMwmHints;

/* Motif window hints */
#define MWM_HINTS_FUNCTIONS     (1L << 0)
#define MWM_HINTS_DECORATIONS   (1L << 1)
#define MWM_HINTS_INPUT_MODE    (1L << 2)

/* bit definitions for MwmHints.functions */
#define MWM_FUNC_ALL            (1L << 0)
#define MWM_FUNC_RESIZE         (1L << 1)
#define MWM_FUNC_MOVE           (1L << 2)
#define MWM_FUNC_MINIMIZE       (1L << 3)
#define MWM_FUNC_MAXIMIZE       (1L << 4)
#define MWM_FUNC_CLOSE          (1L << 5)

/* values for MwmHints.input_mode */
#define MWM_INPUT_MODELESS                      0
#define MWM_INPUT_PRIMARY_APPLICATION_MODAL     1
#define MWM_INPUT_SYSTEM_MODAL                  2
#define MWM_INPUT_FULL_APPLICATION_MODAL        3

/* bit definitions for MwmHints.decorations */
#define MWM_DECOR_ALL                 (1L << 0)
#define MWM_DECOR_BORDER              (1L << 1)
#define MWM_DECOR_RESIZEH             (1L << 2)
#define MWM_DECOR_TITLE               (1L << 3)
#define MWM_DECOR_MENU                (1L << 4)
#define MWM_DECOR_MINIMIZE            (1L << 5)
#define MWM_DECOR_MAXIMIZE            (1L << 6)

#define PROP_MOTIF_WM_HINTS_ELEMENTS  4
#define PROP_MWM_HINTS_ELEMENTS       PROP_MOTIF_WM_HINTS_ELEMENTS

/* Alarm Events */
#define NOT_SET        0
#define SHOW_TIP       1
#define HIDE_TASK_BAR  2

/*************************************************************************
  Subroutine Prototypes
**************************************************************************/
void EndLessLoop(void);
void ReadFvwmPipe(void);
void ProcessMessage(unsigned long type,unsigned long *body);
void SendFvwmPipe(char *message,unsigned long window);
void DeadPipe(int nonsense);
void Alarm(int nonsense);
void SetAlarm(int event);
void ClearAlarm(void);
void CheckForTip(int x, int y);
void MakeMeWindow(void);
void WaitForExpose(void);
void RedrawWindow(int force);
void StartMeUp(void);
void ShutMeDown(int exitstat);
void ConsoleMessage(char *fmt,...);
int OpenConsole(void);
void ConsoleMessage(char *fmt,...);
static void ParseConfig();
void LoopOnEvents(void);
void AdjustWindow(int width, int height);
char *makename(char *string,long flags);
void ChangeWindowName(char *str);
void LinkAction(char *string);
void AddToSkipList(char *string);
int InSkipList(char *string);
void PrintSkipList(void);
void FvwmNameMessage(long *body);
void SetMwmHints(unsigned int value,unsigned int funcs,unsigned int input);
void ConstrainSize (XSizeHints *hints, int *widthp, int *heightp);
void RevealTaskBar(void);
void HideTaskBar(void);
void WarpTaskBar(int x);
void PurgeConfigEvents(void);

void Swallow(unsigned long *body);

XErrorHandler ErrorHandler(Display *d, XErrorEvent *event);

#endif
