#include "../../libs/fvwmlib.h"     
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



#ifdef BROKEN_SUN_HEADERS
#include "../../fvwm/sun_headers.h"
#endif

#ifdef HAVE_WAITPID
#define ReapChildrenPid(pid) waitpid(pid, NULL, WNOHANG)
#else
#define ReapChildrenPid(pid) wait4(pid, NULL, WNOHANG, NULL)
#endif

#ifdef sony_news
/* XXX: no pid_t defines */
typedef int pid_t;
/* XXX: emulate setvbuf() */
#define setvbuf(a,b,c,d) setlinebuf(a)
#endif
