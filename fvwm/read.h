/* -*-c-*- */
#ifndef FVWM_READ_H
#define FVWM_READ_H
#include <stdio.h>
#include "condrc.h"
#include "execcontext.h"
#include "cmdparser.h"

/**
 * Full pathname of file read in progress, or NULL.
 **/
extern const char *get_current_read_file(void);
extern const char *get_current_read_dir(void);


/**
 * Read and execute each line from stream.
 *
 * Parameters eventp, fw, context, and Module supply the context
 * for executing the commands.
 * cond_rc_t is passed thru in case piperead is running in a function.
 **/
void run_command_stream(
	cond_rc_t *cond_rc, FILE *f, const exec_context_t *exc);


/**
 * Given a filename, open it and execute the commands therein.
 *
 * If the filename is not an absolute path, search for it in
 * fvwm_userdir (set in main()) or in FVWM_DATADIR.  Return 1
 * if the file was found and executed.
 **/
int run_command_file(
	char *filename, const exec_context_t *exc, cmdparser_context_t *pc);

#endif
