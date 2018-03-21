all:	server client rec

server: udpserver.c quick.c quick.h
				g++ udpserver.c -o server -lssl -lpthread -lrt;

client: udpclient.c quick.c quick.h
				g++ udpclient.c -lssl -lcrypto -lpthread -lrt -o client;
rec: 		only_rec.c quick.c quick.h
				g++ only_rec.c -lssl -lcrypto -lpthread -lrt -o rec;

clean:
					rm client server rec;
