#ifndef MODULE_H
#define MODULE_H

struct queue_buff_struct
{
  struct queue_buff_struct *next;
  unsigned long *data;
  int size;
  int done;
};

extern int npipes;
extern int *readPipes;
extern int *writePipes;
extern struct queue_buff_struct **pipeQueue;

#define START_FLAG 0xffffffff

#define M_NEW_PAGE           (1)
#define M_NEW_DESK           (1<<1)
#define M_ADD_WINDOW         (1<<2)
#define M_RAISE_WINDOW       (1<<3)
#define M_LOWER_WINDOW       (1<<4)
#define M_CONFIGURE_WINDOW   (1<<5)
#define M_FOCUS_CHANGE       (1<<6)
#define M_DESTROY_WINDOW     (1<<7)
#define M_ICONIFY            (1<<8)
#define M_DEICONIFY          (1<<9)
#define M_WINDOW_NAME        (1<<10)
#define M_ICON_NAME          (1<<11)
#define M_RES_CLASS          (1<<12)
#define M_RES_NAME           (1<<13)
#define M_END_WINDOWLIST     (1<<14)
#define M_ICON_LOCATION      (1<<15)
#define M_MAP                (1<<16)
#define M_ERROR              (1<<17)
#define M_CONFIG_INFO        (1<<18)
#define M_END_CONFIG_INFO    (1<<19)
#define M_ICON_FILE          (1<<20)
#define M_DEFAULTICON        (1<<21)
#define M_STRING             (1<<22)
#define M_MINI_ICON          (1<<23)
#define M_WINDOWSHADE        (1<<24)
#define M_DEWINDOWSHADE      (1<<25)
#define MAX_MESSAGES         26
#define MAX_MASK             ((1<<MAX_MESSAGES)-1)

#define HEADER_SIZE         4
#define MAX_BODY_SIZE      (24)
#define MAX_PACKET_SIZE    (HEADER_SIZE+MAX_BODY_SIZE)

void KillModuleByName(char *name);
void AddToModList(char *tline);

#endif /* MODULE_H */



