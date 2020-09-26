#ifndef _SAFEMALLOC_H
#define _SAFEMALLOC_H

#include <stdarg.h>
#include <stddef.h>

void	*fxmalloc(size_t);
void	*fxcalloc(size_t, size_t);
void	*fxrealloc(void *, size_t, size_t);
char	*fxstrdup(const char *);
int	 xasprintf(char **, const char *, ...);
int	 xvasprintf(char **, const char *, va_list);

#endif
