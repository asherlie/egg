CC=gcc
CFLAGS= -Wall -Wextra -Werror -pthread

all: egg
egg: egg.c

.PHONY:
clean:
	rm -rf egg *.o
