/*----------------------------------------------------------------------------------------------------------------------------------
    Module    : vms.c for Fvwm on Open VMS
    Author    : Fabien Villard (Villard_F@Decus.Fr)
    Date      : 5-JAN-1999
    Action    : special functions for running Fvwm with OpenVms. They can be helpfull for other VMS ports.
    Functions :
        internal functions : AttentionAST() and TimerAST()
        exported functions : VMS_msg(), VMS_SplitCommand() and VMS_select_pipes()
    Copyright : Fabien Villard, 1999.  No guarantees or warantees are provided or implied in any way whatsoever. Use this program
                at your own risk.  Permission to use, modify, and redistribute this program is hereby given, provided that this
                copyright is kept intact.
----------------------------------------------------------------------------------------------------------------------------------*/
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <stdarg.h>
#include <unixio.h>

#include "vms.h"
#include <starlet.h>
#include <descrip.h>
#include <lib$routines.h>
#include <libdtdef.h>
#include <ssdef.h>
#include <iodef.h>

/* --- Define to debug VMS_select_pipes() routine --- */
#undef DEBUG_SELECT

/* --- Define to debug VMS_SplitCommand() routine --- */
#define DEBUG_SPLIT

/* --- Define this to post a read attention AST on a write pipe in VMS_select_pipes(). --- */
#undef POST_READATTN_AST

/* --- Define to debug VMS_ExecL() routine --- */
#define DEBUG_EXECL
  
/* --- Define to close or sync output file after each line in VMS_msg() --- */
#undef DEBUG_CLOSE_OUTPUT
#undef DEBUG_SYNC_OUTPUT

#ifdef DEBUG_SELECT
  #define DBG_SEL 1
#else
  #define DBG_SEL 0
#endif

#ifdef DEBUG_SPLIT
  #define DBG_SPL 1
#else
  #define DBG_SPL 0
#endif

#ifdef DEBUG_EXECL
  #define DBG_EXL 1
#else
  #define DBG_EXL 0
#endif

#define SELECT_ERROR -1
#define SELECT_TIMEOUT 0
#define MAX_DEVICE 32                       /* - Maximum FD to check - */
#define MAX_DEVICENAME 256

#define DEV_UNUSED 0
#define DEV_READPIPE 1
#define DEV_WRITEPIPE 2
#define DEV_EFN 3

/* --- File descriptors information. We allocate statically for it's a very oten used function. --- */
typedef struct {
  int eventFlag;                           /* - Event flag pour les attentes passives - */
  unsigned short vmsChannel;               /* - Canal VMS sur la mailbox - */
  char deviceName[MAX_DEVICENAME];         /* - Nom du device (la mailbox ou les X events) - */
  char devType;                            /* - Type de device : DEV_xxx - */
  char qioPending;                         /* - Une QIO est en attente (1) ou non (0) - */
} Pipe_Infos_Type;

typedef struct {
  short condVal;
  short retCount;
  int devInfo;
} Iosb_Type;

/* --- Global variables. I do not like them, but, because of the AST it's the easiest way :-) They have to be volatile because
       they are modified by AST and main function. --- */
volatile unsigned int SpecialEfn;          /* - One ef for all. This to avoid multi ef clusters problem - */
volatile char SpecialEfnToClear;           /* - Special ef is set by us - */

/*----------------------------------------------------------------------------------------------------------------------------------
    Fonctions : AttentionAST et TimedAST
      Actions : attention AST : activated by the attention QIO (ReadAttention or WriteAttention) on the pipe's mailbox. It gets
                the ef to set in the AstPrm.
                Timed AST : activated when time has come.
                This two functions set also the special ef on which we then waitfr. This is to avoid multi ef clusters problem.
----------------------------------------------------------------------------------------------------------------------------------*/
void AttentionAST(int efn) {
  unsigned int sysStat, efnState;
  sysStat = sys$setef(efn);
  if (! (sysStat & 1)) fvwm_msg(ERR, "AttentionAST", "<Asynchrone> Error setting efn %d (%d)", efn, sysStat);
  else if (DBG_SEL) fvwm_msg(DBG, "AttentionAST", "<Asynchrone> Efn %d set.", efn);
  if (0 < SpecialEfn) {
    sysStat = sys$setef(SpecialEfn);
    if (! (sysStat & 1)) fvwm_msg(ERR, "AttentionAST", "<Asynchrone> Error setting special efn %d (%d)", SpecialEfn, sysStat);
    else if (DBG_SEL) fvwm_msg(DBG, "AttentionAST", "<Asynchrone> Special efn %d set. We'll have to clear it.", SpecialEfn);
    SpecialEfnToClear = 1;
  }
}
void TimedAST(int filler) {
  unsigned int sysStat;
  if (0 < SpecialEfn) {
    sysStat = sys$setef(SpecialEfn);
    if (! (sysStat & 1)) fvwm_msg(ERR, "TimedAST", "<Asynchrone> Error setting special efn %d (%d)", SpecialEfn, sysStat);
    else if (DBG_SEL) fvwm_msg(DBG, "TimedAST", "<Asynchrone> Special efn %d set. We'll have to clear it.", SpecialEfn);
    SpecialEfnToClear = 1;
  }
}

#define VMS_TestStatus(stat, message, degage) {\
  if (! (stat & 1)) {\
    if (DBG_SEL) fvwm_msg(ERR, "VMS_select_pipes", message, stat);\
    if (degage) {\
      result = SELECT_ERROR;\
      goto End_VMS_select_pipes;\
    }\
  }\
}

/*----------------------------------------------------------------------------------------------------------------------------------
    Fonction : VMS_select_pipes
      Action : replacement for select() for pipes and other devices (such as X communications) with an ef already assigned.
               Prototype is identical to select().
----------------------------------------------------------------------------------------------------------------------------------*/
int VMS_select_pipes(int nbPipes, fd_set *readFds, fd_set *writeFds, fd_set *filler, struct timeval *timeoutVal) {
  int iFd, result, nbPipesReady, sysStat, libStat, timeoutEf;
  char *retPtr;
  unsigned short vmsChannel;
  unsigned int efnState, qioFuncCode;
  struct dsc$descriptor_s deviceNameDesc;
  Pipe_Infos_Type infosPipes[MAX_DEVICE];
  Iosb_Type qioIosb;
  struct { unsigned int l0, l1; } timeoutBin;
  char specialEfnToFree;

  if (DBG_SEL) fvwm_msg(DBG, "VMS_select_pipes", "********************************************************************************");

  SpecialEfn = -1;
  specialEfnToFree = SpecialEfnToClear = 0;
  nbPipesReady = result = 0;
  deviceNameDesc.dsc$b_dtype = DSC$K_DTYPE_T;
  deviceNameDesc.dsc$b_class = DSC$K_CLASS_S;

  /* --- Too much pipes : trace and truncat. --- */
  if (MAX_DEVICE < nbPipes) {
    if (DBG_SEL) fvwm_msg(ERR, "VMS_select_pipes", "Too much pipes (%d instead of %d max), reduced.", nbPipes, MAX_DEVICE);
    nbPipes = MAX_DEVICE;
  }

  /* --- Pre-work of selected fd to get their types and search special ef. --- */
  if (NULL != readFds || NULL != writeFds) {
    for (iFd = 0; iFd < nbPipes; iFd++) {
      infosPipes[iFd].devType = DEV_UNUSED;
      infosPipes[iFd].qioPending = 0;
      infosPipes[iFd].vmsChannel = 0;
      if ( (NULL != readFds && FD_ISSET(iFd, readFds)) || (NULL != writeFds && FD_ISSET(iFd, writeFds)) ) {
        if (FD_ISSET(iFd, readFds)) infosPipes[iFd].devType = DEV_READPIPE;
        else if (FD_ISSET(iFd, writeFds)) infosPipes[iFd].devType = DEV_WRITEPIPE;
        /* --- Get VMS mailbox name. This function is VMS specific. --- */
        retPtr = getname(iFd, infosPipes[iFd].deviceName);
        /* --- If we fail, it's probably because the FD is an EF. --- */
        if (NULL == retPtr) {
          if (DBG_SEL) fvwm_msg(DBG, "VMS_select_pipes", "Special efn trouve : %d", iFd);
          infosPipes[iFd].devType = DEV_EFN;
          SpecialEfn = iFd;
          }
        else {
          /* --- Trying to connect to the mailbox --- */
          deviceNameDesc.dsc$w_length = strlen(infosPipes[iFd].deviceName);
          deviceNameDesc.dsc$a_pointer = infosPipes[iFd].deviceName;
          sysStat = sys$assign(&deviceNameDesc, &(infosPipes[iFd].vmsChannel), 0, NULL, 0);
          VMS_TestStatus(sysStat, "Error : sys$assign returns %d", 1);
        }
      }
    }

    /* --- No special ef found. Create one. Then in both cases, clear it. --- */
    if (-1 == SpecialEfn) {
      specialEfnToFree = 1;
      libStat = lib$get_ef(&SpecialEfn);
      VMS_TestStatus(libStat, "No event flags left (Error %d)", 1);
      if (DBG_SEL) fvwm_msg(DBG, "VMS_select_pipes", "Special efn created : %d", SpecialEfn);
    }
    sysStat = sys$clref(SpecialEfn);
    VMS_TestStatus(sysStat, "Can't clear special event flag (Error %d)", 1);

    /* --- DO the real work on fd --- */
    for (iFd = 0; iFd < nbPipes; iFd++) {
      if (DEV_READPIPE == infosPipes[iFd].devType || DEV_WRITEPIPE == infosPipes[iFd].devType) {
        /* --- Allocate and initialize an ef --- */
        libStat = lib$get_ef(&(infosPipes[iFd].eventFlag));
        VMS_TestStatus(libStat, "No more event flags (Error %d)", 1);
        sysStat = sys$clref(infosPipes[iFd].eventFlag);
        VMS_TestStatus(sysStat, "Can't clear ef (err %d)", 1);
        if (DBG_SEL) {
          fvwm_msg(DBG, "VMS_select_pipes", "FD %2d  %s  Nom <%s>  Canal %d  Efn %d cleared", iFd,
                   (DEV_READPIPE == infosPipes[iFd].devType)?"READ":(DEV_WRITEPIPE == infosPipes[iFd].devType)?"WRIT":"UNKN",
                   infosPipes[iFd].deviceName, infosPipes[iFd].vmsChannel, infosPipes[iFd].eventFlag);
        }

        /* --- Read pipe, we wait for a writer to... write. --- */
        if (FD_ISSET(iFd, readFds)) {
          qioFuncCode = IO$_SETMODE | IO$M_WRTATTN;
          sysStat = sys$qio(0, infosPipes[iFd].vmsChannel, qioFuncCode, &qioIosb,
                            NULL, 0, AttentionAST, infosPipes[iFd].eventFlag, 0, 0, 0, 0);
          VMS_TestStatus(qioIosb.condVal, "Attention QIO failed (Secondary error %d)", 0);
          VMS_TestStatus(sysStat, "Attention QIO failed (Error %d)", 1);
          infosPipes[iFd].qioPending = 1;
          if (DBG_SEL) fvwm_msg(DBG, "VMS_select_pipes", "Attention QIO ok. qioIosb.condVal = %d (%x)", qioIosb.condVal, qioIosb.condVal);
          }

        /* --- Write pipe. Sometimes we can wait for a reader to post a read request. But when the process is not symetrical
               we may stay blocked here. So, better consider pipe is always ready. --- */
        else if (FD_ISSET(iFd, writeFds)) {
          #ifdef POST_READATTN_AST
            qioFuncCode = IO$_SETMODE | IO$M_READATTN;
            sysStat = sys$qio(0, infosPipes[iFd].vmsChannel, qioFuncCode, &qioIosb,
                              NULL, 0, AttentionAST, infosPipes[iFd].eventFlag, 0, 0, 0, 0);
            VMS_TestStatus(qioIosb.condVal, "Read attention QIO failed (Secondary error %d)", 0);
            VMS_TestStatus(sysStat, "Read attention QIO failed (Error %d)", 1);
            infosPipes[iFd].qioPending = 1;
            if (DBG_SEL) fvwm_msg(DBG, "VMS_select_pipes", "Read attention QIO ok. qioIosb.condVal = %d (%x)", qioIosb.condVal, qioIosb.condVal);
          #else
            AttentionAST(infosPipes[iFd].eventFlag);
            if (DBG_SEL) fvwm_msg(DBG, "VMS_select_pipes", "Attention automatique");
          #endif
        }
        }

      /* --- Not a pipe. It's probably an ef already allocated elsewhere, by us for example. --- */
      else if (DEV_EFN == infosPipes[iFd].devType) {
        infosPipes[iFd].qioPending = 0;
        strcpy(infosPipes[iFd].deviceName, "Device_non_pipe");
        infosPipes[iFd].vmsChannel = 0;
        infosPipes[iFd].eventFlag = iFd;
        /* --- We clear this special ef only if we own it. In the other case, the owner had coped with that. --- */
        if (iFd != SpecialEfn) {
          sysStat = sys$clref(infosPipes[iFd].eventFlag);
          VMS_TestStatus(sysStat, "Can't clear ef (Error %d)", 1);
        }
        if (DBG_SEL) fvwm_msg(DBG, "VMS_select_pipes", "FD %2d EFN   Nom <%s>  Canal %d  Efn %d", iFd, infosPipes[iFd].deviceName,
                            infosPipes[iFd].vmsChannel, infosPipes[iFd].eventFlag);
      }
    }
  }

  /* --- Time out event --- */
  if (NULL != timeoutVal) {
    float timeoutFloat;
    unsigned int libCvtFunc = LIB$K_DELTA_SECONDS_F;
    libStat = lib$get_ef(&timeoutEf);
    VMS_TestStatus(libStat, "No event flags left (Error %d)", 1);
    if (0 != timeoutVal->tv_sec || 0 != timeoutVal->tv_usec) {
      sysStat = sys$clref(timeoutEf);
      VMS_TestStatus(sysStat, "Can't clear timer ef (Error %d)", 1);
      timeoutFloat = (float)timeoutVal->tv_sec + (float)timeoutVal->tv_usec / 1000000.0;
      if (DBG_SEL) fvwm_msg(DBG, "VMS_select_pipes", "Timer %f, %d sec %d usec", timeoutFloat, timeoutVal->tv_sec, timeoutVal->tv_usec);
      libStat = lib$cvtf_to_internal_time(&libCvtFunc, &timeoutFloat, &timeoutBin);
      VMS_TestStatus(libStat, "Can't convert time in float (Error %d)", 1);
      sysStat = sys$setimr(timeoutEf, &timeoutBin, TimedAST, 0, 0);
      VMS_TestStatus(sysStat, "Can't post the timed AST (Error %d)", 1);
      if (DBG_SEL) fvwm_msg(DBG, "VMS_select_pipes", "Timed AST with ef %d in %f s", timeoutEf, timeoutFloat);
      }
    /* --- Null time out --- */
    else {
      sysStat = sys$clref(timeoutEf);
      VMS_TestStatus(sysStat, "Can't clear (timeout = 0) timer ef (Error %d)", 1);
    }
  }

  /* --- Wait for one of all those events :-) --- */
  if (DBG_SEL) fvwm_msg(DBG, "VMS_select_pipes", "Waiting events...");
  sysStat = sys$waitfr(SpecialEfn);
  VMS_TestStatus(sysStat, "Wait events request failed (Error %d)", 1);
  if (DBG_SEL) fvwm_msg(DBG, "VMS_select_pipes", "One event happened.", time(NULL));

  /* --- Some events did come --- */
  if (NULL != readFds) FD_ZERO(readFds);
  if (NULL != writeFds) FD_ZERO(writeFds);

  /* --- Time out --- */
  if (NULL != timeoutVal) {
    sysStat = sys$readef(timeoutEf, &efnState);
    VMS_TestStatus(sysStat, "Can't read timeout ef state (Error %d)", 1);
    if (SS$_WASSET == sysStat) {
      timeoutVal->tv_sec = 0;
      timeoutVal->tv_usec = 0;
      result = SELECT_TIMEOUT;
      goto End_VMS_select_pipes;
      }
    /* --- Cancel timeout --- */
    else {
      sysStat = sys$cantim(0, 0);
      VMS_TestStatus(sysStat, "Can't cancel timer request (Error %d)", 0);
    }
  }
    
  /* --- Checking other events --- */
  for (iFd = 0; iFd < nbPipes; iFd++) {
    if (DEV_UNUSED != infosPipes[iFd].devType) {
      sysStat = sys$readef(infosPipes[iFd].eventFlag, &efnState);
      VMS_TestStatus(sysStat, "Can't read ef state (Error %d)", 1);
      if (SS$_WASSET == sysStat) {
        nbPipesReady++;
        infosPipes[iFd].qioPending = 0;
        if (DBG_SEL) fvwm_msg(DBG, "VMS_select_pipes", "Event flag %d set !", infosPipes[iFd].eventFlag);
        if (DEV_READPIPE == infosPipes[iFd].devType) FD_SET(iFd, readFds);
        else if (DEV_WRITEPIPE == infosPipes[iFd].devType) FD_SET(iFd, writeFds);
        else FD_SET(iFd, readFds);
      }
    }
  }

  /* --- Don't give false informations : if we set the special event and we don't own it, don't say it. --- */
  /*
  if (! specialEfnToFree && SpecialEfnToClear) {
    nbPipesReady--;
    if (FD_ISSET(SpecialEfn, readFds))  FD_CLR(SpecialEfn, readFds);
    else if (FD_ISSET(SpecialEfn, writeFds)) FD_CLR(SpecialEfn, writeFds);
  }
  */

  result = nbPipesReady;
  if (DBG_SEL) fvwm_msg(DBG, "VMS_select_pipes", "Nombre de pipes prets : %d", nbPipesReady);

End_VMS_select_pipes:;
  /* --- Clear resources --- */
  for (iFd = 0; iFd < nbPipes; iFd++) {
    if (DEV_UNUSED != infosPipes[iFd].devType) {
      if (1 == infosPipes[iFd].qioPending) {
        sysStat = sys$cancel(infosPipes[iFd].vmsChannel);
        VMS_TestStatus(sysStat, "Can't cancel QIO (Error %d)", 0);
      }
      if (0 != infosPipes[iFd].vmsChannel) {
        sysStat = sys$dassgn(infosPipes[iFd].vmsChannel);
        VMS_TestStatus(sysStat, "Can't deallocate channel (Error %d)", 0);
      }
      if (DEV_EFN != infosPipes[iFd].devType) {
        libStat = lib$free_ef(&(infosPipes[iFd].eventFlag));
        VMS_TestStatus(libStat, "Can't free ef (Error %d)", 0);
      }
    }
  }
  if (specialEfnToFree) {
    libStat = lib$free_ef(&SpecialEfn);
    VMS_TestStatus(libStat, "Can't free special ef (Error %d)", 0);
  }
  if (NULL != timeoutVal) {
    libStat = lib$free_ef(&timeoutEf);
    VMS_TestStatus(libStat, "Can't free timer ef (Error %d)", 0);
  }

  if (DBG_SEL) fvwm_msg(DBG, "VMS_select_pipes", "********************************************************************************");
  return(result);
}

/*----------------------------------------------------------------------------------------------------------------------------------
    Function : VMS_msg
      Action : replacement for fvwm_msg
----------------------------------------------------------------------------------------------------------------------------------*/
char FabName[] = "Sys$Login:Fvwm.log";

void VMS_msg(int type,char *id,char *msg,...) {
  char *typestr;
  va_list args;
  char buffer1[1024], buffer2[1024];
  static FILE *fabFD = NULL;
  static int fabNew = 1;

  switch(type) {
    case DBG: typestr="Dbg"; break;
    case ERR: typestr="Err"; break;
    case WARN: typestr="Wrn"; break;
    case INFO:
    default:
      typestr="";
      break;
  }

  va_start(args,msg);

  sprintf(buffer1,"[FVWM-%d/%d-%s-%s] ", time(NULL), clock(), typestr, id);
  vsprintf(buffer2, msg, args);
  strcat(buffer1, buffer2);

  if (NULL == fabFD) {
    if (1 == fabNew) {
      fabFD = fopen(FabName, "w");
      fabNew = 0;
      }
    else fabFD = fopen(FabName, "a");
  }

  if (NULL != fabFD) {
    fprintf(fabFD, "%s\n", buffer1);
    #ifdef DEBUG_SYNC_OUTPUT
      fsync(fileno(fabFD));
    #endif
    #ifdef DEBUG_CLOSE_OUTPUT
      fclose(fabFD);
      fabFD = NULL;
    #endif
    }
  else fprintf(stderr, "FAB-ERROR : file %s not opened <%s>", FabName, strerror(errno));
  va_end(args);
}

/*----------------------------------------------------------------------------------------------------------------------------------
    Function : VMS_SplitCommand
      Action : split command string in an array of string pointers to words of command. Array is passed. Each pointer is
               freed if necessary. The pointer after the last used argument pointer is set to NULL.
----------------------------------------------------------------------------------------------------------------------------------*/
void VMS_SplitCommand(
    char *cmd,                                          /* - Command to split - */
    char **argums,                                      /* - Array of arguments - */
    int maxArgums,                                      /* - Size of the array of argumets - */
    int *nbArgums) {                                    /* - Number of arguments created - */
  int iArgum, argumLen;
  char *tmpChar, *tmpArgum;

  if (DBG_SPL) fvwm_msg(DBG,"VMS_SplitCommand","Splitting <%s>", cmd);
  tmpChar = tmpArgum = cmd;
  iArgum = 0; argumLen = 0;

  /* --- Split command in arguments --- */
  while (0 != *tmpChar) {
    if (' ' == *tmpChar) {
      if (DBG_SPL) fvwm_msg(DBG,"VMS_SplitCommand","End word...");
      if (0 < argumLen) {
        if (NULL != argums[iArgum]) free(argums[iArgum]);
        argums[iArgum] = (char *)malloc(argumLen + 1);
        if (NULL == argums[iArgum]) {
          VMS_msg(ERR, "VMS_SplitCommand", "Can't allocate memory");
          exit(0);
        }
        strncpy(argums[iArgum], tmpArgum, argumLen);
        argums[iArgum][argumLen] = 0;
        if (DBG_SPL) fvwm_msg(DBG,"VMS_SplitCommand","New word <%s>, Len: %d", argums[iArgum], strlen(argums[iArgum]));
        iArgum++;
        tmpArgum = tmpChar + 1;
        argumLen = 0;
      }
      }
    else argumLen++;
    tmpChar++;
  }

  /* --- Get last argument --- */
  if (0 < argumLen) {
    argums[iArgum] = (char *)malloc(argumLen + 1);
    if (NULL == argums[iArgum]) {
      VMS_msg(ERR, "VMS_SplitCommand", "Can't allocate memory");
      exit(0);
    }
    strncpy(argums[iArgum], tmpArgum, argumLen);
    argums[iArgum][argumLen] = 0;
    if (DBG_SPL) fvwm_msg(DBG,"VMS_SplitCommand","Last word <%s>, Len: %d", argums[iArgum], strlen(argums[iArgum]));
    iArgum++;
  }

  argums[iArgum] = NULL;
  *nbArgums = iArgum;

  /* --- Remove quotes in each argument --- */
  for (iArgum = 0; iArgum < *nbArgums; iArgum++) {
    if ('"' == argums[iArgum][0]) {
      tmpChar = (char *)malloc(strlen(argums[iArgum]));
      if (NULL == tmpChar) {
        VMS_msg(ERR, "VMS_SplitCommand", "Can't allocate memory");
        exit(0);
      }
      strncpy(tmpChar, argums[iArgum] + 1, strlen(argums[iArgum]) - 1);
      tmpChar[strlen(argums[iArgum]) - 1] = 0;
      free(argums[iArgum]);
      argums[iArgum] = tmpChar;
    }
    if ('"' == argums[iArgum][strlen(argums[iArgum]) - 1]) argums[iArgum][strlen(argums[iArgum]) - 1] = 0;
    if (DBG_SPL) fvwm_msg(DBG,"VMS_SplitCommand","Argument %d <%s>", iArgum, argums[iArgum]);
  }
}

/*----------------------------------------------------------------------------------------------------------------------------------
    Function : VMS_ExecL
      Action : replacemant for execl. It splits command and calls execv() from the Dec RTL. This function assumes its
               params are passed a special way. It can't be considered working in all cases. In fact it works only in one :-)
               The array containing splitted command is statically allocated, so we never free it and allocate it just once.
----------------------------------------------------------------------------------------------------------------------------------*/
int VMS_ExecL(
    const char *fileSpec,              /* - Conventionnaly, image to run. With Unix, often a shell - */
    const char *arg0,                  /* - The same as fileSpec, by convention - */
    ...) {                             /* - Real unix command, eventually splitted - */

  int execStat, nbArgums;
  static char **argums = NULL;
  char *cmdPtr;
  va_list vaCmd;

  if (DBG_EXL) fvwm_msg(DBG,"VMS_ExecL","Entering function...");

  /* --- Allocates the arguments array, once --- */
  if (NULL == argums) {
    argums = (char **)malloc(sizeof(char *) * VMS_MAX_ARGUMENT_IN_CMD);
    if (NULL == argums) {
      fvwm_msg(ERR,"VMS_ExecL","Error allocating argums <%s>", strerror(errno));
      exit(100);
    }
    memset(argums, 0, sizeof(char *) * VMS_MAX_ARGUMENT_IN_CMD);
  }

  /* --- Initialize the varying argument list --- */
  va_start(vaCmd, arg0);

  /* --- Assumes the shell command is in the fourth argument, third would be "-c" or something indicating the image to
         be run by the shell. Well of course, it can be something very different. --- */
  cmdPtr = va_arg(vaCmd, char*);
  cmdPtr = va_arg(vaCmd, char*);
  if (DBG_EXL) fvwm_msg(DBG,"VMS_ExecL","cmd <%s>", cmdPtr);

  /* --- Splits command in the argument array --- */
  VMS_SplitCommand(cmdPtr, argums, VMS_MAX_ARGUMENT_IN_CMD, &nbArgums);
  if (DBG_EXL) fvwm_msg(DBG,"VMS_ExecL","cmd now broken in %d arguments", nbArgums);

  /* --- Calling exec : if it fails, don't return the fact. Thus, the unix code won't remark the problem and continue
         the parent process. --- */
  execStat = execv(argums[0], argums);
  if (-1 == execStat) fvwm_msg(ERR,"VMS_ExecL","execl failed (%s)", strerror(errno));

  return(0);
}
