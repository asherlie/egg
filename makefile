CC=gcc
CFLAGS=-Wall -Wextra -Werror -pthread -DCOLOR_SUPPORT

all: egg
egg: egg.c

.PHONY:
clean:
	rm -rf egg *.o
