CC = gcc
CFLAGS = -g -Wall

ring: ring.o entry.o
	$(CC) ring.o entry.o -lm -o ring

ring.o: ring.c entry.h
	$(CC) $(CFLAGS) -c ring.c

entry.o: entry.c entry.h
	$(CC) $(CFLAGS) -c entry.c

clean: 
	rm -f -v ./*.o ring