#/usr/bin/sh

# This isn't in cmake because these tools are only needed for
# developers that want to regenerate the FvwmScript syntax.

LEX = flex
YACC = yacc

rm -f lex.yy.c y.tab.c y.tab.h script.tab.c script.tab.h || true
if flex --nounput --help 2>/dev/null > /dev/null; then \
        flex   --nounput scanner.l; \
else \
        flex   scanner.l; \
fi && mv lex.yy.c scanner.c
YACC=yacc; if which bison >/dev/null 2>&1; then YACC="bison -y"; fi; \
$YACC  -d script.y && mv *.tab.c script.c
if test -f y.tab.h -o -f script.tab.h; then \
        if cmp -s *.tab.h script.h; then \
                rm -f y.tab.h; \
        else \
                mv *.tab.h script.h; \
        fi; \
else :; fi
