
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>

#include <libs/fvwmlib.h>


typedef struct str_struct
{
  struct str_struct *next;
  char *s;
  int is_var;
} str;

/* split string val into a list of str's, each containing 
   an ordinary substring or a variable reference of the form $(bla). 
   The list is appended to pre, the last list entry is returned.
*/
static str *
split_string (char *val, str *pre)
{
  char *s, *t;
  char *v = val;
  str *curr = pre;
  
  while (v && *v) 
    {
      s = strstr (v, "$(");
      if (s) 
	{
	  t = strstr (s, ")");
	}

      if (s && t) 
	{
	  /* append the part before $( */
	  curr->next = (str *) safemalloc (sizeof (str));
	  curr = curr->next;
	  *s = '\0';
	  curr->s = strdup (v);
	  curr->is_var = 0;
	  *s = '$';
	  
	  /* append the variable reference, silently omit the ) */
	  curr->next = (str *) safemalloc (sizeof (str));
	  curr = curr->next;
	  curr->next = NULL;
	  *t = '\0';
	  curr->s = strdup (s + 2);    
	  curr->is_var = 1;
	  *t = ')';
	  
	  v = t + 1;
	}
      else
	{
	  /* append the part after the last variable reference */
	  curr->next = (str *) safemalloc (sizeof (str));
	  curr = curr->next;
	  curr->next = NULL;
	  curr->s = strdup (v);
	  curr->is_var = 0;
	  v = NULL;
	}
    }
  return curr;
}


static char *
combine_string (str *p)
{
  str *r, *next;
  int l;
  char *res;
  
  for (l = 1, r = p; r != NULL; r = r->next)
    {
      l += strlen (r->s);
    }
  res = (char *) safemalloc (l * sizeof (char));
  *res = '\0';
  for (r = p; r != NULL; r = next)
    {
      strcat (res, r->s);
      next = r->next;
      free (r);
    }
  return res;
}


char *
recursive_replace (GtkWidget *d, char *val)
{
  str *head, *r, *tail, *next;
  char *nval;

  head = (str *) safemalloc (sizeof (str));
  head->is_var = 0;
  head->s = "";
  head->next = NULL;

  split_string (val, head);
  for (r = head; r->next != NULL; r = r->next)
    {
      next = r->next;
      if (next->is_var)
	{
	  nval = (char *) gtk_object_get_data (GTK_OBJECT (d), next->s);
	  if (nval)
	    {
              /* this changes r->next, thus freeing next is safe */
	      tail = split_string (nval, r);
	      tail->next = next->next;
              free (next);
	    }
	  else 
	    {
	      next->is_var = 0;
	    }
	}
    }
  return combine_string (head);
}



