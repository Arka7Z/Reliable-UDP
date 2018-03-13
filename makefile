all:	server client
server: udpserver.c;
	gcc udpserver.c -o server -lssl -lpthread;
client:	udpclient.c;
	gcc udpclient.c -o client -lssl -lcrypto -lpthread;
clean:	;
	rm client server;
