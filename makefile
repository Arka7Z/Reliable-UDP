all:	server client

server: udpserver.c quick.c quick.h
				gcc udpserver.c -o server -lssl -lpthread -lrt;

client: udpclient.c quick.c quick.h
				gcc udpclient.c -lssl -lcrypto -lpthread -lrt -o client;

clean:
					rm client server;
