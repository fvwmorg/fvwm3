#ifndef _REPEAT_
#define _REPEAT_

#include "fvwm.h"

typedef enum
{
  REPEAT_NONE = 0,
  REPEAT_STRING,
  REPEAT_BUILTIN,
  REPEAT_FUNCTION,
  REPEAT_TOP_FUNCTION,
  REPEAT_MODULE,
  REPEAT_MENU,
  REPEAT_POPUP,
  REPEAT_PAGE,
  REPEAT_DESK,
  REPEAT_DESK_AND_PAGE,
  REPEAT_FVWM_WINDOW
} repeat_type;

extern char *repeat_last_function;
extern char *repeat_last_complex_function;
extern char *repeat_last_builtin_function;
extern char *repeat_last_module;
/*
extern char *repeat_last_top_function;
extern char *repeat_last_menu;
extern FvwmWindow *repeat_last_fvwm_window;
*/

void repeat_function(F_CMD_ARGS);
void update_last_string(char **pdest, char **pdest2, char *src, Bool no_store);

#endif /* _REPEAT_ */
