/*
** XResource.c:
** These routines provide modules with an interface to parse all kinds of
** configuration options (X resources, command line options and configuration
** file lines) in the same way (Xrm database).
*/

#include "config.h"

#include <X11/Xlib.h>
#include <X11/Xresource.h>
#include <string.h>

#include "fvwmlib.h"



/***************************************************************************
 * If you have a module MyModule and want to parse X resources as well as
 * command line options and a config file:
 *
 *** EXAMPLE ***************************************************************/
#if 0
    #include <fvwmlib.h>

    void main(int argc, char **argv)
    {
      const char *MyName = "MyModule";
      XrmDatabase db = NULL;
      char *value;
      char *line;

      /* our private options */
      const XrmOptionDescRec my_opts[] = {
        { "-iconic",   ".Iconic", XrmoptionNoArg,  "any_string" },
        { "-foo",      "*bar",    XrmoptionSepArg, NULL }
      };
      int opt_argc = argc - 6; /* options start at 6th argument for modules */
      char **opt_argv = argv + 6;

      /* ... (open config file, etc.) */ 

      /* Get global X resources */
      MergeXResources(NULL, &db, False);

      /* config file descriptor in fd; config file takes precedence over X
       * resources (this may not be what you want). */
      for (GetConfigLine(fd, &line); line != NULL; GetConfigLine(fd, &line))
      {
        if (!MergeConfigLineResource(&db, line, MyName, '*'))
        {
          /* Parse other lines here (e.g. "IconPath") */
        }
        else
        {
          /* You may still have to parse the line here yourself (e.g.
           * FvwmButtons may have multiple lines for the same resource). */
        }
      }

      /* command line takes precedence over all */
      MergeCmdLineResources(&db, (XrmOptionDescList)my_opts, 2, MyName,
			    &opt_argc, opt_argv, True /*no default options*/);

      /* Now parse the database values: */
      if (GetResourceString(db, "iconic", MyName, &value))
      {
	/* Just see if there is *any* string and don't mind it's value. */
        /* flags |= ICONIC */
      }
      if (GetResourceString(db, "bar", MyName, &value))
      {
        /* ... */
      }

      /* ... */
      XrmDestroyDatabase(db);
    }
#endif

/*** END OF EXAMPLE ********************************************************/




/* Default option table */
static XrmOptionDescRec default_opts[] =
{
  { "-fg",       "*Foreground", XrmoptionSepArg, NULL },
  { "-bg",       "*Background", XrmoptionSepArg, NULL },
  { "-fn",       "*Font",       XrmoptionSepArg, NULL },
  { "-geometry", "*Geometry",   XrmoptionSepArg, NULL },
  { "-title",    "*Title",      XrmoptionSepArg, NULL }
  /* Remember to update NUM_DEFAULT_OPTIONS if you change this list! */
};
#define NUM_DEFAULT_OPTS 5



/* internal function */
static void DoMergeString(char *resource, XrmDatabase *ptarget, Bool override)
{
  XrmDatabase db;

  if (!resource)
    return;
  db = XrmGetStringDatabase(resource);
  XrmCombineDatabase(db, ptarget, override);
}

/***************************************************************************
 *
 * Merges all X resources for the display/screen into a Xrm database.
 * If the database does not exist (*pdb == NULL), a new database is created.
 * If override is True, existing entries of the same name are overwritten.
 *
 * Please remember to destroy the database with XrmDestroyDatabase(*pdb)
 * if you do not need it amymore.
 *
 ***************************************************************************/
void MergeXResources(Display *dpy, XrmDatabase *pdb, Bool override)
{
  if (!*pdb)
    /* create new database */
    XrmPutStringResource(pdb, "", "");
  DoMergeString(XResourceManagerString(dpy), pdb, override);
  DoMergeString(XScreenResourceString(DefaultScreenOfDisplay(dpy)), pdb,
		override);
}

/***************************************************************************
 *
 * Parses the command line given through pargc/argv and puts recognized
 * entries into the Xrm database *pdb (if *pdb is NULL a new database is
 * created). The caller may provide an option list in XrmOptionDescList
 * format (see XrmParseCommand manpage) and/or parse only standard options
 * (fg, bg, geometry, fn, title). User given options have precedence over
 * standard options which are disabled if fNoDefaults is True. Existing
 * values are overwritten.
 *
 * All recognised options are removed from the command line (*pargc and
 * argv are updated accordingly).
 *
 * Please remember to destroy the database with XrmDestroyDatabase(*pdb)
 * if you do not need it amymore.
 *
 ***************************************************************************/
void MergeCmdLineResources(XrmDatabase *pdb, XrmOptionDescList opts,
			   int num_opts, char *name, int *pargc, char **argv,
			   Bool fNoDefaults)
{
  if (opts && num_opts > 0)
    XrmParseCommand(pdb, opts, num_opts, name, pargc, argv);
  if (!fNoDefaults)
    XrmParseCommand(pdb, default_opts, NUM_DEFAULT_OPTS,
		    name, pargc, argv);
}

/***************************************************************************
 *
 * Takes a line from a config file and puts a corresponding value into the
 * Xrm database *pdb (will be created if *pdb is NULL). 'prefix' is the
 * name of the module. A specific type of binding in the database must be
 * provided in bindstr (either "*" or "."). Leading unquoted whitespace are
 * stripped from value. Existing values in the database are overwritten.
 * True is returned if the line was indeed merged into the database (i.e. it
 * had the correct format) or False if not.
 *
 * Example: If prefix = "MyModule" and bindstr = "*", the line
 *
 *   *MyModuleGeometry   80x25+0+0
 *
 * will be put into the database as if you had this line in your .Xdefaults:
 *
 *   MyModule*Geometry:  80x25+0+0
 *
 * Please remember to destroy the database with XrmDestroyDatabase(*pdb)
 * if you do not need it amymore.
 *
 ***************************************************************************/
Bool MergeConfigLineResource(XrmDatabase *pdb, char *line, char *prefix,
			     char *bindstr)
{
  int len;
  char *end;
  char *value;
  char *myvalue;
  char *resource;

  /* translate "*(prefix)(suffix)" to "(prefix)(binding)(suffix)",
   * e.g. "*FvwmPagerGeometry" to "FvwmPager.Geometry" */
  if (!line || *line != '*')
    return False;

  line++;
  len = (prefix) ? strlen(prefix) : 0;
  if (!prefix || strncasecmp(line, prefix, len))
    return False;

  line += len;
  end = line;
  while (*end && !isspace(*end))
    end++;
  if (line == end)
    return False;
  value = end;
  while (*value && isspace(*value))
    value++;

  /* prefix*suffix: value */
  resource = (char *)safemalloc(len + (end - line) + 2);
  strcpy(resource, prefix);
  strcat(resource, bindstr);
  strncat(resource, line, end - line);

  len = strlen(value);
  myvalue = (char *)safemalloc(len + 1);
  strcpy(myvalue, value);
  for (len--; len >= 0 && isspace(myvalue[len]); len--)
    myvalue[len] = 0;

  /* merge string into database */
  XrmPutStringResource(pdb, resource, myvalue);

  free(resource);
  free(myvalue);
  return True;
}

/***************************************************************************
 *
 * Reads the string-value for the pair prefix/resource from the Xrm database
 * db and returns a pointer to it. The string may only be read and must not
 * be freed by the caller. 'prefix' is the class name (usually the name of
 * the module). If no value is found in the database, *val will be NULL.
 * True is returned if a value was found, False if not. If you are only
 * interested if there is a string, but not it's value, you can set val to
 * NULL.
 *
 * Example:
 *
 *   GetResourceString(db, "Geometry", "MyModule", &s)
 *
 * returns the string value of the "Geometry" resource for MyModule in s.
 *
 ***************************************************************************/
Bool GetResourceString(XrmDatabase db, const char *resource,
		       const char *prefix, char **val)
{
  XrmValue xval = { 0, NULL };
  char *str_type;
  char *name;

  name = (char *)safemalloc(strlen(resource) + strlen(prefix) + 2);
  strcpy(name, prefix);
  strcat(name, ".");
  strcat(name, resource);

  if (!XrmGetResource(db, name, name, &str_type, &xval) || xval.addr == NULL)
    {
      free(name);
      if (val)
	*val = NULL;
      return False;
    }
  free(name);
  if (val)
    *val = xval.addr;

  return True;
}
