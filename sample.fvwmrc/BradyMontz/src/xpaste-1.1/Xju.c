/*
 * Miscellaneous X utility routines
 *
 * $Id$
 */
#include <varargs.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>

extern Widget root;

void SetArgs(va_alist)
va_dcl
{
	/* SetArgs takes a variable number of arguments, of the form 
	   of char *argtype, XtArgVal value pairs. This ends with
	   a NULL. SetArgs then returns the corresponding argument list.
	   SetArgs' second argument should be a pointer to a Cardinal, 
	   which will contain the number of args in the new arglist, 
	   and its first argument should be a pointer to an Arg
	   pointer which will be set to the returned arg list */
	va_list argindex;
	unsigned i;
	Arg **arglist;
	Cardinal *count;

	va_start(argindex);

	/* Get first two arguments */
	arglist = va_arg(argindex, Arg **);
	count = va_arg(argindex, Cardinal *);

	/* Count number of passed Arglist pairs */
	*count = 0;
	while(1){
		if(NULL == va_arg(argindex, char *))
			break;
		(void) va_arg(argindex, XtArgVal);
		(*count)++;
	}
	va_end(argindex);

	/* Allocate sufficient memory for new arg list */
	*arglist = (Arg *)XtMalloc((*count)*sizeof(Arg));

	/* Set new Arg list */
	va_start(argindex);

	/* skip past first two arguments */
	(void) va_arg(argindex, Arg **);
	(void) va_arg(argindex, int *);

	for(i=0;i<*count;i++){
		XtSetArg((*arglist)[i], va_arg(argindex, char *),
			 va_arg(argindex, XtArgVal));
	}
	va_end(argindex);
}
	

Widget CreateWidget(va_alist) 
va_dcl
{
	/* CreateWidget takes a variable number of arguments,
	   the first of which are the following:
		widget name, widget class, widget parent.
           All following arguments are char *argtype, XtArgVal pairs. The
	   last argument must be a NULL. 

	   CreateWidget will construct a widget of the given class,
	   with the given name and parent. An argument list for the
	   widget constructed out of the char *argtype, XtArgVal pairs
	   will be created as well. CreateWidget will call 
	   XtCreateManagedWidget, and return the resulting widget id */

	va_list argindex;
	unsigned i;
	Arg *arglist;
	Cardinal count;
	Widget w, parent;
	char *name;
	WidgetClass class;

	va_start(argindex);

	/* Get first three arguments */
	name = va_arg(argindex, char *);
	class = va_arg(argindex, WidgetClass);
	parent = va_arg(argindex, Widget);

	/* Count number of passed Arglist pairs */
	count = 0;
	while(1){
		if (NULL == va_arg(argindex, char *))
			break;
		(void) va_arg(argindex, XtArgVal);
		count++;
	}
	va_end(argindex);

	/* Allocate sufficient memory for new arg list */
	arglist = (Arg *)XtMalloc((count)*sizeof(Arg));

	/* Set new Arg list */
	va_start(argindex);

	/* skip past first three arguments */
	(void) va_arg(argindex, char *);
	(void) va_arg(argindex, WidgetClass);
	(void) va_arg(argindex, Widget);

	for(i=0;i<count;i++){
		XtSetArg(arglist[i], va_arg(argindex, char *),
			 va_arg(argindex, XtArgVal));
	}
	va_end(argindex);

	if(parent == NULL) (void) printf("%s\n", name);
	w = XtCreateManagedWidget(name, class, parent,
				  arglist, count);

	XtFree((char *)arglist);
	return(w);
}
	
void QueryWidget(va_alist)
va_dcl
{
	/* QueryWidget takes a variable number of arguments,
	   the first of which is the widget being queried.
           All following arguments are char *argtype, *XtArgVal pairs. The
	   last argument must be a NULL. 

	   QueryWidget will query the specified widget using XtGetValues,
	   and will return the results in the same way as XtGetValues. */

	va_list argindex;
	unsigned i;
	Arg *arglist;
	Cardinal count;
	Widget w;

	va_start(argindex);

	/* Get first argument */
	w = va_arg(argindex, Widget);

	/* Count number of passed Arglist pairs */
	count = 0;
	while(1){
		if (NULL == va_arg(argindex, char *))
			break;
		(void) va_arg(argindex, XtArgVal);
		count++;
	}
	va_end(argindex);

	/* Allocate sufficient memory for new arg list */
	arglist = (Arg *)XtMalloc(count*sizeof(Arg));

	/* Set new Arg list */
	va_start(argindex);

	/* skip past first argument */
	(void) va_arg(argindex, Widget);

	for(i=0;i<count;i++){
		XtSetArg(arglist[i], va_arg(argindex, char *),
			 va_arg(argindex, XtArgVal));
	}
	va_end(argindex);

	XtGetValues(w, arglist, count);
	XtFree((char *)arglist);
}
	
void ConfigureWidget(va_alist)
va_dcl
{
	/* ConfigureWidget takes a variable number of arguments, the
	   first of which is the widget being queried. All following arguments
	   are char *argtype, XtArgVal pairs. The last argument must be a
	   NULL.

	   ConfigureWidget will configure the specified widget using 
	   XtSetValues. */

	va_list argindex;
	unsigned i;
	Arg *arglist;
	Cardinal count;
	Widget w;

	va_start(argindex);

	/* Get first argument */
	w = va_arg(argindex, Widget);

	/* Count number of passed Arglist pairs */
	count = 0;
	while(1){
		if (NULL== va_arg(argindex, char *))
			break;
		(void) va_arg(argindex, XtArgVal);
		count++;
	}
	va_end(argindex);

	/* Allocate sufficient memory for new arg list */
	arglist = (Arg *)XtMalloc(count*sizeof(Arg));

	/* Set new Arg list */
	va_start(argindex);

	/* skip past first argument */
	(void) va_arg(argindex, Widget);

	for(i=0;i<count;i++){
		XtSetArg(arglist[i], va_arg(argindex, char *),
			 va_arg(argindex, XtArgVal));
	}
	va_end(argindex);

	XtSetValues(w, arglist, count);
	XtFree((char *)arglist);
}
