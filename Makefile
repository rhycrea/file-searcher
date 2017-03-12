CC = gcc
CFLAGS = -g -ansi

all: clean binarysearcher run

.PHONY: clean

clean:
	rm -f binarysearcher 

binarysearcher: 
	$(CC) binarysearcher.c $(CFLAGS) -o binarysearcher

run: 
	./binarysearcher ./sample command.txt
