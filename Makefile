CC=gcc
#CFLAGS=-std=c99 -Wall -Werror
CFLAGS=-std=gnu99 -Wall
#CFLAGS=-g -std=c99 -Wall -Werror -DDBUG
#CFLAGS=-g -std=gnu99 -Wall -Werror -DDBUG

mymysh : mymysh.o history.o

mymysh.o : mymysh.c history.h

history.o : history.c

clean :
	rm -f mymysh *.o core
