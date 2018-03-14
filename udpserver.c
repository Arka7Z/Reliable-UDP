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
#include <pthread.h>
#include <semaphore.h>
#define BUFSIZE 1024
#define RECV_Q_LIMIT 30

typedef struct rec_data_node{
  unsigned char* data;
  int bytes;
  int byte_seq_num;
  struct rec_data_node* next;
}rec_data_node;

typedef union
{
    int no;
    char bytes[4];

} int_to_char;


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

int filesize, remain_data;
char filename[BUFSIZE];
int recv_seq_num,exp_seq_num=1,last_in_order=0;
FILE *received_file;
rec_data_node* rec_Q_head=NULL;
int rec_Q_size=0;
pthread_mutex_t rec_Q_mutex,remain_data_mutex;
sem_t rec_full,rec_empty;

void error(char *msg)
{
  perror(msg);
  exit(1);
}



void udp_send(unsigned char* send_buf, int sockfd, struct sockaddr_in addr,int addr_len, int size)
{
  if(sendto(sockfd,send_buf, size, 0, &addr, addr_len)<0)
      error("ERROR in sending ACK\n");
}

void send_ack(int ack_num)
{
    char ack[BUFSIZE];
    memset(ack,'\0',sizeof(ack));
    sprintf(ack,"%s,%d","ACK",ack_num);
    //if(sendto(sockfd,ack, BUFSIZE, 0, &clientaddr, clientlen)<0)
      //  error("ERROR in sending ACK\n");
    udp_send(ack,sockfd, clientaddr, clientlen, BUFSIZE);
    printf("ACK for seq num: %d sent\n",ack_num );
}


rec_data_node appRecv()
{
  sem_wait(&rec_full);
  pthread_mutex_lock(&rec_Q_mutex);
  rec_data_node ret=*(rec_Q_head);
  rec_Q_head=rec_Q_head->next;

  pthread_mutex_unlock(&rec_Q_mutex);
  sem_post(&rec_empty);
  return ret;
}

void recvbuffer_handle(unsigned char* recv_buf)
{
  //printf("IN rec buffer handle\n" );
  int ret=-1;
  int_to_char num_char;
  char packet_buf[BUFSIZE];
  int bytes_received;

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


      //printf("just before seq if\n" );
      if(recv_seq_num==exp_seq_num )
      {

          //fwrite(recv_buf+8,1,bytes_received,received_file);

          // IF BUFFER FILLED DROP IT
          //printf("HERE HERE\n" );
          sem_wait(&rec_empty);
          // INSERT INTO BUFFER HERE
          pthread_mutex_lock(&rec_Q_mutex);
          printf("packet received with sequence number = %d and bytes received = %d \n",recv_seq_num,bytes_received);

          if (rec_Q_head==NULL)
          {
              rec_data_node* new_node=(rec_data_node*)malloc(sizeof(rec_data_node));
              new_node->data=(unsigned char*)(malloc(sizeof(char)*(BUFSIZE-8)));
              new_node->bytes=bytes_received;
              new_node->byte_seq_num=exp_seq_num;
              memcpy(new_node->data,recv_buf+8,bytes_received);
              new_node->next=NULL;
              rec_Q_head=new_node;
          }
          else
          {
              rec_data_node *cursor = rec_Q_head;
              while(cursor->next != NULL)
                      cursor = cursor->next;
              rec_data_node* new_node=(rec_data_node*)malloc(sizeof(rec_data_node));
              new_node->data=(unsigned char*)(malloc(sizeof(char)*(BUFSIZE-8)));
              new_node->byte_seq_num=exp_seq_num;
              new_node->bytes=bytes_received;
              memcpy(new_node->data,recv_buf+8,bytes_received);
              new_node->next=NULL;
              cursor->next = new_node;

          }
          rec_Q_size++;
          printf("REC q size: %d\n",rec_Q_size );
          sem_post(&rec_full);
          pthread_mutex_unlock(&rec_Q_mutex);



          send_ack(recv_seq_num+bytes_received-1);
          printf("Returned from send ack\n" );
          last_in_order=recv_seq_num+bytes_received-1;

          pthread_mutex_lock(&remain_data_mutex);
          remain_data -= bytes_received;
          pthread_mutex_unlock(&remain_data_mutex);

          printf("Decremented remain data\n" );
          exp_seq_num+=bytes_received;
      }
      else if(recv_seq_num!=exp_seq_num )
      {
          send_ack(last_in_order);
          printf("received sequence number (%d) doesn't match with expected sequence number (%d) , continuing \n",recv_seq_num,exp_seq_num);
          printf("sending ACK for sequence number %d again\n",last_in_order);
      }
      else
      {
          printf("in else, received sequence number (%d) doesn't match with expected sequence number (%d) , continuing \n",recv_seq_num,exp_seq_num);
          send_ack(last_in_order);
          printf("received sequence number (%d) doesn't match with expected sequence number (%d) , continuing \n",recv_seq_num,exp_seq_num);
          printf("sending ACK for sequence number %d again\n",last_in_order);
      }

      printf("remaining data = %d bytes \n ",remain_data);

}

void* udp_recieve(void* param)
{


  unsigned char* recv_buf;
  recv_buf=(unsigned char*)(malloc(sizeof(char)*BUFSIZE));                            // RECIEVE MESSAGE FROM CLIENT IN recv_buf
  memset(recv_buf,'\0',sizeof(recv_buf));


  while(1)
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
              int_to_char num_char;
              num_char.bytes[0]=recv_buf[0];
              num_char.bytes[1]=recv_buf[1];
              num_char.bytes[2]=recv_buf[2];
              num_char.bytes[3]=recv_buf[3];

              recv_seq_num= num_char.no;                  // RECIEVED SEQ NUM
              printf("rec seq num: %d\n",recv_seq_num );
              recvbuffer_handle(recv_buf);



  }


  printf("file received \n");

}

int main(int argc, char **argv)
{


      pthread_mutex_init(&rec_Q_mutex, NULL);
      pthread_mutex_init(&remain_data_mutex, NULL);
      sem_init(&rec_full,0,0);
      sem_init(&rec_empty,0,100);

      if (argc != 2 && argc !=3) {
        printf("Arguments provided: %d\n",argc);
        fprintf(stderr, "usage: %s <port_for_server>\n", argv[0]);
        exit(1);
      }
      portno = atoi(argv[1]);


      //  socket: create the socket
      if (argc==3)
       drop_prob=atof(argv[2]);
      sockfd = socket(AF_INET, SOCK_DGRAM, 0);
      if (sockfd < 0)
        error("ERROR opening socket");


      optval = 1;

      setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,
	         (const void *)&optval , sizeof(int));


      bzero((char *) &serveraddr, sizeof(serveraddr));
      serveraddr.sin_family = AF_INET;
      serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
      serveraddr.sin_port = htons((unsigned short)portno);


      if (bind(sockfd, (struct sockaddr *) &serveraddr,
	       sizeof(serveraddr)) < 0)
        error("ERROR on binding");


      clientlen = sizeof(clientaddr);

      srand(time(0));
      while (1)
    {

            /*
             * recvfrom: receive a UDP datagram from a client
             */

            char hello_message[3*BUFSIZE],hello[BUFSIZE];

            char msg[3*BUFSIZE];
            char code[BUFSIZE];
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

            pthread_mutex_lock(&remain_data_mutex);
            remain_data = filesize;
            pthread_mutex_unlock(&remain_data_mutex);

            received_file = fopen(filename, "ab");

            pthread_t receive_thread;
            pthread_create(&receive_thread,NULL,udp_recieve,NULL);

            while(1)
            {
              pthread_mutex_lock(&remain_data_mutex);
              if(remain_data>0)
              {
                pthread_mutex_unlock(&remain_data_mutex);
                rec_data_node data_received=appRecv();

                fwrite(data_received.data,1,data_received.bytes,received_file);
                //printf("data RECIEVED: %s\n\n",data_received.data );
              }
              else
              {

                break;
              }

            }

            pthread_cancel(receive_thread);
            printf("Thread Joined\n" );

            fclose(received_file);
            sleep(5);


    }
}
