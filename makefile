CC=gcc
CFLAGS=-O3 -Wall -Wextra -Werror -pthread -DCOLOR_SUPPORT# -DDEBUG

all: egg
egg: egg.c pp.c

.PHONY:
clean:
	rm -rf egg *.o
