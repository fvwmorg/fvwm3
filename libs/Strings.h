#ifndef FVWMLIB_STRINGS_H
#define FVWMLIB_STRINGS_H


/**
 * Concatenate three strings.
 *
 * Parameters may be NULL to signify the empty string.
 *
 * Returns pointer to static storage, overwritten on the next call.
 **/
char *CatString3(const char *a, const char *b, const char *c);
#define CatString2(a,b) CatString3(a,b,NULL)


/**
 * Copy string into newly-malloced memory, stripping leading and
 * trailing spaces.  The string is terminated by either a NUL or 
 * a newline character.
 **/
void CopyString(char **dest, const char *source);


/**
 * Like CopyString, but strips leading and trailing (double) quotes if any.
 **/
void CopyStringWithQuotes(char **dest, const char *src);


/**
 * Copy string into newly-malloced memory, stripping leading and
 * trailing spaces.  The difference between this and CopyString()
 * is that newlines are treated as whitespace by stripcpy(), whereas
 * CopyString() treats a newline as a string terminator (like the NUL
 * character.
 **/
char *stripcpy( const char *source );


/**
 * Return TRUE if the two strings are equal.  Case is ignored.
 **/
int StrEquals( const char *s1, const char *s2 );


/**
 * Return TRUE if the string has the given prefix.  Case is ignored.
 **/
int StrHasPrefix( const char* string, const char* prefix );

/**
 * Adds single quotes arround the string and escapes single quotes with
 * backslashes.  The result is placed in the given dest, not allocated.
 * The end of destination, i.e. pointer to '\0' is returned.
 * You should allocate dest yourself, at least strlen(source) * 2 + 3.
 **/
char *QuoteString(char *dest, const char *source);

#endif
