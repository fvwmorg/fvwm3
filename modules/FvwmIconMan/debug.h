#ifndef IN_DEBUG_H
#define IN_DEBUG_H

#if 0
#define PRINT_DEBUG      
#endif

#if 1
#define OUTPUT_FILE "/dev/console"
#else
#define OUTPUT_FILE "/usr/bradym/log"
#endif


#if !defined (PRINT_DEBUG) && defined (__GNUC__)
#define ConsoleDebug(flag, fmt, args...)
#else
extern void ConsoleDebug(int flag, char *fmt, ...);
#endif

extern int OpenConsole (void);

#define DEBUG_ALWAYS 1

extern int CORE, FUNCTIONS, X11, FVWM, CONFIG, WINLIST, MEM;

#endif
