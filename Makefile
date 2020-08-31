# Program to compile C programs.
CC = cc

# Extra flags to give to C compiler.
CFLAGS = -std=c99 -Wall

# Extra flags for compiler when they are about to invoke the linker.
LDFLAGS = -ledit -lm

default: blisp

blisp: blisp.o mpc.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o blisp blisp.o mpc.o

mpc.o: mpc.c mpc.h
	$(CC) $(CFLAGS) -c mpc.c

blisp.o: blisp.c
	$(CC) $(CFLAGS) -c blisp.c

# Removes the executable, all object files, and all backup files.
clean:
	rm blisp *.o *~