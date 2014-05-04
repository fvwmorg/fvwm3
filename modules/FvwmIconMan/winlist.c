/* -*-c-*- */
/* This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see: <http://www.gnu.org/licenses/>
 */

#include "config.h"

#include <limits.h>
#include "libs/wild.h"
#include "FvwmIconMan.h"

#define HASHTAB_SIZE 257

typedef WinList HashTab[HASHTAB_SIZE];
static HashTab hash_tab;

void print_stringlist (StringList *list)
{
  StringEl *p;
  char *s;

  ConsoleDebug (WINLIST, "\tmask = 0x%x\n", list->mask);
  for (p = list->list; p; p = p->next) {
    switch (p->type) {
    case ALL_NAME:
      s = "all";
      break;

    case TITLE_NAME:
      s = "title";
      break;

    case ICON_NAME:
      s = "icon";
      break;

    case RESOURCE_NAME:
      s = "resource";
      break;

    case CLASS_NAME:
      s = "class";
      break;

    default:
      s = "unknown type";
    }
    ConsoleDebug (WINLIST, "\t%s = %s\n", s, p->string);
  }
}

void add_to_stringlist (StringList *list, char *s)
{
  StringEl *new;
  NameType type;
  char *pat;

  ConsoleDebug (WINLIST, "In add_to_stringlist: %s\n", s);

  pat = strchr (s, '=');
  if (pat) {
    *pat++ = '\0';
    if (!strcasecmp (s, "icon"))
      type = ICON_NAME;
    else if (!strcasecmp (s, "title"))
      type = TITLE_NAME;
    else if (!strcasecmp (s, "resource"))
      type = RESOURCE_NAME;
    else if (!strcasecmp (s, "class"))
      type = CLASS_NAME;
    else {
      ConsoleMessage ("Bad element in show/dontshow list: %s\n", s);
      return;
    }
  }
  else {
    pat = s;
    type = ALL_NAME;
  }

  ConsoleDebug (WINLIST, "add_to_stringlist: %s %s\n",
		type == ALL_NAME ? "all" : s, pat);

  new = xmalloc (sizeof (StringEl));
  new->string = xmalloc ((strlen (pat) + 1) * sizeof (char));
  new->type = type;

  strcpy (new->string, pat);
  new->next = list->list;
  if (list->list)
    list->mask |= type;
  else
    list->mask = type;
  list->list = new;

  ConsoleDebug (WINLIST, "Exiting add_to_stringlist\n");
}

static int matches_string (NameType type, char *pattern, char *tname,
			   char *iname, char *rname, char *cname)
{
  int ans = 0;

  ConsoleDebug (WINLIST, "matches_string: type: 0x%x pattern: %s\n",
		type, pattern);
  ConsoleDebug (WINLIST, "\tstrings: %s:%s %s:%s\n", tname, iname,
		rname, cname);

  if (tname && (type == ALL_NAME || type == TITLE_NAME)) {
    ans |= matchWildcards (pattern, tname);
  }
  if (iname && (type == ALL_NAME || type == ICON_NAME)) {
    ans |= matchWildcards (pattern, iname);
  }
  if (rname && (type == ALL_NAME || type == RESOURCE_NAME)) {
    ans |= matchWildcards (pattern, rname);
  }
  if (cname && (type == ALL_NAME || type == CLASS_NAME)) {
    ans |= matchWildcards (pattern, cname);
  }

  ConsoleDebug (WINLIST, "\tmatches_string: %d\n", ans);
  return ans;
}

int check_resolution(WinManager *manager, WinData *win)
{
  int flag = 0;
  int reverse = 0;
  rectangle g;

  assert (manager);

  if (IS_ICONIFIED(win) && !IS_ICON_SUPPRESSED(win))
  {
	  g = win->icon_g;
  }
  else
  {
	  g.x = win->x;
	  g.y = win->y;
	  g.width = win->width;
	  g.height = win->height;
  }
  switch (manager->res) {
  case SHOW_GLOBAL:
    flag = 1;
    break;

  case NO_SHOW_DESKTOP:
    reverse = 1;
    /* fall through to next case */
  case SHOW_DESKTOP:
    if (IS_STICKY_ACROSS_PAGES(win) || win->desknum == globals.desknum)
      flag = 1;
    break;

  case NO_SHOW_PAGE:
    reverse = 1;
    /* fall through to next case */
  case SHOW_PAGE:
    if (IS_STICKY_ACROSS_PAGES(win)) {
      flag = 1;
    } else if (win->desknum == globals.desknum) {
      /* win and screen intersect if they are not disjoint in x and y */
      flag = fvwmrect_do_rectangles_intersect(&g, &manager->managed_g);
    }
    break;

  case NO_SHOW_SCREEN:
    reverse = 1;
    /* fall through to next case */
  case SHOW_SCREEN:
    if (win->desknum == globals.desknum) {
      /* win and screen intersect if they are not disjoint in x and y */
      flag = fvwmrect_do_rectangles_intersect(&g, &manager->managed_g);
    }
    break;
  }
  flag ^= reverse;

  return flag;
}

static int iconmanager_show (WinManager *man, char *tname, char *iname,
			     char *rname, char *cname)
{
  StringEl *string;
  int in_showlist = 0, in_dontshowlist = 0;

  assert (man);

#ifdef FVWM_DEBUG_MSGS
  ConsoleDebug (WINLIST, "In iconmanager_show: %s:%s : %s %s\n", tname, iname,
		rname, cname);
  ConsoleDebug (WINLIST, "dontshow:\n");
  print_stringlist (&man->dontshow);
  ConsoleDebug (WINLIST, "show:\n");
  print_stringlist (&man->show);
#endif /* FVWM_DEBUG_MSGS */

  for (string = man->dontshow.list; string; string = string->next) {
    ConsoleDebug (WINLIST, "Matching: %s\n", string->string);
    if (matches_string (string->type, string->string, tname, iname,
			rname, cname)) {
      ConsoleDebug (WINLIST, "Don't show\n");
      in_dontshowlist = 1;
      break;
    }
  }

  if (!in_dontshowlist) {
    if (man->show.list == NULL) {
      in_showlist = 1;
    }
    else {
      for (string = man->show.list; string; string = string->next) {
	ConsoleDebug (WINLIST, "Matching: %s\n", string->string);
	if (matches_string (string->type, string->string, tname, iname,
			    rname, cname)) {
	  ConsoleDebug (WINLIST, "Show\n");
	  in_showlist = 1;
	  break;
	}
      }
    }
  }

  ConsoleDebug (WINLIST, "returning: %d %d %d\n", in_dontshowlist,
		in_showlist, !in_dontshowlist && in_showlist);

  return (!in_dontshowlist && in_showlist);
}

WinData *new_windata (void)
{
  WinData *new = xcalloc (1, sizeof (WinData));
  new->desknum = ULONG_MAX;
  new->x = ULONG_MAX;
  new->y = ULONG_MAX;
  new->app_id = ULONG_MAX;

  return new;
}

void free_windata (WinData *p)
{
  if (globals.select_win == p) {
    ConsoleMessage ("Internal error in free_windata\n");
    globals.select_win = NULL;
    abort();
  }

  Free (p->resname);
  Free (p->classname);
  Free (p->iconname);
  Free (p->titlename);
  Free (p->display_string);
  Free (p);
}


/* This ALWAYS gets called when one of the name strings changes */

WinManager *figure_win_manager (WinData *win, Uchar name_mask)
{
  int i;
  char *tname = win->titlename;
  char *iname = win->iconname;
  char *rname = win->resname;
  char *cname = win->classname;
  WinManager *man;

  assert (tname || iname || rname || cname);
  ConsoleDebug (WINLIST, "set_win_manager: %s %s %s %s\n", tname, iname, rname, cname);

  for (i = 0, man = &globals.managers[0]; i < globals.num_managers;
       i++, man++) {
    if (iconmanager_show (man, tname, iname, rname, cname) &&
	check_resolution(man, win)) {
      if (man != win->manager) {
	assert (man->magic == 0x12344321);
      }
      return man;
    }
  }

  /* No manager wants this window */
  return NULL;
}

int check_win_complete (WinData *p)
{
  if (p->complete)
    return 1;

  ConsoleDebug (WINLIST, "Checking completeness:\n");
  ConsoleDebug (WINLIST, "\ttitlename: %s\n",
		(p->titlename ? p->titlename : "No Title name"));
  ConsoleDebug (WINLIST, "\ticonname: %s\n",
		(p->iconname ? p->iconname : "No Icon name"));
  ConsoleDebug (WINLIST, "\tres: %s\n",
		(p->resname ? p->resname : "No p->resname"));
  ConsoleDebug (WINLIST, "\tclass: %s\n",
		(p->classname ? p->classname : "No p->classname"));
  ConsoleDebug (WINLIST, "\tdisplaystring: %s\n",
		(p->display_string ? p->display_string :
		 "No p->display_string"));
  ConsoleDebug (WINLIST, "\t(x, y): (%ld, %ld)\n", p->x, p->y);
  ConsoleDebug (WINLIST, "\tapp_id: 0x%lx %d\n", p->app_id, p->app_id_set);
  ConsoleDebug (WINLIST, "\tdesknum: %ld\n", p->desknum);
  ConsoleDebug (WINLIST, "\tmanager: 0x%lx\n", (unsigned long)p->manager);

  if (p->geometry_set &&
      p->resname &&
      p->classname &&
      p->iconname &&
      p->titlename &&
      p->app_id_set) {
    p->complete = 1;
    ConsoleDebug (WINLIST, "\tcomplete: 1\n\n");
    return 1;
  }

  ConsoleDebug (WINLIST, "\tcomplete: 0\n\n");
  return 0;
}

void init_winlists (void)
{
  int i;
  for (i = 0; i < HASHTAB_SIZE; i++) {
    hash_tab[i].n = 0;
    hash_tab[i].head = NULL;
    hash_tab[i].tail = NULL;
  }
}

void delete_win_hashtab (WinData *win)
{
  int entry;
  WinList *list;

  entry = win->app_id & 0xff;
  list = &hash_tab[entry];

  if (win->win_prev)
    win->win_prev->win_next = win->win_next;
  else
    list->head = win->win_next;
  if (win->win_next)
    win->win_next->win_prev = win->win_prev;
  else
    list->tail = win->win_prev;
  list->n--;
}

void insert_win_hashtab (WinData *win)
{
  int entry;
  WinList *list;
  WinData *p;

  entry = win->app_id & 0xff;
  list = &hash_tab[entry];

  for (p = list->head; p && win->app_id > p->app_id;
       p = p->win_next);

  if (p) {
    /* insert win before p */
    win->win_next = p;
    win->win_prev = p->win_prev;
    if (p->win_prev)
      p->win_prev->win_next = win;
    else
      list->head = win;
    p->win_prev = win;
  }
  else {
    /* put win at end of list */
    win->win_next = NULL;
    win->win_prev = list->tail;
    if (list->tail)
      list->tail->win_next = win;
    else
      list->head = win;
    list->tail = win;
  }
  list->n++;
}

WinData *find_win_hashtab (Ulong id)
{
  WinList *list;
  int entry = id & 0xff;
  WinData *p;

  list = &hash_tab[entry];

  for (p = list->head; p && p->app_id != id; p = p->win_next);

  return p;
}

void walk_hashtab (void (*func)(void *))
{
  int i;
  WinData *p;

  for (i = 0; i < HASHTAB_SIZE; i++) {
    for (p = hash_tab[i].head; p; p = p->win_next)
      func (p);
  }
}

int accumulate_walk_hashtab (int (*func)(void *))
{
  int i, ret = 0;
  WinData *p;

  for (i = 0; i < HASHTAB_SIZE; i++) {
    for (p = hash_tab[i].head; p; p = p->win_next)
      ret += func (p);
  }

  return ret;
}
