/* -*-c-*- */

#ifndef _REPEAT_
#define _REPEAT_

typedef enum
{
	REPEAT_NONE = 0,
	REPEAT_COMMAND,
	/* I think we don't need all these
	   REPEAT_BUILTIN,
	   REPEAT_FUNCTION,
	   REPEAT_TOP_FUNCTION,
	   REPEAT_MODULE,
	*/
	REPEAT_MENU,
	REPEAT_POPUP,
	REPEAT_PAGE,
	REPEAT_DESK,
	REPEAT_DESK_AND_PAGE,
	REPEAT_FVWM_WINDOW
} repeat_t;

extern char *repeat_last_function;
extern char *repeat_last_complex_function;
extern char *repeat_last_builtin_function;
extern char *repeat_last_module;
/*
  extern char *repeat_last_top_function;
  extern char *repeat_last_menu;
  extern FvwmWindow *repeat_last_fvwm_window;
*/

Bool set_repeat_data(void *data, repeat_t type, const func_t *builtin);

#endif /* _REPEAT_ */
