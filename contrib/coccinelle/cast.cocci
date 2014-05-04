@@
type T;
expression E;
@@

- (T *)safemalloc(E)
+ xmalloc(E)

@@
expression E;
@@

- safemalloc(E)
+ xmalloc(E)

@@
type T;
expression E;
@@

- (T *)safestrdup(E)
+ xstrdup(E)

@@
expression E;
@@

- safestrdup(E)
+ xstrdup(E)

@@
type T;
expression E, E1;
@@

- (T *)safecalloc(E, E1)
+ xcalloc(E, E1)

@@
expression E, E1;
@@

- safecalloc(E, E1)
+ xcalloc(E, E1)

@@
type T;
expression E, E1, E2;
@@

- (T *)saferealloc(E, E1)
+ xrealloc(E, E1, sizeof(E))

@@
expression E, E1;
@@

- saferealloc(E, E1)
+ xrealloc(E, E1, sizeof(E))

@@
expression E, E1;
@@

- xrealloc(E, E1)
+ xrealloc(E, E1, sizeof(E))
