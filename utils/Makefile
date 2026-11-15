CFLAGS = -Wall -pedantic -std=c11 -I. -g

OFILES = queue.o hash.o webpage.o pageio.o indexio.o lqueue.o lhash.o

all: $(OFILES)
	ar rcs ../lib/libutils.a $(OFILES)

%.o: %.c %.h
	gcc $(CFLAGS) -c $< -o $@

hash.o: queue.h

lqueue.o: lqueue.c lqueue.h queue.h
	gcc $(CFLAGS) -pthread -c $< -o $@

lhash.o: lhash.c lhash.h hash.h
	gcc $(CFLAGS) -pthread -c $< -o $@

clean:
	rm -f $(OFILES) ../lib/libutils.a
