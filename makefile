CC=gcc
CFLAGS= -Wall
EXEC=client serveur
SRC= shell.c user_input.c redirections.c
OBJ=$(SRC:.c=.o)

all: iedchat iedchat_serv

iedchat : client.c sock_read_line.o client.h commod.h
	$(CC) -o iedchat client.c sock_read_line.o $(CFLAGS) -lncursesw

iedchat_serv : serveur.c sock_read_line.o serveur.h commod.h
	$(CC) -o iedchat_serv serveur.c sock_read_line.o $(CFLAGS)

sock_read_line.o : sock_read_line.c commod.h
	$(CC) -o $@ -c $< $(CFLAGS) -g

.PHONY: clean cleanall

clean:
	rm -rf *.o

cleanall: clean
	rm -rf $(EXEC)
