/*
 * *************************************************************************
 * This module is all original code
 * by Rob Nation
 * Copyright 1993, Robert Nation
 *     You may use this code for any purpose, as long as the original
 *     copyright remains in the source code and all documentation
 *
 * Changed 09/24/98 by Dan Espen:
 * - remove logic that processed and saved module configuration commands.
 * Its now in "modconf.c".
 * *************************************************************************
 */
#include "config.h"

#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>

#include "fvwm.h"
#include "misc.h"
#include "parse.h"
#include "screen.h"
#include "module.h"

extern Boolean debugging;

char *fvwm_file = NULL;

int numfilesread = 0;

static int last_read_failed=0;

static const char *read_system_rc_cmd="Read system"FVWMRC;


/*
 * func to do actual read/piperead work
 * Arg 1 is file name to read.
 *   If the filename starts with "/", just read it.
 *   Otherwise we need the user home directory which is either
 *   "." or ".fvwm/".
 *   If the file starts with a dot, and  no ".fvwm" dir, read
 *     .x, sysconfdir/x.
 *   If the file starts with a dot, and  a ".fvwm" dir, read
 *     .fvwm/x, sysconfdir/x, .x.
 *   If the file doesn't start with a dot, and no ".fvwm" dir, read
 *     x, sysconfdir/x.
 *   If the file doesn't start with a dot, and a ".fvwm" dir, read
 *     .fvwm/x, sysconfdir/x.
 *
 * Arg 2 (optional) "Quiet" to suppress message on missing file.
 */
static void ReadSubFunc(XEvent *eventp,Window junk,FvwmWindow *tmp_win,
                        unsigned long context, char *action,int* Module,
                        int piperead)
{
  char *filename= NULL,*Home, *home_file, *ofilename = NULL;
  char *option;                         /* optional arg to read */
  char *rest,*tline,line[1024];
  FILE *fd;
  int thisfileno;
  int dot_flipper;
  char missing_quiet;                   /* missing file msg control */
  char *cmdname;

  /* domivogt (30-Dec-1998: I tried using conditional evaluation instead
   * of the cmdname variable ( piperead?"PipeRead":"Read" ), but gcc seems
   * to treat this expression as a pointer to a character pointer, not just
   * as a character pointer, but it doesn't complain either. Or perhaps
   * insure++ gets this wrong? */
  if (piperead)
    cmdname = "PipeRead";
  else
    cmdname = "Read";

  thisfileno = numfilesread;
  numfilesread++;
  dot_flipper = 0;                      /* init */

/*  fvwm_msg(INFO,cmdname,"action == '%s'",action); */

  rest = GetNextToken(action,&ofilename); /* read file name arg */
  if(ofilename == NULL)
  {
    fvwm_msg(ERR, cmdname,"missing parameter");
    last_read_failed = 1;
    return;
  }
  missing_quiet='n';                    /* init */
  rest = GetNextToken(rest,&option);    /* read optional arg */
  if (option != NULL) {                 /* if there is a second arg */
    if (strncasecmp(option,"Quiet",5)==0) { /* is the arg "quiet"? */
      missing_quiet='y';                /* no missing file message wanted */
    } /* end quiet arg */
    free(option);                       /* arg not needed after this */
  } /* end there is a second arg */

  filename = ofilename;

/*  fvwm_msg(INFO, cmdname,"trying '%s'",filename); */

  if (piperead) {                       /* if pipe */
    fd = popen(filename,"r");           /* popen */
  } else if (ofilename[0] == '/') {     /* if absolute path */
    fd = fopen(filename,"r");           /* fopen the name given */
  } else {                              /* else its a relative path */
    Home = user_home_ptr;               /* try user home first */
    home_file = safemalloc(strlen(Home) + strlen(ofilename)+3);
    strcpy(home_file,Home);
    if (ofilename[0] == '.') {          /* if filename already has dot */
      dot_flipper = 1;                  /* don't add another one */
    }
    strcat(home_file,ofilename+dot_flipper);
    filename = home_file;
    fd = fopen(filename,"r");
    if(fd == 0) {                       /* if not in users home */
      if((filename != NULL)&&(filename!= ofilename))
          free(filename);
      Home = FVWM_CONFIGDIR;          /* where fvwm is installed */
      home_file = safemalloc(strlen(Home) + strlen(ofilename)+3);
      strcpy(home_file,Home);
      strcat(home_file,"/");
      strcat(home_file,ofilename+dot_flipper);
      filename = home_file;
      fd = fopen(filename,"r");
      /* This next bit is for maximum backward compatibility */
      if (fd == 0 &&                    /* file still not found */
          dot_flipper == 1  &&          /* and we are dot flipping */
          missing_quiet == 'n'  &&      /* and its not a quiet read */
          strcmp(user_home_ptr, ".") != 0) { /* and using a subdirectory */
        fd = fopen(ofilename,"r");      /* try the actual name as is */
      } /* end try .x */
    } /* end, not in users home dir */
  }

  if(fd == NULL)
  {
    if (missing_quiet == 'n') {         /* if quiet option not on */
      if (piperead)
	fvwm_msg(ERR, cmdname, "command '%s' not run", ofilename);
      else if (ofilename[0] == '/')
	fvwm_msg(ERR, cmdname,
		 "file '%s' not found.", filename);
      else
	fvwm_msg(ERR, cmdname,
		 "file '%s' not found, looked for %s%s and %s", 
                 ofilename+dot_flipper,user_home_ptr,ofilename,filename);
    } /* end quiet option not on */
    if((ofilename != filename)&&(filename != NULL))
    {
      free(filename);
    }
    if(ofilename != NULL)
    {
      free(ofilename);
    }
    last_read_failed = 1;
    return;
  }
  if((ofilename != NULL)&&(filename!= ofilename))
    free(ofilename);
  fcntl(fileno(fd), F_SETFD, 1);
  if (!piperead)
  {
    if(fvwm_file != NULL)
      free(fvwm_file);
    fvwm_file = filename;
  }
  else
  {
    if (filename)
      free(filename);
  }

  tline = fgets(line,(sizeof line)-1,fd);
  while(tline)
  {
    int l;
    while(tline && (l = strlen(line)) < sizeof(line) && l >= 2 &&
          line[l-2]=='\\' && line[l-1]=='\n')
    {
      tline = fgets(line+l-2,sizeof(line)-l+1,fd);
    }
    tline=line;
    while(isspace(*tline))
      tline++;
    if (debugging)
    {
      fvwm_msg(DBG,"ReadSubFunc","about to exec: '%s'",tline);
    }
    ExecuteFunction(tline,tmp_win,eventp,context,*Module);
    tline = fgets(line,(sizeof line)-1,fd);
  }

  if (piperead)
    pclose(fd);
  else
    fclose(fd);
  last_read_failed = 0;
}

void ReadFile(F_CMD_ARGS)
{
  int this_read = numfilesread;

  if (debugging)
  {
    fvwm_msg(DBG,"ReadFile","about to attempt '%s'",action);
  }

  ReadSubFunc(eventp,w,tmp_win,context,action,Module,0);

  if (fFvwmInStartup && last_read_failed && this_read == 0)
  {
    fvwm_msg(INFO,"Read","trying to read system rc file");
    ExecuteFunction((char *)read_system_rc_cmd,NULL,&Event,C_ROOT,-1);
  }
}

void PipeRead(F_CMD_ARGS)
{
  int this_read = numfilesread;

  if (debugging)
  {
    fvwm_msg(DBG,"PipeRead","about to attempt '%s'",action);
  }

  ReadSubFunc(eventp,w,tmp_win,context,action,Module,1);

  if (fFvwmInStartup && last_read_failed && this_read == 0)
  {
    fvwm_msg(INFO,"PipeRead","trying to read system rc file");
    ExecuteFunction((char *)read_system_rc_cmd,NULL,&Event,C_ROOT,-1);
  }
}

