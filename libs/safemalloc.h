/* -*-c-*- */

/*
 *
 *  Procedure:
 *      safe?alloc - mallocs/callocs specified space or exits if there's a
 *                   problem
 *
 */
char *safemalloc(int length);
char *safecalloc(int num, int length);
char *saferealloc(char *ptr, int length);
char *safestrdup(const char *s);
