
int strcasecmp(register char *s1,register char *s2)
{
  while (tolower(*s1) == tolower(*s2))
  {
    ++s1;
    ++s2;
    if (!*s1 || !*s2)
      break;
  }
  return (tolower(*s1) - tolower(*s2));
}

