/*
   File:		Parse.h
   Date:		Wed Jan 29 21:13:36 1997
   Author:		Albrecht Kadlec (albrecht)
*/

#ifndef _Parse_h
#define _Parse_h


#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

char *PeekArgument(const char *pstr);
char *GetArgument(char **pstr);
int CmpArgument(const char *pstr,char *tok);
int MatchArgument(const char *pstr,char *tok);
#define NukeArgument(pstr) free(GetArgument(pstr))


/*
   function:		FindToken
   description:		find the entry of type 'struct_entry'
                        holding 'key' in 'table'
   returns:		pointer to the matching entry
                        NULL if not found

   table must be sorted in ascending order for FindToken
*/

#define FindToken(key,table,struct_entry)				\
        (struct_entry *) bsearch(key,					\
				 (char *)(table), 			\
				 sizeof(table) / sizeof(struct_entry),	\
				 sizeof(struct_entry),			\
				 XCmpToken)

int XCmpToken();    /* (char *s, char **t); but avoid compiler warning */


#if 0
/* e.g: */

  struct entry			/* these are stored in the table */
  {    char *token;
       /* ... */		/* any info */
  };

  struct entry table[] = { /* ... */ };	/* define entries here */

  char *word = GetArgument( /* ... */);
  entry_ptr = FindToken(word,table,struct entry);

  (struct token *)bsearch("Style",
                          (char *)table, sizeof (table)/sizeof (struct entry),
                          sizeof(struct entry), CmpToken);
#endif /* 0 */

#if 0
/* Note that lfind() is not part of the ANSI standard.  This is never used
 * currently; I think we should just keep it that way...
 */
# define LFindToken(key,table,struct_entry)				\
         (struct_entry *) lfind(key,					\
				 (char *)(table), 			\
				 sizeof(table) / sizeof(struct_entry),	\
				 sizeof(struct_entry),			\
				 XCmpToken)
#endif /* 0 */

#endif /* _Parse_h */
