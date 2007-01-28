/* -*-c-*- */
#ifndef IN_READCONFIG_H
#define IN_READCONFIG_H

#define MODS_USED ((ALL_MODIFIERS) & ~(LockMask))

typedef enum
{
	READ_LINE = 1,
	READ_OPTION = 2,
	READ_ARG = 4,
	READ_REST_OF_LINE = 12,
} ReadOption;

extern void read_in_resources(void);
extern void print_bindings(Binding *list);
extern void print_args(int numargs, BuiltinArg *args);
extern Binding *ParseMouseEntry(char *tline);

extern void run_function_list(Function *func);
extern void run_binding(WinManager *man, Action action);

#endif /* IN_READCONFIG_H */
