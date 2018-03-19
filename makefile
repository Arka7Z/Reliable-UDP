all:	server client

server: udpserver.c quick.c quick.h
				g++ udpserver.c -o server -lssl -lpthread -lrt;

client: udpclient.c quick.c quick.h
				g++ udpclient.c -lssl -lcrypto -lpthread -lrt -o client;

clean:
					rm client server;
