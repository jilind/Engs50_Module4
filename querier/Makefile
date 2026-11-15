CFLAGS = -Wall -pedantic -std=c11 -I../utils -L../lib -g
LIBS =-lutils -lcurl

all: query

query: query.c ../lib/libutils.a
	gcc $(CFLAGS) query.c $(LIBS) -o $@

../lib/libutils.a:
	$(MAKE) -C ../utils

valgrind: query
	printf 'dartmouth or college\n' | valgrind --leak-check=full --show-leak-kinds=all ./query ../pagedir ../index.txt -q

clean:
	rm -f query    
