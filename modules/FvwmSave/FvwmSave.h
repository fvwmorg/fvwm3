#include "fvwmlib.h"       

#define STICKY 1
#define ICONIFIED     32 /* is it an icon now? */

struct list
{
  unsigned long id;
  int frame_height;
  int frame_width;
  int base_width;
  int base_height;
  int width_inc;
  int height_inc;
  int frame_x;
  int frame_y;
  int title_height;
  int boundary_width;
  unsigned long flags;
  unsigned long gravity;
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
void add_window(unsigned long new_win, unsigned long *body);
void DeadPipe(int nonsense);
void process_message(unsigned long type,unsigned long *body);
void do_save(void);
void list_new_page(unsigned long *body);

