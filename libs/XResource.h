#ifndef LIB_XRESOURCE_H
#define LIB_XRESOURCE_H

/*
 * Wrappers around Xrm routines (XResources.c)
 */
void MergeXResources(Display *dpy, XrmDatabase *pdb, Bool override);
void MergeCmdLineResources(
	XrmDatabase *pdb, XrmOptionDescList opts, int num_opts, char *name,
	int *pargc, char **argv, Bool fNoDefaults);
Bool MergeConfigLineResource(
	XrmDatabase *pdb, char *line, char *prefix, char *bindstr);
Bool GetResourceString(
	XrmDatabase db, const char *resource, const char *prefix,
	XrmValue *xval);

#endif /* LIB_XRESOURCE_H */
