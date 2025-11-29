CC = gcc
CFLAGS = -Wall -Wextra -O2

all: server client server_poll client_poll

server: server.c
	$(CC) $(CFLAGS) -o server server.c

client: client.c
	$(CC) $(CFLAGS) -o client client.c

server_poll: server_poll.c
	$(CC) $(CFLAGS) -o server_poll server_poll.c

client_poll: client_poll.c
	$(CC) $(CFLAGS) -o client_poll client_poll.c

clean:
	rm -f server client server_poll client_poll

.PHONY: all clean
