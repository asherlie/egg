CC=gcc
CFLAGS=-O3 -Wall -Wextra -Werror -Wpedantic -pthread -DCOLOR_SUPPORT# -DDEBUG

all: egg
pp.o: pp.c pp.h
egg: egg.c pp.o

.PHONY:
clean:
	rm -rf egg *.o
