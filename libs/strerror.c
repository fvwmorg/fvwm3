
char *strerror(int num)
{
  extern int sys_nerr;
  extern char *sys_errlist[];

  if (num >= 0 && num < sys_nerr)
    return(sys_errlist[num]);
  else
    return "Unknown error number";
}


