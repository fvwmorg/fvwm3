/* -*-c-*- */

#ifndef CHARMAP_H
#define CHARMAP_H

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

#endif /* CHARMAP_H */
