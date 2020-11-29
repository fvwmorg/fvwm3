/* -*-c-*- */

#ifndef FVWMLIB_CHARMAP_H
#define FVWMLIB_CHARMAP_H

/* ---------------------------- included header files ---------------------- */

/* ---------------------------- global definitions ------------------------- */

/* ---------------------------- global macros ------------------------------ */

/* ---------------------------- type definitions --------------------------- */

typedef struct
{
	char key;
	int  value;
} charmap_t;

/* ---------------------------- forward declarations ----------------------- */

/* ---------------------------- exported variables (globals) --------------- */

/* ---------------------------- interface functions ------------------------ */

int charmap_string_to_mask(
	int *ret, const char *string, charmap_t *table, char *errstring);
char charmap_mask_to_char(int mask, charmap_t *table);

char *charmap_table_to_string(int mask, charmap_t *table);

#endif /* FVWMLIB_CHARMAP_H */
