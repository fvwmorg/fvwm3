#include "../../libs/fvwmlib.h"       
#define STICKY         (1<<2) /* Does window stick to glass? */
#define ONTOP          (1<<1) /* does window stay on top */
#define BORDER         (1<<13) /* Is this decorated with border*/
#define TITLE          (1<<14) /* Is this decorated with title */
#define ICONIFIED      (1<<16) /* is it an icon now? */
#define TRANSIENT      (1<<17) /* is it a transient window? */
struct target_struct
{
  char res[256];
  char class[256];
  char name[256];
  char icon_name[256];
  unsigned long id;
  unsigned long frame;
  long frame_x;
  long frame_y;
  long frame_w;
  long frame_h;
  long base_w;
  long base_h;
  long width_inc;
  long height_inc;
  long desktop;
  unsigned long gravity;
  unsigned long flags;
  long title_h;
  long border_w;
};

struct Item
{
  char* col1;
  char* col2;
  struct Item* next;
};

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
void GetTargetWindow(Window *app_win);
void RedrawWindow(void);
void change_window_name(char *str);
Pixel GetColor(char *name);
void nocolor(char *a, char *b);
void CopyString(char **dest, char *source);
char *CatString2(char *a, char *b);
void AddToList(char *, char *);
void MakeList(void);
void freelist(void);

void list_configure(unsigned long *body);
void list_window_name(unsigned long *body);
void list_icon_name(unsigned long *body);
void list_class(unsigned long *body);
void list_res_name(unsigned long *body);
void list_end(void);

