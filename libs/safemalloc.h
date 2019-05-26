void	*fxmalloc(size_t);
void	*fxcalloc(size_t, size_t);
void	*fxrealloc(void *, size_t, size_t);
char	*fxstrdup(const char *);
int	 xasprintf(char **, const char *, ...);
int	 xvasprintf(char **, const char *, va_list);
