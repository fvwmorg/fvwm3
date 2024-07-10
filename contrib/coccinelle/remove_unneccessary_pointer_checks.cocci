@Remove_unnecessary_pointer_checks@
expression x;
@@
(
-if (\(x != 0 \| x != NULL\))
    free(x);
|
-if (\(x != 0 \| x != NULL\)) {
    free(x);
    x = \(0 \| NULL\);
-}
)

@Remove_unnecessary_pointer_checks2@
expression a, b;
@@
(
-if (\(a != 0 \| a != NULL\) && \(b != 0 \| b != NULL\))
+if (a)
    free(b);
|
-if (\(a != 0 \| a != NULL\) && \(b != 0 \| b != NULL\)) {
+if (a) {
    free(b);
    b = \(0 \| NULL\);
 }
|
-if (\(a != 0 \| a != NULL\) && \(b == 0 \| b == NULL\))
+if (!b)
    free(a);
|
-if (\(a != 0 \| a != NULL\) && \(b == 0 \| b == NULL\)) {
+if (!b) {
    free(a);
    a = \(0 \| NULL\);
 }
)
