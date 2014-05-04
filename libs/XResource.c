/* -*-c-*- */
/* Copyright (C) 1999  Dominik Vogt */
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

/*
** XResource.c:
** These routines provide modules with an interface to parse all kinds of
** configuration options (X resources, command line options and configuration
** file lines) in the same way (Xrm database).
*/

#include "config.h"

#include <X11/Xlib.h>
#include <X11/Xresource.h>

#include "fvwmlib.h"
#include "XResource.h"



/*
 * If you have a module MyModule and want to parse X resources as well as
 * command line options and a config file:
 *
 *** EXAMPLE */
#if 0
    #include <fvwmlib.h>

    void main(int argc, char **argv)
    {
      const char *MyName = "MyModule";
      XrmDatabase db = NULL;
      XrmValue *rm_value;
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
	  /* Parse other lines here (e.g. "ImagePath") */
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
      if (GetResourceString(db, "iconic", MyName, &rm_value))
      {
	/* Just see if there is *any* string and don't mind it's value. */
	/* flags |= ICONIC */
      }
      if (GetResourceString(db, "bar", MyName, &rm_value))
      {
	/* ... */
      }

      /* ... */
      XrmDestroyDatabase(db);
    }
#endif

/*** END OF EXAMPLE ***/




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

/*
 *
 * Merges all X resources for the display/screen into a Xrm database.
 * If the database does not exist (*pdb == NULL), a new database is created.
 * If override is True, existing entries of the same name are overwritten.
 *
 * Please remember to destroy the database with XrmDestroyDatabase(*pdb)
 * if you do not need it amymore.
 *
 */
void MergeXResources(Display *dpy, XrmDatabase *pdb, Bool override)
{
  if (!*pdb)
    /* create new database */
    XrmPutStringResource(pdb, "", "");
  DoMergeString(XResourceManagerString(dpy), pdb, override);
  DoMergeString(XScreenResourceString(DefaultScreenOfDisplay(dpy)), pdb,
		override);
}

/*
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
 */
void MergeCmdLineResources(XrmDatabase *pdb, XrmOptionDescList opts,
			   int num_opts, char *name, int *pargc, char **argv,
			   Bool fNoDefaults)
{
  if (!name)
    return;
  if (opts && num_opts > 0)
    XrmParseCommand(pdb, opts, num_opts, name, pargc, argv);
  if (!fNoDefaults)
    XrmParseCommand(pdb, default_opts, NUM_DEFAULT_OPTS,
		    name, pargc, argv);
}

/*
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
 */
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
  while (*end && !isspace((unsigned char)*end))
    end++;
  if (line == end)
    return False;
  value = end;
  while (*value && isspace((unsigned char)*value))
    value++;

  /* prefix*suffix: value */
  /* TA:  FIXME!  xasprintf() */
  resource = xmalloc(len + (end - line) + 2);
  strcpy(resource, prefix);
  strcat(resource, bindstr);
  strncat(resource, line, end - line);

  len = strlen(value);
  myvalue = xmalloc(len + 1);
  strcpy(myvalue, value);
  for (len--; len >= 0 && isspace((unsigned char)myvalue[len]); len--)
    myvalue[len] = 0;

  /* merge string into database */
  XrmPutStringResource(pdb, resource, myvalue);

  free(resource);
  free(myvalue);
  return True;
}

/*
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
 *   GetResourceString(db, "Geometry", "MyModule", &r)
 *
 * returns the resource value of the "Geometry" resource for MyModule in r.
 *
 */
Bool GetResourceString(
  XrmDatabase db, const char *resource, const char *prefix, XrmValue *xval)
{
  char *str_type;
  char *name;
  char *Name;
  int i;

  /* TA:  FIXME!  xasprintf() */
  name = xmalloc(strlen(resource) + strlen(prefix) + 2);
  Name = xmalloc(strlen(resource) + strlen(prefix) + 2);
  strcpy(name, prefix);
  strcat(name, ".");
  strcat(name, resource);
  strcpy(Name, name);
  if (isupper(name[0]))
    name[0] = tolower(name[0]);
  if (islower(Name[0]))
    Name[0] = toupper(Name[0]);
  i = strlen(prefix) + 1;
  if (isupper(name[i]))
    name[i] = tolower(name[i]);
  if (islower(Name[i]))
    Name[i] = toupper(Name[i]);
  if (!XrmGetResource(db, name, Name, &str_type, xval) ||
      xval->addr == NULL || xval->size == 0)
  {
    free(name);
    free(Name);
    xval->size = 0;
    xval->addr = NULL;

    return False;
  }
  free(name);
  free(Name);

  return True;
}
