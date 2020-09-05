# Program to compile C programs.
CC = cc

# Extra flags to give to C compiler.
CFLAGS = -std=c99 -Wall -g3

# Extra flags for compiler before invoking the linker.
LDFLAGS = -ledit -lm

default: blisp

all: blisp

blisp: blisp.o mpc.o lval.o lenv.o builtin.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o blisp blisp.o mpc.o lval.o lenv.o builtin.o

builtin.o: builtin.c builtin.h
	$(CC) $(CFLAGS) -c builtin.c

lval.o: lval.c lval.h
	$(CC) $(CFLAGS) -c lval.c

lenv.o: lenv.c lenv.h
	$(CC) $(CFLAGS) -c lenv.c

mpc.o: mpc.c mpc.h
	$(CC) $(CFLAGS) -c mpc.c

blisp.o: blisp.c blisp.h
	$(CC) $(CFLAGS) -c blisp.c

# Removes the executable, all object files, and all backup files.
# -f ignores non-existent files so no error messages show up.
clean:
	rm -f blisp *.o *~