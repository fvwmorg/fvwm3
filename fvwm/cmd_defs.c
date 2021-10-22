/* This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see: <http://www.gnu.org/licenses/>
 */

#include "config.h"

#include <stdio.h>
#include <stdbool.h>

#include "cmd_defs.h"
#include "libs/safemalloc.h"

RB_GENERATE(cmd_args_tree, cmd_args_entry, entry, cmd_args_cmp);
extern struct cmd_definition cmd_def_colorset;

static struct cmd_args_entry *cmd_args_find(struct cmd_args *, char);
static struct cmd_args *cmd_args_parse(const char *, int, char **);

struct cmd_definition *cdc[] = {
	&cmd_def_colorset,
	NULL,
};

struct cmd_definition *
cmd_generate(int argc, char **argv)
{
	struct cmd_definition	*new_cmd, *found_cmd;

	if ((found_cmd = cmd_lookup(argv[0])) == NULL)
		return (NULL);

	new_cmd = fxcalloc(1, sizeof *new_cmd);
	memcpy(new_cmd, found_cmd, sizeof *new_cmd);

	new_cmd->args = cmd_args_parse(new_cmd->getopt, argc, argv);

	return (new_cmd);
}

struct cmd_definition *
cmd_lookup(const char *cmd)
{
	struct cmd_definition	**cd, *this;

	for (cd = cdc; *cd != NULL; cd++) {
		this = *cd;

		if (this->name != NULL && strcasecmp(this->name, cmd) == 0)
			return (this);
	}

	return (NULL);
}

int
cmd_args_cmp(struct cmd_args_entry *a1, struct cmd_args_entry *a2)
{
	return (a1->flag - a2->flag);
}

/* Find a flag in the arguments tree. */
static struct cmd_args_entry *
cmd_args_find(struct cmd_args *args, char ch)
{
	struct cmd_args_entry	entry;

	entry.flag = ch;
	return (RB_FIND(cmd_args_tree, &args->tree, &entry));
}

static struct cmd_args *
cmd_args_parse(const char *format, int argc, char **argv)
{
	struct cmd_args	*ca;
	int		 opt;

	ca = fxcalloc(1, sizeof *ca);

	optind = 1;

	while ((opt = getopt(argc, argv, format)) != -1) {
		if (opt < 0)
			continue;
		if (strchr(format, opt) == NULL) {
			cmd_args_free(ca);
			return (NULL);
		}
		cmd_args_set(ca, opt, optarg);
	}
	argc -= optind;
	argv += optind;

	ca->argc = argc;
	ca->argv = argv;

	return (ca);
}

/* Free an arguments set. */
void
cmd_args_free(struct cmd_args *ca)
{
	struct cmd_args_entry	*entry, *entry1;

	RB_FOREACH_SAFE(entry, cmd_args_tree, &ca->tree, entry1) {
		RB_REMOVE(cmd_args_tree, &ca->tree, entry);
		free(entry->value);
		free(entry);
	}

	free(ca);
}

bool
cmd_args_has(struct cmd_args *ca, char ch)
{
	return (cmd_args_find(ca, ch) != NULL);
}

void
cmd_args_set(struct cmd_args *ca, char ch, const char *value)
{
	struct cmd_args_entry	*entry;

	/* Replace existing argument. */
	if ((entry = cmd_args_find(ca, ch)) != NULL) {
		free(entry->value);
		entry->value = NULL;
	} else {
		entry = fxcalloc(1, sizeof *entry);
		entry->flag = ch;
		RB_INSERT(cmd_args_tree, &ca->tree, entry);
	}

	if (value != NULL)
		entry->value = fxstrdup(value);
}

const char *
cmd_args_get(struct cmd_args *ca, char ch)
{
	struct cmd_args_entry	*entry;

	if ((entry = cmd_args_find(ca, ch)) == NULL)
		return (NULL);
	return (entry->value);
}

long long
cmd_args_to_num(struct cmd_args *ca, char ch, long long minval, long long maxval,
    char **cause)
{
	const char		*errstr;
	long long 	 	 ll;
	struct cmd_args_entry	*entry;

	if ((entry = cmd_args_find(ca, ch)) == NULL) {
		*cause = fxstrdup("missing");
		return (0);
	}

	ll = strtonum(entry->value, minval, maxval, &errstr);
	if (errstr != NULL) {
		*cause = fxstrdup(errstr);
		return (0);
	}

	*cause = NULL;
	return (ll);
}
