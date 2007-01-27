#ifndef LIB_XERROR_H
#define LIB_XERROR_H

/* not really a wrapper, but useful and X related */
void PrintXErrorAndCoredump(Display *dpy, XErrorEvent *error, char *MyName);

#endif /* LIB_XERROR_H */
