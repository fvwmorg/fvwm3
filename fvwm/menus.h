/* -*-c-*- */

#ifndef _MENUS_
#define _MENUS_

#define MENU_IS_LEFT  0x01
#define MENU_IS_RIGHT 0x02
#define MENU_IS_UP    0x04
#define MENU_IS_DOWN  0x08

/*
 * MISCELLANEOUS MENU STUFF
 */

#define IS_MENU_RETURN(x) \
  ((x)==MENU_DONE || (x)==MENU_ABORTED || (x)==MENU_SUBMENU_TORN_OFF)

struct MenuRoot;
struct MenuStyle;
struct MenuReturn;
struct MenuParameters;
struct MenuOptions;
struct MenuItem;
struct MenuRepaintTransparentParameters;

/*
 * EXPORTED FUNCTIONS
 */

void menus_init(void);
struct MenuRoot *menus_find_menu(char *name);
void menus_remove_style_from_menus(struct MenuStyle *ms);
struct MenuRoot *FollowMenuContinuations(
	struct MenuRoot *mr, struct MenuRoot **pmrPrior);
struct MenuRoot *NewMenuRoot(char *name);
void AddToMenu(struct MenuRoot *, char *, char *, Bool, Bool, Bool);
void menu_enter_tear_off_menu(const exec_context_t *exc);
void menu_close_tear_off_menu(FvwmWindow *fw);
Bool menu_redraw_transparent_tear_off_menu(FvwmWindow *fw, Bool pr_only);
void do_menu(struct MenuParameters *pmp, struct MenuReturn *pret);
char *get_menu_options(
	char *action, Window w, FvwmWindow *fw, XEvent *e, struct MenuRoot *mr,
	struct MenuItem *mi, struct MenuOptions *pops);
Bool DestroyMenu(
	struct MenuRoot *mr, Bool do_recreate, Bool is_command_request);
void add_another_menu_item(char *action);
void change_mr_menu_style(struct MenuRoot *mr, char *stylename);
void UpdateAllMenuStyles(void);
void UpdateMenuColorset(int cset);
void SetMenuCursor(Cursor cursor);
void update_transparent_menu_bg(
	struct MenuRepaintTransparentParameters *prtm,
	int current_x, int current_y, int step_x, int step_y,
	int end_x, int end_y);
void repaint_transparent_menu(
	struct MenuRepaintTransparentParameters *prtmp,
	Bool first, int x, int y, int end_x, int end_y, Bool is_bg_set);
Bool menu_expose(XEvent *event, FvwmWindow *fw);
int menu_binding(Display *dpy, binding_t type, int button, KeySym keysym,
	int context, int modifier, char *action, char *menuStyle);
#endif /* _MENUS_ */
