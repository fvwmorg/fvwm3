@Remove_unnecessary_pointer_checks@
expression x;
@@
-if (\(x != 0 \| x != NULL\))
    free(x);