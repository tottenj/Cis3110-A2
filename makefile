CC = gcc
CFLAGS = -Wall -g -std=c11

A2checker: main.o
	$(CC) -pg main.o -o A2checker -lpthread

main.o: main.c
	$(CC) $(CFLAGS) -c main.c -o main.o

clean:
	rm *.o A2checker