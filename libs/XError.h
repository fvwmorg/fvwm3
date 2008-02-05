#ifndef FERROR_H
#define FERROR_H

void do_coredump(void);

/* not really a wrapper, but useful and X related */
void PrintXErrorAndCoredump(Display *dpy, XErrorEvent *error, char *MyName);

typedef int (*ferror_handler_t)(Display *, XErrorEvent *);
void ferror_set_temp_error_handler(ferror_handler_t new_handler);
void ferror_reset_temp_error_handler(void);
int ferror_call_next_error_handler(Display *dpy, XErrorEvent *error);

#endif /* FERROR_H */
