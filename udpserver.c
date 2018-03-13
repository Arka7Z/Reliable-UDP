/*
 * udpserver.c - A UDP echo server
 * usage: udpserver <port_for_server>
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <openssl/md5.h>
#define BUFSIZE 1024

/*
 * error - wrapper for perror
 */
void error(char *msg)
{
  perror(msg);
  exit(1);
}

typedef union
{
    int no;
    char bytes[4];

} int_to_char;



int main(int argc, char **argv)
{
      int sockfd; /* socket file descriptor - an ID to uniquely identify a socket by the application program */
      int portno; /* port to listen on */
      int clientlen; /* byte size of client's address */
      struct sockaddr_in serveraddr; /* server's addr */
      struct sockaddr_in clientaddr; /* client addr */
      struct hostent *hostp; /* client host info */
      char buf[BUFSIZE]; /* message buf */
      char *hostaddrp; /* dotted decimal host addr string */
      int optval; /* flag value for setsockopt */
      int n; /* message byte size */
      double drop_prob=0.003;
      /*
       * check command line arguments
       */
      if (argc != 2 && argc !=3) {
        printf("Arguments provided: %d\n",argc);
        fprintf(stderr, "usage: %s <port_for_server>\n", argv[0]);
        exit(1);
      }
      portno = atoi(argv[1]);

      /*
       * socket: create the socket
       */
      if (argc==3)
       drop_prob=atof(argv[2]);
      sockfd = socket(AF_INET, SOCK_DGRAM, 0);
      if (sockfd < 0)
        error("ERROR opening socket");

      /* setsockopt: Handy debugging trick that lets
       * us rerun the server immediately after we kill it;
       * otherwise we have to wait about 20 secs.
       * Eliminates "ERROR on binding: Address already in use" error.
       */
      optval = 1;

      setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,
	         (const void *)&optval , sizeof(int));

      /*
       * build the server's Internet address
       */
      bzero((char *) &serveraddr, sizeof(serveraddr));
      serveraddr.sin_family = AF_INET;
      serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
      serveraddr.sin_port = htons((unsigned short)portno);

      /*
       * bind: associate the parent socket with a port
       */
      if (bind(sockfd, (struct sockaddr *) &serveraddr,
	       sizeof(serveraddr)) < 0)
        error("ERROR on binding");

      /*
       * main loop: wait for a datagram, then echo it
       */
      clientlen = sizeof(clientaddr);

      srand(time(0));
      while (1)
    {

            /*
             * recvfrom: receive a UDP datagram from a client
             */
            ///////////////////////////////////////////////////


            char hello_message[3*BUFSIZE],hello[BUFSIZE];
            int filesize;
            char msg[3*BUFSIZE];
            char code[BUFSIZE];
            char filename[BUFSIZE];
            char filesize_string[BUFSIZE];

            char ack[BUFSIZE];

            bzero(msg, sizeof(msg));

            if(recvfrom(sockfd, msg, sizeof(msg) , 0, (struct sockaddr *) &clientaddr, &clientlen) < 0)
              error("ERROR on receiving hello");


            hostaddrp = inet_ntoa(clientaddr.sin_addr);
            if (hostaddrp == NULL)
              error("ERROR on inet_ntoa\n");

            printf("\nserver received datagram from (%s)\n", hostaddrp);

            char* tokens;
            tokens = strtok (msg,",");
            int i=0;
            while (tokens != NULL && i<=2)
            {

              if(i==0)
              {
                //printf("code decoded \t");
                strcpy(code,tokens);
              }

              else if(i==1)
              {
                // printf("filename decoded\t" );
                strcpy(filename,tokens);
              }
              else if(i==2)
              {
                // printf("filesize decoded\n" );
                strcpy(filesize_string,tokens);
              }

              tokens = strtok (NULL, ",");
              i++;
            }

            filesize=atoi(filesize_string);

            if(strcmp(code,"hello")!=0)
            {
              printf("\n Not a new Connection Request, restarting\n");
              continue;
            }

            bzero(ack,sizeof(ack));
            strcpy(ack,"hello_ACK\0");

            printf(" \n sending %s  \n ",ack);

            if(sendto (sockfd, ack, strlen(ack), 0, &clientaddr, clientlen) < 0)
              error("ERROR in sending hello_ACK");


            printf("filename : %s , filesize: %d , code: %s \n",filename, filesize, code);

            int_to_char num_char;
            char packet_buf[BUFSIZE];
            int recv_seq_num,exp_seq_num=1,last_in_order=0,remain_data = filesize;
            FILE *received_file;
            received_file = fopen(filename, "ab");

            int bytes_received;
            char recv_buf[BUFSIZE+1];                            // RECIEVE MESSAGE FROM CLIENT IN recv_buf
            memset(recv_buf,'\0',sizeof(recv_buf));


            while(remain_data>0)
            {


                      double r = (((double) rand()) / (RAND_MAX));
                      printf(" R is %f\n",r);
                        if (r<= drop_prob && (exp_seq_num!=1 ||exp_seq_num!=2) )
                            {
                            printf("DROPPING PACKETS\n");
                            sleep(2);
                            continue;
                            }
                      memset(recv_buf,'\0',sizeof(recv_buf));
                      if(recvfrom(sockfd,recv_buf , BUFSIZE , 0, &clientaddr, &clientlen)<0)
                            error("ERROR on receiving data from client \n");


                                                                  // getting the sequence number
                      num_char.bytes[0]=recv_buf[0];
                      num_char.bytes[1]=recv_buf[1];
                      num_char.bytes[2]=recv_buf[2];
                      num_char.bytes[3]=recv_buf[3];

                      recv_seq_num= num_char.no;                  // RECIEVED SEQ NUM

                                                                  // getting the number of bytes
                      num_char.bytes[0]=recv_buf[4];
                      num_char.bytes[1]=recv_buf[5];
                      num_char.bytes[2]=recv_buf[6];
                      num_char.bytes[3]=recv_buf[7];

                      bytes_received=num_char.no;                 // BYTES RECIEVED

                      if(remain_data<1016)
                        bytes_received=remain_data;

                      if(recv_seq_num==exp_seq_num )
                      {
                            printf("packet received with sequence number = %d and bytes received = %d \n",recv_seq_num,bytes_received);

                            fwrite(recv_buf+8,1,bytes_received,received_file);
                            memset(ack,'\0',sizeof(ack));
                            sprintf(ack,"%s,%d","ACK",recv_seq_num+bytes_received-1);

                            if(sendto(sockfd,ack, BUFSIZE, 0, &clientaddr, clientlen)<0)
                              error("ERROR in sending ACK\n");
                            printf(" ACK for num:%d\n",recv_seq_num+bytes_received-1);
                            last_in_order=recv_seq_num+bytes_received-1;
                            remain_data -= bytes_received;
                            exp_seq_num+=bytes_received;

                      }
                      else if(recv_seq_num!=exp_seq_num )
                      {
                            memset(ack,'\0',sizeof(ack));
                            sprintf(ack,"%s,%d","ACK",last_in_order);

                            if(sendto(sockfd,ack, BUFSIZE, 0, &clientaddr, clientlen)<0)
                              error("ERROR in sending ACK\n");


                            printf("received sequence number (%d) doesn't match with expected sequence number (%d) , continuing \n",recv_seq_num,exp_seq_num);
                            printf("sending ACK for sequence number %d again\n",last_in_order);
                            continue;
                      }
                      else
                      {
                            printf("in else, received sequence number (%d) doesn't match with expected sequence number (%d) , continuing \n",recv_seq_num,exp_seq_num);
                            memset(ack,'\0',sizeof(ack));
                            sprintf(ack,"%s,%d","ACK",last_in_order);

                            if(sendto(sockfd,ack, BUFSIZE, 0, &clientaddr, clientlen)<0)
                              error("ERROR in sending ACK\n");


                            printf("received sequence number (%d) doesn't match with expected sequence number (%d) , continuing \n",recv_seq_num,exp_seq_num);
                            printf("sending ACK for sequence number %d again\n",last_in_order);
                            continue;
                            continue;
                      }

                      printf("remaining data = %d bytes \n ",remain_data);

            }

            fclose(received_file);
            printf("file received \n");



            int fd=open(filename, "rb");                                                // COMPUTING MD5 CHECKSUM

            close(fd);


            sleep(5);


    }
}
