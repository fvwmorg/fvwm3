#ifndef _LOG_H
#define _LOG_H

#define printflike(a, b) __attribute__ ((format (printf, a, b)))
#define FVWM3_LOGFILE_DEFAULT "fvwm3-output.log"

void	log_set_level(int);
int	log_get_level(void);
void	log_open(const char *);
void	log_toggle(const char *);
void	log_close(void);
void printflike(2, 3) fvwm_debug(const char *, const char *, ...);

#endif
