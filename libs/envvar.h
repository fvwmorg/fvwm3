#ifndef FVWMLIB_ENVVAR_H
#define FVWMLIB_ENVVAR_H

/*-------------------------------------------------------------------------
 *
 *  NAME          envExpand
 *
 *  FUNCTION      Expand environment variables in a string.
 *
 *  SYNOPSIS      #include "envvar.h"
 *                int envExpand(char *s, int maxstrlen);
 *
 *  INPUT         s       string to expand environment variables in.
 *                maxstrlen     max length of string, including '\0'.
 *
 *  OUTPUT        s       the string with environment variables expanded.
 *
 *  RETURNS       Number of changes done.
 *
 *  NOTES         A non-existing variable is substituted with the empty
 *                string.
 *
 */
int envExpand(char *s, int maxstrlen);


/*-------------------------------------------------------------------------
 *
 *  NAME          envDupExpand
 *
 *  FUNCTION      Expand environment variables into a new string.
 *
 *  SYNOPSIS      #include "envvar.h"
 *                char *envDupExpand(const char *s, int extra);
 *
 *  INPUT         s       string to expand environment variables in.
 *                extra   number of extra bytes to allocate in the
 *                        string, in addition to the string contents
 *                        and the terminating '\0'.
 *
 *  RETURNS       A dynamically allocated string with environment
 *                variables expanded.
 *                Use free() to deallocate the buffer when it is no
 *                longer needed.
 *                NULL is returned if there is not enough memory.
 *
 *  NOTES         A non-existing variable is substituted with the empty
 *                string.
 *
 */
char *envDupExpand(const char *s, int extra);


#endif
