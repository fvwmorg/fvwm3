#include "config.h"
#include "fvwmlib.h"

/*************************************************************************
 *
 * Subroutine Prototypes
 * 
 *************************************************************************/
void Loop(int *fd);
void SendInfo(int *fd,char *message,unsigned long window);
char *safemalloc(int length);
void DeadPipe(int nonsense);
void process_message(unsigned long type,unsigned long *body);

void list_add(unsigned long *body);
void list_configure(unsigned long *body);
void list_destroy(unsigned long *body);
void list_focus(unsigned long *body);
void list_toggle(unsigned long *body);
void list_new_page(unsigned long *body);
void list_new_desk(unsigned long *body);
void list_raise(unsigned long *body);
void list_lower(unsigned long *body);
void list_unknown(unsigned long *body);
void list_iconify(unsigned long *body);
void list_icon_loc(unsigned long *body);
void list_deiconify(unsigned long *body);
void list_map(unsigned long *body);
void list_window_name(unsigned long *body);
void list_icon_name(unsigned long *body);
void list_class(unsigned long *body);
void list_res_name(unsigned long *body);
void list_end(void);


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
