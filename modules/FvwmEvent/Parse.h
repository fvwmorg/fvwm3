/*
   File:		Parse.h
   Date:		Wed Jan 29 21:13:36 1997
   Author:		Albrecht Kadlec (albrecht)

   Description:		

   Modifications:	$Log$
   Modifications:	Revision 1.1  1998/10/14 00:03:23  tibbs
   Modifications:	Initial revision
   Modifications:	
*/

#ifndef _Parse_h
#define _Parse_h


#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <search.h>
/*#include "../../libs/fvwmlib.h"*/

char *PeekArgument(const char *pstr);
char *GetArgument(char **pstr);
int CmpArgument(const char *pstr,char *tok);
int MatchArgument(const char *pstr,char *tok);
#define NukeArgument(pstr) free(GetArgument(pstr))

int mystrncasecmp(char *a, char *b,int n);


/*
   function:		FindToken, LFindToken
   description:		find the entry of type 'struct_entry' 
                        holding 'key' in 'table'
   returns:		pointer to the matching entry
                        NULL if not found

   table must be sorted in ascending order for FindToken,
   this is not necessary for LFindToken (slower)
*/

#define FindToken(key,table,struct_entry)				\
        (struct_entry *) bsearch(key,					\
				 (char *)(table), 			\
				 sizeof(table) / sizeof(struct_entry),	\
				 sizeof(struct_entry),			\
				 XCmpToken)

#define LFindToken(key,table,struct_entry)				\
        (struct_entry *) lfind(key,					\
				 (char *)(table), 			\
				 sizeof(table) / sizeof(struct_entry),	\
				 sizeof(struct_entry),			\
				 XCmpToken)

int XCmpToken();    /* (char *s, char **t); but avoid compiler warning */
                                             /* needed by (L)FindToken */

#if 0
   e.g:
        struct entry			/* these are stored in the table */
        {    char *token;			
             ...			/* any info */
        };

        struct entry table[]= { ... };	/* define entries here */

        char *word=GetArgument(...);
        entry_ptr = FindToken(word,table,struct entry);

        (struct token *)bsearch("Style",
                          (char *)table, sizeof (table)/sizeof (struct entry),
                          sizeof(struct entry), CmpToken);
#endif

#endif /* _Parse_h */
