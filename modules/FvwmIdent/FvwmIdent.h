/* -*-c-*- */
#include "libs/fvwmlib.h"
#include <fvwm/fvwm.h>
#include <libs/vpacket.h>

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
  long layer;
  unsigned long gravity;
  window_flags flags;
  long title_h;
  long border_w;
  long ewmh_hint_layer;
  unsigned long ewmh_hint_desktop;
  long ewmh_window_type;
};

struct Item
{
  char* col1;
  char* col2;
  struct Item* next;
};

/*
 *
 * Subroutine Prototypes
 *
 */
void Loop(int *fd);
RETSIGTYPE DeadPipe(int nonsense) __attribute__((noreturn));
void process_message(unsigned long type, unsigned long *body);
void PixmapDrawWindow(int h, int w);
void DrawItems(Drawable d, int x, int y, int w, int h);
void change_window_name(char *str);
Pixel GetColor(char *name);
void nocolor(char *a, char *b);
void DestroyList(void);
void AddToList(char *, char *);
void MakeList(void);

void change_defaults(char *body);
void list_configure(unsigned long *body);
void list_window_name(unsigned long *body);
void list_icon_name(unsigned long *body);
void list_class(unsigned long *body);
void list_res_name(unsigned long *body);
void list_property_change(unsigned long *body);
void list_end(void);

static int ErrorHandler(Display *d, XErrorEvent *event);
