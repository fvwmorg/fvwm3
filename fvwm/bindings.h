#ifndef _BINDINGS_
#define _BINDINGS_

#include "fvwm.h"

void key_binding(F_CMD_ARGS);
void mouse_binding(F_CMD_ARGS);
unsigned int MaskUsedModifiers(unsigned int in_modifiers);
unsigned int GetUnusedModifiers(void);
void ignore_modifiers(F_CMD_ARGS);

#endif /* _BINDINGS_ */
