/* -*-c-*- */
/* Copyright (C) 2002 the late Joey Shutup.
 *
 * http://www.streetmap.co.uk/streetmap.dll?postcode2map?BS24+9TZ
 *
 * No guarantees or warranties or anything are provided or implied in any way
 * whatsoever.  Use this program at your own risk.  Permission to use this
 * program for any purpose is given, as long as the copyright is kept intact.
 */
/*
 * This program is free software; you can redistribute it and/or modify
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include "config.h"

#include <stdio.h>
#include <signal.h>

#include "libs/fvwmlib.h"
#include "libs/Module.h"
#include "libs/fvwmsignal.h"

/* Globals */
static char *name;
static int namelen;
static int fd[2];                       /* communication pipes */

/* forward declarations */
static RETSIGTYPE signal_handler(int signal);
static void set_signals(void);
static void parse_config(void);
static void parse_config_line(char *line);
static void parse_message_line(char *line);
static void main_loop(void) __attribute__((__noreturn__));
static void parse_colorset(char *line);

int main(int argc, char **argv)
{
  name = GetFileNameFromPath(argv[0]);
  namelen = strlen(name);

  fprintf(stderr,
	  "%s is obsolete, see the Colorset section of the fvwm(1) man page\n",
	  name);
  set_signals();

  /* try to make sure it is fvwm that spawned this module */
  if (argc != 6) {
    fprintf(stderr, "%s "VERSION" should only be executed by fvwm!\n", name);
    exit(1);
  }

  /* note the communication pipes */
  fd[0] = atoi(argv[1]);
  fd[1] = atoi(argv[2]);

  /* get the initial configuration options */
  parse_config();

  /* tell fvwm we're running */
  SendFinishedStartupNotification(fd);

  /* sit around waiting for something to do */
  main_loop();
}

static void main_loop(void)
{
  FvwmPacket *packet;
  fd_set_size_t fd_width;
  fd_set in_fdset;

  fd_width = fd[1] + 1;
  FD_ZERO(&in_fdset);

  while (True) {
    /* garbage collect */
    alloca(0);

    FD_SET(fd[1], &in_fdset);
    /* wait for an instruction from fvwm or a timeout */
    if (fvwmSelect(fd_width, &in_fdset, NULL, NULL, NULL) < 0)
    {
      fprintf(stderr, "%s: select error!\n", name);
      exit(-1);
    }

    packet = ReadFvwmPacket(fd[1]);
    if (!packet)
      exit(0);

    /* handle dynamic config lines and text messages */
    switch (packet->type) {
    case M_CONFIG_INFO:
      parse_config_line((char *)&packet->body[3]);
      SendUnlockNotification(fd);
      break;
    case M_STRING:
      parse_message_line((char *)&packet->body[3]);
      SendUnlockNotification(fd);
      break;
    }
  }
}

/* config options, the first NULL is replaced with *FvwmThemeColorset */
/* the second NULL is replaced with *FvwmThemeReadWriteColors */
/* FvwmTheme ignores any "colorset" lines since it causes them */
static char *config_options[] =
{
  "ImagePath",
  "ColorLimit",
  NULL,
  NULL,
  NULL
};

static void parse_config_line(char *line)
{
  char *rest;

  switch(GetTokenIndex(line, config_options, -1, &rest))
  {
  case 0: /* ImagePath */
    break;
  case 1: /* ColorLimit */
    break;
  case 2: /* *FvwmThemColorset */
    parse_colorset(rest);
    break;
  case 3: /* *FvwmThemeReadWriteColors */
    SendText(fd, "ReadWriteColors", 0);
    break;
  }
}

/* translate a colorset spec into a colorset structure */
static void parse_colorset(char *line)
{
  SendText(fd, CatString2("Colorset ", line), 0);
}

/* SendToModule options */
static char *message_options[] = {"Colorset", NULL};

static void parse_message_line(char *line)
{
  char *rest;

  switch(GetTokenIndex(line, message_options, -1, &rest)) {
  case 0:
    parse_colorset(rest);
    break;
  }
}

static void parse_config(void)
{
  char *line;

  /* prepare the tokenizer array, [0,1] are ImagePath and ColorLimit */
  config_options[2] = safemalloc(namelen + 10);
  sprintf(config_options[2], "*%sColorset", name);
  config_options[3] = safemalloc(namelen + 17);
  sprintf(config_options[3], "*%sReadWriteColors", name);

  /* set a filter on the config lines sent */
  line = safemalloc(namelen + 2);
  sprintf(line, "*%s", name);
  InitGetConfigLine(fd, line);
  free(line);

  /* tell fvwm what we want to receive */
  SetMessageMask(fd, M_CONFIG_INFO | M_END_CONFIG_INFO | M_SENDCONFIG
		 | M_STRING);

  /* loop on config lines, a bit strange looking because GetConfigLine
   * is a void(), have to test $line */
  while (GetConfigLine(fd, &line), line)
    parse_config_line(line);

  SetSyncMask(fd, M_CONFIG_INFO | M_STRING);
}

static void set_signals(void) {
#ifdef HAVE_SIGACTION
  struct sigaction  sigact;

  sigemptyset(&sigact.sa_mask);
  sigaddset(&sigact.sa_mask, SIGPIPE);
  sigaddset(&sigact.sa_mask, SIGINT);
  sigaddset(&sigact.sa_mask, SIGHUP);
  sigaddset(&sigact.sa_mask, SIGTERM);
# ifdef SA_INTERRUPT
  sigact.sa_flags = SA_INTERRUPT;
# else
  sigact.sa_flags = 0;
# endif
  sigact.sa_handler = signal_handler;
  sigaction(SIGPIPE, &sigact, NULL);
  sigaction(SIGINT, &sigact, NULL);
  sigaction(SIGHUP, &sigact, NULL);
  sigaction(SIGTERM, &sigact, NULL);
#else
  /* We don't have sigaction(), so fall back to less robust methods.  */
#ifdef USE_BSD_SIGNALS
  fvwmSetSignalMask(sigmask(SIGPIPE)
		    | sigmask(SIGINT)
		    | sigmask(SIGHUP)
		    | sigmask(SIGTERM));
#endif
  signal(SIGPIPE, signal_handler);
  signal(SIGINT, signal_handler);
  signal(SIGHUP, signal_handler);
  signal(SIGTERM, signal_handler);
#ifdef HAVE_SIGINTERRUPT
  siginterrupt(SIGPIPE, 1);
  siginterrupt(SIGINT, 1);
  siginterrupt(SIGHUP, 1);
  siginterrupt(SIGTERM, 1);
#endif
#endif
}

static RETSIGTYPE signal_handler(int signal) {
  fprintf(stderr, "%s quiting on signal %d\n", name, signal);
  exit(signal);
}
