/**************************************************************************/
/* Prototypes that don't exist on suns */
/* If I do ALL this, I can compile OK with -Wall -Wstrict-prototypes on the
 * Sun's */

#if defined(sun) && !defined(SVR4)
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>


extern int select(int, fd_set *, fd_set *, fd_set *, struct timeval *);

extern int system(char *command);
extern int toupper(int);
extern int tolower(int);

/* stdio things */
extern int fputc(char, FILE *);
extern int fgetc(FILE *);
extern int fputs(char *, FILE *);
extern char *mktemp(char *);
extern int pclose(FILE *);
extern int sscanf(char *input, char *format, ...);
extern int printf(char *format, ...);
extern int fprintf(FILE *file,char *format, ...);
extern int fseek(FILE *file,long offset,int);
extern int fclose(FILE *file);
extern int fread(char *data, int size, int count, FILE *file);
extern int fflush(FILE *file);
extern void perror(char *s);

/* string manipulation */
extern int strncasecmp(char *str1, char *str2, int length);
extern int strcasecmp(char *str1, char *str2);
extern int putenv(char *);

/* sunOS defines SIG_IGN, but they get it wrong, as far as GCC
 * is concerned */
#ifdef SIG_IGN
#undef SIG_IGN
#endif
#define SIG_IGN         (void (*)(int))1
int wait3(int *, int, struct rusage *);
int sigsetmask(int);
int sigblock(int);
  
int setitimer(int, struct itimerval *, struct itimerval *);
int getitimer(int, struct itimerval *);
int bzero(char *, int);

long time(long *);
int gethostname(char *name, int namelen);
/**************************************************************************/
#endif
