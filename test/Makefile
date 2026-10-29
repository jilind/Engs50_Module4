CFLAGS = -Wall -pedantic -std=c11 -I../utils -L../lib -g
LIBS =-lutils -lcurl

all: testpageio testindexio

testpageio: testpageio.c ../lib/libutils.a
	gcc $(CFLAGS) testpageio.c  $(LIBS) -o $@

testindexio: testindexio.c ../lib/libutils.a
	gcc $(CFLAGS) testindexio.c  $(LIBS) -o $@

../lib/libutils.a:
	$(MAKE) -C ../utils

clean:
	rm -f testpageio testindexio test.index
