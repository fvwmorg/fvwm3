#ifndef _LOG_H
#define _LOG_H

#define printflike(a, b) __attribute__ ((format (printf, a, b)))

void	log_set_level(int);
int	log_get_level(void);
void	log_open(void);
void	log_toggle(void);
void	log_close(void);
void printflike(1, 2) log_debug(const char *, ...);

#endif
