/* File:    fvwmdebug.c
 *
 * Description:
 *      Implement some debugging/log functions that can be used by fvwm only.
 *
 * Created:
 *       12 Dec 1998 - Dominik Vogt <dominik_vogt@hp.com>
 */

#ifndef _DEBUG_
#define _DEBUG_

#include "config.h"
#include "fvwm.h"
#include <stdio.h>

/* Don't put this into the #ifdef, since some compilers don't like completely
 * empty source files.
 */
int DB_DUMMY_EXPORTED_SYMBOL;

#ifdef DEBUG
/* WI stands for 'window info' */
void DB_WI_WINDOWS(char *label, FvwmWindow *fw)
{
  fprintf(stderr, "%s: FvwmWindow=0x%x, next=0x%x, prev=0x%x, stack_next=0x%x, stack_prev=0x%x\n", label?label:"", fw, fw->next, fw->prev, fw->stack_next, fw->stack_prev);
  return;
}

void DB_WI_SUBWINS(char *label, FvwmWindow *fw)
{
  fprintf(stderr, "%s: FvwmWindow=0x%x, frame=0x%x, w=0x%x, parent=0x%x, title_w=0x%x, icon_w=0x%x, icon_pixmap_w=0x%x\n", label?label:"", fw, fw->frame, fw->w, fw->Parent, fw->title_w, fw->icon_w, fw->icon_pixmap_w);
  return;
}

void DB_WI_FRAMEWINS(char *label, FvwmWindow *fw)
{
  fprintf(stderr, "%s: FvwmWindow=0x%x, side windows: 0x%x 0x%x 0x%x 0x%x, corner windows: 0x%x 0x%x 0x%x 0x%x\n", label?label:"", fw, fw->frame, fw->sides[0], fw->sides[1], fw->sides[2], fw->sides[3], fw->corners[0], fw->corners[1], fw->corners[2], fw->corners[3]);
  return;
}

void DB_WI_BUTTONWINS(char *label, FvwmWindow *fw)
{
  fprintf(stderr, "%s: FvwmWindow=0x%x, left button windows: 0x%x 0x%x 0x%x 0x%x 0x%x, right button windows: 0x%x 0x%x 0x%x 0x%x 0x%x\n", label?label:"", fw, fw->frame, fw->left_w[0], fw->left_w[1], fw->left_w[2], fw->left_w[3], fw->left_w[4], fw->right_w[0], fw->right_w[1], fw->right_w[2], fw->right_w[3], fw->right_w[4]);
  return;
}

void DB_WI_FRAME(char *label, FvwmWindow *fw)
{
  fprintf(stderr, "%s: FvwmWindow=0x%x, frame=0x%x, name=%s, flags=0x%08x, frame_x=%d, frame_y=%d, frame_width=%d, frame_height=%d, orig_x=%d, orig_y=%d, orig_wd=%d, orig_ht=%d\n", label?label:"", fw, fw->frame, fw->name?fw->name:"(NULL)",fw->flags,fw->frame_x, fw->frame_y, fw->frame_width, fw->frame_height, fw->orig_x, fw->orig_y, fw->orig_wd, fw->orig_ht);
  return;
}

void DB_WI_ICON(char *label, FvwmWindow *fw)
{
  fprintf(stderr, "%s: FvwmWindow=0x%x, icon_w=0x%x, icon_pixmap_w=0x%x, icon_x_loc=%d, icon_xl_loc=%d, icon_y_loc=%d, icon_w_width=%d, icon_w_height=%d, icon_t_width=%d, icon_p_width=%d, icon_p_height=%d, icon_name=%d\n", label?label:"", fw, fw->icon_w, fw->icon_pixmap_w,fw->icon_x_loc,fw->icon_xl_loc,fw->icon_y_loc,fw->icon_w_width,fw->icon_w_height,fw->icon_t_width,fw->icon_p_width,fw->icon_p_height,fw->icon_name);
  return;
}

void DB_WI_SIZEHINTS(char *label, FvwmWindow *fw)
{
  fprintf(stderr, "%s: FvwmWindow=0x%x, base_width=%d, base_height=%d, width_inc=%d, height_inc=%d, min_width=%d, max_width=%d, min_height=%d, max_height=%d, win_gravity=0x%x", label?label:"", fw, fw->hints.flags, fw->hints.base_width, fw->hints.base_height, fw->hints.width_inc, fw->hints.height_inc, fw->hints.min_width, fw->hints.max_width, fw->hints.min_height, fw->hints.max_height, fw->hints.win_gravity);
  if (fw->hints.flags & PAspect) fprintf(stderr,"max_aspect.x=%d, max_aspect.y=%d, min_aspect.x=%d, min_aspect.y=%d", fw->hints.max_aspect.x, fw->hints.max_aspect.y, fw->hints.min_aspect.x, fw->hints.min_aspect.y);
  fprintf(stderr,"\n");
  return;
}

void DB_WI_TITLE(char *label, FvwmWindow *fw)
{
  fprintf(stderr, "%s: FvwmWindow=0x%x, name=%s, icon_name=%s, title_x=%d, title_y=%d, title_width=%d, title_height=%d\n", label?label:"", fw, fw->name?fw->name:"(NULL)", fw->icon_name?fw->icon_name:"(NULL)", fw->title_x, fw->title_y, fw->title_width, fw->title_height);
  return;
}

void DB_WI_BORDER(char *label, FvwmWindow *fw)
{
  fprintf(stderr, "%s: FvwmWindow=0x%x, bw=%d, old_bw=%d, boundary_width=%d, corner_width=%d\n", label?label:"", fw, fw->bw, fw->old_bw, fw->boundary_width, fw->corner_width);
  return;
}

void DB_WI_XWINATTR(char *label, FvwmWindow *fw)
{
  /* too much work for now */
  return;
}

void DB_WI_ALL(char *label, FvwmWindow *fw)
{
  fprintf(stderr,"%s: --- all fvwm window values for fw=0x%x ---\n", label,fw);
  DB_WI_WINDOWS("",fw);
  DB_WI_SUBWINS("",fw);
  DB_WI_BUTTONWINS("",fw);
  DB_WI_FRAMEWINS("",fw);
  DB_WI_FRAME("",fw);
  DB_WI_ICON("",fw);
  DB_WI_SIZEHINTS("",fw);
  DB_WI_TITLE("",fw);
  DB_WI_BORDER("",fw);
  DB_WI_XWINATTR("",fw);
  fprintf(stderr,"%s: ---------- end ----------\n",label);
  return;
}

#endif

#endif /* _DEBUG_ */
