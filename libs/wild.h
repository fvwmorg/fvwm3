/* -*-c-*- */

#ifndef WILD_H
#define WILD_H

/*
 *      Does `string' match `pattern'? '*' in pattern matches any sub-string
 *      (including the null string) '?' matches any single char. For use
 *      by filenameforall. Note that '*' matches across directory boundaries
 *
 *      This code donated by  Paul Hudson <paulh@harlequin.co.uk>
 *      It is public domain, no strings attached. No guarantees either.
 *
 */
int matchWildcards(const char *pattern, const char *string);

#endif /* WILD_H */
