/* -*-c-*- */
/* This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef FVWMLIB_PARSE_H
#define FVWMLIB_PARSE_H

/* ---------------------------- included header files ----------------------- */

/* ---------------------------- global definitions -------------------------- */

#define MAX_TOKEN_LENGTH 1023

/* ---------------------------- global macros ------------------------------- */

/***********************************************************************
 * Stuff for consistent parsing
 ***********************************************************************/
#define IsQuote(c) ((c) == '"' || (c) == '\'' || (c) =='`')
#define IsBlockStart(c) ((c) == '[' || (c) == '{' || (c) == '(')
#define IsBlockEnd(c,cs) (((c) == ']' && (cs) == '[') || ((c) == '}' && (cs) == '{') || ((c) == ')' && (cs) == '('))

/*
 *  function:            FindToken
 *  description:         find the entry of type 'struct_entry'
 *                       holding 'key' in 'table'
 *  returns:             pointer to the matching entry
 *                       NULL if not found
 *
 *  table must be sorted in ascending order for FindToken.
 */
#define FindToken(key,table,struct_entry)                               \
	(struct_entry *) bsearch(key,                                   \
				 (char *)(table),                       \
				 sizeof(table) / sizeof(struct_entry),  \
				 sizeof(struct_entry),                  \
				 (int(*)(const void*, const void*))XCmpToken)

/* ---------------------------- type definitions ---------------------------- */

/* ---------------------------- exported variables (globals) ---------------- */

/* ---------------------------- interface functions ------------------------- */

char *EscapeString(char *s, const char *qchars, char echar);
char *SkipQuote(char *s, const char *qlong, const char *qstart,
		const char *qend);
char *GetQuotedString(char *sin, char **sout, const char *delims,
		      const char *qlong, const char *qstart, const char *qend);
char *SkipSpaces(char *indata, char *spaces, int snum);
char *DoPeekToken(char *indata, char **token, char *spaces, char *delims,
		  char *out_delim);
char *PeekToken(char *indata, char **token);
int MatchToken(char *pstr,char *tok);

/* old style parse routine: */
char *DoGetNextToken(char *indata,char **token, char *spaces, char *delims,
		     char *out_delim);
char *GetNextToken(char *indata,char **token);
char *GetNextSimpleOption(char *indata, char **option);
char *GetNextFullOption(char *indata, char **option);
char *SkipNTokens(char *indata, unsigned int n);
char *GetModuleResource(char *indata, char **resource, char *module_name);
int GetSuffixedIntegerArguments(
  char *action, char **ret_action, int *retvals, int num, char *suffixlist,
  int *ret_suffixnum);
int GetIntegerArgumentsAnyBase(
	char *action, char **ret_action, int *retvals,int num);
int SuffixToPercentValue(int value, int suffix, int *unit_table);
int GetIntegerArguments(char *action, char**ret_action, int *retvals,int num);
int GetTokenIndex(char *token, char **list, int len, char **next);
char *GetNextTokenIndex(char *action, char **list, int len, int *index);
int GetRectangleArguments(char *action, int *width, int *height);
int GetOnePercentArgument(char *action, int *value, int *unit_io);
int GetTwoPercentArguments(
	char *action, int *val1, int *val2, int *val1_unit, int *val2_unit);
int ParseToggleArgument(
	char *action, char **ret_action, int default_ret, char no_toggle);
int XCmpToken(const char *s, const char **t);
char *GetFileNameFromPath(char *path);

#endif
