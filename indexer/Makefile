CFLAGS = -Wall -pedantic -std=c11 -I../utils -L../lib  -g
LIBS = -lutils -lcurl

all: indexer

indexer: indexer.c ../lib/libutils.a
	gcc $(CFLAGS) indexer.c $(LIBS) -o $@

../lib/libutils.a:
	$(MAKE) -C ../utils

valgrind: indexer
	valgrind --leak-check=full --show-leak-kinds=all ./indexer ../pagedir ../indexnm.txt

clean:
	rm -f indexer
