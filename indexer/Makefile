CFLAGS = -Wall -pedantic -std=c11 -I../utils -L../lib  -g
LIBS = -lutils -lcurl

all: indexer indexer_pt

indexer: indexer.c ../lib/libutils.a
	gcc $(CFLAGS) indexer.c $(LIBS) -o $@

indexer_pt: indexer_pt.c ../lib/libutils.a
	gcc $(CFLAGS) -pthread indexer_pt.c $(LIBS) -o $@

../lib/libutils.a:
	$(MAKE) -C ../utils

valgrind: indexer
	valgrind --leak-check=full --show-leak-kinds=all ./indexer ../pagedir ../index_valgrind.txt

valgrind_pt: indexer_pt
	valgrind --leak-check=full --show-leak-kinds=all ./indexer_pt ../pagedir ../index_valgrind_pt.txt 5

clean:
	rm -f indexer indexer_pt
