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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* ---------------------------- included header files ---------------------- */

#include "config.h"
#include "libs/defaults.h"
#include "libs/Parse.h"

#include <assert.h>
#include <stdio.h>

#include "fvwm.h"
#include "execcontext.h"
#include "conditional.h"
#include "functable.h"
#include "commands.h"
#include "functable_complex.h"
#include "cmdparser.h"
#include "cmdparser_hooks.h"
#include "cmdparser_old.h"
#include "misc.h"
#include "expand.h"

/* ---------------------------- local definitions -------------------------- */

#if 0 /*!!!*/
#define OCP_DEBUG 1
#else
#define OCP_DEBUG 0
#endif

/* ---------------------------- local macros ------------------------------- */

/* ---------------------------- imports ------------------------------------ */

/* ---------------------------- included code files ------------------------ */

/* ---------------------------- local types -------------------------------- */

/* ---------------------------- forward declarations ----------------------- */

/* ---------------------------- local variables ---------------------------- */

/* ---------------------------- exported variables (globals) --------------- */

/* ---------------------------- local functions ---------------------------- */

static void ocp_debug(cmdparser_context_t *c, const char *msg)
{
	int i;

	if (!OCP_DEBUG)
	{
		return;
	}
	fprintf(stderr, "%s: %p: %s\n", __func__, c, (msg != NULL) ? msg : "");
	if (!c->is_created)
	{
		fprintf(stderr, "  not created\n");
		return;
	}
	fprintf(stderr, "  depth    : %d\n", c->call_depth);
	fprintf(stderr, "  orig line: '%s'\n", c->line ? c->line : "(nil)");
	fprintf(stderr, "  curr line: '%s'\n", c->cline ? c->cline : "(nil)");
	fprintf(
		stderr, "  exp  line: (do_free = %d) '%s'\n",
		c->do_free_expline, c->expline ? c->expline : "(nil)");
	fprintf(
		stderr, "  command  : '%s'\n",
		c->command ? c->command : "(nil)");
	fprintf(
		stderr, "  complexf : '%s'\n",
		c->complex_function_name ? c->complex_function_name : "(nil)");
	if (c->all_pos_args_string == NULL)
	{
		return;
	}
	fprintf(
		stderr, "  all args : '%s'\n",
		c->all_pos_args_string ? c->all_pos_args_string: "(nil)");
	fprintf(stderr, "  pos args :");
	for (
		i = 0; i < CMDPARSER_NUM_POS_ARGS &&
			c->pos_arg_tokens[i] != NULL; i++)
	{
		fprintf(stderr, " %d:'%s'", i,
			c->pos_arg_tokens[i] ? c->pos_arg_tokens[i]: "(nil)");
	}
	fprintf(stderr, "\n");

	return;
}

static int ocp_create_context(
	cmdparser_context_t *dest_c, cmdparser_context_t *caller_c, char *line,
	char *all_pos_args_string, char *pos_arg_tokens[])
{
	/* allocate the necessary resources */
	memset(dest_c, 0, sizeof(*dest_c));
	dest_c->line = line;
	/*!!!should this be in handle_line_start instead???*/
	if (caller_c != NULL)
	{
		if (dest_c->call_depth > MAX_FUNCTION_DEPTH)
		{
			fvwm_debug(
				__func__,
				"called with a depth of %i, "
				"stopping function execution!",
				dest_c->call_depth);

			return -1;
		}
	}
	else
	{
		dest_c->call_depth = 1;
	}
	if (all_pos_args_string != NULL)
	{
		int i;

		dest_c->all_pos_args_string = all_pos_args_string;
		for (
			i = 0; i < CMDPARSER_NUM_POS_ARGS &&
				pos_arg_tokens[i] != NULL; i++)
		{
			dest_c->pos_arg_tokens[i] = pos_arg_tokens[i];
		}
	}
	/*!!!allocate stuff*/
	dest_c->is_created = 1;

	return 0;
}

static void ocp_destroy_context(cmdparser_context_t *c)
{
	/* free the resources allocated in the create function */
	if (!c->is_created)
	{
		return;
	}
	if (c->command != NULL)
	{
		free(c->command);
	}
	if (c->expline != NULL && c->do_free_expline)
	{
		free(c->expline);
	}
	c->is_created = 0;

	return;
}

static int ocp_handle_line_start(cmdparser_context_t *c)
{
	/*!!! remove assertion later*/
	assert(c->cline == NULL);
	if (!c->line)
	{
		/* nothing to do */
		return 1;
	}
	/* ignore whitespace at the beginning of all config lines */
	c->cline = SkipSpaces(c->line, NULL, 0);
	if (!c->cline || c->cline[0] == 0)
	{
		/* impossibly short command */
		return -1;
	}
	if (c->cline[0] == '#')
	{
		/* a comment */
		return 1;
	}

	return 0;
}

static cmdparser_prefix_flags_t ocp_handle_line_prefix(cmdparser_context_t *c)
{
	cmdparser_prefix_flags_t flags;
	char *token;
	char *restofline;

	if (OCP_DEBUG)
	{
		fprintf(stderr, "%s: '%s'\n", __func__, c->cline);
	}
	flags = CP_PREFIX_NONE;
	if (c->cline[0] == '-')
	{
		c->cline++;
		flags |= CP_PREFIX_MINUS;
		if (OCP_DEBUG)
		{
			fprintf(stderr, "minus -> '%s'\n", c->cline);
		}
	}
	token = PeekToken(c->cline, &restofline);
	if (OCP_DEBUG)
	{
		fprintf(
			stderr, "cl '%s', tok '%s', rl '%s'\n", c->cline,
			token, restofline);
	}
	while (token != NULL)
	{
		if (OCP_DEBUG)
		{
			fprintf(stderr, "loop: token '%s'\n", token);
		}
		if (StrEquals(token, PRE_SILENT))
		{
			flags |= CP_PREFIX_SILENT;
			c->cline = restofline;
			token = PeekToken(c->cline, &restofline);
			if (OCP_DEBUG)
			{
				fprintf(
					stderr,
					"-> silent: cl '%s', tok '%s'\n",
					c->cline, token);
			}
		}
		else if (StrEquals(token, PRE_KEEPRC))
		{
			flags |= CP_PREFIX_KEEPRC;
			c->cline = restofline;
			token = PeekToken(c->cline, &restofline);
			if (OCP_DEBUG)
			{
				fprintf(
					stderr,
					"-> keeprc: cl '%s', tok '%s'\n",
					c->cline, token);
			}
		}
		else
		{
			break;
		}
	}
	if (OCP_DEBUG)
	{
		fprintf(stderr, "done: flags 0x%x\n", flags);
	}

	return flags;
}

static int ocp_is_module_config(cmdparser_context_t *c)
{
	return (c->command != NULL && *c->command == '*') ? 1 : 0;
}

static const char *ocp_parse_command_name(
	cmdparser_context_t *c, void *func_rc, const void *exc)
{
	GetNextToken(c->cline, &c->command);
#if 1 /*!!!*/
fprintf(stderr, "%s: c->command '%s'\n", __func__, (c->command) ? c->command : "(nil)");
#endif
	if (c->command != NULL)
	{
		char *tmp = c->command;

#if 1 /*!!!*/
fprintf(stderr, "%s: expand c->command\n", __func__);
#endif
		c->command = expand_vars(
			c->command, c, False, False, func_rc, exc);
#if 1 /*!!!*/
fprintf(stderr, "%s: c->command '%s'\n", __func__, (c->command) ? c->command : "(nil)");
#endif
		free(tmp);
	}
	if (c->command && !ocp_is_module_config(c))
	{
#if 1
		/* DV: with this piece of code it is impossible to have a
		 * complex function with embedded whitespace that begins with a
		 * builtin function name, e.g. a function "echo hello". */
		/* DV: ... and without it some of the complex functions will
		 * fail */
		char *tmp = c->command;

#if 1 /*!!!*/
fprintf(stderr, "%s: remove trailing spaces\n", __func__);
#endif
		while (*tmp && !isspace(*tmp))
		{
			tmp++;
		}
		*tmp = 0;
#if 1 /*!!!*/
fprintf(stderr, "%s: c->command '%s'\n", __func__, (c->command) ? c->command : "(nil)");
#endif
#endif
		return c->command;
	}
	else
	{
		return NULL;
	}
}

static void ocp_expand_command_line(
	cmdparser_context_t *c, int is_addto, void *func_rc, const void *exc)
{
	c->expline = expand_vars(
		c->cline, c, is_addto, ocp_is_module_config(c), func_rc, exc);
	c->cline = c->expline;
	c->do_free_expline = 1;

	return;
}

static void ocp_release_expanded_line(cmdparser_context_t *c)
{
	c->do_free_expline = 0;

	return;
}

static cmdparser_execute_type_t ocp_find_something_to_execute(
	cmdparser_context_t *c, const func_t **ret_builtin,
	FvwmFunction **ret_complex_function)
{
	int is_function_builtin;

#if 1 /*!!!*/
fprintf(stderr, "%s: c->command '%s'\n", __func__, (c->command) ? c->command : "(nil)");
#endif
	*ret_complex_function = NULL;
	/* Note: the module config command, "*" can not be handled by the
	 * regular command table because there is no required white space after
	 * the asterisk. */
	if (ocp_is_module_config(c))
	{
		/*!!!strip asterisk??? */
		return CP_EXECTYPE_MODULECONFIG;
	}
	if (*ret_builtin == NULL)
	{
		/* in a new parser we would look for a builtin function here,
		 * but in the old parser this has already been done */
	}
	is_function_builtin = 0;
	if (*ret_builtin != NULL)
	{
		if ((*ret_builtin)->func_c == F_FUNCTION)
		{
			c->cline = SkipNTokens(c->cline, 1);
			if (OCP_DEBUG)
			{
				fprintf(stderr, "func cline '%s'\n", c->cline);
			}
			free(c->command);
			c->command = NULL;
			is_function_builtin = 1;
			*ret_builtin = NULL;
		}
		else
		{
			c->cline = SkipNTokens(c->cline, 1);
			return CP_EXECTYPE_BUILTIN_FUNCTION;
		}
	}
#if 1 /*!!!*/
	assert(*ret_builtin == NULL);
#endif
	do
	{
		char *complex_function_name;
		char *rest_of_line;

		/* find_complex_function expects a token, not just a quoted
		 * string */
		complex_function_name = PeekToken(c->cline, &rest_of_line);
		if (complex_function_name == NULL)
		{
			break;
		}
#if 1 /*!!!*/
fprintf(stderr, "%s: lookup cf '%s'\n", __func__, complex_function_name);
#endif
		*ret_complex_function =
			find_complex_function(complex_function_name);
#if 1 /*!!!*/
fprintf(stderr, "%s: lookup cf '%s' -> %p\n", __func__, complex_function_name, *ret_complex_function);
#endif
		if (*ret_complex_function != NULL)
		{
			c->cline = rest_of_line;
			c->complex_function_name = complex_function_name;
			return CP_EXECTYPE_COMPLEX_FUNCTION;
		}
		if (is_function_builtin)
		{
			c->cline = rest_of_line;
			c->complex_function_name = complex_function_name;
			return CP_EXECTYPE_COMPLEX_FUNCTION_DOES_NOT_EXIST;
		}
	} while (0);

	return CP_EXECTYPE_UNKNOWN;
}

/* ---------------------------- local variables ---------------------------- */

static cmdparser_hooks_t old_parser_hooks =
{
	ocp_create_context,
	ocp_handle_line_start,
	ocp_handle_line_prefix,
	ocp_parse_command_name,
	ocp_is_module_config,
	ocp_expand_command_line,
	ocp_release_expanded_line,
	ocp_find_something_to_execute,
	ocp_destroy_context,
	ocp_debug
};

/* ---------------------------- interface functions ------------------------ */

/* return the hooks structure of the old parser */
const cmdparser_hooks_t *cmdparser_old_get_hooks(void)
{
	return &old_parser_hooks;
}
