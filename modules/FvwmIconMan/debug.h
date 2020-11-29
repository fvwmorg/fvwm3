/* -*-c-*- */
#ifndef FVWMICONMAN_DEBUG_H
#define FVWMICONMAN_DEBUG_H

#if 0
# define OUTPUT_FILE "/dev/console"
/* # define OUTPUT_FILE "/tmp/FvwmIconMan" */
#else
# define OUTPUT_FILE NULL
#endif

extern int OpenConsole(const char *filenm);
extern void ConsoleMessage(const char *fmt, ...)
	__attribute__ ((__format__ (__printf__, 1, 2)));
extern void ConsoleDebug(int flag, const char *fmt, ...)
	__attribute__ ((__format__ (__printf__, 2, 3)));

extern int CORE, FUNCTIONS, X11, FVWM, CONFIG, WINLIST;

#endif /* FVWMICONMAN_DEBUG_H */
