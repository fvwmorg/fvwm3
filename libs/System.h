#ifndef LIB_SYSTEM_H
#define LIB_SYSTEM_H

fd_set_size_t GetFdWidth(void);
extern fd_set_size_t fvwmlib_max_fd;
void fvwmlib_init_max_fd(void);

int getostype(char *buf, int max);
void setPath(char **p_path, const char *newpath, int free_old_path);
char *searchPath(
	const char *pathlist, const char *filename, const char *suffix,
	int type);



/* An interface for verifying cached files. */
typedef unsigned long FileStamp;
FileStamp getFileStamp(const char *name);
void setFileStamp(FileStamp *stamp, const char *name);
Bool isFileStampChanged(const FileStamp *stamp, const char *name);

/* mkstemp */
int fvwm_mkstemp (char *TEMPLATE);

#endif /* LIB_SYSTEM_H */
