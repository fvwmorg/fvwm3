#ifndef FVWM_CMD_DEFS_H
#define FVWM_CMD_DEFS_H

#include <stdlib.h>
#include <sys/tree.h>

enum cmd_success {
	CMD_RET_ERROR = 1,
	CMD_RET_NORMAL,
	CMD_RET_UNIMPLEMENTED
};

RB_HEAD(cmd_args_tree, cmd_args_entry);
struct cmd_args {
	struct cmd_args_tree	  tree;
	int			  argc;
	char			**argv;
};

struct cmd_args_entry {
	u_char				 flag;
	char				*value;
	RB_ENTRY(cmd_args_entry)	 entry;
};

struct cmd_definition {
	struct cmd_args		*args;
	const char 		*name;
	const char 		*getopt;
	const char 		*usage;

	enum cmd_success (*exec)(struct cmd_definition *self);

	TAILQ_ENTRY(cmd_definition) 	 entry;
};

struct cmd_definition *cmd_lookup(const char *);
struct cmd_definition *cmd_generate(int, char **);
int cmd_args_cmp(struct cmd_args_entry *, struct cmd_args_entry *);
bool cmd_args_has(struct cmd_args *, char);
void cmd_args_set(struct cmd_args *, char, const char *);
const char *cmd_args_get(struct cmd_args *, char);
long long cmd_args_to_num(struct cmd_args *, char, long long, long long, char **);

void cmd_args_free(struct cmd_args *);

#endif
