CC=gcc
CFLAGS=-Wall -Wextra -std=c99 -I.
DEPS=

OBJ += main.o vma.o DoublyLinkedList.o 

%.o: %.c 
		$(CC) -g -c -o $@ $< $(CFLAGS) -lm

build: $(OBJ) $(DEPS)
		$(CC) -g -o vma $^ $(CFLAGS) -lm

pack:
	zip -FSr 314CA_DulgheruElena_Tema1.zip README Makefile *.c *.h

vma: build

.PHONY: clean

run_vma:
		./vma

clean:
	rm -f *.o vma


