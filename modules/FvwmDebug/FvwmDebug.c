#define OUTPUT_FLAGS
/* This module, and the entire FvwmDebug program, and the concept for
 * interfacing this module to the Window Manager, are all original work
 * by Robert Nation
 *
 * Copyright 1994, Robert Nation. No guarantees or warantees or anything
 * are provided or implied in any way whatsoever. Use this program at your
 * own risk. Permission to use this program for any purpose is given,
 * as long as the copyright is kept intact. */

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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>

#include "FvwmDebug.h"

#include "fvwm/fvwm.h"
#include "libs/Module.h"
#include "libs/vpacket.h"

char *MyName;
int fd_width;
int fd[2];
int verbose=0;
FILE *output=NULL;

/*
** spawn_xtee - code to execute xtee from a running executable &
** redirect stdout & stderr to it.  Currently sends both to same xtee,
** but you rewrite this to spawn off 2 xtee's - one for each stream.
*/

pid_t spawn_xtee(void)
{
  pid_t pid;
  int PIPE[2];
  char *argarray[256];

  setvbuf(stdout,NULL,_IOLBF,0); /* line buffered */

  if (pipe(PIPE))
  {
    perror("spawn_xtee");
    fprintf(stderr, "ERROR ERRATA -- Failed to create pipe for xtee.\n");
    return 0;
  }

  argarray[0] = "xtee";
  argarray[1] = "-nostdout";
  argarray[2] = NULL;

  if (!(pid = fork())) /* child */
  {
    dup2(PIPE[0], STDIN_FILENO);
    close(PIPE[0]);
    close(PIPE[1]);
    execvp("xtee",argarray);
    exit(1); /* shouldn't get here... */
  }
  else /* parent */
  {
    if (ReapChildrenPid(pid) != pid)
    {
      dup2(PIPE[1], STDOUT_FILENO);
      dup2(PIPE[1], STDERR_FILENO);
      close(PIPE[0]);
      close(PIPE[1]);
    }
  }

  return pid;
} /* spawn_xtee */

/***********************************************************************
 *
 *  Procedure:
 *	main - start of module
 *
 ***********************************************************************/
int main(int argc, char **argv)
{
  const char *temp, *s;

  /* Save our program  name - for error messages */
  temp = argv[0];
  s=strrchr(argv[0], '/');
  if (s != NULL)
    temp = s + 1;

  MyName = safemalloc(strlen(temp)+2);
  strcpy(MyName, "*");
  strcat(MyName, temp);

  if((argc < 6)||(argc > 8)) {
    fprintf(stderr,"%s Version %s should only be executed by fvwm!\n",MyName,
	    VERSION);
    exit(1);
  }

  if (argc > 6)
  {
    if (!(strcmp(argv[6], "-v")))
      verbose++;
    else
    {
      output = fopen( argv[6], "w" );
      if (output == NULL)
      {
	fprintf(stderr,"%s Version %s: Failed to open %s\n",
		MyName,VERSION,argv[6]);
	exit(1);
      }
    }
  }
  if (argc > 7)
  {
    if (verbose)
    {
      output = fopen( argv[7], "w" );
      if (output == NULL)
      {
	fprintf(stderr,"%s Version %s: Failed to open %s\n",
		MyName,VERSION,argv[7]);
	exit(1);
      }
    }
    else
    {
      if (!(strcmp(argv[7], "-v")))
	verbose++;
      else
      {
        fprintf(stderr,"%s Version %s received two filename args!\n",
		MyName,VERSION);
	exit(1);
      }
    }
  }
  if (output==NULL)
    output=stderr;


  /* Dead pipe == Fvwm died */
  signal(SIGPIPE, DeadPipe);

  fd[0] = atoi(argv[1]);
  fd[1] = atoi(argv[2]);

#if 0
  spawn_xtee();
#endif

  /* Data passed in command line */
  fprintf(output,"Application Window 0x%s\n",argv[4]);
  fprintf(output,"Application Context %s\n",argv[5]);

  fd_width = GetFdWidth();

  /* select all extended messages */
  SetMessageMask(fd, M_EXTENDED_MSG | MAX_XMSG_MASK);

  /* Create a list of all windows */
  /* Request a list of all windows,
   * wait for ConfigureWindow packets */
  SendInfo(fd,"Send_WindowList",0);

  /* tell fvwm we're running */
  SendFinishedStartupNotification(fd);

  Loop(fd);
  return 0;
}


/***********************************************************************
 *
 *  Procedure:
 *	Loop - wait for data to process
 *
 ***********************************************************************/
void Loop(const int *fd)
{
    while (1) {
      FvwmPacket* packet = ReadFvwmPacket(fd[1]);
      if ( packet == NULL )
        exit(0);
      process_message( packet->type, packet->body );
    }
}


/***********************************************************************
 *
 *  Procedure:
 *	Process message - examines packet types, and takes appropriate action
 *
 ***********************************************************************/
void process_message(unsigned long type, const unsigned long *body)
{
  switch(type)
    {
    case M_NEW_PAGE:
      fprintf(output,"new page\n");
      list_new_page(body);
      break;
    case M_NEW_DESK:
      fprintf(output,"new desk\n");
      list_new_desk(body);
      break;
    case M_OLD_ADD_WINDOW:
      fprintf(output,"Old Add Window\n");
      list_old_configure(body);
    case M_OLD_CONFIGURE_WINDOW:
      fprintf(output,"Old Configure Window\n");
      list_old_configure(body);
      break;
    case M_RAISE_WINDOW:
      fprintf(output,"Raise\n");
      list_winid(body);
      break;
    case M_LOWER_WINDOW:
      fprintf(output,"Lower\n");
      list_winid(body);
      break;
    case M_FOCUS_CHANGE:
      fprintf(output,"Focus\n");
      list_focus(body);
      break;
    case M_DESTROY_WINDOW:
      fprintf(output,"Destroy\n");
      list_winid(body);
      break;
    case M_ICONIFY:
      fprintf(output,"iconify\n");
      list_icon(body);
      break;
    case M_DEICONIFY:
      fprintf(output,"Deiconify\n");
      list_icon(body);
      break;
    case M_WINDOW_NAME:
      fprintf(output,"window name\n");
      list_window_name(body);
      break;
    case M_VISIBLE_NAME:
      fprintf(output,"visible window name\n");
      list_window_name(body);
      break;
    case M_ICON_NAME:
      fprintf(output,"icon name\n");
      list_window_name(body);
      break;
    case MX_VISIBLE_ICON_NAME:
      fprintf(output,"visible icon name\n");
      list_window_name(body);
      break;
    case M_RES_CLASS:
      fprintf(output,"window class\n");
      list_window_name(body);
      break;
    case M_RES_NAME:
      fprintf(output,"class resource name\n");
      list_window_name(body);
      break;
    case M_END_WINDOWLIST:
      fprintf(output,"Send_WindowList End\n");
      break;
    case M_ICON_LOCATION:
      fprintf(output,"icon location\n");
      list_icon(body);
      break;
    case M_ICON_FILE:
      fprintf(output,"icon file\n");
      list_window_name(body);
      break;
    case M_MAP:
      fprintf(output,"map\n");
      list_winid(body);
      break;
    case M_ERROR:
      fprintf(output,"Error\n");
      list_error(body);
      break;
    case M_MINI_ICON:
      fprintf(output,"Mini icon\n");
      list_winid(body);
      break;
    case M_WINDOWSHADE:
      fprintf(output,"Windowshade\n");
      list_winid(body);
      break;
    case M_DEWINDOWSHADE:
      fprintf(output,"Dewindowshade\n");
      list_winid(body);
      break;
    case M_RESTACK:
      fprintf(output,"Restack\n");
      list_restack(body);
      break;
    case M_ADD_WINDOW:
      fprintf(output,"Add Window\n");
      list_configure(body);
      break;
    case M_CONFIGURE_WINDOW:
      fprintf(output,"Configure Window\n");
      list_configure(body);
      break;
    default:
      list_unknown(body);
      fprintf(output, " 0x%x\n", (int)type);
      break;
    }
}



/***********************************************************************
 *
 *  Procedure:
 *	SIGPIPE handler - SIGPIPE means fvwm is dying
 *
 ***********************************************************************/
void DeadPipe(int nonsense)
{
  (void)nonsense;
  fprintf(stderr,"FvwmDebug: DeadPipe\n");
  exit(0);
}

/***********************************************************************
 *
 *  Procedure:
 *	list_old_configure - displays packet contents to output
 *
 ***********************************************************************/
void list_old_configure(const unsigned long *body)
{
  fprintf(output,"\t ID %lx\n",body[0]);
  fprintf(output,"\t frame ID %lx\n",body[1]);
  fprintf(output,"\t fvwm ptr %lx\n",body[2]);
  fprintf(output,"\t frame x %ld\n",(long)body[3]);
  fprintf(output,"\t frame y %ld\n",(long)body[4]);
  fprintf(output,"\t frame w %ld\n",(long)body[5]);
  fprintf(output,"\t frame h %ld\n",(long)body[6]);
  fprintf(output,"\t desk %ld\n",(long)body[7]);
  fprintf(output,"\t flags %lx\n",body[8]);
  fprintf(output,"\t title height %ld\n",(long)body[9]);
  fprintf(output,"\t border width %ld\n",(long)body[10]);
  fprintf(output,"\t window base width %ld\n",(long)body[11]);
  fprintf(output,"\t window base height %ld\n",(long)body[12]);
  fprintf(output,"\t window resize width increment %ld\n",(long)body[13]);
  fprintf(output,"\t window resize height increment %ld\n",(long)body[14]);
  fprintf(output,"\t window min width %ld\n",(long)body[15]);
  fprintf(output,"\t window min height %ld\n",(long)body[16]);
  fprintf(output,"\t window max %ld\n",(long)body[17]);
  fprintf(output,"\t window max %ld\n",(long)body[18]);
  fprintf(output,"\t icon label window %lx\n",body[19]);
  fprintf(output,"\t icon pixmap window %lx\n",body[20]);
  fprintf(output,"\t window gravity %lx\n",body[21]);
  fflush(output);
}

/***********************************************************************
 *
 *  Procedure:
 *	list_new_page - displays packet contents to output
 *
 ***********************************************************************/
void list_new_page(const unsigned long *body)
{
  fprintf(output,"\t x %ld\n",(long)body[0]);
  fprintf(output,"\t y %ld\n",(long)body[1]);
  fprintf(output,"\t desk %ld\n",(long)body[2]);
  fflush(output);
}

/***********************************************************************
 *
 *  Procedure:
 *	list_new_desk - displays packet contents to output
 *
 ***********************************************************************/
void list_new_desk(const unsigned long *body)
{
  fprintf(output,"\t desk %ld\n",(long)body[0]);
  fflush(output);
}

/***********************************************************************
 *
 *  Procedure:
 *	list_focus - displays packet contents to output
 *
 ***********************************************************************/
void list_focus(const unsigned long *body)
{
  fprintf(output,"\t ID %lx\n",body[0]);
  fprintf(output,"\t frame ID %lx\n",body[1]);
  fprintf(output,"\t focus not by builtin %lx\n",body[2]);
  fflush(output);
}

/***********************************************************************
 *
 *  Procedure:
 *	list_winid - displays packet contents to for three-field
 *                   window ID-bearing packets
 *
 ***********************************************************************/
void list_winid(const unsigned long *body)
{
  fprintf(output,"\t ID %lx\n",body[0]);
  fprintf(output,"\t frame ID %lx\n",body[1]);
  fprintf(output,"\t fvwm ptr %lx\n",body[2]);
  fflush(output);
}


/***********************************************************************
 *
 *  Procedure:
 *	list_unknown - handles an unrecognized packet.
 *
 ***********************************************************************/
void list_unknown(const unsigned long *body)
{
  (void)body;
  fprintf(output,"Unknown packet type");
  fflush(output);
}

/***********************************************************************
 *
 *  Procedure:
 *	list_iconify - displays packet contents to output
 *
 ***********************************************************************/
void list_icon(const unsigned long *body)
{
  fprintf(output,"\t ID %lx\n",body[0]);
  fprintf(output,"\t frame ID %lx\n",body[1]);
  fprintf(output,"\t fvwm ptr %lx\n",body[2]);
  fprintf(output,"\t icon x %ld\n",(long)body[3]);
  fprintf(output,"\t icon y %ld\n",(long)body[4]);
  fprintf(output,"\t icon w %ld\n",(long)body[5]);
  fprintf(output,"\t icon h %ld\n",(long)body[6]);
  fflush(output);
}

/***********************************************************************
 *
 *  Procedure:
 *	list_window_name - displays packet contents to output
 *
 ***********************************************************************/

void list_window_name(const unsigned long *body)
{
  fprintf(output,"\t ID %lx\n",body[0]);
  fprintf(output,"\t frame ID %lx\n",body[1]);
  fprintf(output,"\t fvwm ptr %lx\n",body[2]);
  fprintf(output,"\t name %s\n",(const char *)(&body[3]));
  fflush(output);
}

/***********************************************************************
 *
 *  Procedure:
 *	list_restack - displays packet contents to output
 *
 ***********************************************************************/

void list_restack(const unsigned long *body)
{
  int i = 3;

  fprintf(output,"\t Restack beginning with:\n");
  fprintf(output,"\t\tID %lx\n",body[0]);
  fprintf(output,"\t\tframe ID %lx\n",body[1]);
  fprintf(output,"\t\tfvwm ptr %lx\n",body[2]);

  while (body[i] != 0 && i<FvwmPacketBodyMaxSize)
  {
    fprintf(output,"\t Next window:\n");
    fprintf(output,"\t\tID %lx\n",body[i]);
    fprintf(output,"\t\tframe ID %lx\n",body[i+1]);
    fprintf(output,"\t\tfvwm ptr %lx\n",body[i+2]);
    i += 3;
  }
  fflush(output);
}

/***********************************************************************
 *
 *  Procedure:
 *	list_error - displays packet contents to output
 *
 ***********************************************************************/

void list_error(const unsigned long *body)
{
  fprintf(output,"\t %s\n",(const char *)(&body[3]));
  fflush(output);
}

/***********************************************************************
 *
 *  Procedure:
 *	list_configure - displays packet contents to output
 *
 ***********************************************************************/
void list_configure(const unsigned long *body)
{
  struct ConfigWinPacket *p = (void *)body;

  fprintf(output,"\t ID %lx\n",p->w);
  fprintf(output,"\t frame ID %lx\n",p->frame);
  fprintf(output,"\t fvwm ptr %lx\n",(unsigned long)p->fvwmwin);
  fprintf(output,"\t frame x %ld\n",p->frame_x);
  fprintf(output,"\t frame y %ld\n",p->frame_y);
  fprintf(output,"\t frame w %ld\n",p->frame_width);
  fprintf(output,"\t frame h %ld\n",p->frame_height);
  fprintf(output,"\t desk %ld\n",p->desk);
  fprintf(output,"\t layer %ld\n",p->layer);
  fprintf(output,"\t title height %ld\n",p->title_height);
  fprintf(output,"\t border width %hd\n",p->border_width);
  fprintf(output,"\t window base width %ld\n",p->hints_base_width);
  fprintf(output,"\t window base height %ld\n",p->hints_base_height);
  fprintf(output,"\t window resize width increment %ld\n",p->hints_width_inc);
  fprintf(output,"\t window resize height increment %ld\n",p->hints_height_inc);
  fprintf(output,"\t window min width %ld\n",p->hints_min_width);
  fprintf(output,"\t window min height %ld\n",p->hints_min_height);
  fprintf(output,"\t window max width %ld\n",p->hints_max_width);
  fprintf(output,"\t window max height %ld\n",p->hints_max_height);
  fprintf(output,"\t icon label window %lx\n",p->icon_w);
  fprintf(output,"\t icon pixmap window %lx\n",p->icon_pixmap_w);
  fprintf(output,"\t window gravity %lx\n",p->hints_win_gravity);
  fprintf(output,"\t forecolor %lx\n",p->TextPixel);
  fprintf(output,"\t backcolor %lx\n",p->BackPixel);
  if (verbose)
  {
    fprintf(output,"\t Packet flags\n");
    fprintf(output,"\t\tis_sticky: %d\n",IS_STICKY(p));
    fprintf(output,"\t\thas_icon_font: %d\n",HAS_ICON_FONT(p));
    fprintf(output,"\t\thas_window_font: %d\n",HAS_WINDOW_FONT(p));
    fprintf(output,"\t\tdo_circulate_skip: %d\n",DO_SKIP_CIRCULATE(p));
    fprintf(output,"\t\tdo_circulate_skip_icon: %d\n",
	    DO_SKIP_ICON_CIRCULATE(p));
    fprintf(output,"\t\tdo_circulate_skip_shaded: %d\n",
	    DO_SKIP_SHADED_CIRCULATE(p));
    fprintf(output,"\t\tdo_grab_focus_when_created: %d\n", DO_GRAB_FOCUS(p));
    fprintf(output,"\t\tdo_grab_focus_when_transient_created: %d\n",
	    DO_GRAB_FOCUS_TRANSIENT(p));
    fprintf(output,"\t\tdo_ignore_restack: %d\n", DO_IGNORE_RESTACK(p));
    fprintf(output,"\t\tdo_lower_transient: %d\n", DO_LOWER_TRANSIENT(p));
    fprintf(output,"\t\tdo_not_show_on_map: %d\n", DO_NOT_SHOW_ON_MAP(p));
    fprintf(output,"\t\tdo_not_pass_click_focus_click: %d\n",
	    DO_NOT_PASS_CLICK_FOCUS_CLICK(p));
    fprintf(output,"\t\tdo_not_raise_click_focus_click: %d\n",
	    DO_NOT_RAISE_CLICK_FOCUS_CLICK(p));
    fprintf(output,"\t\tdo_raise_mouse_focus_click: %d\n",
	    DO_RAISE_MOUSE_FOCUS_CLICK(p));
    fprintf(output,"\t\tdo_raise_transient: %d\n", DO_RAISE_TRANSIENT(p));
    fprintf(output,"\t\tdo_resize_opaque: %d\n", DO_RESIZE_OPAQUE(p));
    fprintf(output,"\t\tdo_shrink_windowshade: %d\n", DO_SHRINK_WINDOWSHADE(p));
    fprintf(output,"\t\tdo_stack_transient_parent: %d\n",
	    DO_STACK_TRANSIENT_PARENT(p));
    fprintf(output,"\t\tdo_start_iconic: %d\n", DO_START_ICONIC(p));
    fprintf(output,"\t\tdo_window_list_skip: %d\n", DO_SKIP_WINDOW_LIST(p));
    fprintf(output,"\t\tfocus_mode: %d\n", GET_FOCUS_MODE(p));
    fprintf(output,"\t\thas_depressable_border: %d\n",
	    HAS_DEPRESSABLE_BORDER(p));
    fprintf(output,"\t\thas_mwm_border: %d\n", HAS_MWM_BORDER(p));
    fprintf(output,"\t\thas_mwm_buttons: %d\n", HAS_MWM_BUTTONS(p));
    fprintf(output,"\t\thas_mwm_override: %d\n", HAS_MWM_OVERRIDE_HINTS(p));
    fprintf(output,"\t\thas_no_icon_title: %d\n", HAS_NO_ICON_TITLE(p));
    fprintf(output,"\t\thas_override_size: %d\n", HAS_OVERRIDE_SIZE_HINTS(p));
    fprintf(output,"\t\thas_stippled_title: %d\n", HAS_STIPPLED_TITLE(p));
    fprintf(output,"\t\tis_fixed: %d\n", IS_FIXED(p));
    fprintf(output,"\t\tis_sticky_icon: %d\n", IS_ICON_STICKY(p));
    fprintf(output,"\t\tis_icon_suppressed: %d\n", IS_ICON_SUPPRESSED(p));
    fprintf(output,"\t\tis_lenient: %d\n", IS_LENIENT(p));
    fprintf(output,"\t\tdoes_wm_delete_window: %d\n", WM_DELETES_WINDOW(p));
    fprintf(output,"\t\tdoes_wm_take_focus: %d\n", WM_TAKES_FOCUS(p));
    fprintf(output,"\t\tdo_iconify_after_map: %d\n", DO_ICONIFY_AFTER_MAP(p));
    fprintf(output,"\t\tdo_reuse_destroyed: %d\n", DO_REUSE_DESTROYED(p));
    fprintf(output,"\t\thas_border: %d\n", HAS_BORDER(p));
    fprintf(output,"\t\thas_title: %d\n", HAS_TITLE(p));
    fprintf(output,"\t\tis_deiconify_pending: %d\n", IS_DEICONIFY_PENDING(p));
    fprintf(output,"\t\tis_fully_visible: %d\n", IS_FULLY_VISIBLE(p));
    fprintf(output,"\t\tis_iconified: %d\n", IS_ICONIFIED(p));
    fprintf(output,"\t\tis_iconfied_by_parent: %d\n",
	    IS_ICONIFIED_BY_PARENT(p));
    fprintf(output,"\t\tis_icon_entered: %d\n", IS_ICON_ENTERED(p));
    fprintf(output,"\t\tis_icon_font_loaded: %d\n", IS_ICON_FONT_LOADED(p));
    fprintf(output,"\t\tis_icon_moved: %d\n", IS_ICON_MOVED(p));
    fprintf(output,"\t\tis_icon_ours: %d\n", IS_ICON_OURS(p));
    fprintf(output,"\t\tis_icon_shaped: %d\n", IS_ICON_SHAPED(p));
    fprintf(output,"\t\tis_icon_unmapped: %d\n", IS_ICON_UNMAPPED(p));
    fprintf(output,"\t\tis_mapped: %d\n", IS_MAPPED(p));
    fprintf(output,"\t\tis_map_pending: %d\n", IS_MAP_PENDING(p));
    fprintf(output,"\t\tis_maximized: %d\n", IS_MAXIMIZED(p));
    fprintf(output,"\t\tis_name_changed: %d\n", IS_NAME_CHANGED(p));
    fprintf(output,"\t\tis_partially_visible: %d\n", IS_PARTIALLY_VISIBLE(p));
    fprintf(output,"\t\tis_pixmap_ours: %d\n", IS_PIXMAP_OURS(p));
    fprintf(output,"\t\tis_placed_wb3: %d\n", IS_PLACED_WB3(p));
    fprintf(output,"\t\tis_placed_by_fvwm: %d\n", IS_PLACED_BY_FVWM(p));
    fprintf(output,"\t\tis_size_inc_set: %d\n", IS_SIZE_INC_SET(p));
    fprintf(output,"\t\tis_transient: %d\n", IS_TRANSIENT(p));
    fprintf(output,"\t\tis_window_drawn_once: %d\n", IS_WINDOW_DRAWN_ONCE(p));
    fprintf(output,"\t\tis_viewport_moved: %d\n", IS_VIEWPORT_MOVED(p));
    fprintf(output,"\t\tis_window_being_moved_opaque: %d\n",
	    IS_WINDOW_BEING_MOVED_OPAQUE(p));
    fprintf(output,"\t\tis_window_font_loaded: %d\n", IS_WINDOW_FONT_LOADED(p));
    fprintf(output,"\t\tis_window_shaded %d\n", IS_SHADED(p));
    fprintf(output,"\t\ttitle_dir: %d\n", GET_TITLE_DIR(p));
    fflush(output);
  }
}
