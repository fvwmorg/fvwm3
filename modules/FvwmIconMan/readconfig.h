typedef enum {
  READ_LINE = 1,
  READ_OPTION = 2,
  READ_ARG = 4,
  READ_REST_OF_LINE = 12
} ReadOption;

extern void read_in_resources (char *file);
extern void print_bindings (Binding *list);
extern void print_args (int numargs, BuiltinArg *args);
extern Binding *ParseMouseEntry (char *tline);

extern void run_function_list (Function *func);
extern void run_binding (WinManager *man, Action action);

#define MODS_USED (ShiftMask | ControlMask | Mod1Mask | \
		   Mod2Mask| Mod3Mask| Mod4Mask| Mod5Mask)

