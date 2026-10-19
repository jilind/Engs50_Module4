CFLAGS = -Wall -pedantic -std=c11 -I. -g

OFILES = queue.o hash.o webpage.o

all: ../lib/libutils.a

../lib/libutils.a: $(OFILES)
	ar rcs $@ $(OFILES)

%.o: %.c %.h
	gcc $(CFLAGS) -c $<

clean:
	rm -f $(OFILES) ../lib/libutils.a
