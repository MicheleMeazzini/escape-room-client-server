CC = gcc
CFLAGS = -Wall -c

all: server client other

server: server.o
	$(CC) -Wall server.o -o server

client: client.o 
	$(CC) -Wall client.o -o client

other: client.o 
	$(CC) -Wall client.o -o other

server.o: server.c
	$(CC) $(CFLAGS) server.c -o server.o

client.o: client.c 
	$(CC) $(CFLAGS) client.c -o client.o

clean:
	rm *.o server client

run: all
	gnome-terminal -- bash -c "./server 4242; exec bash" \
                   --tab --title="Client" -- bash -c "./client 6000; exec bash"
				   --tab --title="Other Terminal" --command="bash -c './other 6100; exec bash'"