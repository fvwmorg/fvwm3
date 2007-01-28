/* -*-c-*- */
#include "config.h"
#include "libs/fvwmlib.h"

/*
 *
 * Subroutine Prototypes
 *
 */
void Loop(const int *fd);
RETSIGTYPE DeadPipe(int nonsense) __attribute__((noreturn));
void process_message(unsigned long type, const unsigned long *body);

void list_old_configure(const unsigned long *body);
void list_new_page(const unsigned long *body);
void list_new_desk(const unsigned long *body);
void list_focus(const unsigned long *body);
void list_winid(const unsigned long *body);
void list_unknown(const unsigned long *body);
void list_icon(const unsigned long *body);
void list_window_name(const unsigned long *body);
void list_restack(const unsigned long *body);
void list_error(const unsigned long *body);
void list_configure(const unsigned long *body);


#ifdef HAVE_WAITPID
#  define ReapChildrenPid(pid) waitpid(pid, NULL, WNOHANG)
#elif HAVE_WAIT4
#  define ReapChildrenPid(pid) wait4(pid, NULL, WNOHANG, NULL)
#else
#  error One of waitpid or wait4 is needed.
#endif

#if HAVE_SETVBUF
#  if SETVBUF_REVERSED
#    define setvbuf(stream,buf,mode,size) setvbuf(stream,mode,buf,size)
#  endif
#else
#  define setvbuf(a,b,c,d) setlinebuf(a)
#endif

