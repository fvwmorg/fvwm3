#include "fvwmlib.h"     

#define ACTION1 1
#define ACTION2 2
#define ACTION3 4

struct list
{
  unsigned long id;
  unsigned long last_focus_time;
  unsigned long actions;
  struct list *next;
};

/*************************************************************************
 *
 * Subroutine Prototypes
 * 
 *************************************************************************/
void Loop(int *fd);
void SendInfo(int *fd,char *message,unsigned long window);
char *safemalloc(int length);
struct list *find_window(unsigned long id);
void remove_window(unsigned long id);
void add_window(unsigned long new_win);
void update_focus(struct list *l, unsigned long);
void DeadPipe(int nonsense);
void find_next_event_time(void);
void process_message(unsigned long type,unsigned long *body);

