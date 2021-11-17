/* -*-c-*- */

#ifndef CMDPARSER_H
#define CMDPARSER_H

/* ---------------------------- included header files ---------------------- */

/* ---------------------------- global definitions ------------------------- */

/* $0 through to $9 */
#define CMDPARSER_NUM_POS_ARGS 10

/* ---------------------------- global macros ------------------------------ */

/* ---------------------------- type definitions --------------------------- */

/* Enum for all the possible prefixes. */
typedef enum
{
	CP_PREFIX_NONE = 0,
	CP_PREFIX_MINUS = 0x1,
	CP_PREFIX_SILENT = 0x2,
	CP_PREFIX_KEEPRC = 0x4
} cmdparser_prefix_flags_t;

/* Enum for types of things to execute. */
typedef enum
{
	CP_EXECTYPE_UNKNOWN = 0,
	CP_EXECTYPE_BUILTIN_FUNCTION,
	CP_EXECTYPE_COMPLEX_FUNCTION,
	CP_EXECTYPE_COMPLEX_FUNCTION_DOES_NOT_EXIST,
	CP_EXECTYPE_MODULECONFIG
} cmdparser_execute_type_t;

/* move this to the implementation file and use void* instead??? */

typedef struct
{
	/* !!!note: need to define which bits in here may be accessed by the
	 * user and which are internal */
	unsigned char is_created : 1;
	/* the original command line */
	char *line;
	/* the current command line */
	char *cline;
	/* the expanded command line */
	char *expline;
	unsigned char do_free_expline : 1;
	/* the command name */
	char *command;
	/* name of the complex function if present */
	char *complex_function_name;
	/* current function nesting depth */
	int call_depth;
	/* A string with all positional arguments of a complex function.  May
	 * be NULL if not in the context of a complex function. */
	char *all_pos_args_string;
	/* An array of the first CMDPARSER_NUM_POS_ARGS positional arguments up
	 * to and excluding the first NULL pointer. Must (not) all be NULL if
	 * all_pos_args_string is (not) NULL. */
	char *pos_arg_tokens[CMDPARSER_NUM_POS_ARGS];
} cmdparser_context_t;

#endif /* CMDPARSER_H */
