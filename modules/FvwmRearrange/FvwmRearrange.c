/*************************************************************************
 * FvwmRearrange.c -- fvwm module to cascade windows
 *
 * See man page for usage.
 *
 * Written by Andrew Veliath
 * Copyright 1996
 * Permission is granted to distrubute this software for any  purpose,
 * as the copyright and this copyright notice remain intact.
 *
 * Combined FvwmTile and FvwmCascade to FvwmRearrange module.
 * 9-Nov-1998 Dominik Vogt
 *
 ************************************************************************/

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>

#ifdef HAVE_SYS_BSDTYPES_H
#include <sys/bsdtypes.h>
#endif

#if HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

#include <X11/Xlib.h>

#include "fvwmlib.h"
#include "../../fvwm/module.h"
#include "../../fvwm/fvwm.h"

typedef struct window_item {
    Window frame;
    int th, bw;
    unsigned long width, height;
    struct window_item *prev, *next;
} window_item, *window_list;

/* vars */
Display *dpy;
int dwidth, dheight;
char *argv0;
int fd[2], fd_width;
window_list wins = NULL, wins_tail = NULL;
int wins_count = 0;
FILE *console;

/* switches */
int ofsx = 0, ofsy = 0;
int maxw = 0, maxh = 0;
int maxx, maxy;
int untitled = 0, transients = 0;
int maximized = 0;
int all = 0;
int desk = 0;
int reversed = 0, raise_window = 1;
int resize = 0;
int nostretch = 0;
int sticky = 0;
int flatx = 0, flaty = 0;
int incx = 0, incy = 0;
int horizontal = 0;
int maxnum = 0;

char FvwmTile;
char FvwmCascade;

void insert_window_list(window_list *wl, window_item *i)
{
    if (*wl) {
	if ((i->prev = (*wl)->prev))
	    i->prev->next = i;
	i->next = *wl;
	(*wl)->prev = i;
    } else
	i->next = i->prev = NULL;
    *wl = i;
}

void free_window_list(window_list *wl)
{
    window_item *q;
    while (*wl) {
	q = *wl;
	free(*wl);
	*wl = (*wl)->next;
    }
}

int is_suitable_window(unsigned long *body)
{
    XWindowAttributes xwa;
    unsigned long flags = body[8];

    if ((flags&WINDOWLISTSKIP) && !all)
	return 0;

    if ((flags&MAXIMIZED) && !maximized)
	return 0;
    
    if ((flags&STICKY) && !sticky)
	return 0;
    
    if (!XGetWindowAttributes(dpy, (Window)body[1], &xwa))
	return 0;
    
    if (xwa.map_state != IsViewable)
	return 0;

    if (!(flags&MAPPED))
	return 0;

    if (flags&ICONIFIED)
	return 0;

    if (!desk) {
	int x = (int)body[3], y = (int)body[4];
	int w = (int)body[5], h = (int)body[6];
	if (!((x < dwidth) && (y < dheight)
	      && (x + w > 0) && (y + h > 0)))
	    return 0;
    }
	    
    if (!(flags&TITLE) && !untitled)
	return 0;

    if ((flags&TRANSIENT) && !transients)
	return 0;

    return 1;
}

int get_window(void)
{
    unsigned long header[HEADER_SIZE], *body;
    int count, last = 0;
    fd_set infds;
    FD_ZERO(&infds);
    FD_SET(fd[1], &infds);
    select(fd_width,&infds, 0, 0, NULL);
    if ((count = ReadFvwmPacket(fd[1],header,&body)) > 0) {
	switch (header[1])
	{
	case M_CONFIGURE_WINDOW:
	    if (is_suitable_window(body)) {
		window_item *wi =
		    (window_item*)safemalloc(sizeof( window_item ));
		wi->frame = (Window)body[1];
		wi->th = (int)body[9];
		wi->bw = (int)body[10];
		wi->width = body[5];
		wi->height = body[6];
		if (!wins_tail) wins_tail = wi;
		insert_window_list(&wins, wi);
		++wins_count;
	    }
	    last = 1;
	    break;
	    
	case M_END_WINDOWLIST:
	    break;
	    
	default:
	    fprintf(console, 
		    "%s: internal inconsistency: unknown message\n",
		    argv0);
	    break;
	}
	free(body);
    }
    return last;
}

void wait_configure(window_item *wi)
{
    int found = 0;
    unsigned long header[HEADER_SIZE], *body;
    int count;
    fd_set infds;
    FD_ZERO(&infds);
    FD_SET(fd[1], &infds);
    select(fd_width,&infds, 0, 0, NULL);
    while (!found)	
	if ((count = ReadFvwmPacket(fd[1],header,&body)) > 0) {
	    if  ((header[1] == M_CONFIGURE_WINDOW)
		 && (Window)body[1] == wi->frame)
		found = 1;
	    free(body);
	}
}

int atopixel(char *s, unsigned long f)
{
    int l = strlen(s);
    if (l < 1) return 0;
    if (isalpha(s[l - 1])) {
	char s2[24];
	strcpy(s2,s);
	s2[strlen(s2) - 1] = 0;	
	return atoi(s2);
    }
    return (atoi(s) * f) / 100;
}

void tile_windows(void)
{
    char msg[128];
    int cur_x = ofsx, cur_y = ofsy;
    int wdiv, hdiv, i, j, count = 1;
    window_item *w = reversed ? wins_tail : wins;    

    if (horizontal) {
	if ((maxnum > 0) && (maxnum < wins_count)) {
	    count = wins_count / maxnum;
	    if (wins_count % maxnum) ++count;
	    hdiv = (maxy - ofsy + 1) / maxnum;
	} else {
	    maxnum = wins_count;
	    hdiv = (maxy - ofsy + 1) / wins_count;
	}
	wdiv = (maxx - ofsx + 1) / count;

	for (i = 0; w && (i < count); ++i)  {
	    for (j = 0; w && (j < maxnum); ++j) {
		int nw = wdiv - w->bw * 2;
		int nh = hdiv - w->bw * 2 - w->th;
		
		if (resize) {
		    if (nostretch) {
			if (nw > w->width)
			    nw = w->width;
			if (nh > w->height)
			    nh = w->height;
		    }
		    sprintf(msg, "Resize %lup %lup",
			    (nw > 0) ? nw : w->width,
			    (nh > 0) ? nh : w->height);	
		    SendInfo(fd,msg,w->frame);
		}
		sprintf(msg, "Move %up %up", cur_x, cur_y);
		SendInfo(fd,msg,w->frame);
		if (raise_window)
		    SendInfo(fd,"Raise",w->frame);
		cur_y += hdiv;
		wait_configure(w);
		w = reversed ? w->prev : w->next;
	    }
	    cur_x += wdiv;
	    cur_y = ofsy;
	}
    } else  {
	if ((maxnum > 0) && (maxnum < wins_count)) {
	    count = wins_count / maxnum;
	    if (wins_count % maxnum) ++count;
	    wdiv = (maxx - ofsx + 1) / maxnum;
	} else {
	    maxnum = wins_count;
	    wdiv = (maxx - ofsx + 1) / wins_count;
	}
	hdiv = (maxy - ofsy + 1) / count;

	for (i = 0; w && (i < count); ++i)  {
	    for (j = 0; w && (j < maxnum); ++j) {
		int nw = wdiv - w->bw * 2;
		int nh = hdiv - w->bw * 2 - w->th;
		
		if (resize) {
		    if (nostretch) {
			if (nw > w->width)
			    nw = w->width;
			if (nh > w->height)
			    nh = w->height;
		    }
		    sprintf(msg, "Resize %lup %lup",
			    (nw > 0) ? nw : w->width,
			    (nh > 0) ? nh : w->height);
		    SendInfo(fd,msg,w->frame);
		}
		sprintf(msg, "Move %up %up", cur_x, cur_y);
		SendInfo(fd,msg,w->frame);
		if (raise_window)
		    SendInfo(fd,"Raise",w->frame);
		cur_x += wdiv;
		wait_configure(w);
		w = reversed ? w->prev : w->next;
	    }
	    cur_x = ofsx;
	    cur_y += hdiv;
	}
    }    
}

void cascade_windows(void)
{
    char msg[128];
    int cur_x = ofsx, cur_y = ofsy;
    window_item *w = reversed ? wins_tail : wins;
    while (w)
    {
	unsigned long nw = 0, nh = 0;
	if (raise_window)
	    SendInfo(fd,"Raise",w->frame);
	sprintf(msg, "Move %up %up", cur_x, cur_y);
	SendInfo(fd,msg,w->frame);
	if (resize) {
	    if (nostretch) {		
		if (maxw
		    && (w->width > maxw))
		    nw = maxw;
		if (maxh
		    && (w->height > maxh))
		    nh = maxh;
	    } else {
		nw = maxw; 
		nh = maxh;
	    }
	    if (nw || nh) {
		sprintf(msg, "Resize %lup %lup", 
			nw ? nw : w->width, 
			nh ? nh : w->height);
		SendInfo(fd,msg,w->frame);
	    }
	}
	wait_configure(w);
	if (!flatx)
	    cur_x += w->bw;
	cur_x += incx;
	if (!flaty)
	    cur_y += w->bw + w->th;
	cur_y += incy;
	w = reversed ? w->prev : w->next;
    }
}

void parse_args(char *s, int argc, char *argv[], int argi)
{
    int nsargc = 0;
    /* parse args */
    for (; argi < argc; ++argi)
    {
	if (!strcmp(argv[argi],"-tile") || !strcmp(argv[argi],"-cascade")) {
	  /* ignore */
	}
	else if (!strcmp(argv[argi],"-u")) {
	    untitled = 1;
	}
	else if (!strcmp(argv[argi],"-t")) {
	    transients = 1;
	}
	else if (!strcmp(argv[argi], "-a")) {
	    all = untitled = transients = maximized = 1;
	    if (FvwmCascade)
	      sticky = 1;
	}
	else if (!strcmp(argv[argi], "-r")) {
	    reversed = 1;
	}
	else if (!strcmp(argv[argi], "-noraise")) {
	    raise_window = 0;
	}
	else if (!strcmp(argv[argi], "-noresize")) {
	    resize = 0;
	}
	else if (!strcmp(argv[argi], "-nostretch")) {
	    nostretch = 1;
	}
	else if (!strcmp(argv[argi], "-desk")) {
	    desk = 1;
	}
	else if (!strcmp(argv[argi], "-flatx")) {
	    flatx = 1;
	}
	else if (!strcmp(argv[argi], "-flaty")) {
	    flaty = 1;
	}
	else if (!strcmp(argv[argi], "-r")) {
	    reversed = 1;
	}
	else if (!strcmp(argv[argi], "-h")) {
	    horizontal = 1;
	}
	else if (!strcmp(argv[argi], "-m")) {
	    maximized = 1;
	}
	else if (!strcmp(argv[argi], "-s")) {
	    sticky = 1;
	}
	else if (!strcmp(argv[argi], "-mn") && ((argi + 1) < argc)) {
	    maxnum = atoi(argv[++argi]);
	}
	else if (!strcmp(argv[argi], "-resize")) {
	    resize = 1;
	}
	else if (!strcmp(argv[argi], "-nostretch")) {
	    nostretch = 1;
	}
	else if (!strcmp(argv[argi], "-incx") && ((argi + 1) < argc)) {
	    incx = atopixel(argv[++argi], dwidth);
	}
	else if (!strcmp(argv[argi], "-incy") && ((argi + 1) < argc)) {
	    incy = atopixel(argv[++argi], dheight);
	}
	else {
	    if (++nsargc > 4) {
		fprintf(console,
			"%s: %s: ignoring unknown arg %s\n",
			argv0, s, argv[argi]);
		continue;
	    }
	    if (nsargc == 1) {
		ofsx = atopixel(argv[argi], dwidth);
	    } else if (nsargc == 2) {
		ofsy = atopixel(argv[argi], dheight);
	    } else if (nsargc == 3) {
	        if (FvwmCascade)
		  maxw = atopixel(argv[argi], dwidth);
		else /* FvwmTile */
		  maxx = atopixel(argv[argi], dwidth);
	    } else if (nsargc == 4) {
	        if (FvwmCascade)
		  maxh = atopixel(argv[argi], dheight);
		else /* FvwmTile */
		  maxy = atopixel(argv[argi], dheight);
	    }
	}
    }
}

#ifdef USERC
int parse_line(char *s, char ***args)
{
    int count = 0, i = 0;
    char *arg_save[48];
    strtok(s, " ");
    while ((s = strtok(NULL, " ")))
	arg_save[count++] = s;
    *args = (char **)safemalloc(sizeof( char * ) * count);
    for (; i < count; ++i)
	(*args)[i] = arg_save[i];
    return count;
}

#ifdef FVWM1
char *GetConfigLine(char *filename, char *match)
{
    FILE *f = fopen(filename, "r");
    if (f) {
	int l = strlen(match), found = 0;
	char line[256], *s = line;
	line[0] = 0;
	s = fgets(line, 256, f);
	while (s && !found) {
	    if (strncmp(line, match, l) == 0) {
		found = 1;
		break;
	    }
	    s = fgets(line, 256, f);
	}
	fclose(f);
	if (found) {
	    char *ret;
	    int l2 = strlen(line);
	    ret = (char *)safemalloc(sizeof(char) * l2);
	    strcpy(ret, line);
	    if (ret[l2 - 1] == '\n')
		ret[l2 - 1] = 0;
	    return ret;
	} else
	    return NULL;
    } else
	return NULL;
}
#endif /* FVWM1 */
#endif /* USERC */

void DeadPipe(int sig) { exit(0); }

int main(int argc, char *argv[])
{
#ifdef USERC
    char match[128];
    int config_line_count, len;
    char *config_line;
#endif

    console = fopen("/dev/console","w");
    if (!console) console = stderr;

    if (!(argv0 = strrchr(argv[0],'/')))
	argv0 = argv[0];
    else 
	++argv0;

    if (argc < 6) {
	fprintf(stderr,
#ifdef FVWM1
		"%s: module should be executed by fvwm only\n",
#else
		"%s: module should be executed by fvwm2 only\n",
#endif
		argv0);
	exit(-1);
    }

    fd[0] = atoi(argv[1]);
    fd[1] = atoi(argv[2]);

    if (!(dpy = XOpenDisplay(NULL))) {
	fprintf(console, "%s: couldn't open display %s\n",
		argv0,
		XDisplayName(NULL));
	exit(-1);
    }
    signal (SIGPIPE, DeadPipe);

    {
	int s = DefaultScreen(dpy);
	dwidth = DisplayWidth(dpy, s);
	dheight = DisplayHeight(dpy, s);
    }

    fd_width = GetFdWidth();
    
#ifdef USERC
    strcpy(match, "*");
    strcat(match, argv0);
    len = strlen(match);
#ifdef FVWM1
    if ((config_line = GetConfigLine(argv[3], match))) {
	char **args = NULL;
	config_line_count = parse_line(config_line, &args);
	parse_args("config args", 
		   config_line_count, args, 0);
	free(config_line);
	free(args);
    }
#else
    GetConfigLine(fd, &config_line);
    while (config_line != NULL) {
	if (strncmp(match,config_line,len)==0) {
	    char **args = NULL;
	    int cllen = strlen(config_line);
	    if (config_line[cllen - 1] == '\n')
		config_line[cllen - 1] = 0;
	    config_line_count = parse_line(config_line, &args);
	    parse_args("config args", 
		       config_line_count, args, 0);
	    free(args);
	}
	GetConfigLine(fd, &config_line);
    }
#endif /* FVWM1 */
#endif /* USERC */

    if (strcmp(argv0, "FvwmCascade") && (!strcmp(argv0, "FvwmTile") ||
	(argc >= 7 && !strcmp(argv[6], "-tile"))))
      {
	FvwmTile = 1;
	FvwmCascade = 0;
	resize = 1;
      }
    else
      {
	FvwmCascade = 1;
	FvwmTile = 0;
	resize = 0;
      }
    parse_args("module args", argc, argv, 6);

#ifdef FVWM1
    {
	char msg[256];
	sprintf(msg, "SET_MASK %lu\n",(unsigned long)(
	    M_CONFIGURE_WINDOW|
	    M_END_WINDOWLIST
	    ));
	SendInfo(fd,msg,0);
	
#ifdef FVWM1_MOVENULL
	/* avoid interactive placement in fvwm version 1 */
	if (!ofsx) ++ofsx;
	if (!ofsy) ++ofsy;
#endif
    }
#else
    SetMessageMask(fd,
		   M_CONFIGURE_WINDOW 
		   | M_END_WINDOWLIST
	);
#endif

    if (FvwmTile)
    {
      if (!maxx) maxx = dwidth;
      if (!maxy) maxy = dheight;
    }

    SendInfo(fd,"Send_WindowList",0);
    while (get_window());
    if (wins_count)
    {
      if (FvwmCascade)
	cascade_windows();
      else /* FvwmTile */
	tile_windows();
    }
    free_window_list(&wins);
    if (console != stderr)
	fclose(console);
    return 0;
}
