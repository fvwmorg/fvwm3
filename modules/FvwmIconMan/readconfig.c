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

#include "config.h"
#include <ctype.h>
#include <stdlib.h>
#include "FvwmIconMan.h"
#include "readconfig.h"
#include <libs/defaults.h>
#include <libs/fvwmlib.h>
#include <libs/FScreen.h>
#include <libs/FShape.h>
#include <libs/Module.h>

static char const rcsid[] =
  "$Id$";

/************************************************************************
 *
 * Builtin functions:
 *
 ************************************************************************/

extern int builtin_quit (int numargs, BuiltinArg *args);
extern int builtin_printdebug (int numargs, BuiltinArg *args);
extern int builtin_gotobutton (int numargs, BuiltinArg *args);
extern int builtin_gotomanager (int numargs, BuiltinArg *args);
extern int builtin_refresh (int numargs, BuiltinArg *args);
extern int builtin_select (int numargs, BuiltinArg *args);
extern int builtin_sendcommand (int numargs, BuiltinArg *args);
extern int builtin_bif (int numargs, BuiltinArg *args);
extern int builtin_bifn (int numargs, BuiltinArg *args);
extern int builtin_print (int numargs, BuiltinArg *args);
extern int builtin_jmp (int numargs, BuiltinArg *args);
extern int builtin_ret (int numargs, BuiltinArg *args);
extern int builtin_searchforward (int numargs, BuiltinArg *args);
extern int builtin_searchback (int numargs, BuiltinArg *args);
extern int builtin_warp (int numargs, BuiltinArg *args);

/* compiler pseudo-functions */
static int builtin_label (int numargs, BuiltinArg *args);

typedef struct {
  char *name;
  int (*func)(int numargs, BuiltinArg *args);
  int numargs;
  BuiltinArgType args[MAX_ARGS];
} FunctionType;

/*
 * these are now sorted so we can use bsearch on them.
 */
FunctionType builtin_functions[] = {
  { "bif",         builtin_bif,         2, { ButtonArg, JmpArg } },
  { "bifn",        builtin_bifn,        2, { ButtonArg, JmpArg } },
  { "gotobutton",  builtin_gotobutton,  1, { ButtonArg } },
  { "gotomanager", builtin_gotomanager, 1, { ManagerArg } },
  { "jmp",         builtin_jmp,         1, { JmpArg } },
  { "label",	   builtin_label,	1, { StringArg } },
  { "print",       builtin_print,       1, { StringArg } },
  { "printdebug",  builtin_printdebug,  0, {0} },
  { "quit",        builtin_quit,        0, {0} },
  { "refresh",     builtin_refresh,     0, {0} },
  { "ret",         builtin_ret,         0, {0} },
  { "searchback",  builtin_searchback,  1, { StringArg } },
  { "searchforward", builtin_searchforward, 1, { StringArg } },
  { "select",      builtin_select,      0, {0} },
  { "sendcommand", builtin_sendcommand, 1, { StringArg } },
  { "warp",        builtin_warp,        0, {0} }
};

static int num_builtins = sizeof (builtin_functions) / sizeof (FunctionType);

/************************************************************************/


struct charstring
{
  char key;
  int  value;
};

struct charstring key_modifiers[]=
{
  {'s',ShiftMask},
  {'c',ControlMask},
  {'m',Mod1Mask},
  {'1',Mod1Mask},
  {'2',Mod2Mask},
  {'3',Mod3Mask},
  {'4',Mod4Mask},
  {'5',Mod5Mask},
  {'a',AnyModifier},
  {'n',0},
  {0,0}
};

#if FVWM_VERSION == 1
static FILE *config_fp = NULL;
#endif


/* This is only used for printing out the .fvwmrc line if an error
   occured */

#define PRINT_LINE_LENGTH 80
static char current_line[PRINT_LINE_LENGTH];

static void save_current_line (char *s)
{
  char *p = current_line;

  while (*s && p < current_line + PRINT_LINE_LENGTH - 1) {
    if (*s == '\n') {
      *p = '\0';
      return;
    }
    else {
      *p++ = *s++;
    }
  }
  *p = '\0';
}

void print_args (int numargs, BuiltinArg *args)
{
#ifdef PRINT_DEBUG
  int i;

  for (i = 0; i < numargs; i++) {
    switch (args[i].type) {
    case NoArg:
      ConsoleDebug (CONFIG, "NoArg ");
      break;

    case IntArg:
      ConsoleDebug (CONFIG, "Int: %d ", args[i].value.int_value);
      break;

    case StringArg:
      ConsoleDebug (CONFIG, "String: %s ", args[i].value.string_value);
      break;

    case ButtonArg:
      ConsoleDebug (CONFIG, "Button: %d %d ",
		    args[i].value.button_value.offset,
		    args[i].value.button_value.base);
      break;

    case WindowArg:
      ConsoleDebug (CONFIG, "Window: %d %d ",
		    args[i].value.button_value.offset,
		    args[i].value.button_value.base);
      break;

    case ManagerArg:
      ConsoleDebug (CONFIG, "Manager: %d %d ",
		    args[i].value.button_value.offset,
		    args[i].value.button_value.base);
      break;

    case JmpArg:
      ConsoleDebug (CONFIG, "Unprocessed Label Jump: %s ",
		    args[i].value.string_value);
      break;

    default:
      ConsoleDebug (CONFIG, "bad ");
      break;
    }
  }
  ConsoleDebug (CONFIG, "\n");
#endif
}

#ifdef PRINT_DEBUG
static void print_binding (Binding *binding)
{
  int i;
  Function *func;

  if (binding->type == MOUSE_BINDING) {
    ConsoleDebug (CONFIG, "\tMouse: %d\n", binding->Button_Key);
  }
  else {
    ConsoleDebug (CONFIG, "\tKey or action: %d %s\n", binding->Button_Key,
		  binding->key_name);
  }

  ConsoleDebug (CONFIG, "\tModifiers: %d\n", binding->Modifier);
  ConsoleDebug (CONFIG, "\tAction: %s\n", binding->Action);
  ConsoleDebug (CONFIG, "\tFunction struct: 0x%x\n", binding->Action2);
  func = (Function *)(binding->Action2);
  while (func) {
    for (i = 0; i < num_builtins; i++) {
      if (func->func == builtin_functions[i].func) {
	ConsoleDebug (CONFIG, "\tFunction: %s 0x%x ",
		      builtin_functions[i].name, func->func);
	break;
      }
    }
    if (i > num_builtins) {
      ConsoleDebug (CONFIG, "\tFunction: not found 0x%x ", func->func);
    }
    print_args (func->numargs, func->args);
    func = func->next;
  }
}

void print_bindings (Binding *list)
{
  ConsoleDebug (CONFIG, "binding list:\n");
  while (list != NULL) {
    print_binding (list);
    ConsoleDebug (CONFIG, "\n");
    list = list->NextBinding;
  }
}
#else
void print_bindings (Binding *list)
{
}
#endif

static int iswhite (char c)
{
  if (c == ' ' || c == '\t' || c == '\0')
    return 1;
  return 0;
}

static void trim (char *p)
{
  int length = strlen (p) -1;
  int index;
  for(index = length; index > 0; index --)
  {
    if (p[index] == ' ' || p[index] == '\t')
      p[index] = '\0';
    else return;
  }
}

static void skip_space (char **p)
{
  while (**p == ' ' || **p == '\t')
    (*p)++;
}

static void add_to_binding (Binding **list, Binding *binding)
{
  ConsoleDebug (CONFIG, "In add_to_binding:\n");

  binding->NextBinding = *list;
  *list = binding;
  return;
}

static int extract_int (char *p, int *n)
{
  char *s;
  int sign = 1;

  while (isspace ((unsigned char)*p) && *p)
    p++;

  if (*p == '-') {
    sign = -1;
    p++;
  }
  else if (*p == '+') {
    sign = 1;
    p++;
  }

  if (*p == '\0') {
    return 0;
  }

  for (s = p; *s; s++) {
    if (*s < '0' || *s > '9') {
      return 0;
    }
  }

  *n = atoi (p) * sign;

  return 1;
}

/****************************************************************************
 *
 * Gets the next "word" of input from char string indata.
 * "word" is a string with no spaces, or a qouted string.
 * Return value is ptr to indata,updated to point to text after the word
 * which is extracted.
 * token is the extracted word, which is copied into a malloced
 * space, and must be freed after use.
 *
 **************************************************************************/

static void find_context(char *string, int *output, struct charstring *table,
			 char *tline)
{
  int i=0,j=0;
  Bool matched;
  char tmp1;

  *output=0;
  i=0;
  while(i<strlen(string))
    {
      j=0;
      matched = FALSE;
      while((!matched)&&(table[j].key != 0))
	{
	  /* in some BSD implementations, tolower(c) is not defined
	   * unless isupper(c) is true */
	  tmp1=string[i];
	  if(isupper(tmp1))
	    tmp1=tolower(tmp1);
	  /* end of ugly BSD patch */

	  if(tmp1 == table[j].key)
	    {
	      *output |= table[j].value;
	      matched = TRUE;
	    }
	  j++;
	}
      if(!matched)
	{
	  ConsoleMessage ("Bad line: %s\n", current_line);
	  ConsoleMessage ("Bad context: %s\n", string);
	}
      i++;
    }
  return;
}

static int init_config_file (char *file)
{
#if FVWM_VERSION == 1
  if ((config_fp = fopen (file, "r")) == NULL)  {
    ConsoleMessage ("Couldn't open file: %s\n", file);
    return 0;
  }
#else
  InitGetConfigLine(Fvwm_fd,Module);         /* speed up */
#endif
  return 1;
}

static void close_config_file (void)
{
#if FVWM_VERSION == 1
  if (config_fp)
    fclose (config_fp);
#endif
}

static char *parse_button (char *string, BuiltinArg *arg, int *flag,
			   char *pstop_char)
{
  char *rest, *token;
  ButtonValue *bv;
  int n;

  ConsoleDebug (CONFIG, "parse_term: %s\n", string);

  *flag = 1;

  arg->type = ButtonArg;
  bv = &arg->value.button_value;
  bv->offset = 0;
  bv->base = AbsoluteButton;

  rest = DoGetNextToken (string, &token, NULL, ",", pstop_char);
  if (token == NULL) {
    bv->base = NoButton;
    *flag = 0;
    Free(token);
    return NULL;
  }
  if (!strcasecmp (token, "focus")) {
    bv->base = FocusButton;
  }
  else if (!strcasecmp (token, "select")) {
    bv->base = SelectButton;
  }
  else if (!strcasecmp (token, "up")) {
    bv->base = UpButton;
  }
  else if (!strcasecmp (token, "down")) {
    bv->base = DownButton;
  }
  else if (!strcasecmp (token, "left")) {
    bv->base = LeftButton;
  }
  else if (!strcasecmp (token, "right")) {
    bv->base = RightButton;
  }
  else if (!strcasecmp (token, "next")) {
    bv->base = NextButton;
  }
  else if (!strcasecmp (token, "prev")) {
    bv->base = PrevButton;
  }
  else if (extract_int (token, &n)) {
    bv->base = AbsoluteButton;
    bv->offset = n;
  }
  else {
    ConsoleMessage ("Bad button: %s\n", token);
    bv->base = NoButton;
    Free (token);
    *flag = 0;
    return NULL;
  }

  Free (token);
  return rest;
}

static void free_function_list (Function *func)
{
  int i;
  Function *fp = func;

  while (fp) {
    for (i = 0; i < fp->numargs; i++) {
      if (fp->args[i].type == StringArg)
	Free (fp->args[i].value.string_value);
    }
    func = fp;
    fp = fp->next;
    Free (func);
  }
}

static int funccasecmp(const void *key /* actually char* */,
			       const void *member /* actually FunctionType* */)
{
  return strcasecmp((char *)key, ((FunctionType *)member)->name);
}

/*
 * The label function. Should never be called, but we need a pointer to it,
 * and it's useful for debugging purposes to have it defined.
 */
static int builtin_label (int numargs, BuiltinArg *args) {
  int j;
  /* we should _never_ be called */
  ConsoleMessage ( "label" );
  for (j=0; j<numargs; ++j) {
    switch (args[j].type) {
    case StringArg:
      ConsoleMessage ( " %s",args[j].value.string_value );
      break;

    default:
      ConsoleMessage ( " [unknown arg #: %d]", args[j].type );
    }
  }
  ConsoleMessage( " was called. This should not happen.\n" );
  return 0;
}

/* the number of JmpArg arguments that have been created, but not yet
   resolved into IntArgs. */
static int JmpArgs=0;
/* icky, I know, but it'll save unnecessary error-checking. */

static Function *parse_function (char **line, char *pstop_char)
{
  Function *ftype = (Function *)safemalloc (sizeof (Function));
  char *ptr, *name, *tok;
  int j, flag;
  FunctionType *builtin_functions_i;

  ConsoleDebug (CONFIG, "in parse_function\n");

  ptr = DoGetNextToken (*line, &name, NULL, ",", pstop_char);
  if (name == NULL) {
    Free(ftype);
    *line = NULL;
    return NULL;
  }

  builtin_functions_i=bsearch((void *)name, (void *)builtin_functions,
			      num_builtins, sizeof(FunctionType),
			      funccasecmp);
  if (builtin_functions_i) {
    Free (name);
    ftype->func = builtin_functions_i->func;
    ftype->numargs = builtin_functions_i->numargs;
    ftype->next = NULL;

    for (j = 0; j < builtin_functions_i->numargs && *pstop_char != ','; j++) {
      ftype->args[j].type = builtin_functions_i->args[j];
      switch (builtin_functions_i->args[j]) {
      case IntArg:
	ptr = DoGetNextToken (ptr, &tok, NULL, ",", pstop_char);
	if (!tok) {
	  ConsoleMessage ("%s: too few arguments\n",
			  builtin_functions_i->name);
	  Free(ftype);
	  *line = NULL;
	  return NULL;
	}
	if (extract_int (tok, &ftype->args[j].value.int_value) == 0) {
	  ConsoleMessage ("%s: expect integer argument: %s\n",
			  builtin_functions_i->name, tok);
	  Free (tok);
	  Free(ftype);
	  *line = NULL;
	  return NULL;
	}
	Free (tok);
	break;

      case StringArg:
	ptr = DoGetNextToken (ptr, &ftype->args[j].value.string_value,NULL,
			      ",", pstop_char);
	if (!ftype->args[j].value.string_value) {
	  ConsoleMessage ("%s: too few arguments\n",
			  builtin_functions_i->name);
	  *line = NULL;
	  Free(ftype->args[j].value.string_value);
	  Free(ftype);
	  return NULL;
	}
	ftype->args[j].type = builtin_functions_i->args[j];
	break;

      case ButtonArg:
      case WindowArg:
      case ManagerArg:
	ptr = parse_button (ptr, &ftype->args[j], &flag, pstop_char);
	if (!flag) {
	  ConsoleMessage ("%s: too few arguments\n",
			  builtin_functions_i->name);
	  Free(ftype);
	  *line = NULL;
	  return NULL;
	}
	ftype->args[j].type = builtin_functions_i->args[j];
	break;

	/* JmpArg can be a string or an int. However, if 'JmpArg'
	 * is recorded as the argument type in the argument array
	 * for a command, it means it is a string; the code for
	 * 'IntArg' is used instead for numbers. Note also that
	 * if the C function recieves a 'JmpArg' argument it means something
	 * went wrong, since they should all be translated to integer
	 * jump offsets at compile time.
	 */
      case JmpArg:
	ptr = DoGetNextToken (ptr, &tok, NULL, ",", pstop_char);
	if (!tok) {
	  ConsoleMessage ("%s: too few arguments\n",
			  builtin_functions_i->name);
	  Free(tok);
	  Free(ftype);
	  *line=NULL;
	  return NULL;
	}
	if (extract_int(tok, &ftype->args[j].value.int_value) == 0) {
	  ftype->args[j].value.string_value=tok;
	  ftype->args[j].type = JmpArg;
	  ++JmpArgs;
	} else {
	  ftype->args[j].type = IntArg;
	  Free(tok);
	}
	break;

      default:
	ConsoleMessage ("internal error in parse_function\n");
	Free(ftype);
	*line = NULL;
	return NULL;
      }
    }

    if (j != builtin_functions_i->numargs) {
      ConsoleMessage ("%s: too few arguments\n", builtin_functions_i->name);
      Free(ftype);
      *line = NULL;
      return NULL;
    }

    *line = ptr;
    return ftype;
  }

  ConsoleMessage ("Unknown function: %s\n", name);
  Free (name);

  *line = NULL;
  return NULL;
}


/* This is O(N^2) where N = number of instructions. Seems we could do better.
   We'll see how this addition settles before monkeying with it */

static Function *parse_function_list (char *line)
{
  Function *ret = NULL, *tail = NULL, *f, *i;
  char *token;
  int jump_count, j;
  char stop_char;
  char c;

  JmpArgs=0;
  while (line && (f = parse_function(&line, &stop_char))) {
    ConsoleDebug (CONFIG, "parse_function: 0x%lx\n", (unsigned long)f->func);
    /* extra code to check for and remove a 'label' pseudo-function */
    if (f->func==builtin_label) {
      /* scan backwards to fix up references */
      jump_count=0;
      for (i=tail; i!=NULL; i=i->prev) {
	/* scan the command arguments for a 'JmpArg' type */
	for (j=0; j<(i->numargs); ++j) {
	  if (i->args[j].type==JmpArg) {
	    /* we have a winner! */
	    if (!strcasecmp(f->args[0].value.string_value,
			    i->args[j].value.string_value)) {
	      /* the label matches it, so replace with the jump_count */
	      i->args[j].type = IntArg;
	      i->args[j].value.int_value = jump_count;
	      --JmpArgs;
	    }
	  }
	}
	++jump_count;
      }
      Free(f); /* label pseudo-functions never get added to the chain */
    } else {
      if (tail)
	tail->next = f;
      else
	ret = f;
      f->prev=tail;
      tail = f;
    }
    DoGetNextToken (line, &token, NULL, ",", &c);
    if (token && stop_char != ',') {
      ConsoleMessage ("Bad function list, comma expected\n");
      Free (token);
      return NULL;
    }
    stop_char = c;
    Free(token);
  }

  if (JmpArgs!=0) {
    /* someone made a typo and we need to scan to find out what it
     * was.
     */
    for (f=tail; f; f=f->prev) {
      for (j=0; j<(f->numargs); ++j) {
	if (f->args[j].type==JmpArg) {
	  ConsoleMessage ("Attempt to jump to non-existant label %s; "
			  "aborting function list.\n",
			  f->args[j].value.string_value);
	  --JmpArgs;
	}
      }
    }
    if (JmpArgs!=0)
      ConsoleMessage ( "Internal Error: JmpArgs %d not accounted for!\n",
		       JmpArgs );
    tail=NULL;
    f=NULL;
    free_function_list(ret);
    ret=NULL;
    return NULL;
  }
  if (ret == NULL)
    ConsoleMessage ("No function defined\n");
  return ret;
}


Binding *ParseMouseEntry (char *tline)
{
  char modifiers[20],*action,*token;
  Binding *new;
  int button;
  int n1=0,n2=0;
  int mods;

  /* tline points after the key word "key" */
  action = DoGetNextToken(tline,&token, NULL, ",", NULL);
  if(token != NULL) {
    n1 = sscanf(token,"%d",&button);
    if (n1 == 1 && (button < 0 || button > NUMBER_OF_MOUSE_BUTTONS))
    {
      /* syntax error */
      n1 = 0;
    }
    Free(token);
  }

  action = DoGetNextToken(action,&token, NULL, ",", NULL);
  if(token != NULL) {
    n2 = sscanf(token,"%19s",modifiers);
    Free(token);
  }
  if((n1 != 1)||(n2 != 1))
    ConsoleMessage ("Mouse binding: Syntax error");

  find_context(modifiers,&mods,key_modifiers,tline);
  if((mods & AnyModifier)&&(mods&(~AnyModifier))) {
    ConsoleMessage ("Binding specified AnyModifier and other modifers too. "
                    "Excess modifiers will be ignored.");
  }

  new = (Binding *)safemalloc(sizeof(Binding));
  memset(new, 0, sizeof(Binding));
  new->type = MOUSE_BINDING;
  new->Button_Key = button;
  new->Modifier = mods;
  new->Action = stripcpy(action);
  new->Action2 = parse_function_list (action);

  if (!new->Action2) {
    ConsoleMessage ("Bad action: %s\n", action);
    Free (new->Action);
    Free (new);
    return NULL;
  }

  ConsoleDebug (CONFIG, "Mouse: %d %d %s\n", new->Button_Key,
		new->Modifier, (char*)new->Action);

  return new;
}

static Binding *ParseKeyEntry (char *tline)
{
  char *action,modifiers[20],key[20],*ptr, *token, *actionstring, *keystring;
  Binding *new = NULL, *temp, *last = NULL;
  Function *func = NULL;
  int i,min,max;
  int n1=0,n2=0;
  KeySym keysym;
  int mods;

  /* tline points after the key word "key" */
  ptr = tline;

  ptr = DoGetNextToken(ptr,&token, NULL, ",", NULL);
  if(token != NULL) {
    n1 = sscanf(token,"%19s",key);
    Free(token);
  }

  action = DoGetNextToken(ptr,&token, NULL, ",", NULL);
  if(token != NULL) {
    n2 = sscanf(token,"%19s",modifiers);
    Free(token);
  }

  if((n1 != 1)||(n2 != 1))
    ConsoleMessage ("Syntax error in line %s",tline);

  find_context(modifiers,&mods,key_modifiers,tline);
  if((mods & AnyModifier)&&(mods&(~AnyModifier))) {
    ConsoleMessage ("Binding specified AnyModifier and other modifers too. Excess modifiers will be ignored.");
  }

  /*
   * Don't let a 0 keycode go through, since that means AnyKey to the
   * XGrabKey call in GrabKeys().
   */
  if ((keysym = XStringToKeysym(key)) == NoSymbol ||
      XKeysymToKeycode(theDisplay, keysym) == 0) {
    ConsoleMessage ("Can't find keysym: %s\n", key);
    return NULL;
  }


  XDisplayKeycodes(theDisplay, &min, &max);
  for (i=min; i<=max; i++) {
    if (XKeycodeToKeysym(theDisplay, i, 0) == keysym) {
      if (!func) {
	func = parse_function_list (action);
	if (!func) {
	  ConsoleMessage ("Bad action: %s\n", action);
	  return NULL;
	}
	actionstring = stripcpy(action);
	keystring = stripcpy(key);
      } else {
	actionstring = keystring = NULL;
      }
      temp = new;
      new  = (Binding *)safemalloc(sizeof(Binding));
      memset(new, 0, sizeof(Binding));
      new->type = KEY_BINDING;
      new->Button_Key = i;
      new->key_name = keystring;
      new->Modifier = mods;
      new->Action = actionstring;
      new->Action2 = func;
      new->NextBinding = temp;
      if (!last) {
	last = new;
      }

      ConsoleDebug (CONFIG, "Key: %d %s %d %s\n", i, new->key_name,
		    mods, (char*)new->Action);
    }
  }
  return new;
}

static Binding *ParseSimpleEntry (char *tline)
{
  Binding *new;
  Function *func;

  func = parse_function_list (tline);
  if (func == NULL)
    return NULL;

  new = (Binding *)safemalloc (sizeof (Binding));
  memset(new, 0, sizeof(Binding));
  new->type = KEY_BINDING;
  new->key_name = "select";
  new->Action = stripcpy (tline);
  new->Action2 = func;

  return new;
}

void run_binding (WinManager *man, Action action)
{
  Binding *binding = man->bindings[action];
  ConsoleDebug (CONFIG, "run_binding:\n");
  print_bindings (binding);

  if (binding && binding->Action2 && ((Function *)(binding->Action2))->func) {
    run_function_list (binding->Action2);
  }
}

void execute_function (char *string)
{
  Function *func = parse_function_list (string);
  if (func == NULL) {
    return;
  }
  else {
    run_function_list (func);
    free_function_list (func);
  }
}

static int GetConfigLineWrapper (int *fd, char **tline)
{
#if FVWM_VERSION == 1

  static char buffer[1024];
  char *temp;

  if (fgets (buffer, 1024, config_fp)) {
    *tline = buffer;
    temp = strchr (*tline, '\n');
    if (temp) {
      *temp = '\0';
    }
    else {
      ConsoleMessage ("line too long\n");
      exit (1);
    }
    return 1;
  }

#else

  char *temp;

  GetConfigLine (fd, tline);
  if (*tline) {
    if (strncasecmp(*tline, "Colorset", 8) == 0)
    {
      LoadColorset(&(*tline)[8]);
    }
    else if (strncasecmp(*tline, XINERAMA_CONFIG_STRING,
			 sizeof(XINERAMA_CONFIG_STRING) - 1) == 0)
    {
      FScreenConfigureModule(
	(*tline) + sizeof(XINERAMA_CONFIG_STRING) - 1);
    }
    else if (strncasecmp(*tline, "IgnoreModifiers", 15) == 0)
    {
      sscanf((*tline) + 16, "%d", &mods_unused);
    }
    temp = strchr (*tline, '\n');
    if (temp) {
      *temp = '\0';
    }
    /* grok the global config lines */
/*    if (strncasecmp(*tline, DEFGRAPHSTR, DEFGRAPHLEN)==0) {
      ParseGraphics(theDisplay, *tline, G);
      SavePictureCMap (theDisplay, G->viz, G->cmap, G->depth);
    }
*/    /* add colorlimit in here */
    return 1;
  }

#endif

  return 0;
}

static char *read_next_cmd (ReadOption flag)
{
  static ReadOption status;
  static char *buffer;
  static char *retstring, displaced, *cur_pos;

  retstring = NULL;
  if (flag != READ_LINE && !(flag & status))
    return NULL;

  switch (flag) {
  case READ_LINE:
    while (GetConfigLineWrapper (Fvwm_fd, &buffer)) {
      cur_pos = buffer;
      skip_space (&cur_pos);
      if (!strncasecmp (Module, cur_pos, ModuleLen)) {
        retstring = cur_pos;
        cur_pos += ModuleLen;
        displaced = *cur_pos;
        if (displaced == '*')
          status = READ_OPTION;
        else if (displaced == '\0')
          status = READ_LINE;
        else if (iswhite (displaced))
          status = READ_ARG;
        else
          status = READ_LINE;
        break;
      }
    }
    break;

  case READ_OPTION:
    *cur_pos = displaced;
    retstring = ++cur_pos;
    while (*cur_pos != '*' && !iswhite (*cur_pos))
      cur_pos++;
    displaced = *cur_pos;
    *cur_pos = '\0';
    if (displaced == '*')
      status = READ_OPTION;
    else if (displaced == '\0')
      status = READ_LINE;
    else if (iswhite (displaced))
      status = READ_ARG;
    else
      status = READ_LINE;
    break;

  case READ_ARG:
    *cur_pos = displaced;
    skip_space (&cur_pos);
    retstring = cur_pos;
    while (!iswhite (*cur_pos))
      cur_pos++;
    displaced = *cur_pos;
    *cur_pos = '\0';
    if (displaced == '\0')
      status = READ_LINE;
    else if (iswhite (displaced))
      status = READ_ARG;
    else
      status = READ_LINE;
    break;

  case READ_REST_OF_LINE:
    status = READ_LINE;
    *cur_pos = displaced;
    skip_space (&cur_pos);
    retstring = cur_pos;
    break;
  }

  if (retstring && retstring[0] == '\0')
    retstring = NULL;

  return retstring;
}

static char *conditional_copy_string (char **s1, char *s2)
{
  if (*s1)
    return *s1;
  else
    return copy_string (s1, s2);
}

static NameType parse_format_dependencies (char *format)
{
  NameType flags = NO_NAME;

  ConsoleDebug (CONFIG, "Parsing format: %s\n", format);

  while (*format) {
    if (*format != '%') {
      format++;
    }
    else {
      format++;
      if (*format == 'i')
	flags |= ICON_NAME;
      else if (*format == 't')
	flags |= TITLE_NAME;
      else if (*format == 'c')
	flags |= CLASS_NAME;
      else if (*format == 'r')
	flags |= RESOURCE_NAME;
      else if (*format != '%')
	ConsoleMessage ("Bad format string: %s\n", format);
    }
  }
#ifdef PRINT_DEBUG
  ConsoleDebug (CONFIG, "Format depends on: ");
  if (flags & ICON_NAME)
    ConsoleDebug (CONFIG, "Icon ");
  if (flags & TITLE_NAME)
    ConsoleDebug (CONFIG, "Title ");
  if (flags & CLASS_NAME)
    ConsoleDebug (CONFIG, "Class ");
  if (flags & RESOURCE_NAME)
    ConsoleDebug (CONFIG, "Resource ");
  ConsoleDebug (CONFIG, "\n");
#endif

  return flags;
}

#define SET_MANAGER(manager,field,value)                           \
   do {                                                            \
     int id = manager;                                             \
     if (id == -1) {                                               \
       for (id = 0; id < globals.num_managers; id++) {             \
	 globals.managers[id]. field = value;                      \
       }                                                           \
     }                                                             \
     else if (id < globals.num_managers) {                         \
       globals.managers[id]. field = value;                        \
     }                                                             \
     else {                                                        \
       ConsoleMessage ("Internal error in SET_MANAGER: %d\n", id); \
     }                                                             \
   } while (0)

static void handle_button_config (int manager, int context, char *option)
{
  char *p;
  ButtonState state;

  p = read_next_cmd (READ_ARG);
  if (!p) {
    ConsoleMessage ("Bad line: %s\n", current_line);
    ConsoleMessage ("Need argument to %s\n", option);
    return;
  }
  else if (!strcasecmp (p, "flat")) {
    state = BUTTON_FLAT;
  }
  else if (!strcasecmp (p, "up")) {
    state = BUTTON_UP;
  }
  else if (!strcasecmp (p, "down")) {
    state = BUTTON_DOWN;
  }
  else if (!strcasecmp (p, "raisededge")) {
    state = BUTTON_EDGEUP;
  }
  else if (!strcasecmp (p, "sunkedge")) {
    state = BUTTON_EDGEDOWN;
  }
  else {
    ConsoleMessage ("Bad line: %s\n", current_line);
    ConsoleMessage ("This isn't a valid button state: %s\n", p);
    return;
  }
  ConsoleDebug (CONFIG, "Setting buttonState[%s] to %s\n",
		contextDefaults[context].name, p);
  SET_MANAGER (manager, buttonState[context], state);

  /* check for optional fore color */
  p = read_next_cmd (READ_ARG);
  if ( !p )
    return;

  SET_MANAGER (manager, foreColorName[context],
	       copy_string (&globals.managers[id].foreColorName[context], p));

  /* check for optional back color */
  p = read_next_cmd (READ_ARG);
  if ( !p )
    return;

  ConsoleDebug (CONFIG, "Setting backColorName[%s] to %s\n",
		contextDefaults[context].name, p);
  SET_MANAGER (manager, backColorName[context],
	       copy_string (&globals.managers[id].backColorName[context], p));
}

void read_in_resources (char *file)
{
  char *p, *q;
  int i, n, manager;
  char *option1;
  Binding *binding = NULL;
  Resolution r;
  Reverse rv;

  if (!init_config_file (file))
    return;

  while ((p = read_next_cmd (READ_LINE))) {
    ConsoleDebug (CONFIG, "line: %s\n", p);
    save_current_line (p);

    option1 = read_next_cmd (READ_OPTION);
    if (option1 == NULL)
      continue;

    ConsoleDebug (CONFIG, "option1: %s\n", option1);
    if (!strcasecmp (option1, "nummanagers")) {
      /* If in transient mode, just use the default of 1 manager */
      if (!globals.transient) {
	p = read_next_cmd (READ_ARG);
	if (!p) {
	  ConsoleMessage ("Bad line: %s\n", current_line);
	  continue;
	}
	if (extract_int (p, &n) == 0) {
	  ConsoleMessage ("This is not a number: %s\n", p);
	  ConsoleMessage ("Bad line: %s\n", current_line);
	  continue;
	}
	if (n > 0) {
	  allocate_managers (n);
	  ConsoleDebug (CONFIG, "num managers: %d\n", n);
	}
	else {
	  ConsoleMessage ("Bad line: %s\n", current_line);
	  ConsoleMessage ("You can't have zero managers. "
			  "I'll give you one.\n");
	  allocate_managers (1);
	}
      }
    }
    else {
      /* these all can specify a specific manager */

      if (globals.managers == NULL) {
	ConsoleDebug (CONFIG, "I'm assuming you only want one manager\n");
	allocate_managers (1);
      }

      manager = 0;

      if (option1[0] >= '0' && option1[0] <= '9') {
	if (globals.transient) {
	  ConsoleDebug (CONFIG, "In transient mode. Ignoring this line\n");
	  continue;
	}
	if (extract_int (option1, &manager) == 0 ||
	    manager <= 0 || manager > globals.num_managers) {
	  ConsoleMessage ("Bad line: %s\n", current_line);
	  ConsoleMessage ("This is not a valid manager: %s.\n", option1);
	  manager = 0;
	}
	option1 = read_next_cmd (READ_OPTION);
	if (!option1) {
	  ConsoleMessage ("Bad line: %s\n", current_line);
	  continue;
	}
      }
      else if (!strcasecmp (option1, "transient")) {
	if (globals.transient) {
	  ConsoleDebug (CONFIG, "Transient manager config line\n");
	  manager = 1;
	  option1 = read_next_cmd (READ_OPTION);
	  if (!option1) {
	    ConsoleMessage ("Bad line: %s\n", current_line);
	    continue;
	  }
	}
	else {
	  ConsoleDebug (CONFIG, "Not in transient mode. Ignoring this line\n");
	  continue;
	}
      }

      manager--; /* -1 means global */

      ConsoleDebug (CONFIG, "Applying %s to manager %d\n", option1, manager);

      if (!strcasecmp (option1, "action")) {
	p = read_next_cmd (READ_ARG);
	if (!p) {
	  ConsoleMessage ("Bad line: %s\n", current_line);
	  continue;
	}

	if (!strcasecmp (p, "mouse")) {
	  i = MOUSE;
	}
	else if (!strcasecmp (p, "key")) {
	  i = KEYPRESS;
	}
	else if (!strcasecmp (p, "select")) {
	  i = SELECT;
	}
	else {
	  ConsoleMessage ("Bad line: %s\n", current_line);
	  ConsoleMessage ("This isn't a valid action name: %s\n", p);
	  continue;
	}

	q = read_next_cmd (READ_REST_OF_LINE);
	if (!q) {
	  ConsoleMessage ("Bad line: %s\n", current_line);
	  ConsoleMessage ("Need an action\n");
	  continue;
	}

	switch (i) {
	case MOUSE:
	  binding = ParseMouseEntry (q);
	  break;

	case KEYPRESS:
	  binding = ParseKeyEntry (q);
	  break;

	case SELECT:
	  binding = ParseSimpleEntry (q);
	  break;
	}

	if (binding == NULL) {
	  ConsoleMessage ("Offending line: %s\n", current_line);
	  ConsoleMessage ("Bad action\n");
	  continue;
	}

	if (manager == -1) {
	  int j;
	  for (j = 0; j < globals.num_managers; j++) {
	    add_to_binding (&globals.managers[j].bindings[i], binding);
	  }
	}
	else if (manager < globals.num_managers) {
	  add_to_binding (&globals.managers[manager].bindings[i], binding);
	}
	else {
	  ConsoleMessage ("Bad line: %s\n", current_line);
	  ConsoleMessage ("There's no manager %d\n", manager);
	}
      }
      else if (!strcasecmp (option1, "colorset")) {
	p = read_next_cmd (READ_ARG);
	if (!p) {
	  ConsoleMessage ("Bad line: %s\n", current_line);
	  continue;
	}
	if (extract_int (p, &n) == 0) {
	  ConsoleMessage ("This is not a number: %s\n", p);
	  ConsoleMessage ("Bad line: %s\n", current_line);
	  continue;
	}
        for ( i = 0; i < NUM_CONTEXTS; i++ ) {
	  SET_MANAGER(manager, colorsets[i], n);
	  AllocColorset(n);
	}
      }
      else if (!strcasecmp (option1, "background")) {
	p = read_next_cmd (READ_ARG);
	if (!p) {
	  ConsoleMessage ("Bad line: %s\n", current_line);
	  continue;
	}
	ConsoleDebug (CONFIG, "default background: %s\n", p);

        for ( i = 0; i < NUM_CONTEXTS; i++ )
	  SET_MANAGER (manager, backColorName[i],
	    conditional_copy_string (&globals.managers[id].backColorName[i],
				     p));
      }
      else if (!strcasecmp (option1, "buttongeometry")) {
	p = read_next_cmd (READ_ARG);
	if (!p) {
	  ConsoleMessage ("Bad line: %s\n", current_line);
	  continue;
	}

	SET_MANAGER (manager, button_geometry_str,
		     copy_string (&globals.managers[id].button_geometry_str, p));
      }
      else if (!strcasecmp (option1, "dontshow")) {
	char *token = NULL;
	p = read_next_cmd (READ_REST_OF_LINE);
	if (!p) {
	  ConsoleMessage ("Bad line: %s\n", current_line);
	  continue;
	}
	p = DoGetNextToken (p, &token, NULL, ",", NULL);
	if (!token) {
	  ConsoleMessage ("Bad line: %s\n", current_line);
	  continue;
	}
	do {
	  ConsoleDebug (CONFIG, "dont show: %s\n", token);
	  if (manager == -1) {
	    int i;
	    for (i = 0; i < globals.num_managers; i++)
	      add_to_stringlist (&globals.managers[i].dontshow, token);
	  }
	  else {
	    add_to_stringlist (&globals.managers[manager].dontshow, token);
	  }
	  Free (token);
	  p = DoGetNextToken (p, &token, NULL, ",", NULL);
	} while (token);
	if (token)
	  Free(token);
      }
      else if (!strcasecmp (option1, "drawicons")) {
        if (!FMiniIconsSupported)
        {
          ConsoleMessage ("DrawIcons support not compiled in\n");
        }
        else
        {
	  p = read_next_cmd (READ_ARG);
	  if (!p) {
            ConsoleMessage ("Bad line: %s\n", current_line);
            ConsoleMessage ("Need argument to drawicons\n");
            continue;
          }
          if (!strcasecmp (p, "true")) {
            i = 1;
          }
          /* [NFM 3 Dec 97] added support for FvwmIconMan*drawicons "always" */
          else if (!strcasecmp (p, "always")) {
            i = 2;
          }
          else if (!strcasecmp (p, "false")) {
	    i = 0;
          }
          else {
            ConsoleMessage ("Bad line: %s\n", current_line);
            ConsoleMessage ("What is this: %s?\n", p);
            continue;
          }
          ConsoleDebug (CONFIG, "Setting drawicons to: %d\n", i);
          SET_MANAGER (manager, draw_icons, i);
        }
      }
      else if (!strcasecmp (option1, "followfocus")) {
	p = read_next_cmd (READ_ARG);
	if (!p) {
	  ConsoleMessage ("Bad line: %s\n", current_line);
	  ConsoleMessage ("Need argument to followfocus\n");
	  continue;
	}
	if (!strcasecmp (p, "true")) {
	  i = 1;
	}
	else if (!strcasecmp (p, "false")) {
	  i = 0;
	}
	else {
	  ConsoleMessage ("Bad line: %s\n", current_line);
	  ConsoleMessage ("What is this: %s?\n", p);
	  continue;
	}
	ConsoleDebug (CONFIG, "Setting followfocus to: %d\n", i);
	SET_MANAGER (manager, followFocus, i);
      }
      else if (!strcasecmp (option1, "showonlyiconic")) {
       p = read_next_cmd (READ_ARG);
       if (!p) {
         ConsoleMessage ("Bad line: %s\n", current_line);
         ConsoleMessage ("Need argument to showonlyiconic\n");
         continue;
       }
       if (!strcasecmp (p, "true")) {
         i = 1;
       }
       else if (!strcasecmp (p, "false")) {
         i = 0;
       }
       else {
         ConsoleMessage ("Bad line: %s\n", current_line);
         ConsoleMessage ("What is this: %s?\n", p);
         continue;
       }
       ConsoleDebug (CONFIG, "Setting showonlyiconic to: %d\n", i);
       SET_MANAGER (manager, showonlyiconic, i);
      }
      else if (!strcasecmp (option1, "font")) {
	p = read_next_cmd (READ_REST_OF_LINE);
	trim(p);
	if (!p) {
	  ConsoleMessage ("Bad line: %s\n", current_line);
	  continue;
	}
	ConsoleDebug (CONFIG, "font: %s\n", p);

	SET_MANAGER (manager, fontname,
		     copy_string (&globals.managers[id].fontname, p));
      }
      else if (!strcasecmp (option1, "foreground")) {
	p = read_next_cmd (READ_ARG);
	if (!p) {
	  ConsoleMessage ("Bad line: %s\n", current_line);
	  continue;
	}
	ConsoleDebug (CONFIG, "default foreground: %s\n", p);

        for ( i = 0; i < NUM_CONTEXTS; i++ )
	SET_MANAGER (manager, foreColorName[i],
           conditional_copy_string (&globals.managers[id].foreColorName[i],
				    p));
      }
      else if (!strcasecmp (option1, "format")) {
	char *token;
	NameType flags;

	p = read_next_cmd (READ_REST_OF_LINE);
	if (!p) {
	  ConsoleMessage ("Bad line: %s\n", current_line);
	  continue;
	}
	DoGetNextToken (p, &token, NULL, ",", NULL);
	if (!token)
	  {
	    token = (char *)safemalloc(1);
	    *token = 0;
	  }

	SET_MANAGER (manager, formatstring,
		     copy_string (&globals.managers[id].formatstring, token));
	flags = parse_format_dependencies (token);
	SET_MANAGER (manager, format_depend, flags);
	Free (token);
      }
      else if (!strcasecmp (option1, "geometry")) {
	ConsoleMessage ("Geometry option no longer supported.\n");
	ConsoleMessage ("Use ManagerGeometry and ButtonGeometry.\n");
      }
      else if (!strcasecmp (option1, "iconname")) {
	char *token;
	p = read_next_cmd (READ_REST_OF_LINE);
	if (!p) {
	  ConsoleMessage ("Bad line: %s\n", current_line);
	  continue;
	}
	DoGetNextToken (p, &token, NULL, ",", NULL);
	if (!token)
	  {
	    token = (char *)safemalloc(1);
	    *token = 0;
	  }

	SET_MANAGER (manager, iconname,
		     copy_string (&globals.managers[id].iconname, token));
	Free (token);
      }
      else if (!strcasecmp (option1, "managergeometry")) {
	p = read_next_cmd (READ_ARG);
	if (!p) {
	  ConsoleMessage ("Bad line: %s\n", current_line);
	  continue;
	}

	SET_MANAGER (manager, geometry_str,
		     copy_string (&globals.managers[id].geometry_str, p));
      }
      else if (!strcasecmp (option1, "resolution")) {
	p = read_next_cmd (READ_ARG);
	if (!p) {
	  ConsoleMessage ("Bad line: %s\n", current_line);
	  continue;
	}
	ConsoleDebug (CONFIG, "resolution: %s\n", p);
	if (!strcasecmp (p, "global"))
	  r = SHOW_GLOBAL;
	else if (!strcasecmp (p, "desk"))
	  r = SHOW_DESKTOP;
	else if (!strcasecmp (p, "page"))
	  r = SHOW_PAGE;
	else if (!strcasecmp (p, "screen"))
	  r = SHOW_SCREEN;
	else if (!strcasecmp (p, "!desk"))
	  r = NO_SHOW_DESKTOP;
	else if (!strcasecmp (p, "!page"))
	  r = NO_SHOW_PAGE;
	else if (!strcasecmp (p, "!screen"))
	  r = NO_SHOW_SCREEN;
	else {
	  ConsoleMessage ("Bad line: %s\n", current_line);
	  ConsoleMessage ("What kind of resolution is this?\n");
	  continue;
	}

	SET_MANAGER (manager, res, r);
      }
      else if (!strcasecmp (option1, "reverse")) {
	p = read_next_cmd (READ_ARG);
	if (!p) {
	  ConsoleMessage ("Bad line: %s\n", current_line);
	  continue;
	}
	ConsoleDebug (CONFIG, "reverse: %s\n", p);
	if (!strcasecmp (p, "none"))
	  rv = REVERSE_NONE;
	else if (!strcasecmp (p, "icon"))
	  rv = REVERSE_ICON;
	else if (!strcasecmp (p, "normal"))
	  rv = REVERSE_NORMAL;
	else {
	  ConsoleMessage ("Bad line: %s\n", current_line);
	  ConsoleMessage ("What kind of reverse is this?\n");
	  continue;
	}

	SET_MANAGER (manager, rev, rv);
      }
      else if (!strcasecmp (option1, "shape")) {
	p = read_next_cmd (READ_ARG);
	if (!p) {
	  ConsoleMessage ("Bad line: %s\n", current_line);
	  ConsoleMessage ("Need argument to followfocus\n");
	  continue;
	}
	if (!strcasecmp (p, "true")) {
	  i = 1;
	}
	else if (!strcasecmp (p, "false")) {
	  i = 0;
	}
	else {
	  ConsoleMessage ("Bad line: %s\n", current_line);
	  ConsoleMessage ("What is this: %s?\n", p);
	  continue;
	}
	if (!FHaveShapeExtension && i) {
	  ConsoleMessage ("Shape support not compiled in\n");
	  continue;
	}
	ConsoleDebug (CONFIG, "Setting shape to: %d\n", i);
	SET_MANAGER (manager, shaped, i);
      }
      else if (!strcasecmp (option1, "show")) {
	char *token = NULL;
	p = read_next_cmd (READ_REST_OF_LINE);
	if (!p) {
	  ConsoleMessage ("Bad line: %s\n", current_line);
	  continue;
	}
	p = DoGetNextToken (p, &token, NULL, ",", NULL);
	if (!token) {
	  ConsoleMessage ("Bad line: %s\n", current_line);
	  continue;
	}
	do {
	  ConsoleDebug (CONFIG, "show: %s\n", token);
	  if (manager == -1) {
	    int i;
	    for (i = 0; i < globals.num_managers; i++)
	      add_to_stringlist (&globals.managers[i].show, token);
	  }
	  else {
	    add_to_stringlist (&globals.managers[manager].show, token);
	  }
	  Free (token);
	  p = DoGetNextToken (p, &token, NULL, ",", NULL);
	} while (token);
	if (token)
	  Free(token);
      }
      else if (!strcasecmp (option1, "showtitle")) {
	ConsoleMessage ("Bad line: %s\n", current_line);
	ConsoleMessage ("showtitle is no longer an option. Use format\n");
	continue;
      }
      else if (!strcasecmp (option1, "sort")) {
	p = read_next_cmd (READ_ARG);
	if (!p) {
	  ConsoleMessage ("Bad line: %s\n", current_line);
	  ConsoleMessage ("Need argument to sort\n");
	  continue;
	}
	if (!strcasecmp (p, "name")) {
	  i = SortName;
	}
	else if (!strcasecmp (p, "namewithcase")) {
	  i = SortNameCase;
	}
	else if (!strcasecmp (p, "id")) {
	  i = SortId;
	}
	else if (!strcasecmp (p, "none")) {
	  i = SortNone;
	}
	else if (!strcasecmp (p, "false") || !strcasecmp (p, "true")) {
	  /* Old options */
	  ConsoleMessage ("FvwmIconMan*sort option no longer takes "
			  "true or false value\n"
			  "Please read the latest manpage\n");
	  continue;
	}
	else {
	  ConsoleMessage ("Bad line: %s\n", current_line);
	  ConsoleMessage ("What is this: %s?\n", p);
	  continue;
	}
	ConsoleDebug (CONFIG, "Setting sort to: %d\n", i);
	SET_MANAGER (manager, sort, i);
      }
      else if (!strcasecmp (option1, "NoIconAction")) {
	char *token;
	p = read_next_cmd (READ_REST_OF_LINE);
	if (!p) {
	  ConsoleMessage ("Bad line: %s\n", current_line);
	  continue;
	}
	DoGetNextToken (p, &token, NULL, ",", NULL);
	if (!token)
	  {
	    token = (char *)safemalloc(1);
	    *token = 0;
	  }

	SET_MANAGER (manager, AnimCommand,
		     copy_string (&globals.managers[id].AnimCommand, token));
	Free (token);
      }
      else if (!strcasecmp (option1, "title")) {
	char *token;
	p = read_next_cmd (READ_REST_OF_LINE);
	if (!p) {
	  ConsoleMessage ("Bad line: %s\n", current_line);
	  continue;
	}
	DoGetNextToken (p, &token, NULL, ",", NULL);
	if (!token)
	  {
	    token = (char *)safemalloc(1);
	    *token = 0;
	  }

	SET_MANAGER (manager, titlename,
		     copy_string (&globals.managers[id].titlename, token));
	Free (token);
      }
      else if (!strcasecmp (option1, "iconButton")) {
	handle_button_config (manager, ICON_CONTEXT, option1);
      }
      else if (!strcasecmp (option1, "plainButton")) {
	handle_button_config (manager, PLAIN_CONTEXT, option1);
      }
      else if (!strcasecmp (option1, "selectButton")) {
	handle_button_config (manager, SELECT_CONTEXT, option1);
      }
      else if (!strcasecmp (option1, "focusButton")) {
	handle_button_config (manager, FOCUS_CONTEXT, option1);
      }
      else if (!strcasecmp (option1, "focusandselectButton")) {
	handle_button_config (manager, FOCUS_SELECT_CONTEXT, option1);
      }
      else if (!strcasecmp (option1, "titlebutton")) {
	handle_button_config (manager, TITLE_CONTEXT, option1);
      }
      else if (!strcasecmp (option1, "titlecolorset")) {
	p = read_next_cmd (READ_ARG);
	if (!p) {
	  ConsoleMessage ("Bad line: %s\n", current_line);
	  continue;
	}
	if (extract_int (p, &n) == 0) {
	  ConsoleMessage ("This is not a number: %s\n", p);
	  ConsoleMessage ("Bad line: %s\n", current_line);
	  continue;
	}
	SET_MANAGER(manager, colorsets[TITLE_CONTEXT], n);
	AllocColorset(n);
      }
      else if (!strcasecmp (option1, "focusandselectcolorset")) {
	p = read_next_cmd (READ_ARG);
	if (!p) {
	  ConsoleMessage ("Bad line: %s\n", current_line);
	  continue;
	}
	if (extract_int (p, &n) == 0) {
	  ConsoleMessage ("This is not a number: %s\n", p);
	  ConsoleMessage ("Bad line: %s\n", current_line);
	  continue;
	}
	SET_MANAGER(manager, colorsets[FOCUS_SELECT_CONTEXT], n);
	AllocColorset(n);
      }
      else if (!strcasecmp (option1, "focuscolorset")) {
	p = read_next_cmd (READ_ARG);
	if (!p) {
	  ConsoleMessage ("Bad line: %s\n", current_line);
	  continue;
	}
	if (extract_int (p, &n) == 0) {
	  ConsoleMessage ("This is not a number: %s\n", p);
	  ConsoleMessage ("Bad line: %s\n", current_line);
	  continue;
	}
	SET_MANAGER(manager, colorsets[FOCUS_CONTEXT], n);
	AllocColorset(n);
      }
      else if (!strcasecmp (option1, "selectcolorset")) {
	p = read_next_cmd (READ_ARG);
	if (!p) {
	  ConsoleMessage ("Bad line: %s\n", current_line);
	  continue;
	}
	if (extract_int (p, &n) == 0) {
	  ConsoleMessage ("This is not a number: %s\n", p);
	  ConsoleMessage ("Bad line: %s\n", current_line);
	  continue;
	}
	SET_MANAGER(manager, colorsets[SELECT_CONTEXT], n);
	AllocColorset(n);
      }
      else if (!strcasecmp (option1, "plaincolorset")) {
	p = read_next_cmd (READ_ARG);
	if (!p) {
	  ConsoleMessage ("Bad line: %s\n", current_line);
	  continue;
	}
	if (extract_int (p, &n) == 0) {
	  ConsoleMessage ("This is not a number: %s\n", p);
	  ConsoleMessage ("Bad line: %s\n", current_line);
	  continue;
	}
	SET_MANAGER(manager, colorsets[PLAIN_CONTEXT], n);
	AllocColorset(n);
      }
      else if (!strcasecmp (option1, "iconcolorset")) {
	p = read_next_cmd (READ_ARG);
	if (!p) {
	  ConsoleMessage ("Bad line: %s\n", current_line);
	  continue;
	}
	if (extract_int (p, &n) == 0) {
	  ConsoleMessage ("This is not a number: %s\n", p);
	  ConsoleMessage ("Bad line: %s\n", current_line);
	  continue;
	}
	SET_MANAGER(manager, colorsets[ICON_CONTEXT], n);
	AllocColorset(n);
      }
      else if (!strcasecmp (option1, "usewinlist")) {
	p = read_next_cmd (READ_ARG);
	if (!p) {
	  ConsoleMessage ("Bad line: %s\n", current_line);
	  ConsoleMessage ("Need argument to usewinlist\n");
	  continue;
	}
	if (!strcasecmp (p, "true")) {
	  i = 1;
	}
	else if (!strcasecmp (p, "false")) {
	  i = 0;
	}
	else {
	  ConsoleMessage ("Bad line: %s\n", current_line);
	  ConsoleMessage ("What is this: %s?\n", p);
	  continue;
	}
	ConsoleDebug (CONFIG, "Setting usewinlist to: %d\n", i);
	SET_MANAGER (manager, usewinlist, i);
      }
      else {
	ConsoleMessage ("Bad line: %s\n", current_line);
	ConsoleMessage ("Unknown option: %s\n", p);
      }
    }
  }

  if (globals.managers == NULL) {
    ConsoleDebug (CONFIG, "I'm assuming you only want one manager\n");
    allocate_managers (1);
  }
  print_managers();
  close_config_file();
}

