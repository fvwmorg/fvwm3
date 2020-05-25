@@
expression L, F;
@@

- fvwm_msg(
+ fvwm_debug(
- L, F,
+ __func__,
...);

@@
expression S;
@@

- fprintf(stderr,
+ fvwm_debug(__func__,
...);

