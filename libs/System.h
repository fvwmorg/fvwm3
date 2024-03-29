#ifndef FVWMLIB_SYSTEM_H
#define FVWMLIB_SYSTEM_H

#include "config.h"

fd_set_size_t GetFdWidth(void);
extern fd_set_size_t fvwmlib_max_fd;
void fvwmlib_init_max_fd(void);

int getostype(char *buf, int max);

/* An interface for verifying cached files. */
typedef unsigned long FileStamp;
FileStamp getFileStamp(const char *name);
void setFileStamp(FileStamp *stamp, const char *name);
Bool isFileStampChanged(const FileStamp *stamp, const char *name);

/* mkstemp */
int fvwm_mkstemp (char *TEMPLATE);

#endif /* FVWMLIB_SYSTEM_H */
