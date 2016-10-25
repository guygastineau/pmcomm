CC = g++
CFLAGS = -g -Wall -I../include
LDFLAGS = -L.. -lpmcomm

all: example

example: main.o
	$(CC) main.o $(LDFLAGS) -o example

main.o: main.cpp
	$(CC) -c $(CFLAGS) main.cpp -o main.o

.PHONY: clean
clean:
	rm -f main.o example
