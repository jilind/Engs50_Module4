CFLAGS = -Wall -pedantic -std=c11 -I. -g

OFILES = queue.o hash.o webpage.o pageio.o indexio.o

all: $(OFILES)
	ar rcs ../lib/libutils.a $(OFILES)

%.o: %.c %.h
	gcc $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OFILES) ../lib/libutils.a
