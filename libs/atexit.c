#include <config.h>

int
atexit(void (*func)())
{
#if HAVE_ON_EXIT
  return on_exit(func, 0);
#else
  return 1;
#endif
}
