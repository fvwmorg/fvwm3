#ifndef FVWMLIB_PARSE_H
#define FVWMLIB_PARSE_H



/***********************************************************************
 * Stuff for consistent parsing
 ***********************************************************************/
#define IsQuote(c) ((c) == '"' || (c) == '\'' || (c) =='`')
#define IsBlockStart(c) ((c) == '[' || (c) == '{' || (c) == '(')
#define IsBlockEnd(c,cs) (((c) == ']' && (cs) == '[') || ((c) == '}' && (cs) == '{') || ((c) == ')' && (cs) == '('))
#define MAX_TOKEN_LENGTH 1023

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
int GetSuffixedIntegerArguments(char *action, char **ret_action, int retvals[],
				int num, char *suffixlist,
				int ret_suffixnum[]);
int SuffixToPercentValue(int value, int suffix, int unit_table[]);
int GetIntegerArguments(char *action, char**ret_action, int retvals[],int num);
int GetTokenIndex(char *token, char *list[], int len, char **next);
char *GetNextTokenIndex(char *action, char *list[], int len, int *index);
int GetRectangleArguments(char *action, int *width, int *height);
int GetOnePercentArgument(char *action, int *value, int *unit_io);
int GetTwoPercentArguments(char *action, int *val1, int *val2, int *val1_unit,
			   int *val2_unit);
int ParseToggleArgument(char *action, char **ret_action, int default_ret,
			char no_toggle);

/*
   function:            FindToken
   description:         find the entry of type 'struct_entry'
                        holding 'key' in 'table'
   returns:             pointer to the matching entry
                        NULL if not found

   table must be sorted in ascending order for FindToken.
*/


#define FindToken(key,table,struct_entry)                               \
        (struct_entry *) bsearch(key,                                   \
                                 (char *)(table),                       \
                                 sizeof(table) / sizeof(struct_entry),  \
                                 sizeof(struct_entry),                  \
                                 (int(*)(const void*, const void*))XCmpToken)

int XCmpToken(const char *s, const char **t);
char *GetFileNameFromPath(char *path);

#endif

