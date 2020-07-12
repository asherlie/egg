CC=gcc
CFLAGS=-O3 -Wall -Wextra -Werror -pthread -DCOLOR_SUPPORT

all: egg
egg: egg.c

.PHONY:
clean:
	rm -rf egg *.o
