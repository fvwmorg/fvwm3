# This is not portable, nevermind.

all: hints_test

clean:
	rm -f core hints_test.o hints_test

hints_test.o: hints_test.c
	gcc -g -O2 -Wall -I/usr/X11R6/include -L/usr/X11R6/lib -c hints_test.c

hints_test: hints_test.o
	gcc -g -O2 -Wall -o hints_test hints_test.o -I/usr/X11R6/include -L/usr/X11R6/lib -L../../libs/ -lX11 -lfvwm

