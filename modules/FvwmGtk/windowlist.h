#ifndef _WINDOWLIST_H
#define _WINDOWLIST_H


typedef struct {
  unsigned int no_geometry : 1;
  unsigned int no_mini_icon : 1;
  unsigned int use_icon_name : 1;
  unsigned int current_desk : 1;
  unsigned int one_desk : 1;
  unsigned int omit_iconified : 1;
  unsigned int omit_sticky : 1;
  unsigned int omit_normal : 1;
  int desk;
} window_list_options;


typedef struct {
  unsigned long w;
  char *name;
  char *icon_name;
  char *mini_icon;
  int desk;
  int layer;
  Bool iconified;
  Bool sticky;
  Bool skip;
  int x, y, width, height;
} window_list_entry;


window_list_entry *lookup_window_list_entry (unsigned long w);
void window_list (int argc, char **argv);
void construct_window_list (void);


#endif /* _WINDOWLIST_H */
