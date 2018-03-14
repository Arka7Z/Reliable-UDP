all:	server client
server: udpserver.c;
	gcc udpserver.c -o server -lssl -lpthread -lrt;
client:	udpclient.c;
	gcc udpclient.c -o client -lssl -lcrypto -lpthread -lrt;
clean:	;
	rm client server;
