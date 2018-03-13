/*
 * udpclient.c - A simple UDP client
 * usage: udpclient <host> <port>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <openssl/md5.h>
#include <pthread.h>

#define BUFSIZE 1024
#define SLEEP_VAL 2
#define MSS 1024
#define MSS_DATA 1016


 // Structures definition
 typedef struct data_node
 {
     unsigned char* data;
     int bytes;
     int byte_seq_num;
     int sent;
     struct data_node* next;
 }data_node;

 typedef struct node
 {
     unsigned char* data;
     int seq_num;
     struct node* next;
 }node;

 typedef union
 {
     int no;
     char bytes[4];

 } int_to_char;

 // Socket Variables
 int sockfd, portno, n;
 int serverlen;
 struct sockaddr_in serveraddr;
 struct hostent *server;
 char *hostname;

 // Globals for sharing among threads

 pthread_mutex_t send_Q_mutex, send_global_mutex, first_run_mutex, one_buff_present;
 data_node* send_Q_head=NULL;
 int send_Q_size=0;
 int last_ack=0,last_one_ack=-1,last_two_ack=-2;
 static int alarm_fired = 0,alarm_is_on=1;
 int base=0,curr=0;
 int cwnd=3*MSS_DATA;
 int bytes_running=1;

void error(char *msg)
{
    perror(msg);
    exit(0);
}


void mysig(int sig)
{
    pid_t pid;
    printf("*******TIMEOUT********* \n");
    if (sig == SIGALRM && alarm_is_on)
    {
        alarm_fired = 1;                    // FIRE ALARM
    }
    else if(sig==SIGALRM)
        ;
    else
        ;
    signal(SIGALRM,mysig);
}

void* rate_control(void* param)
{
  (void) signal(SIGALRM, mysig);
    pthread_mutex_lock(&one_buff_present);
  unsigned char packet_buf[BUFSIZE]={0};
  int first_entry=1;
  //printf("In rate control\n");
  int_to_char char_num;


  //alarm_fired=0;
  while(1)
  {

    pthread_mutex_lock(&send_Q_mutex);
    data_node* current_to_send=send_Q_head;
    pthread_mutex_unlock(&send_Q_mutex);

        pthread_mutex_lock(&send_Q_mutex);
        pthread_mutex_lock(&send_global_mutex);
      //  printf("Lock acquired, alarm_fired: %d, alarm_is_on: %d\n",alarm_fired, alarm_is_on );
        while((1 && !alarm_fired) || first_entry)
        {
          //printf("Inside alarm_fired check\n" );
          if(current_to_send==NULL)
            printf("NULL NULLL\n" );
          if(current_to_send!=NULL)
          {

            if(curr+current_to_send->bytes<=base+cwnd && current_to_send->sent==0)
            {
              // send the packet
              memset(packet_buf,0,sizeof(packet_buf));
                //printf("INSIDE\n" );
              char_num.no=curr+1;
              packet_buf[0]=char_num.bytes[0];
              packet_buf[1]=char_num.bytes[1];
              packet_buf[2]=char_num.bytes[2];
              packet_buf[3]=char_num.bytes[3];

              char_num.no=current_to_send->bytes;
              packet_buf[4]=char_num.bytes[0];
              packet_buf[5]=char_num.bytes[1];
              packet_buf[6]=char_num.bytes[2];
              packet_buf[7]=char_num.bytes[3];
              memcpy(packet_buf+8,current_to_send->data,BUFSIZE-8);         // packet constructed in packet_buf

              if(sendto (sockfd, packet_buf, BUFSIZE , 0, &serveraddr, serverlen) < 0 )
              {
                  printf("ERROR on sending packet with seq number = %d",curr+1);
                  exit(-1);
              }
              printf("Packet sent with seq num %d, size %d\n",curr+1, char_num.no );
              int_to_char num_char;
              num_char.bytes[0]=packet_buf[4];
              num_char.bytes[1]=packet_buf[5];
              num_char.bytes[2]=packet_buf[6];
              num_char.bytes[3]=packet_buf[7];
              printf("Actual size sent: %d\n",num_char.no );
              pthread_mutex_unlock(&first_run_mutex);
              first_entry=0;
              curr+=current_to_send->bytes;
              current_to_send->sent=1;
              current_to_send=current_to_send->next;

            }
            else
            {
              if(curr-base==cwnd || curr+current_to_send->bytes>base+cwnd)
              // START TIMER
              {//printf("starting timer\n" );

              alarm_is_on=1;
              alarm(SLEEP_VAL);
              }
              break;
            }
          }
          else
            {

              break;
            }
        }
        pthread_mutex_unlock(&send_global_mutex);
        pthread_mutex_unlock(&send_Q_mutex);

        // IN CASE OF TIMEOUT
        //printf("checking for timeout: %d\n",alarm_fired );
        if(alarm_fired)
        {
          pthread_mutex_lock(&send_Q_mutex);
          //printf("ACQUIRED LOCK IN RETRANSMIT\n");
          data_node* tmp_trav=send_Q_head;
          if(tmp_trav==NULL)
            printf("NULL NULL\n" );
          while(tmp_trav!=NULL)
          {
            //printf("seq num is %d, base:%d, curr: %d\n",tmp_trav->byte_seq_num ,base, curr);
            if(tmp_trav->byte_seq_num<curr)
            {
              //send this packet

              memset(packet_buf,0,sizeof(packet_buf));

              char_num.no=tmp_trav->byte_seq_num;
              packet_buf[0]=char_num.bytes[0];
              packet_buf[1]=char_num.bytes[1];
              packet_buf[2]=char_num.bytes[2];
              packet_buf[3]=char_num.bytes[3];

              char_num.no=tmp_trav->bytes;
              packet_buf[4]=char_num.bytes[0];
              packet_buf[5]=char_num.bytes[1];
              packet_buf[6]=char_num.bytes[2];
              packet_buf[7]=char_num.bytes[3];
              memcpy(packet_buf+8,tmp_trav->data,BUFSIZE-8);         // packet constructed in packet_buf

              printf("Retransmitting packet with byte seq num: %d, size: %d\n",tmp_trav->byte_seq_num,tmp_trav->bytes);
              int_to_char num_char;
              num_char.bytes[0]=packet_buf[4];
              num_char.bytes[1]=packet_buf[5];
              num_char.bytes[2]=packet_buf[6];
              num_char.bytes[3]=packet_buf[7];
              printf("Actual size sent: %d\n",num_char.no );
              if(sendto (sockfd, packet_buf, BUFSIZE , 0, &serveraddr, serverlen) < 0 )
              {
                  printf("ERROR on sending packet with seq number = %d",tmp_trav->byte_seq_num);
                  exit(-1);
              }
              tmp_trav=tmp_trav->next;
            }
            else
              break;
          }
          pthread_mutex_unlock(&send_Q_mutex);

          alarm_is_on=1;
          alarm(SLEEP_VAL);

          cwnd=MSS_DATA;

          // change
          alarm_fired=0;
        }
  }
}

int app_send(unsigned char* packet_buf, int bytes)
{
  (void) signal(SIGALRM, mysig);
  int ret=-1;

  //add it to sender buffer
  pthread_mutex_lock(&send_Q_mutex);
  if (send_Q_head==NULL)
  {
      data_node* new_node=(data_node*)malloc(sizeof(data_node));
      new_node->data=(unsigned char*)(malloc(sizeof(char)*(BUFSIZE-8)));
      new_node->bytes=bytes;
      new_node->byte_seq_num=bytes_running;
      new_node->sent=0;
      memcpy(new_node->data,packet_buf,bytes);
      new_node->next=NULL;
      send_Q_head=new_node;
      bytes_running+=bytes;
  }
  else
  {
      data_node *cursor = send_Q_head;
      while(cursor->next != NULL)
              cursor = cursor->next;
      data_node* new_node=(data_node*)malloc(sizeof(data_node));
      new_node->data=(unsigned char*)(malloc(sizeof(char)*(BUFSIZE-8)));
      new_node->byte_seq_num=bytes_running;
      new_node->bytes=bytes;
      new_node->sent=0;
      memcpy(new_node->data,packet_buf,bytes);
      new_node->next=NULL;
      cursor->next = new_node;
      bytes_running+=bytes;
  }
  send_Q_size++;
  pthread_mutex_unlock(&one_buff_present);
  ret=1;
  printf("send q size: %d\n",send_Q_size );
  pthread_mutex_unlock(&send_Q_mutex);

  return ret;

}

void* receive_ack(void* param)
{
  pthread_mutex_lock(&first_run_mutex);
  (void) signal(SIGALRM, mysig);
  unsigned char buf[BUFSIZE];
  int ack_seq_num;
  while(1)
  {

        memset(buf,'\0',sizeof(buf));
        printf("Listening for ack\n");
        n = recvfrom(sockfd, buf, sizeof(buf), 0, &serveraddr, &serverlen);


        if(n < 0)
        {
            if (errno == EWOULDBLOCK)
            {
                fprintf(stderr, "socket timeout\n");
                alarm_fired=1;
                sleep(2);
                continue;
            }
            else
            {
                printf("ERROR in ACK received error at seq_number ");
                //exit(-1);
                ;
            }
        }
        else
        {

            char* tokens;
            tokens = strtok(buf,",");
            int i=0;
            char code[10];
            char seq_string[BUFSIZE];
            while (tokens != NULL && i<=1)
            {

                if(i==0)
                {
                    strcpy(code,tokens);
                }

                else if(i==1)
                {
                    strcpy(seq_string,tokens);
                }

                tokens = strtok (NULL, ",");
                i++;
            }


            ack_seq_num = atoi(seq_string);

            if (cwnd==0)
                cwnd=1;
            printf("ACK NUM: %d, curr: %d, cwnd: %d, base: %d\n",ack_seq_num, curr, cwnd,base);



            if(cwnd<MSS_DATA)
                cwnd=MSS_DATA;


            pthread_mutex_lock(&send_Q_mutex);
            pthread_mutex_lock(&send_global_mutex);

            if(ack_seq_num>base)
                    cwnd+=(MSS_DATA);
            if(send_Q_head!=NULL)
            {
                data_node* tmp_trav=send_Q_head;
                while(tmp_trav!=NULL && tmp_trav->byte_seq_num<=ack_seq_num)
                    {
                        data_node* del=tmp_trav;
                        tmp_trav=tmp_trav->next;
                        //free(del);
                    }
               if(tmp_trav!=NULL)
                    send_Q_head=tmp_trav;
            }
            pthread_mutex_unlock(&send_Q_mutex);
            if(strcmp(code,"ACK")==0 && ack_seq_num == curr)
            {
                base=ack_seq_num;
                alarm_is_on=0;

            }
            else if(strcmp(code,"ACK")==0 && ack_seq_num < curr)
            {
                base=ack_seq_num;
                alarm_is_on=1;
                alarm(SLEEP_VAL);
            }
            else
            {
                pthread_mutex_unlock(&send_global_mutex);
                continue;
            }
            pthread_mutex_unlock(&send_global_mutex);

        }

}

}

int main(int argc, char **argv)
{

      (void) signal(SIGALRM, mysig);

    pthread_mutex_init(&send_Q_mutex, NULL);
    pthread_mutex_init(&send_global_mutex, NULL);
    pthread_mutex_init(&first_run_mutex, NULL);
    pthread_mutex_init(&one_buff_present, NULL);
    pthread_t rate_control_thread;
    pthread_t receive_ack_thread;
    pthread_mutex_lock(&first_run_mutex);
    pthread_mutex_lock(&one_buff_present);


    int read_count=0;


    char buf[BUFSIZE];
    int retransmitted = 0;
    /* check command line arguments */
    if (argc != 3)
    {
       fprintf(stderr,"usage: %s <hostname> <port>\n", argv[0]);
       exit(0);
    }
    hostname = argv[1];
    portno = atoi(argv[2]);

    /* socket: create the socket */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");

    /* gethostbyname: get the server's DNS entry */
    server = gethostbyname(hostname);
    if (server == NULL)
    {
        fprintf(stderr,"ERROR, no such host as %s\n", hostname);
        exit(-1);
    }

    /* build the server's Internet address */
    memset((char *) &serveraddr,0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
	  (char *)&serveraddr.sin_addr.s_addr, server->h_length);
    serveraddr.sin_port = htons(portno);
    serverlen = sizeof(serveraddr);


    char hello_message[3*BUFSIZE],hello[BUFSIZE];
    int filesize;
    strcpy(hello,"hello\0");



    memset(buf,'\0',sizeof(buf));


    printf("Please enter the file name: ");
    scanf("%s",buf);
    char filename[1000];
    strcpy(filename,buf);
    struct stat st;
    stat(buf, &st);
    filesize = st.st_size;
    FILE* fp = fopen(buf, "rb");
    sprintf(hello_message,"%s,%s,%d",hello,buf,filesize);
    memset(buf,'\0',sizeof(buf));


    struct timeval tv;

    tv.tv_sec = 1;                       // TIMEOUT IN SECONDS
    tv.tv_usec = 0;                      // DEFAULT


    if(setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
        printf("Cannot Set SO_RCVTIMEO for socket\n");

    int ack_seq_num=0, to_trans=(filesize/1016)+1;
    while(1)
    {
        if( sendto (sockfd, hello_message, strlen(hello_message), 0, &serveraddr, serverlen) < 0 )
            error("ERROR in hello");
        printf("waiting for hello_ACK\n");
        if(recvfrom(sockfd, buf, sizeof(buf)-1,0,&serveraddr, &serverlen) < 0)
        {
             error("ERROR in hello ACK");
        }
        printf("\n");

        if(strcmp(buf,"hello_ACK") == 0)
        {
            printf("\n hello ACK received \n" );
            break;
        }
    }

    node* head=NULL;
    int  remain_data = filesize,sent_bytes;
    int i=0;

    int seq_number = 1;


    int_to_char char_num;

    int nread ;
    int front=-1,rear=0;

    int baseptr=0,currptr=0;
    int dont_read=0;

    unsigned char packet_buf[BUFSIZE-8]={0};
    pthread_create(&rate_control_thread,NULL,rate_control,NULL);
    pthread_create(&receive_ack_thread,NULL,receive_ack,NULL);
    while(1)
        {

            while(dont_read!=1)
            {
                memset(packet_buf,0,sizeof(packet_buf));
                nread = fread(packet_buf,1,BUFSIZE-8,fp);
                read_count++;
                if(nread<=0)
                    {
                        dont_read=1;
                        break;
                     }
                remain_data-=nread;

                if(nread > 0)                                // SENDING THE DATA
                {
                  app_send(packet_buf,nread);
                }

            }

            // LISTENING FOR THE ACK. Format: %s,%d ACK, Ack number
            sleep(2);


            if (remain_data==0)
            {
                printf("Finished reading the file\n");
                sleep(4);
                if (feof(fp))
                {
                    printf("File Sent\n");
                    printf("Number of retransmitted packets = %d\n",retransmitted);
                }
                if (ferror(fp))
                    printf("Error Sending file\n");
                if(ack_seq_num==to_trans)
                break;
            }

        }
        fclose(fp);

        pthread_cancel(rate_control_thread);
        pthread_cancel(receive_ack_thread);
        close(sockfd);

        return 0;
}
