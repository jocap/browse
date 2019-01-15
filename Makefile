PREFIX = /usr/local

CFLAGS += -Wall -Wextra -pedantic -Wno-missing-braces
CFLAGS += -Werror 

browse: browse.c term.c term.h
	cc $(CFLAGS) $(LDFLAGS) -o $@ $< term.c

install: browse
	cp browse ${PREFIX}/bin/browse
	cp browse.1 ${PREFIX}/man/man1/browse.1

uninstall:
	rm ${PREFIX}/bin/browse
	rm ${PREFIX}/man/man1/browse.1
