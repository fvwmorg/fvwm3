
int strncasecmp(register char *s1,register char *s2,register int n)
{
  while ((--n > 0) && (tolower(*s1) == tolower(*s2)))
  {
    ++s1;
    ++s2;
    if (!*s1 || !*s2)
      break;
  }
  return (tolower(*s1) - tolower(*s2));
}
