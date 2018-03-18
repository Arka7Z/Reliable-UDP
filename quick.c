#include "quick.h"



void shift()
{

  int tmp1,tmp2;
  tmp1=last_ack;
  tmp2=last_one_ack;
  last_ack=ack_seq_num;
  last_one_ack=tmp1;
  last_two_ack=tmp2;

}

int check_for_triple_duplicate()
{
  pthread_mutex_lock(&send_global_mutex);
  if(ack_seq_num==last_ack && last_ack==last_one_ack && last_two_ack==last_one_ack && last_two_ack==ack_seq_num)
    return 1;
  else
   return 0;
   pthread_mutex_unlock(&send_global_mutex);
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
int min(int a,int b)
{
  return a<b?a:b;
}
void* rate_control(void* param)
{
  (void) signal(SIGALRM, mysig);
  //  pthread_mutex_lock(&one_buff_present); CHANGE
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
        pthread_mutex_lock(&send_global_mutex);
        pthread_mutex_lock(&send_Q_mutex);


        while((1 && !alarm_fired) || first_entry)
        {

          if(current_to_send!=NULL)
          {

            if(curr+current_to_send->bytes<=base+min(cwnd,fwnd) && current_to_send->sent==0)
            {
              // send the packet
              memset(packet_buf,0,sizeof(packet_buf));

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

            //  pthread_mutex_unlock(&first_run_mutex); CHANGE


              first_entry=0;
              curr+=current_to_send->bytes;
              current_to_send->sent=1;
              current_to_send=current_to_send->next;

            }
            else
            {
              if(curr-base==cwnd || curr+current_to_send->bytes>base+cwnd)
              // START TIMER
              {

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

          data_node* tmp_trav=send_Q_head;

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
          if(fwnd==0)
          {
            memset(packet_buf,0,sizeof(packet_buf));

            char_num.no=0;
            packet_buf[0]=char_num.bytes[0];
            packet_buf[1]=char_num.bytes[1];
            packet_buf[2]=char_num.bytes[2];
            packet_buf[3]=char_num.bytes[3];

            char_num.no=0;
            packet_buf[4]=char_num.bytes[0];
            packet_buf[5]=char_num.bytes[1];
            packet_buf[6]=char_num.bytes[2];
            packet_buf[7]=char_num.bytes[3];
            //memcpy(packet_buf+8,tmp_trav->data,BUFSIZE-8);         // packet constructed in packet_buf

            printf("Sending probe data packet");
            int_to_char num_char;
            num_char.bytes[0]=packet_buf[4];
            num_char.bytes[1]=packet_buf[5];
            num_char.bytes[2]=packet_buf[6];
            num_char.bytes[3]=packet_buf[7];
            if(sendto (sockfd, packet_buf, BUFSIZE , 0, &serveraddr, serverlen) < 0 )
            {
                printf("ERROR on sending packet with seq number = %d",tmp_trav->byte_seq_num);
                exit(-1);
            }
          }

          alarm_is_on=1;
          alarm(SLEEP_VAL);


          printf("UPDATING CWND AND SS_Thresh\n" );
          cwnd=MSS_DATA;
          if(SS_Thresh>1016)
            SS_Thresh=SS_Thresh/2;
          // change
          alarm_fired=0;
        }

        pthread_mutex_lock(&send_global_mutex);

        if(base>=data_to_be_sent)

        {
           pthread_mutex_unlock(&send_global_mutex);
           pthread_mutex_unlock(&send_Q_mutex);
           printf("curr: %d, base is%d\n",curr,base );
           break;
        }
        pthread_mutex_unlock(&send_global_mutex);

  }
}
void createPacket(unsigned char* packet_buf, int bytes)
{
  sem_wait(&send_empty);
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
  // pthread_mutex_unlock(&one_buff_present); CHANGE
  printf("send q size: %d\n",send_Q_size );
  pthread_mutex_unlock(&send_Q_mutex);
  sem_post(&send_full);
}

int app_send(unsigned char* packet_buf, int bytes)
{
  (void) signal(SIGALRM, mysig);
  int ret=-1;
  unsigned char tmp_stor[BUFSIZE-8];

  //add it to sender buffer
  int tmp_bytes=bytes,read_bytes=0;
  if(bytes>(BUFSIZE-8))
  {
    while (tmp_bytes>0)
    {
      memset(tmp_stor,'\0',sizeof(tmp_stor));
      if (tmp_bytes>BUFSIZE-8)
      {
        memcpy(tmp_stor,packet_buf+read_bytes,BUFSIZE-8);
        createPacket(tmp_stor,BUFSIZE-8);
        tmp_bytes-=(BUFSIZE-8);
        read_bytes+=(BUFSIZE-8);
      }
      else
      {
        memcpy(tmp_stor,packet_buf+read_bytes,tmp_bytes);
        createPacket(tmp_stor,tmp_bytes);
        read_bytes+=tmp_bytes;
        tmp_bytes=0;

      }


    }
    ret=1;
  }
  else
  {
    createPacket(packet_buf,bytes);
    ret=1;
    return ret;
  }


  return ret;

}

response parse_packets(unsigned char* buf)
{
  char* tokens;
  response ack;
  if(buf[0]=='A' && buf[1]=='C' && buf[2]=='K')
  {
    tokens = strtok(buf,",");
    int i=0;
    char code[10];
    char seq_string[BUFSIZE];
    while (tokens != NULL && i<=2)
    {

        if(i==0)
        {
            strcpy(code,tokens);
        }

        else if(i==1)
        {
            strcpy(seq_string,tokens);
        }
        else if(i==2)
        {
          fwnd=atoi(tokens);
          printf("Updat fwnd= %d\n",fwnd );
        }

        tokens = strtok (NULL, ",");
        i++;
    }


    ack_seq_num = atoi(seq_string);

    strcpy(ack.code,code);
    ack.isData=0;
    return ack;
  }
  else
  {
    //printf("Data packet received in parsing\n");
    ack.isData=1;
    return ack;
  }
}


void update_window(char* code)
{
  // ack seq num param not reqd
              pthread_mutex_lock(&send_global_mutex);
              pthread_mutex_lock(&send_Q_mutex);

              if(cwnd<MSS_DATA)
                  cwnd=MSS_DATA;
              if(!( (ack_seq_num==last_ack && last_ack==last_one_ack && last_two_ack==last_one_ack && last_two_ack==ack_seq_num)))
              {


                if(cwnd<=SS_Thresh)
                  {
                      if(ack_seq_num>base)
                        cwnd+=(MSS_DATA);
                  }
              }
              else
              {
                if(SS_Thresh>1016)
                  SS_Thresh=SS_Thresh/2;
                cwnd=SS_Thresh;
              }

              if(send_Q_head!=NULL)
              {
                  data_node* tmp_trav=send_Q_head;
                  while(tmp_trav!=NULL && tmp_trav->byte_seq_num<=ack_seq_num)
                      {
                          data_node* del=tmp_trav;
                          tmp_trav=tmp_trav->next;
                          sem_post(&send_empty);
                          send_Q_size--;
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
                  if(cwnd>=SS_Thresh)
                  if(ack_seq_num>base)
                    cwnd+=(MSS_DATA);

              }
              else if(strcmp(code,"ACK")==0 && ack_seq_num < curr)
              {
                  base=ack_seq_num;
                  alarm_is_on=1;
                  alarm(SLEEP_VAL);
              }
              else
              {

                  ;
              }
              pthread_mutex_unlock(&send_global_mutex);

}


// server side

void udp_send(unsigned char* send_buf, int sockfd, struct sockaddr_in addr,int addr_len, int size)
{
  if(sendto(sockfd,send_buf, size, 0, &addr, addr_len)<0)
      error("ERROR in sending ACK\n");
}

void send_ack(int ack_num, int broadcast_window)
{
    char ack[BUFSIZE];
    memset(ack,'\0',sizeof(ack));
    sprintf(ack,"%s,%d,%d","ACK",ack_num,broadcast_window);
    //if(sendto(sockfd,ack, BUFSIZE, 0, &clientaddr, clientlen)<0)
      //  error("ERROR in sending ACK\n");
    udp_send(ack,sockfd, clientaddr, clientlen, BUFSIZE);
    printf("ACK for seq num: %d, rec_window_size: %d sent\n",ack_num, broadcast_window );
}

rec_data_node appRecv()
{
  sem_wait(&rec_full);
  pthread_mutex_lock(&rec_Q_mutex);
  rec_data_node ret=*(rec_Q_head);
  rec_Q_head=rec_Q_head->next;
  rec_Q_size--;
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


      // CHANGE
      // if(rec_remain_data<1016)
      // bytes_received=rec_remain_data;


      //printf("just before seq if\n" );
      if(recv_seq_num==exp_seq_num )
      {


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



          send_ack(recv_seq_num+bytes_received-1,(RECV_Q_LIMIT-rec_Q_size)*MSS_DATA);

          last_in_order=recv_seq_num+bytes_received-1;
          exp_seq_num+=bytes_received;
      }
      else if(recv_seq_num!=exp_seq_num )
      {
          send_ack(last_in_order,(RECV_Q_LIMIT-rec_Q_size)*MSS_DATA);
          printf("received sequence number (%d) doesn't match with expected sequence number (%d) , continuing \n",recv_seq_num,exp_seq_num);
          printf("sending ACK for sequence number %d again\n",last_in_order);
      }
      else
      {
          printf("in else, received sequence number (%d) doesn't match with expected sequence number (%d) , continuing \n",recv_seq_num,exp_seq_num);
          send_ack(last_in_order,(RECV_Q_LIMIT-rec_Q_size)*MSS_DATA);
          printf("received sequence number (%d) doesn't match with expected sequence number (%d) , continuing \n",recv_seq_num,exp_seq_num);
          printf("sending ACK for sequence number %d again\n",last_in_order);
      }



}

void* udp_receive(void* param)
{


  unsigned char* recv_buf;
  recv_buf=(unsigned char*)(malloc(sizeof(char)*BUFSIZE));                            // RECIEVE MESSAGE FROM CLIENT IN recv_buf
  memset(recv_buf,'\0',sizeof(recv_buf));
  sock_addr_len* sockDescriptor=(sock_addr_len*)param;

  while(1)
  {

              memset(recv_buf,'\0',sizeof(recv_buf));

              n = recvfrom(sockfd, recv_buf, BUFSIZE, 0, &sockDescriptor->addr, &sockDescriptor->len);
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
                      }
              }
              else
              {
                  response packet;
                  packet=parse_packets(recv_buf);
                  if(!packet.isData)
                  {

                    char code[10];
                    strcpy(code,packet.code);

                    printf("ACK NUM: %d, curr: %d, cwnd: %d, base: %d\n",ack_seq_num, curr, cwnd,base);

                    if(strcmp(code,"ACK")==0)
                    {
                      shift();
                      update_window(code);
                    }
                  }
                  else
                  {
                    double r = (((double) rand()) / (RAND_MAX));
                    printf(" R is %f\n",r);
                    if (r<= drop_prob && (exp_seq_num!=1 ||exp_seq_num!=2) )
                        {
                          printf("DROPPING PACKETS\n");
                          sleep(2);
                          continue;
                        }
                    int_to_char num_char;
                    num_char.bytes[0]=recv_buf[0];
                    num_char.bytes[1]=recv_buf[1];
                    num_char.bytes[2]=recv_buf[2];
                    num_char.bytes[3]=recv_buf[3];

                    recv_seq_num= num_char.no;                  // RECIEVED SEQ NUM
                    printf("rec seq num: %d\n",recv_seq_num );
                    recvbuffer_handle(recv_buf);
                  }
              }



  }


  printf("file received \n");

}
void init_send_modules(int to_send)
{
  (void) signal(SIGALRM, mysig);

  pthread_mutex_init(&send_Q_mutex, NULL);           // sender
  pthread_mutex_init(&send_global_mutex, NULL);      // sender
  sem_init(&send_full,0,0);                         // sender
  sem_init(&send_empty,0,SEND_Q_LIMIT);             // sender
  data_to_be_sent=to_send;                         // sender
  pthread_create(&rate_control_thread,NULL,rate_control,NULL);             // sender
  base=0;
  curr=0;
  alarm_fired = 0;
  alarm_is_on=1;
  send_Q_size=0;
  cwnd=3*MSS_DATA;
  fwnd=1000*MSS_DATA;
  SS_Thresh= 61440;
  bytes_running=1;
  send_Q_head=NULL;
}

void init_receiver_modules(struct sockaddr_in sockaddr, int socklen)
{
  sock_addr_len* sockDescriptor=(sock_addr_len*)(malloc(sizeof(sock_addr_len)));    // part of recieve
  sockDescriptor->addr=sockaddr;
  sockDescriptor->len=socklen;
  pthread_create(&udp_receive_thread,NULL,udp_receive,sockDescriptor);     // recieve (both have to include)
}

void wait_till_data_sent()
{
  pthread_join(rate_control_thread,NULL);
}
void close_instance()
{
  pthread_cancel(udp_receive_thread);
}
void set_connection_to(char* name, int port)
{
  hostname = name;
  portno = port;

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
  struct timeval tv;

  tv.tv_sec = 1;                       // TIMEOUT IN SECONDS
  tv.tv_usec = 0;                      // DEFAULT


  if(setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
      printf("Cannot Set SO_RCVTIMEO for socket\n");

}
