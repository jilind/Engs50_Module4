CFLAGS = -Wall -pedantic -std=c11 -I../utils -L../lib -g
LIBS =-lutils -lcurl

all: crawler

crawler:
	gcc $(CFLAGS) crawler.c $(LIBS) -o $@

valgrind: crawler
	valgrind --leak-check=full --show-leak-kinds=all ./crawler https://thayer.github.io/engs50/ ../pagedir 1

clean:
	rm -f crawler
