/* -*-c-*- */
#ifndef FVWMLIB_ENVVAR_H
#define FVWMLIB_ENVVAR_H

/*
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

/*
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

/*
 *  FUNCTION      Search for the first environment variable and return
 *                its contents and coordinates in the given string.
 *
 *  INPUT         s       the string to scan.
 *                        may include $ and { } that introduce variable.
 *
 *  OUTPUT        beg     index in the string of matching $.
 *                end     index in the string, first after matching var.
 *
 *  RETURNS       The variable contents; "" if env variable has legal name,
 *                but does not exist; or NULL if no env variables found.
 *                Returned constant string must not be deallocated.
 *
 *  NOTE          This function will only return `legal' variables. There
 *                may be $'s in the string that are not followed by what
 *                is considered a legal variable name introducer. Such
 *                occurrences are skipped.
 *                If nothing is found returns NULL and sets beg and end to 0.
 *
 *  EXAMPLE       getFirstEnv("echo $HOME/.fvwm/config", &beg, &end)
 *                returns "/home/username" and beg=5, end=10.
 *
 */
const char *getFirstEnv(const char *s, int *beg, int *end);

/* This function keeps a list of all strings that were set in the environment.
 * If a variable is written again, the old memory is freed.  This function
 * should be called instead of putenv().
 *
 *   var - environement variable name
 *   env - environment string ("variable=value")
 *
 * Both arguments are copied internally and should be freed after calling this
 * function.
 */
void flib_putenv(char *var, char *env);

/* Replacement for unsetenv(). */
void flib_unsetenv(const char *name);

#endif
