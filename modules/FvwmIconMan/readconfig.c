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
 * along with this program; if not, see: <http://www.gnu.org/licenses/>
 */

#include "config.h"
#include <ctype.h>
#include "FvwmIconMan.h"
#include "readconfig.h"
#include "xmanager.h"
#include "libs/defaults.h"
#include "libs/fvwmlib.h"
#include "libs/FScreen.h"
#include "libs/FShape.h"
#include "libs/Module.h"
#include "libs/Parse.h"
#include "libs/Strings.h"

/*
 *
 * Builtin functions:
 *
 */

extern int builtin_quit(int numargs, BuiltinArg *args);
extern int builtin_printdebug(int numargs, BuiltinArg *args);
extern int builtin_gotobutton(int numargs, BuiltinArg *args);
extern int builtin_gotomanager(int numargs, BuiltinArg *args);
extern int builtin_refresh(int numargs, BuiltinArg *args);
extern int builtin_select(int numargs, BuiltinArg *args);
extern int builtin_sendcommand(int numargs, BuiltinArg *args);
extern int builtin_bif(int numargs, BuiltinArg *args);
extern int builtin_bifn(int numargs, BuiltinArg *args);
extern int builtin_print(int numargs, BuiltinArg *args);
extern int builtin_jmp(int numargs, BuiltinArg *args);
extern int builtin_ret(int numargs, BuiltinArg *args);
extern int builtin_searchforward(int numargs, BuiltinArg *args);
extern int builtin_searchback(int numargs, BuiltinArg *args);
extern int builtin_warp(int numargs, BuiltinArg *args);

/* compiler pseudo-functions */
static int builtin_label(int numargs, BuiltinArg *args);

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
	{ "label",       builtin_label,       1, { StringArg } },
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

static int num_builtins = sizeof(builtin_functions) / sizeof(FunctionType);

/* This is only used for printing out the .fvwmrc line if an error
   occured */

#define PRINT_LINE_LENGTH 80
static char current_line[PRINT_LINE_LENGTH];

static void save_current_line(char *s)
{
	char *p = current_line;

	while (*s && p < current_line + PRINT_LINE_LENGTH - 1) {
		if (*s == '\n')
		{
			*p = '\0';
			return;
		}
		else
		{
			*p++ = *s++;
		}
	}
	*p = '\0';
}

void print_args(int numargs, BuiltinArg *args)
{
#ifdef FVWM_DEBUG_MSGS
	int i;

	for (i = 0; i < numargs; i++) {
		switch (args[i].type) {
		case NoArg:
			ConsoleDebug(CONFIG, "NoArg ");
			break;

		case IntArg:
			ConsoleDebug(
				CONFIG, "Int: %d ", args[i].value.int_value);
			break;

		case StringArg:
			ConsoleDebug(
				CONFIG, "String: %s ",
				args[i].value.string_value);
			break;

		case ButtonArg:
			ConsoleDebug(
				CONFIG, "Button: %d %d ",
				args[i].value.button_value.offset,
				args[i].value.button_value.base);
			break;

		case WindowArg:
			ConsoleDebug(
				CONFIG, "Window: %d %d ",
				args[i].value.button_value.offset,
				args[i].value.button_value.base);
			break;

		case ManagerArg:
			ConsoleDebug(
				CONFIG, "Manager: %d %d ",
				args[i].value.button_value.offset,
				args[i].value.button_value.base);
			break;

		case JmpArg:
			ConsoleDebug(
				CONFIG, "Unprocessed Label Jump: %s ",
				args[i].value.string_value);
			break;

		default:
			ConsoleDebug(CONFIG, "bad ");
			break;
		}
	}
	ConsoleDebug(CONFIG, "\n");
#endif
}

#ifdef FVWM_DEBUG_MSGS
static void print_binding(Binding *binding)
{
	int i;
	Function *func;

	if (binding->type == BIND_BUTTONPRESS)
	{
		ConsoleDebug(CONFIG, "\tMouse: %d\n", binding->Button_Key);
	}
	else
	{
		ConsoleDebug(
			CONFIG, "\tKey or action: %d %s\n",
			binding->Button_Key, binding->key_name);
	}

	ConsoleDebug(CONFIG, "\tModifiers: %d\n", binding->Modifier);
	ConsoleDebug(CONFIG, "\tAction: %s\n", (char *) binding->Action);
	ConsoleDebug(CONFIG, "\tFunction struct: 0x%x\n",
		(unsigned int) binding->Action2);
	func = (Function *)(binding->Action2);
	while (func)
	{
		for (i = 0; i < num_builtins; i++)
		{
			if (func->func == builtin_functions[i].func)
			{
				ConsoleDebug(
					CONFIG, "\tFunction: %s 0x%x ",
					builtin_functions[i].name,
					(unsigned int) func->func);
				break;
			}
		}
		if (i > num_builtins)
		{
			ConsoleDebug(
				CONFIG, "\tFunction: not found 0x%x ",
				(unsigned int) func->func);
		}
		print_args(func->numargs, func->args);
		func = func->next;
	}
}

void print_bindings(Binding *list)
{
	ConsoleDebug(CONFIG, "binding list:\n");
	while (list != NULL)
	{
		print_binding(list);
		ConsoleDebug(CONFIG, "\n");
		list = list->NextBinding;
	}
}
#else
void print_bindings(Binding *list)
{
}
#endif

static int iswhite(char c)
{
	if (c == ' ' || c == '\t' || c == '\0')
	{
		return 1;
	}
	return 0;
}

static void trim(char *p)
{
	int length = strlen(p) -1;
	int index;
	for (index = length; index > 0; index --)
	{
		if (p[index] == ' ' || p[index] == '\t')
		{
			p[index] = '\0';
		}
		else
		{
			return;
		}
	}
}

static void skip_space(char **p)
{
	while (**p == ' ' || **p == '\t')
	{
		(*p)++;
	}
}

static void add_to_binding(Binding **list, Binding *binding)
{
	ConsoleDebug(CONFIG, "In add_to_binding:\n");

	binding->NextBinding = *list;
	*list = binding;
	return;
}

static int extract_int(char *p, int *n)
{
	char *s;
	int sign = 1;

	while (isspace((unsigned char)*p) && *p)
	{
		p++;
	}

	if (*p == '-')
	{
		sign = -1;
		p++;
	}
	else if (*p == '+') {
		sign = 1;
		p++;
	}

	if (*p == '\0')
	{
		return 0;
	}

	for (s = p; *s; s++)
	{
		if (*s < '0' || *s > '9')
		{
			return 0;
		}
	}

	*n = atoi(p) * sign;

	return 1;
}

static char *parse_button(char *string, BuiltinArg *arg, int *flag,
			   char *pstop_char)
{
  char *rest, *token;
  ButtonValue *bv;
  int n;

  ConsoleDebug(CONFIG, "parse_term: %s\n", string);

  *flag = 1;

  arg->type = ButtonArg;
  bv = &arg->value.button_value;
  bv->offset = 0;
  bv->base = AbsoluteButton;

  rest = DoGetNextToken(string, &token, NULL, ",", pstop_char);
  if (token == NULL)
  {
    bv->base = NoButton;
    *flag = 0;
    Free(token);
    return NULL;
  }
  if (!strcasecmp(token, "focus"))
  {
    bv->base = FocusButton;
  }
  else if (!strcasecmp(token, "select"))
  {
    bv->base = SelectButton;
  }
  else if (!strcasecmp(token, "up"))
  {
    bv->base = UpButton;
  }
  else if (!strcasecmp(token, "down"))
  {
    bv->base = DownButton;
  }
  else if (!strcasecmp(token, "left"))
  {
    bv->base = LeftButton;
  }
  else if (!strcasecmp(token, "right"))
  {
    bv->base = RightButton;
  }
  else if (!strcasecmp(token, "next"))
  {
    bv->base = NextButton;
  }
  else if (!strcasecmp(token, "prev"))
  {
    bv->base = PrevButton;
  }
  else if (extract_int(token, &n))
  {
    bv->base = AbsoluteButton;
    bv->offset = n;
  }
  else
  {
    ConsoleMessage("Bad button: %s\n", token);
    bv->base = NoButton;
    Free(token);
    *flag = 0;
    return NULL;
  }

  Free(token);
  return rest;
}

static void free_function_list(Function *func)
{
	int i;
	Function *fp = func;

	while (fp)
	{
		for (i = 0; i < fp->numargs; i++)
		{
			if (fp->args[i].type == StringArg)
			{
				Free(fp->args[i].value.string_value);
			}
		}
		func = fp;
		fp = fp->next;
		Free(func);
	}
}

static int funccasecmp(
	const void *key /* actually char* */,
	const void *member /* actually FunctionType* */)
{
	return strcasecmp((char *)key, ((FunctionType *)member)->name);
}

/*
 * The label function. Should never be called, but we need a pointer to it,
 * and it's useful for debugging purposes to have it defined.
 */
static int builtin_label(int numargs, BuiltinArg *args)
{
	int j;
	/* we should _never_ be called */
	ConsoleMessage( "label" );
	for (j=0; j<numargs; ++j)
	{
		switch (args[j].type)
		{
		case StringArg:
			ConsoleMessage(" %s", args[j].value.string_value);
			break;

		default:
			ConsoleMessage(" [unknown arg #: %d]", args[j].type);
		}
	}
	ConsoleMessage(" was called. This should not happen.\n");
	return 0;
}

/* the number of JmpArg arguments that have been created, but not yet
   resolved into IntArgs. */
static int JmpArgs = 0;
/* icky, I know, but it'll save unnecessary error-checking. */

static Function *parse_function(char **line, char *pstop_char)
{
  Function *ftype = xmalloc(sizeof(Function));
  char *ptr, *name, *tok;
  int j, flag;
  FunctionType *builtin_functions_i;

  ConsoleDebug(CONFIG, "in parse_function\n");

  ptr = DoGetNextToken(*line, &name, NULL, ",", pstop_char);
  if (name == NULL) {
    Free(ftype);
    *line = NULL;
    return NULL;
  }

  builtin_functions_i=bsearch((void *)name, (void *)builtin_functions,
			      num_builtins, sizeof(FunctionType),
			      funccasecmp);
  if (builtin_functions_i) {
    Free(name);
    ftype->func = builtin_functions_i->func;
    ftype->numargs = builtin_functions_i->numargs;
    ftype->next = NULL;

    for (j = 0; j < builtin_functions_i->numargs && *pstop_char != ','; j++) {
      ftype->args[j].type = builtin_functions_i->args[j];
      switch (builtin_functions_i->args[j]) {
      case IntArg:
	ptr = DoGetNextToken(ptr, &tok, NULL, ",", pstop_char);
	if (!tok) {
	  ConsoleMessage("%s: too few arguments\n",
			  builtin_functions_i->name);
	  Free(ftype);
	  *line = NULL;
	  return NULL;
	}
	if (extract_int(tok, &ftype->args[j].value.int_value) == 0) {
	  ConsoleMessage("%s: expect integer argument: %s\n",
			  builtin_functions_i->name, tok);
	  Free(tok);
	  Free(ftype);
	  *line = NULL;
	  return NULL;
	}
	Free(tok);
	break;

      case StringArg:
	ptr = DoGetNextToken(ptr, &ftype->args[j].value.string_value,NULL,
			      ",", pstop_char);
	if (!ftype->args[j].value.string_value) {
	  ConsoleMessage("%s: too few arguments\n",
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
	ptr = parse_button(ptr, &ftype->args[j], &flag, pstop_char);
	if (!flag) {
	  ConsoleMessage("%s: too few arguments\n",
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
	ptr = DoGetNextToken(ptr, &tok, NULL, ",", pstop_char);
	if (!tok) {
	  ConsoleMessage("%s: too few arguments\n",
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
	ConsoleMessage("internal error in parse_function\n");
	Free(ftype);
	*line = NULL;
	return NULL;
      }
    }

    if (j != builtin_functions_i->numargs) {
      ConsoleMessage("%s: too few arguments\n", builtin_functions_i->name);
      Free(ftype);
      *line = NULL;
      return NULL;
    }

    *line = ptr;
    return ftype;
  }

  ConsoleMessage("Unknown function: %s\n", name);
  Free(name);

  *line = NULL;
  return NULL;
}


/* This is O(N^2) where N = number of instructions. Seems we could do better.
   We'll see how this addition settles before monkeying with it */

static Function *parse_function_list(char *line)
{
  Function *ret = NULL, *tail = NULL, *f, *i;
  char *token;
  int jump_count, j;
  char stop_char;
  char c;

  JmpArgs=0;
  while (line && (f = parse_function(&line, &stop_char))) {
    ConsoleDebug(CONFIG, "parse_function: 0x%lx\n", (unsigned long)f->func);
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
    DoGetNextToken(line, &token, NULL, ",", &c);
    if (token && stop_char != ',') {
      ConsoleMessage("Bad function list, comma expected\n");
      Free(token);
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
	  ConsoleMessage("Attempt to jump to non-existant label %s; "
			  "aborting function list.\n",
			  f->args[j].value.string_value);
	  --JmpArgs;
	}
      }
    }
    if (JmpArgs!=0)
      ConsoleMessage( "Internal Error: JmpArgs %d not accounted for!\n",
		       JmpArgs );
    tail=NULL;
    f=NULL;
    free_function_list(ret);
    ret=NULL;
    return NULL;
  }
  if (ret == NULL)
    ConsoleMessage("No function defined\n");
  return ret;
}


Binding *ParseMouseEntry(char *tline)
{
  char modifiers[20],*action,*token;
  Binding *new;
  int button = -1;
  int n1=0,n2=0;
  int mods;

  /* tline points after the key word "key" */
  action = DoGetNextToken(tline,&token, NULL, ",", NULL);
  if(token != NULL) {
    n1 = sscanf(token,"%d",&button);
    if (n1 == 1 && (button < 0 || button > NUMBER_OF_EXTENDED_MOUSE_BUTTONS))
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
    ConsoleMessage("Mouse binding: Syntax error");

  modifiers_string_to_modmask(modifiers, &mods);
  if((mods & AnyModifier)&&(mods&(~AnyModifier))) {
    ConsoleMessage("Binding specified AnyModifier and other modifers too. "
		    "Excess modifiers will be ignored.");
  }

  new = xcalloc(1, sizeof(Binding));
  new->type = BIND_BUTTONPRESS;
  new->Button_Key = button;
  new->Modifier = mods;
  new->Action = stripcpy(action);
  new->Action2 = parse_function_list(action);

  if (!new->Action2) {
    ConsoleMessage("Bad action: %s\n", action);
    Free(new->Action);
    Free(new);
    return NULL;
  }

  ConsoleDebug(CONFIG, "Mouse: %d %d %s\n", new->Button_Key,
		new->Modifier, (char*)new->Action);

  return new;
}

static Binding *ParseKeyEntry(char *tline)
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
    ConsoleMessage("Syntax error in line %s",tline);

  modifiers_string_to_modmask(modifiers, &mods);
  if((mods & AnyModifier)&&(mods&(~AnyModifier))) {
    ConsoleMessage("Binding specified AnyModifier and other modifers too. Excess modifiers will be ignored.");
  }

  /*
   * Don't let a 0 keycode go through, since that means AnyKey to the
   * XGrabKey call in GrabKeys().
   */
  if ((keysym = XStringToKeysym(key)) == NoSymbol ||
      XKeysymToKeycode(theDisplay, keysym) == 0) {
    ConsoleMessage("Can't find keysym: %s\n", key);
    return NULL;
  }


  XDisplayKeycodes(theDisplay, &min, &max);
  for (i=min; i<=max; i++) {
    if (fvwm_KeycodeToKeysym(theDisplay, i, 0, 0) == keysym) {
      if (!func) {
	func = parse_function_list(action);
	if (!func) {
	  ConsoleMessage("Bad action: %s\n", action);
	  return NULL;
	}
	actionstring = stripcpy(action);
	keystring = stripcpy(key);
      } else {
	actionstring = keystring = NULL;
      }
      temp = new;
      new  = xcalloc(1, sizeof(Binding));
      new->type = BIND_KEYPRESS;
      new->Button_Key = i;
      new->key_name = keystring;
      new->Modifier = mods;
      new->Action = actionstring;
      new->Action2 = func;
      new->NextBinding = temp;
      if (!last) {
	last = new;
      }

      ConsoleDebug(CONFIG, "Key: %d %s %d %s\n", i, new->key_name,
		    mods, (char*)new->Action);
    }
  }
  return new;
}

static Binding *ParseSimpleEntry(char *tline)
{
	Binding *new;
	Function *func;

	func = parse_function_list(tline);
	if (func == NULL)
	{
		return NULL;
	}

	new = xcalloc(1, sizeof(Binding));
	new->type = BIND_KEYPRESS;
	new->key_name = "select";
	new->Action = stripcpy(tline);
	new->Action2 = func;

	return new;
}

void run_binding(WinManager *man, Action action)
{
	Binding *binding = man->bindings[action];
	ConsoleDebug(CONFIG, "run_binding:\n");
	print_bindings(binding);

	if (binding && binding->Action2 &&
		((Function *)(binding->Action2))->func)
	{
		run_function_list(binding->Action2);
	}
}

void execute_function(char *string)
{
	Function *func = parse_function_list(string);
	if (func == NULL)
	{
		return;
	}
	else
	{
		run_function_list(func);
		free_function_list(func);
	}
}

static int GetConfigLineWrapper(int *fd, char **tline)
{
	char *temp;

	GetConfigLine(fd, tline);
	if (*tline)
	{
		if (strncasecmp(*tline, "Colorset", 8) == 0)
		{
			LoadColorset(&(*tline)[8]);
		}
		else if (strncasecmp(
			*tline, XINERAMA_CONFIG_STRING,
			sizeof(XINERAMA_CONFIG_STRING) - 1) == 0)
		{
			FScreenConfigureModule(
			(*tline) + sizeof(XINERAMA_CONFIG_STRING) - 1);
		}
		else if (strncasecmp(*tline, "IgnoreModifiers", 15) == 0)
		{
			sscanf((*tline) + 16, "%d", &mods_unused);
		}
		temp = strchr(*tline, '\n');
		if (temp)
		{
			*temp = '\0';
		}
		/* grok the global config lines */

		return 1;
	}

	return 0;
}

static char *read_next_cmd(ReadOption flag)
{
	static ReadOption status;
	static char *buffer;
	static char *retstring, *cur_pos;
	char *end_pos;

	retstring = NULL;
	if (flag != READ_LINE && !(flag & status))
	{
		return NULL;
	}

	switch (flag)
	{
	case READ_LINE:
		while (GetConfigLineWrapper(fvwm_fd, &buffer))
		{
			cur_pos = buffer;
			skip_space(&cur_pos);
			if (!strncasecmp(Module, cur_pos, ModuleLen))
			{
				retstring = cur_pos;
				cur_pos += ModuleLen;

				if (*cur_pos == '*')
				{
					cur_pos++;
				}
				skip_space(&cur_pos);

				if (isalnum(*cur_pos))
				{
					status = READ_OPTION;
				}
				else
				{
					status = READ_LINE;
				}
				break;
			}
		}
		break;

	case READ_OPTION:
		retstring = cur_pos;
		while (*cur_pos != '*' && !iswhite(*cur_pos))
		{
			cur_pos++;
		}
		end_pos = cur_pos;

		if (*cur_pos == '*')
		{
			cur_pos++;
		}
		skip_space(&cur_pos);

		if (!*cur_pos)
		{
			status = READ_LINE;
		}
		else if (isdigit(*retstring) || !strncmp(
			retstring, "transient", 9))
		{
			status = READ_OPTION;
		}
		else if (*cur_pos)
		{
			status = READ_ARG;
		}

		*end_pos = '\0';
		break;

	case READ_ARG:
		retstring = cur_pos;
		while (!iswhite(*cur_pos))
		{
			cur_pos++;
		}
		end_pos = cur_pos;

		skip_space(&cur_pos);

		if (*cur_pos)
		{
			status = READ_ARG;
		}
		else
		{
			status = READ_LINE;
		}

		*end_pos = '\0';
		break;

	case READ_REST_OF_LINE:
		status = READ_LINE;
		retstring = cur_pos;
		break;
	}

	if (retstring && retstring[0] == '\0')
	{
		retstring = NULL;
	}

	return retstring;
}

static char *conditional_copy_string(char **s1, char *s2)
{
	if (*s1)
	{
		return *s1;
	}
	else
	{
		return copy_string(s1, s2);
	}
}

static NameType parse_format_dependencies(char *format)
{
	NameType flags = NO_NAME;

	ConsoleDebug(CONFIG, "Parsing format: %s\n", format);

	while (*format)
	{
		if (*format != '%')
		{
			format++;
		}
		else {
			format++;
			if (*format == 'i')
			{
				flags |= ICON_NAME;
			}
			else if (*format == 't')
			{
				flags |= TITLE_NAME;
			}
			else if (*format == 'c')
			{
				flags |= CLASS_NAME;
			}
			else if (*format == 'r')
			{
				flags |= RESOURCE_NAME;
			}
			else if (*format != '%')
			{
				ConsoleMessage(
					"Bad format string: %s\n", format);
			}
		}
	}
#ifdef FVWM_DEBUG_MSGS
	ConsoleDebug(CONFIG, "Format depends on: ");
	if (flags & ICON_NAME)
	{
		ConsoleDebug(CONFIG, "Icon ");
	}
	if (flags & TITLE_NAME)
	{
		ConsoleDebug(CONFIG, "Title ");
	}
	if (flags & CLASS_NAME)
	{
		ConsoleDebug(CONFIG, "Class ");
	}
	if (flags & RESOURCE_NAME)
	{
		ConsoleDebug(CONFIG, "Resource ");
	}
	ConsoleDebug(CONFIG, "\n");
#endif

	return flags;
}

#define SET_MANAGER(manager,field,value)                                     \
	do                                                                   \
	{                                                                    \
		int id = manager;                                            \
		if (id == -1)                                                \
		{                                                            \
			for (id = 0; id < globals.num_managers; id++)        \
			{                                                    \
				globals.managers[id]. field = value;         \
			}                                                    \
		}                                                            \
		else if (id < globals.num_managers)                          \
		{                                                            \
			globals.managers[id]. field = value;                 \
		}                                                            \
		else                                                         \
		{                                                            \
			ConsoleMessage(                                      \
				"Internal error in SET_MANAGER: %d\n", id);  \
		}                                                            \
	} while (0)

static void handle_button_config(int manager, int context, char *option)
{
	char *p;
	ButtonState state;

	p = read_next_cmd(READ_ARG);
	if (!p)
	{
		ConsoleMessage("Bad line: %s\n", current_line);
		ConsoleMessage("Need argument to %s\n", option);
		return;
	}
	else if (!strcasecmp(p, "flat"))
	{
		state = BUTTON_FLAT;
	}
	else if (!strcasecmp(p, "up"))
	{
		state = BUTTON_UP;
	}
	else if (!strcasecmp(p, "down"))
	{
		state = BUTTON_DOWN;
	}
	else if (!strcasecmp(p, "raisededge"))
	{
		state = BUTTON_EDGEUP;
	}
	else if (!strcasecmp(p, "sunkedge"))
	{
		state = BUTTON_EDGEDOWN;
	}
	else
	{
		ConsoleMessage("Bad line: %s\n", current_line);
		ConsoleMessage("This isn't a valid button state: %s\n", p);
		return;
	}
	ConsoleDebug(
		CONFIG, "Setting buttonState[%s] to %s\n",
		contextDefaults[context].name, p);
	SET_MANAGER(manager, buttonState[context], state);

	/* check for optional fore color */
	p = read_next_cmd(READ_ARG);
	if (!p)
	{
		return;
	}

	SET_MANAGER(
		manager, foreColorName[context],
		copy_string(&globals.managers[id]. foreColorName[context], p));

	/* check for optional back color */
	p = read_next_cmd(READ_ARG);
	if (!p)
	{
		return;
	}

	ConsoleDebug(
		CONFIG, "Setting backColorName[%s] to %s\n",
		contextDefaults[context].name, p);
	SET_MANAGER(
		manager, backColorName[context],
		copy_string(&globals.managers[id].backColorName[context], p));
}

static void add_weighted_sort(WinManager *man, WeightedSort *weighted_sort)
{
	WeightedSort *p;

	if (man->weighted_sorts_len == man->weighted_sorts_size)
	{
		man->weighted_sorts_size += 16;
		man->weighted_sorts = xrealloc(
			(char *)man->weighted_sorts,
			man->weighted_sorts_size, sizeof(WeightedSort));
	}
	p = &man->weighted_sorts[man->weighted_sorts_len];
	p->resname = NULL;
	p->classname = NULL;
	p->titlename = NULL;
	p->iconname = NULL;
	if (weighted_sort->resname)
	{
		copy_string(&p->resname, weighted_sort->resname);
	}
	if (weighted_sort->classname)
	{
		copy_string(&p->classname, weighted_sort->classname);
	}
	if (weighted_sort->titlename)
	{
		copy_string(&p->titlename, weighted_sort->titlename);
	}
	if (weighted_sort->iconname)
	{
		copy_string(&p->iconname, weighted_sort->iconname);
	}
	p->weight = weighted_sort->weight;
	++man->weighted_sorts_len;
}

void read_in_resources(void)
{
  char *p, *q;
  int i, n, manager;
  char *option1;
  Binding *binding = NULL;
  Resolution r;
  Reverse rv;

  InitGetConfigLine(fvwm_fd, Module);

  while ((p = read_next_cmd(READ_LINE))) {
    ConsoleDebug(CONFIG, "line: %s\n", p);
    save_current_line(p);

    option1 = read_next_cmd(READ_OPTION);
    if (option1 == NULL)
      continue;

    ConsoleDebug(CONFIG, "option1: %s\n", option1);
    if (!strcasecmp(option1, "nummanagers")) {
      /* If in transient mode, just use the default of 1 manager */
      if (!globals.transient) {
	p = read_next_cmd(READ_ARG);
	if (!p) {
	  ConsoleMessage("Bad line: %s\n", current_line);
	  continue;
	}
	if (extract_int(p, &n) == 0) {
	  ConsoleMessage("This is not a number: %s\n", p);
	  ConsoleMessage("Bad line: %s\n", current_line);
	  continue;
	}
	if (n > 0) {
	  allocate_managers(n);
	  ConsoleDebug(CONFIG, "num managers: %d\n", n);
	}
	else {
	  ConsoleMessage("Bad line: %s\n", current_line);
	  ConsoleMessage("You can't have zero managers. "
			  "I'll give you one.\n");
	  allocate_managers(1);
	}
      }
    }
    else {
      /* these all can specify a specific manager */

      if (globals.managers == NULL) {
	ConsoleDebug(CONFIG, "I'm assuming you only want one manager\n");
	allocate_managers(1);
      }

      manager = 0;

      if (option1[0] >= '0' && option1[0] <= '9') {
	if (globals.transient) {
	  ConsoleDebug(CONFIG, "In transient mode. Ignoring this line\n");
	  continue;
	}
	if (extract_int(option1, &manager) == 0 ||
	    manager <= 0 || manager > globals.num_managers) {
	  ConsoleMessage("Bad line: %s\n", current_line);
	  ConsoleMessage("This is not a valid manager: %s.\n", option1);
	  manager = 0;
	}
	option1 = read_next_cmd(READ_OPTION);
	if (!option1) {
	  ConsoleMessage("Bad line: %s\n", current_line);
	  continue;
	}
      }
      else if (!strcasecmp(option1, "transient")) {
	if (globals.transient) {
	  ConsoleDebug(CONFIG, "Transient manager config line\n");
	  manager = 1;
	  option1 = read_next_cmd(READ_OPTION);
	  if (!option1) {
	    ConsoleMessage("Bad line: %s\n", current_line);
	    continue;
	  }
	}
	else {
	  ConsoleDebug(CONFIG, "Not in transient mode. Ignoring this line\n");
	  continue;
	}
      }

      manager--; /* -1 means global */

      ConsoleDebug(CONFIG, "Applying %s to manager %d\n", option1, manager);

      if (!strcasecmp(option1, "action")) {
	p = read_next_cmd(READ_ARG);
	if (!p) {
	  ConsoleMessage("Bad line: %s\n", current_line);
	  continue;
	}

	if (!strcasecmp(p, "mouse")) {
	  i = MOUSE;
	}
	else if (!strcasecmp(p, "key")) {
	  i = KEYPRESS;
	}
	else if (!strcasecmp(p, "select")) {
	  i = SELECT;
	}
	else {
	  ConsoleMessage("Bad line: %s\n", current_line);
	  ConsoleMessage("This isn't a valid action name: %s\n", p);
	  continue;
	}

	q = read_next_cmd(READ_REST_OF_LINE);
	if (!q) {
	  ConsoleMessage("Bad line: %s\n", current_line);
	  ConsoleMessage("Need an action\n");
	  continue;
	}

	switch (i) {
	case MOUSE:
	  binding = ParseMouseEntry(q);
	  break;

	case KEYPRESS:
	  binding = ParseKeyEntry(q);
	  break;

	case SELECT:
	  binding = ParseSimpleEntry(q);
	  break;
	}

	if (binding == NULL) {
	  ConsoleMessage("Offending line: %s\n", current_line);
	  ConsoleMessage("Bad action\n");
	  continue;
	}

	if (manager == -1) {
	  int j;
	  for (j = 0; j < globals.num_managers; j++) {
	    add_to_binding(&globals.managers[j].bindings[i], binding);
	  }
	}
	else if (manager < globals.num_managers) {
	  add_to_binding(&globals.managers[manager].bindings[i], binding);
	}
	else {
	  ConsoleMessage("Bad line: %s\n", current_line);
	  ConsoleMessage("There's no manager %d\n", manager);
	}
      }
      else if (!strcasecmp(option1, "colorset")) {
	p = read_next_cmd(READ_ARG);
	if (!p) {
	  ConsoleMessage("Bad line: %s\n", current_line);
	  continue;
	}
	if (extract_int(p, &n) == 0) {
	  ConsoleMessage("This is not a number: %s\n", p);
	  ConsoleMessage("Bad line: %s\n", current_line);
	  continue;
	}
	for ( i = 0; i < NUM_CONTEXTS; i++ ) {
		if (i != DEFAULT && i != TITLE_CONTEXT)
			SET_MANAGER(manager, colorsets[i], -2);
		else
			SET_MANAGER(manager, colorsets[i], n);
	}
	AllocColorset(n);
      }
      else if (!strcasecmp(option1, "background")) {
	p = read_next_cmd(READ_ARG);
	if (!p) {
	  ConsoleMessage("Bad line: %s\n", current_line);
	  continue;
	}
	ConsoleDebug(CONFIG, "default background: %s\n", p);

	for ( i = 0; i < NUM_CONTEXTS; i++ )
	  SET_MANAGER(manager, backColorName[i],
	    conditional_copy_string(&globals.managers[id].backColorName[i],
				     p));
      }
      else if (!strcasecmp(option1, "buttongeometry")) {
	p = read_next_cmd(READ_ARG);
	if (!p) {
	  ConsoleMessage("Bad line: %s\n", current_line);
	  continue;
	}

	SET_MANAGER(manager, button_geometry_str,
		     copy_string(&globals.managers[id].button_geometry_str, p));
      }
      else if (!strcasecmp(option1, "maxbuttonwidth")) {
	      p = read_next_cmd(READ_ARG);
	      if (!p) {
		      n = 0;
	      }
	      else if (extract_int(p, &n) == 0) {
		      ConsoleMessage("This is not a number: %s\n", p);
		      ConsoleMessage("Bad line: %s\n", current_line);
		      continue;
	      }
	      SET_MANAGER(manager, max_button_width, n);
	      SET_MANAGER(manager, max_button_width_columns, 0);
      }
      else if (!strcasecmp(option1, "maxbuttonwidthbycolumns")) {
	      p = read_next_cmd(READ_ARG);
	      if (!p) {
		      n = 0;
	      }
	      else if (extract_int(p, &n) == 0) {
		      ConsoleMessage("This is not a number: %s\n", p);
		      ConsoleMessage("Bad line: %s\n", current_line);
		      continue;
	      }
	      SET_MANAGER(manager, max_button_width, 0);
	      SET_MANAGER(manager, max_button_width_columns, n);
      }
      else if (!strcasecmp(option1, "dontshow")) {
	char *token = NULL;
	p = read_next_cmd(READ_REST_OF_LINE);
	if (!p) {
	  ConsoleMessage("Bad line: %s\n", current_line);
	  continue;
	}
	p = DoGetNextToken(p, &token, NULL, ",", NULL);
	if (!token) {
	  ConsoleMessage("Bad line: %s\n", current_line);
	  continue;
	}
	do {
	  ConsoleDebug(CONFIG, "don't show: %s\n", token);
	  if (manager == -1) {
	    int i;
	    for (i = 0; i < globals.num_managers; i++)
	      add_to_stringlist(&globals.managers[i].dontshow, token);
	  }
	  else {
	    add_to_stringlist(&globals.managers[manager].dontshow, token);
	  }
	  Free(token);
	  p = DoGetNextToken(p, &token, NULL, ",", NULL);
	} while (token);
	if (token)
	  Free(token);
      }
      else if (!strcasecmp(option1, "drawicons")) {
	if (!FMiniIconsSupported)
	{
	  ConsoleMessage("DrawIcons support not compiled in\n");
	}
	else
	{
	  p = read_next_cmd(READ_ARG);
	  if (!p) {
	    ConsoleMessage("Bad line: %s\n", current_line);
	    ConsoleMessage("Need argument to drawicons\n");
	    continue;
	  }
	  if (!strcasecmp(p, "true")) {
	    i = 1;
	  }
	  /* [NFM 3 Dec 97] added support for drawicons "always" */
	  else if (!strcasecmp(p, "always")) {
	    i = 2;
	  }
	  else if (!strcasecmp(p, "false")) {
	    i = 0;
	  }
	  else {
	    ConsoleMessage("Bad line: %s\n", current_line);
	    ConsoleMessage("What is this: %s?\n", p);
	    continue;
	  }
	  ConsoleDebug(CONFIG, "Setting drawicons to: %d\n", i);
	  SET_MANAGER(manager, draw_icons, i);
	}
      }
      else if (!strcasecmp(option1, "followfocus")) {
	p = read_next_cmd(READ_ARG);
	if (!p) {
	  ConsoleMessage("Bad line: %s\n", current_line);
	  ConsoleMessage("Need argument to followfocus\n");
	  continue;
	}
	if (!strcasecmp(p, "true")) {
	  i = 1;
	}
	else if (!strcasecmp(p, "false")) {
	  i = 0;
	}
	else {
	  ConsoleMessage("Bad line: %s\n", current_line);
	  ConsoleMessage("What is this: %s?\n", p);
	  continue;
	}
	ConsoleDebug(CONFIG, "Setting followfocus to: %d\n", i);
	SET_MANAGER(manager, followFocus, i);
      }
      else if (!strcasecmp(option1, "showtransient")) {
       p = read_next_cmd(READ_ARG);
       if (!p) {
	 ConsoleMessage("Bad line: %s\n", current_line);
	 ConsoleMessage("Need argument to showtransient\n");
	 continue;
       }
       if (!strcasecmp(p, "true")) {
	 i = 1;
       }
       else if (!strcasecmp(p, "false")) {
	 i = 0;
       }
       else {
	 ConsoleMessage("Bad line: %s\n", current_line);
	 ConsoleMessage("What is this: %s?\n", p);
	 continue;
       }
       ConsoleDebug(CONFIG, "Setting showtransient to: %d\n", i);
       SET_MANAGER(manager, showtransient, i);
      }
      else if( !strcasecmp(option1, "showonlyfocused")) {
	p = read_next_cmd(READ_ARG);
	if (!p) {
	 ConsoleMessage("Bad line: %s\n", current_line);
	 ConsoleMessage("Need argument to showonlyfocused\n");
	 continue;
	}
	if (!strcasecmp(p, "true")) {
	  i = 1;
	}
	else if (!strcasecmp(p, "false")) {
	  i = 0;
	}
	else {
	  ConsoleMessage("Bad line: %s\n", current_line);
	  ConsoleMessage("What is this: %s?\n", p);
	  continue;
	}
	ConsoleMessage("Show only focused to: %d\n", i);
	SET_MANAGER( manager, showonlyfocused, i);
      }
      else if (!strcasecmp(option1, "showonlyicons")) {
       p = read_next_cmd(READ_ARG);
       if (!p) {
	 ConsoleMessage("Bad line: %s\n", current_line);
	 ConsoleMessage("Need argument to showonlyicons\n");
	 continue;
       }
       if (!strcasecmp(p, "true")) {
	 i = 1;
       }
       else if (!strcasecmp(p, "false")) {
	 i = 0;
       }
       else {
	 ConsoleMessage("Bad line: %s\n", current_line);
	 ConsoleMessage("What is this: %s?\n", p);
	 continue;
       }
       ConsoleDebug(CONFIG, "Setting showonlyicons to: %d\n", i);
       SET_MANAGER(manager, showonlyiconic, i);
      }
      else if (!strcasecmp(option1, "shownoicons")) {
       p = read_next_cmd(READ_ARG);
       if (!p) {
	 ConsoleMessage("Bad line: %s\n", current_line);
	 ConsoleMessage("Need argument to shownoicons\n");
	 continue;
       }
       if (!strcasecmp(p, "true")) {
	 i = 1;
       }
       else if (!strcasecmp(p, "false")) {
	 i = 0;
       }
       else {
	 ConsoleMessage("Bad line: %s\n", current_line);
	 ConsoleMessage("What is this: %s?\n", p);
	 continue;
       }
       ConsoleDebug(CONFIG, "Setting shownoicons to: %d\n", i);
       SET_MANAGER(manager, shownoiconic, i);
      }
      else if (!strcasecmp(option1, "font")) {
	char *f;
	p = read_next_cmd(READ_REST_OF_LINE);
	trim(p);
	if (!p) {
	  ConsoleMessage("Bad line: %s\n", current_line);
	  continue;
	}
	CopyStringWithQuotes(&f, p);
	ConsoleDebug(CONFIG, "font: %s\n", f);

	SET_MANAGER(manager, fontname,
		     copy_string(&globals.managers[id].fontname, f));
	free(f);
      }
      else if (!strcasecmp(option1, "foreground")) {
	p = read_next_cmd(READ_ARG);
	if (!p) {
	  ConsoleMessage("Bad line: %s\n", current_line);
	  continue;
	}
	ConsoleDebug(CONFIG, "default foreground: %s\n", p);

	for ( i = 0; i < NUM_CONTEXTS; i++ )
	SET_MANAGER(manager, foreColorName[i],
	   conditional_copy_string(&globals.managers[id].foreColorName[i],
				    p));
      }
      else if (!strcasecmp(option1, "format")) {
	char *token;
	NameType flags;

	p = read_next_cmd(READ_REST_OF_LINE);
	if (!p) {
	  ConsoleMessage("Bad line: %s\n", current_line);
	  continue;
	}
	DoGetNextToken(p, &token, NULL, ",", NULL);
	if (!token)
	  {
	    token = xmalloc(1);
	    *token = 0;
	  }

	SET_MANAGER(manager, formatstring,
		     copy_string (&globals.managers[id].formatstring, token));
	flags = parse_format_dependencies(token);
	SET_MANAGER(manager, format_depend, flags);
	Free(token);
      }
      else if (!strcasecmp(option1, "geometry")) {
	ConsoleMessage("Geometry option no longer supported.\n");
	ConsoleMessage("Use ManagerGeometry and ButtonGeometry.\n");
      }
      else if (!strcasecmp(option1, "iconname")) {
	char *token;
	p = read_next_cmd(READ_REST_OF_LINE);
	if (!p) {
	  ConsoleMessage("Bad line: %s\n", current_line);
	  continue;
	}
	DoGetNextToken(p, &token, NULL, ",", NULL);
	if (!token)
	  {
	    token = xmalloc(1);
	    *token = 0;
	  }

	SET_MANAGER(manager, iconname,
		     copy_string(&globals.managers[id].iconname, token));
	Free(token);
      }
      else if (!strcasecmp(option1, "managergeometry")) {
	p = read_next_cmd(READ_ARG);
	if (!p) {
	  ConsoleMessage("Bad line: %s\n", current_line);
	  continue;
	}

	SET_MANAGER(manager, geometry_str,
		     copy_string(&globals.managers[id].geometry_str, p));
      }
      else if (!strcasecmp(option1, "resolution")) {
	p = read_next_cmd(READ_ARG);
	if (!p) {
	  ConsoleMessage("Bad line: %s\n", current_line);
	  continue;
	}
	ConsoleDebug(CONFIG, "resolution: %s\n", p);
	if (!strcasecmp(p, "global"))
	  r = SHOW_GLOBAL;
	else if (!strcasecmp(p, "desk"))
	  r = SHOW_DESKTOP;
	else if (!strcasecmp(p, "page"))
	  r = SHOW_PAGE;
	else if (!strcasecmp(p, "screen"))
	  r = SHOW_SCREEN;
	else if (!strcasecmp(p, "!desk"))
	  r = NO_SHOW_DESKTOP;
	else if (!strcasecmp(p, "!page"))
	  r = NO_SHOW_PAGE;
	else if (!strcasecmp(p, "!screen"))
	  r = NO_SHOW_SCREEN;
	else {
	  ConsoleMessage("Bad line: %s\n", current_line);
	  ConsoleMessage("What kind of resolution is this?\n");
	  continue;
	}

	SET_MANAGER(manager, res, r);
      }
      else if (!strcasecmp(option1, "reverse")) {
	p = read_next_cmd(READ_ARG);
	if (!p) {
	  ConsoleMessage("Bad line: %s\n", current_line);
	  continue;
	}
	ConsoleDebug(CONFIG, "reverse: %s\n", p);
	if (!strcasecmp(p, "none"))
	  rv = REVERSE_NONE;
	else if (!strcasecmp(p, "icon"))
	  rv = REVERSE_ICON;
	else if (!strcasecmp(p, "normal"))
	  rv = REVERSE_NORMAL;
	else {
	  ConsoleMessage("Bad line: %s\n", current_line);
	  ConsoleMessage("What kind of reverse is this?\n");
	  continue;
	}

	SET_MANAGER(manager, rev, rv);
      }
      else if (!strcasecmp(option1, "shape")) {
	p = read_next_cmd(READ_ARG);
	if (!p) {
	  ConsoleMessage("Bad line: %s\n", current_line);
	  ConsoleMessage("Need argument to shape\n");
	  continue;
	}
	if (!strcasecmp(p, "true")) {
	  i = 1;
	}
	else if (!strcasecmp(p, "false")) {
	  i = 0;
	}
	else {
	  ConsoleMessage("Bad line: %s\n", current_line);
	  ConsoleMessage("What is this: %s?\n", p);
	  continue;
	}
	if (!FHaveShapeExtension && i) {
	  ConsoleMessage("Shape support not compiled in\n");
	  continue;
	}
	ConsoleDebug(CONFIG, "Setting shape to: %d\n", i);
	SET_MANAGER(manager, shaped, i);
      }
      else if (!strcasecmp(option1, "show")) {
	char *token = NULL;
	p = read_next_cmd(READ_REST_OF_LINE);
	if (!p) {
	  ConsoleMessage("Bad line: %s\n", current_line);
	  continue;
	}
	p = DoGetNextToken(p, &token, NULL, ",", NULL);
	if (!token) {
	  ConsoleMessage("Bad line: %s\n", current_line);
	  continue;
	}
	do {
	  ConsoleDebug(CONFIG, "show: %s\n", token);
	  if (manager == -1) {
	    int i;
	    for (i = 0; i < globals.num_managers; i++)
	      add_to_stringlist(&globals.managers[i].show, token);
	  }
	  else {
	    add_to_stringlist(&globals.managers[manager].show, token);
	  }
	  Free(token);
	  p = DoGetNextToken(p, &token, NULL, ",", NULL);
	} while (token);
	if (token)
	  Free(token);
      }
      else if (!strcasecmp(option1, "showtitle")) {
	ConsoleMessage("Bad line: %s\n", current_line);
	ConsoleMessage("showtitle is no longer an option. Use format\n");
	continue;
      }
      else if (!strcasecmp(option1, "sort")) {
	p = read_next_cmd(READ_ARG);
	if (!p) {
	  ConsoleMessage("Bad line: %s\n", current_line);
	  ConsoleMessage("Need argument to sort\n");
	  continue;
	}
	if (!strcasecmp(p, "name")) {
	  i = SortName;
	}
	else if (!strcasecmp(p, "namewithcase")) {
	  i = SortNameCase;
	}
	else if (!strcasecmp(p, "id")) {
	  i = SortId;
	}
	else if (!strcasecmp(p, "none")) {
	  i = SortNone;
	}
	else if (!strcasecmp(p, "weighted")) {
	  i = SortWeighted;
	}
	else if (!strcasecmp(p, "false") || !strcasecmp(p, "true")) {
	  /* Old options */
	  ConsoleMessage("FvwmIconMan: sort option no longer takes "
			  "true or false value\n"
			  "Please read the latest manpage\n");
	  continue;
	}
	else {
	  ConsoleMessage("Bad line: %s\n", current_line);
	  ConsoleMessage("What is this: %s?\n", p);
	  continue;
	}
	ConsoleDebug(CONFIG, "Setting sort to: %d\n", i);
	SET_MANAGER(manager, sort, i);
      }
      else if (!strcasecmp(option1, "sortweight")) {
	WeightedSort weighted_sort;
	p = read_next_cmd(READ_ARG);
	if (!p) {
	  ConsoleMessage("Bad line: %s\n", current_line);
	  continue;
	}
	if (extract_int(p, &n) == 0) {
	  ConsoleMessage("This is not a number: %s\n", p);
	  ConsoleMessage("Bad line: %s\n", current_line);
	  continue;
	}
	weighted_sort.resname = NULL;
	weighted_sort.classname = NULL;
	weighted_sort.titlename = NULL;
	weighted_sort.iconname = NULL;
	weighted_sort.weight = n;
	p = read_next_cmd(READ_ARG);
	while (p) {
	  if (!strncasecmp(p, "resource=", 9)) {
	    copy_string(&weighted_sort.resname, p + 9);
	  } else if (!strncasecmp(p, "class=", 6)) {
	    copy_string(&weighted_sort.classname, p + 6);
	  } else if (!strncasecmp(p, "title=", 6)) {
	    copy_string(&weighted_sort.titlename, p + 6);
	  } else if (!strncasecmp(p, "icon=", 5)) {
	    copy_string(&weighted_sort.iconname, p + 5);
	  } else {
	    ConsoleMessage("Unknown sortweight field: %s\n", p);
	    ConsoleMessage("Bad line: %s\n", current_line);
	  }
	  p = read_next_cmd(READ_ARG);
	}
	if (manager == -1) {
	  for (i = 0; i < globals.num_managers; i++) {
	    add_weighted_sort(&globals.managers[i], &weighted_sort);
	  }
	}
	else {
	  add_weighted_sort(&globals.managers[manager], &weighted_sort);
	}
	if (weighted_sort.resname) {
	  Free(weighted_sort.resname);
	}
	if (weighted_sort.resname) {
	  Free(weighted_sort.classname);
	}
	if (weighted_sort.resname) {
	  Free(weighted_sort.titlename);
	}
	if (weighted_sort.resname) {
	  Free(weighted_sort.iconname);
	}
      }
      else if (!strcasecmp(option1, "NoIconAction")) {
	char *token;
	p = read_next_cmd(READ_REST_OF_LINE);
	if (!p) {
	  ConsoleMessage("Bad line: %s\n", current_line);
	  continue;
	}
	DoGetNextToken(p, &token, NULL, ",", NULL);
	if (!token)
	  {
	    token = xmalloc(1);
	    *token = 0;
	  }

	SET_MANAGER(manager, AnimCommand,
		     copy_string(&globals.managers[id].AnimCommand, token));
	Free(token);
      }
      else if (!strcasecmp(option1, "title")) {
	char *token;
	p = read_next_cmd(READ_REST_OF_LINE);
	if (!p) {
	  ConsoleMessage("Bad line: %s\n", current_line);
	  continue;
	}
	DoGetNextToken(p, &token, NULL, ",", NULL);
	if (!token)
	  {
	    token = xmalloc(1);
	    *token = 0;
	  }

	SET_MANAGER(manager, titlename,
		     copy_string(&globals.managers[id].titlename, token));
	Free(token);
      }
      else if (!strcasecmp(option1, "iconButton")) {
	handle_button_config(manager, ICON_CONTEXT, option1);
      }
      else if (!strcasecmp(option1, "iconandselectButton")) {
	handle_button_config(manager, ICON_SELECT_CONTEXT, option1);
      }
      else if (!strcasecmp(option1, "plainButton")) {
	handle_button_config(manager, PLAIN_CONTEXT, option1);
      }
      else if (!strcasecmp(option1, "selectButton")) {
	handle_button_config(manager, SELECT_CONTEXT, option1);
      }
      else if (!strcasecmp(option1, "focusButton")) {
	handle_button_config(manager, FOCUS_CONTEXT, option1);
      }
      else if (!strcasecmp(option1, "focusandselectButton")) {
	handle_button_config(manager, FOCUS_SELECT_CONTEXT, option1);
      }
      else if (!strcasecmp(option1, "titlebutton")) {
	handle_button_config(manager, TITLE_CONTEXT, option1);
      }
      else if (!strcasecmp(option1, "titlecolorset")) {
	p = read_next_cmd(READ_ARG);
	if (!p) {
	  ConsoleMessage("Bad line: %s\n", current_line);
	  continue;
	}
	if (extract_int(p, &n) == 0) {
	  ConsoleMessage("This is not a number: %s\n", p);
	  ConsoleMessage("Bad line: %s\n", current_line);
	  continue;
	}
	SET_MANAGER(manager, colorsets[TITLE_CONTEXT], n);
	AllocColorset(n);
      }
      else if (!strcasecmp(option1, "focusandselectcolorset")) {
	p = read_next_cmd(READ_ARG);
	if (!p) {
	  ConsoleMessage("Bad line: %s\n", current_line);
	  continue;
	}
	if (extract_int(p, &n) == 0) {
	  ConsoleMessage("This is not a number: %s\n", p);
	  ConsoleMessage("Bad line: %s\n", current_line);
	  continue;
	}
	SET_MANAGER(manager, colorsets[FOCUS_SELECT_CONTEXT], n);
	AllocColorset(n);
      }
      else if (!strcasecmp(option1, "focuscolorset")) {
	p = read_next_cmd(READ_ARG);
	if (!p) {
	  ConsoleMessage("Bad line: %s\n", current_line);
	  continue;
	}
	if (extract_int(p, &n) == 0) {
	  ConsoleMessage("This is not a number: %s\n", p);
	  ConsoleMessage("Bad line: %s\n", current_line);
	  continue;
	}
	SET_MANAGER(manager, colorsets[FOCUS_CONTEXT], n);
	AllocColorset(n);
      }
      else if (!strcasecmp(option1, "selectcolorset")) {
	p = read_next_cmd(READ_ARG);
	if (!p) {
	  ConsoleMessage("Bad line: %s\n", current_line);
	  continue;
	}
	if (extract_int(p, &n) == 0) {
	  ConsoleMessage("This is not a number: %s\n", p);
	  ConsoleMessage("Bad line: %s\n", current_line);
	  continue;
	}
	SET_MANAGER(manager, colorsets[SELECT_CONTEXT], n);
	AllocColorset(n);
      }
      else if (!strcasecmp(option1, "plaincolorset")) {
	p = read_next_cmd(READ_ARG);
	if (!p) {
	  ConsoleMessage("Bad line: %s\n", current_line);
	  continue;
	}
	if (extract_int(p, &n) == 0) {
	  ConsoleMessage("This is not a number: %s\n", p);
	  ConsoleMessage("Bad line: %s\n", current_line);
	  continue;
	}
	SET_MANAGER(manager, colorsets[PLAIN_CONTEXT], n);
	AllocColorset(n);
      }
      else if (!strcasecmp(option1, "iconcolorset")) {
	p = read_next_cmd(READ_ARG);
	if (!p) {
	  ConsoleMessage("Bad line: %s\n", current_line);
	  continue;
	}
	if (extract_int(p, &n) == 0) {
	  ConsoleMessage("This is not a number: %s\n", p);
	  ConsoleMessage("Bad line: %s\n", current_line);
	  continue;
	}
	SET_MANAGER(manager, colorsets[ICON_CONTEXT], n);
	AllocColorset(n);
      }
      else if (!strcasecmp(option1, "iconandselectcolorset")) {
	p = read_next_cmd(READ_ARG);
	if (!p) {
	  ConsoleMessage("Bad line: %s\n", current_line);
	  continue;
	}
	if (extract_int(p, &n) == 0) {
	  ConsoleMessage("This is not a number: %s\n", p);
	  ConsoleMessage("Bad line: %s\n", current_line);
	  continue;
	}
	SET_MANAGER(manager, colorsets[ICON_SELECT_CONTEXT], n);
	AllocColorset(n);
      }
      else if (!strcasecmp(option1, "usewinlist")) {
	p = read_next_cmd(READ_ARG);
	if (!p) {
	  ConsoleMessage("Bad line: %s\n", current_line);
	  ConsoleMessage("Need argument to usewinlist\n");
	  continue;
	}
	if (!strcasecmp(p, "true")) {
	  i = 1;
	}
	else if (!strcasecmp(p, "false")) {
	  i = 0;
	}
	else {
	  ConsoleMessage("Bad line: %s\n", current_line);
	  ConsoleMessage("What is this: %s?\n", p);
	  continue;
	}
	ConsoleDebug(CONFIG, "Setting usewinlist to: %d\n", i);
	SET_MANAGER(manager, usewinlist, i);
      }
      else if (!strcasecmp(option1, "reliefthickness")) {
	p = read_next_cmd(READ_ARG);
	if (!p) {
	  ConsoleMessage("Bad line: %s\n", current_line);
	  continue;
	}
	if (extract_int(p, &n) == 0) {
	  ConsoleMessage("This is not a number: %s\n", p);
	  ConsoleMessage("Bad line: %s\n", current_line);
	  continue;
	}
	SET_MANAGER(manager, relief_thickness, n);
      }
      else if (!strcasecmp(option1, "tips")) {
	      p = read_next_cmd(READ_ARG);
	      if (!p) {
		      ConsoleMessage("Bad line: %s\n", current_line);
		      continue;
	      }
	      if (!strcasecmp(p, "always")) {
		      i = TIPS_ALWAYS;
	      }

	      else if (!strcasecmp(p, "false")) {
		      i = TIPS_NEVER;
	      }
	      else if (!strcasecmp(p, "needed")) {
		      i = TIPS_NEEDED;
	      }
	      else {
		      ConsoleMessage("Bad line: %s\n", current_line);
		      ConsoleMessage("What is this: %s?\n", p);
		      continue;
	      }
	      SET_MANAGER(manager, tips, i);
      }
      else if (!strcasecmp(option1, "tipsfont")) {
	char *f;
	p = read_next_cmd(READ_REST_OF_LINE);
	trim(p);
	if (!p) {
	  ConsoleMessage("Bad line: %s\n", current_line);
	  continue;
	}
	CopyStringWithQuotes(&f, p);
	ConsoleDebug(CONFIG, "tipsfont: %s\n", f);

	SET_MANAGER(manager, tips_fontname,
		     copy_string(&globals.managers[id].tips_fontname, f));
	free(f);
      }
      else if (!strcasecmp(option1, "tipscolorset")) {
	p = read_next_cmd(READ_ARG);
	if (!p) {
	  ConsoleMessage("Bad line: %s\n", current_line);
	  continue;
	}
	if (extract_int(p, &n) == 0) {
	  ConsoleMessage("This is not a number: %s\n", p);
	  ConsoleMessage("Bad line: %s\n", current_line);
	  continue;
	}
	SET_MANAGER(manager, tips_conf->colorset, n);
	if (n >= 0)
	{
		AllocColorset(n);
	}
      }
      else if (!strcasecmp(option1, "tipsdelays")) {
	p = read_next_cmd(READ_ARG);
	if (!p) {
	  ConsoleMessage("Bad line: %s\n", current_line);
	  continue;
	}
	if (extract_int(p, &n) == 0) {
	  ConsoleMessage("This is not a number: %s\n", p);
	  ConsoleMessage("Bad line: %s\n", current_line);
	  continue;
	}
	if (n < 0) {
		ConsoleMessage("No negative delay: %s\n", p);
		ConsoleMessage("Bad line: %s\n", current_line);
		continue;
	}
	SET_MANAGER(manager, tips_conf->delay, n);
	p = read_next_cmd(READ_ARG);
	if (!p)
	{
		SET_MANAGER(manager, tips_conf->mapped_delay, n);
		continue;
	}
	if (extract_int(p, &n) == 0) {
	  ConsoleMessage("This is not a number: %s\n", p);
	  ConsoleMessage("Bad line: %s\n", current_line);
	  continue;
	}
	if (n < 0) {
		ConsoleMessage("No negative delay: %s\n", p);
		ConsoleMessage("Bad line: %s\n", current_line);
		continue;
	}
	SET_MANAGER(manager, tips_conf->mapped_delay, n);
      }
      else if (!strcasecmp(option1, "tipsformat")) {
	char *token;
	/*NameType flags;*/

	p = read_next_cmd(READ_REST_OF_LINE);
	if (!p) {
	  ConsoleMessage("Bad line: %s\n", current_line);
	  continue;
	}
	DoGetNextToken(p, &token, NULL, ",", NULL);
	if (!token)
	  {
	    token = xmalloc(1);
	    *token = 0;
	  }

	SET_MANAGER(manager, tips_formatstring,
		     copy_string (
			     &globals.managers[id].tips_formatstring, token));
#if 0
	flags = parse_format_dependencies(token);
	SET_MANAGER(manager, format_depend, flags);
	Free(token);
#endif
      }
      else if (!strcasecmp(option1, "tipsborderwidth")) {
	p = read_next_cmd(READ_ARG);
	if (!p) {
	  ConsoleMessage("Bad line: %s\n", current_line);
	  continue;
	}
	if (extract_int(p, &n) == 0) {
	  ConsoleMessage("This is not a number: %s\n", p);
	  ConsoleMessage("Bad line: %s\n", current_line);
	  continue;
	}
	if (n < 0)
	{
		n = 0;
	}
	SET_MANAGER(manager, tips_conf->border_width, n);
      }
      else if (!strcasecmp(option1, "tipsoffsets")) {
	p = read_next_cmd(READ_ARG);
	if (!p) {
	  ConsoleMessage("Bad line: %s\n", current_line);
	  continue;
	}
	if (extract_int(p, &n) == 0) {
	  ConsoleMessage("This is not a number: %s\n", p);
	  ConsoleMessage("Bad line: %s\n", current_line);
	  continue;
	}
	if (n < 1) {
		ConsoleMessage("A tips offset must be positive: %s\n", p);
		ConsoleMessage("Bad line: %s\n", current_line);
		continue;
	}
	SET_MANAGER(manager, tips_conf->placement_offset, n);
	p = read_next_cmd(READ_ARG);
	if (!p)
	{
		ConsoleMessage("Bad line: %s\n", current_line);
		continue;
	}
	if (extract_int(p, &n) == 0) {
	  ConsoleMessage("This is not a number: %s\n", p);
	  ConsoleMessage("Bad line: %s\n", current_line);
	  continue;
	}
	if (n < 1) {
		ConsoleMessage("A tips offset must be positive: %s\n", p);
		ConsoleMessage("Bad line: %s\n", current_line);
		continue;
	}
	SET_MANAGER(manager, tips_conf->justification_offset, n);
      }
      else if (!strcasecmp(option1, "tipsplacement")) {
	      p = read_next_cmd(READ_ARG);
	      if (!p) {
		      ConsoleMessage("Bad line: %s\n", current_line);
		      continue;
	      }
	      if (!strcasecmp(p, "up")) {
		      i = FTIPS_PLACEMENT_UP;
	      }
	      else if (!strcasecmp(p, "down")) {
		      i = FTIPS_PLACEMENT_DOWN;
	      }
	      else if (!strcasecmp(p, "left")) {
		      i = FTIPS_PLACEMENT_LEFT;
	      }
	      else if (!strcasecmp(p, "right")) {
		      i = FTIPS_PLACEMENT_RIGHT;
	      }
	      else if (!strcasecmp(p, "updown")) {
		      i = FTIPS_PLACEMENT_AUTO_UPDOWN;
	      }
	      else if (!strcasecmp(p, "leftright")) {
		      i = FTIPS_PLACEMENT_AUTO_LEFTRIGHT;
	      }
	      else {
		      ConsoleMessage("Bad line: %s\n", current_line);
		      ConsoleMessage("What is this: %s?\n", p);
		      continue;
	      }
	      SET_MANAGER(manager, tips_conf->placement, i);
      }
      else if (!strcasecmp(option1, "tipsjustification")) {
	      p = read_next_cmd(READ_ARG);
	      if (!p) {
		      ConsoleMessage("Bad line: %s\n", current_line);
		      continue;
	      }
	      if (!strcasecmp(p, "center")) {
		      i = FTIPS_JUSTIFICATION_CENTER;
	      }
	      else if (!strcasecmp(p, "leftup")) {
		      i = FTIPS_JUSTIFICATION_LEFT_UP;
	      }
	      else if (!strcasecmp(p, "rightdown")) {
		      i = FTIPS_JUSTIFICATION_RIGHT_DOWN;
	      }
	      else {
		      ConsoleMessage("Bad line: %s\n", current_line);
		      ConsoleMessage("What is this: %s?\n", p);
		      continue;
	      }
	      SET_MANAGER(manager, tips_conf->justification, i);
      }
      else {
	ConsoleMessage("Bad line: %s\n", current_line);
	ConsoleMessage("Unknown option: %s\n", p);
      }
    }
  }

  if (globals.managers == NULL) {
    ConsoleDebug(CONFIG, "I'm assuming you only want one manager\n");
    allocate_managers(1);
  }
  print_managers();
}

void process_dynamic_config_line(char *line)
{
	int manager = 0;
	char *token;
	extern void remanage_winlist(void);

	/* don't support dynamic config for transient */
	if (globals.transient)
	{
		return;
	}

	line += ModuleLen;
	line = GetNextToken(line, &token);
	if (token && isdigit(*token))
	{
		if (extract_int(token, &manager) == 0
			|| manager <= 0 || manager > globals.num_managers)
		{
			manager = 0;
		}
		free(token);
		line = GetNextToken(line, &token);
	}

	if (!token)
	{
		return;
	}
	manager--;

	/* currently supports only "resolution" and "tips" */
	if (strcasecmp(token, "resolution") == 0)
	{
		int value;

		free(token);
		line = GetNextToken(line, &token);
		if (!token)
		{
			return;
		}
		if (!strcasecmp(token, "global"))
			value = SHOW_GLOBAL;
		else if (!strcasecmp(token, "desk"))
			value = SHOW_DESKTOP;
		else if (!strcasecmp(token, "page"))
			value = SHOW_PAGE;
		else if (!strcasecmp(token, "screen"))
			value = SHOW_SCREEN;
		else if (!strcasecmp(token, "!desk"))
			value = NO_SHOW_DESKTOP;
		else if (!strcasecmp(token, "!page"))
			value = NO_SHOW_PAGE;
		else if (!strcasecmp(token, "!screen"))
			value = NO_SHOW_SCREEN;
		else
		{
			fprintf(
				stderr, "%s: unknown resolution %s.\n",
				MyName, token);
			free(token);
			return;
		}

		SET_MANAGER(manager, res, value);
		remanage_winlist();
	}
	else if (strcasecmp(token, "tips") == 0)
	{
		int value;

		free(token);
		line = GetNextToken(line, &token);
		if (!token)
		{
			return;
		}
		if (!strcasecmp(token, "always"))
		{
			value = TIPS_ALWAYS;
		}
		else if (!strcasecmp(token, "false"))
		{
			value = TIPS_NEVER;
		}
		else if (!strcasecmp(token, "needed"))
		{
			value = TIPS_NEEDED;
		}
		else
		{
			ConsoleMessage("Bad line: %s\n", line);
			ConsoleMessage("What is this: %s?\n", token);
			free(token);
			return;
		}
		SET_MANAGER(manager, tips, value);
		if (manager == -1)
		{
			tips_cancel(NULL);
		}
		else
		{
			tips_cancel(&globals.managers[manager]);
		}
	}

	free(token);
}
