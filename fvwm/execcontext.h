/* -*-c-*- */

#ifndef EXECCONTEXT_H
#define EXECCONTEXT_H

/* ---------------------------- included header files ---------------------- */

/* ---------------------------- global definitions ------------------------- */

/* ---------------------------- global macros ------------------------------ */

/* ---------------------------- type definitions --------------------------- */

/* Inernal types */
typedef struct
{
	XEvent te;
} exec_context_privileged_t;

/* Interface types */
typedef enum
{
	EXCT_NULL = '-',
	EXCT_INIT = 'I',
	EXCT_RESTART = 'R',
	EXCT_QUIT = 'Q',
	EXCT_TORESTART = 'r',
	EXCT_EVENT ='E',
	EXCT_MODULE ='M',
	EXCT_MENULOOP ='m',
	EXCT_PAGING = 'P',
	EXCT_SCHEDULE = 'S'
} exec_context_type_t;

typedef struct
{
	XEvent *etrigger;
	XEvent *elast;
} x_context_t;

typedef struct
{
	FvwmWindow *fw;
	Window w;
	unsigned long wcontext;
} window_context_t;

struct fmodule;
typedef struct
{
	struct fmodule *module;
} module_context_t;

typedef struct
{
	exec_context_type_t type;
	x_context_t x;
	window_context_t w;
	module_context_t m;
	/* for internal use *only*. *Never* acces this from outside! */
	exec_context_privileged_t private_data;
} exec_context_t;

typedef enum
{
	ECC_TYPE = 0x1,
	ECC_ETRIGGER = 0x2,
	ECC_FW = 0x4,
	ECC_W = 0x8,
	ECC_WCONTEXT = 0x10,
	ECC_MODULE = 0x20
} exec_context_change_mask_t;

typedef struct
{
	exec_context_type_t type;
	x_context_t x;
	window_context_t w;
	module_context_t m;
} exec_context_changes_t;

/* ---------------------------- forward declarations ----------------------- */

/* ---------------------------- exported variables (globals) --------------- */

/* ---------------------------- interface functions ------------------------ */

/* Creates a new exec_context from the passed in arguments in malloc'ed memory.
 * The context must later be destroyed with exc_destroy_context().
 *
 * Args:
 *   ecc
 *     Pointer to a structure which specifies the initial values of the struct
 *     members.
 *   mask
 *     The mask of members in ecc to use.
 */
const exec_context_t *exc_create_context(
	exec_context_changes_t *ecc, exec_context_change_mask_t mask);

/* Similar to exc_create_context(), but the created context contains only dummy
 * information. */
const exec_context_t *exc_create_null_context(void);

/* Works like exc_create_context(), but initialises all values with the data
 * from excin.  The ecc/mask pair overrides these values. */
const exec_context_t *exc_clone_context(
	const exec_context_t *excin, exec_context_changes_t *ecc,
	exec_context_change_mask_t mask);

/* Destroys an exec_context structure that was created with
 * exc_create_context(). */
void exc_destroy_context(const exec_context_t *exc);

#endif /* EXECCONTEXT_H */
