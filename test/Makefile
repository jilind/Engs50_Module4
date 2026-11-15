CFLAGS = -Wall -pedantic -std=c11 -I../utils -L../lib -g
LIBS =-lutils -lcurl
LQTESTS = testlqueue_singlet testlqueue_doublet
LHTESTS = testlhash_singlet testlhash_doublet

all: testpageio testindexio $(LQTESTS) $(LHTESTS)

testpageio: testpageio.c ../lib/libutils.a
	gcc $(CFLAGS) testpageio.c  $(LIBS) -o $@

testindexio: testindexio.c ../lib/libutils.a
	gcc $(CFLAGS) testindexio.c  $(LIBS) -o $@

$(LQTESTS): %: %.c ../utils/lqueue.h ../lib/libutils.a
	gcc $(CFLAGS) -pthread $< -lutils -o $@

$(LHTESTS): %: %.c ../utils/lhash.h ../lib/libutils.a
	gcc $(CFLAGS) -pthread $< -lutils -o $@

../lib/libutils.a:
	$(MAKE) -C ../utils

clean:
	rm -f testpageio testindexio test.index $(LQTESTS) $(LHTESTS)
