/*
   File:		ModParse.h
   Date:		Wed Jan 29 21:13:36 1997
   Author:		Albrecht Kadlec (albrecht)

   Description:		

   Modifications:	$Log$
   Modifications:	Revision 1.3  1998/10/31 11:52:51  steve
   Modifications:	basic autoconfiguration support
   Modifications:	
   Modifications:	Revision 1.2.4.2  1998/10/28 21:49:20  domivogt
   Modifications:	merged animate_icons and menu_position
   Modifications:	
   Modifications:	Revision 1.2.2.1  1998/10/27 02:34:36  dane
   Modifications:	New Module Parser
   Modifications:	
   Modifications:	Revision 1.1  1998/10/27 01:35:32  dane
   Modifications:	New Module
   Modifications:	

   Changed 08/30/98 by Dan Espen:
   - Copied from FvwmEvent/Parse.h.  Set up for general use by modules.
   Still to be done, try to integrate with Parse.c which is used by fvwm
   itself for parsing.
*/


#ifndef _ModParse_h
#define _ModParse_h


#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <search.h>
#include <stdlib.h>                     /* for free() */

char *PeekArgument(const char *pstr);
char *GetArgument(char **pstr);
int CmpArgument(const char *pstr,char *tok);
int MatchArgument(const char *pstr,char *tok);
#define NukeArgument(pstr) free(GetArgument(pstr))

int mystrncasecmp(char *a, char *b,int n); /* why is this here? */


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

#endif /* _ModParse_h */
