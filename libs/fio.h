/* -*-c-*- */

/* Wrappers for io functions from the standard library that might be
 * interrupted.  These functions are restarted when necessary.
 */

#ifndef FVWMLIB_FIO_H
#define FVWMLIB_FIO_H
#include <stdlib.h>

/* ---------------------------- interface functions ------------------------ */

ssize_t
fvwm_send(int s, const void *buf, size_t len, int flags);
ssize_t
fvwm_recv(int s, void *buf, size_t len, int flags);

#endif /* FVWMLIB_FIO_H */
