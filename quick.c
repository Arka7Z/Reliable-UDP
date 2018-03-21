#include "quick.h"


void error(string msg)
{
    perror(msg.c_str());
    exit(0);
}

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
  sock_addr_len* sockDescriptor=(sock_addr_len*)param;
  (void) signal(SIGALRM, mysig);
  //  pthread_mutex_lock(&one_buff_present); CHANGE
  unsigned char packet_buf[BUFSIZE-8]={0};
  int first_entry=1;
  int to_send_bytes,to_send_seq_num;
  int_to_char char_num;


  //alarm_fired=0;
  while(1)
  {
        pthread_mutex_lock(&send_global_mutex);
        while((1 && !alarm_fired) || first_entry)
        {

          pthread_mutex_lock(&send_cond_mutex);


          if(send_vec.size()!=0 && min((int)cwnd,fwnd)>0)
          {

                if(curr-base<=min((int)cwnd,fwnd))
                {
                  // send the packet
                  memset(packet_buf,0,sizeof(packet_buf));
                  to_send_bytes=min(send_vec.size(),min((int)cwnd,fwnd));
                  to_send_bytes=min(to_send_bytes,BUFSIZE-8);
                  //transfer data to a packet and invoke createPacketAndSend
                  std::copy(send_vec.begin(),send_vec.begin()+to_send_bytes,packet_buf);

                  send_vec.erase(send_vec.begin(),send_vec.begin()+to_send_bytes);
                  printf("Sending packet\n" );
                  //cout<<"Passing packet to createPacketAndSend: "<<packet_buf<<endl;
                  createPacketAndSend(packet_buf,to_send_bytes,curr+1,sockDescriptor);
                  printf("Packet sent with seq num %d, size %d\n",curr+1, to_send_bytes );

                  pthread_cond_signal(&send_cond_var);
                  pthread_mutex_unlock(&send_cond_mutex);
                  first_entry=0;
                  curr+=to_send_bytes;


                }
                else
                {
                    // START TIMER
                      alarm_is_on=1;
                      alarm(SLEEP_VAL);
                      pthread_mutex_unlock(&send_cond_mutex);
                      break;
                }
            }
          else
            {

              pthread_mutex_unlock(&send_cond_mutex);
              break;
            }
        }

        pthread_mutex_unlock(&send_global_mutex);

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
              unsigned char packet[BUFSIZE];
              memset(packet,'\0',sizeof(packet));

              char_num.no=tmp_trav->byte_seq_num;
              packet[0]=char_num.bytes[0];
              packet[1]=char_num.bytes[1];
              packet[2]=char_num.bytes[2];
              packet[3]=char_num.bytes[3];

              char_num.no=tmp_trav->bytes;
              packet[4]=char_num.bytes[0];
              packet[5]=char_num.bytes[1];
              packet[6]=char_num.bytes[2];
              packet[7]=char_num.bytes[3];
              memcpy(packet+8,tmp_trav->data,BUFSIZE-8);         // packet constructed in packet_buf

              printf("Retransmitting packet with byte seq num: %d, size: %d\n",tmp_trav->byte_seq_num,tmp_trav->bytes);
              retransmitted++;
              int_to_char num_char;
              num_char.bytes[0]=packet[4];
              num_char.bytes[1]=packet[5];
              num_char.bytes[2]=packet[6];
              num_char.bytes[3]=packet[7];
              printf("Actual size sent: %d\n",num_char.no );

              if(sendto (sockfd, packet, BUFSIZE , 0,(struct sockaddr*) &sockDescriptor->addr, sockDescriptor->len) < 0 )
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
          if(fwnd==0 || fwnd<BUFSIZE-8 )
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
            if(sendto (sockfd, packet_buf, BUFSIZE , 0,(struct sockaddr*) &sockDescriptor->addr, sockDescriptor->len) < 0 )
            {
                printf("ERROR on sending packet with seq number = %d",tmp_trav->byte_seq_num);
                exit(-1);
            }
          }

          alarm_is_on=1;
          alarm(SLEEP_VAL);


          printf("UPDATING CWND AND SS_Thresh\n" );

            SS_Thresh=(int)(cwnd/2);
            cwnd=MSS_DATA;
          // change
          alarm_fired=0;
        }

        pthread_mutex_lock(&send_global_mutex);

        if(base>=data_to_be_sent)

        {
           pthread_mutex_unlock(&send_global_mutex);
           pthread_mutex_unlock(&send_Q_mutex);
           printf("curr: %d, base is%d\n",curr,base );
           printf("retransmitted: %d\n",retransmitted );
           break;
        }
        pthread_mutex_unlock(&send_global_mutex);

  }
}
void createPacketAndSend(unsigned char* packet_buf, int bytes,int to_send_seq_num,  sock_addr_len* sockDescriptor)
{
  //cout<<"recieved packet: "<<packet_buf<<" bytes: "<<bytes<<endl;
  pthread_mutex_lock(&send_Q_mutex);

  if (send_Q_head==NULL)
  {
      data_node* new_node=(data_node*)malloc(sizeof(data_node));
      new_node->data=(unsigned char*)(malloc(sizeof(char)*(BUFSIZE-8)));
      new_node->bytes=bytes;
      new_node->byte_seq_num=to_send_seq_num;
      new_node->sent=1;
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
      new_node->byte_seq_num=to_send_seq_num;
      new_node->bytes=bytes;
      new_node->sent=1;
      memcpy(new_node->data,packet_buf,bytes);
      new_node->next=NULL;
      cursor->next = new_node;
      bytes_running+=bytes;
  }
  send_Q_size++;
  //printf("send q size: %d\n",send_Q_size );


                    unsigned char packet[BUFSIZE];

                    int_to_char char_num;
                    char_num.no=to_send_seq_num;
                    packet[0]=char_num.bytes[0];
                    packet[1]=char_num.bytes[1];
                    packet[2]=char_num.bytes[2];
                    packet[3]=char_num.bytes[3];

                    char_num.no=bytes;
                    packet[4]=char_num.bytes[0];
                    packet[5]=char_num.bytes[1];
                    packet[6]=char_num.bytes[2];
                    packet[7]=char_num.bytes[3];
                    memcpy(packet+8,packet_buf,BUFSIZE-8);
                  //  printf("%s\n %d\n",packet,sockDescriptor->len );
                    //cout<<"packet is "<<packet_buf<<endl;
                    if(sendto (sockfd, packet, BUFSIZE , 0,(struct sockaddr*) &sockDescriptor->addr, sizeof(sockDescriptor->addr)) < 0 )
                    {
                        printf("ERROR on sending packet with seq number = %d",curr+1);
                        //cout<<errno<<endl;

                        exit(-1);
                    }
  pthread_mutex_unlock(&send_Q_mutex);

}

int app_send(unsigned char* packet_buf, int bytes)
{
  int ret=-1;
  pthread_mutex_lock(&send_cond_mutex);   // change
  while(bytes>SEND_Q_LIMIT*MSS_DATA-send_vec.size())
  {
    pthread_cond_wait(&send_cond_var,&send_cond_mutex);
  }
  vector<unsigned char> tmp_vec(packet_buf,packet_buf+bytes);
  send_vec.insert(send_vec.begin()+send_vec.size(),tmp_vec.begin(),tmp_vec.end());
  //cout<<"Inserting: "<<packet_buf<<endl;
  printf("Inserted in send buffer, size of send buff: %d\n",send_vec.size() );
  pthread_mutex_unlock(&send_cond_mutex);
  ret=1;
  return ret;

}

response parse_packets(unsigned char* buf)
{
  char* tokens;
  response ack;
  if(buf[0]=='A' && buf[1]=='C' && buf[2]=='K')
  {
    tokens = strtok((char*)buf,",");
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

              if((int)cwnd<MSS_DATA)
                  cwnd=MSS_DATA;
              if(!( (ack_seq_num==last_ack && last_ack==last_one_ack && last_two_ack==last_one_ack && last_two_ack==ack_seq_num)))
              {

                // NOT TRIPLE DUPLICATE ACK
                if((int)cwnd<=SS_Thresh)
                  {
                      if(ack_seq_num>base)
                        cwnd+=(MSS_DATA);
                  }
                  if((int)cwnd>=SS_Thresh)
                    if(ack_seq_num>base)
                      cwnd+=(double)(MSS_DATA*MSS_DATA)/cwnd;
              }
              else
              {
                // TRIPLE DUPLICATE ACK
                  SS_Thresh=(int)(cwnd/2);
                  cwnd=SS_Thresh;
              }

              if(send_Q_head!=NULL)
              {
                  data_node* tmp_trav=send_Q_head;
                  while(tmp_trav!=NULL )
                      {
                        if(tmp_trav->byte_seq_num<=ack_seq_num)
                          {
                            data_node* del=tmp_trav;
                            tmp_trav=tmp_trav->next;
                            send_Q_size--;
                          }
                          else
                          break;
                      }
                 if(tmp_trav!=NULL)
                      send_Q_head=tmp_trav;
              }
              pthread_mutex_unlock(&send_Q_mutex);
              if(strcmp(code,"ACK")==0 && ack_seq_num == curr)
              {
                  base=ack_seq_num;
                  alarm_is_on=0;
                  // if(cwnd>=SS_Thresh)
                  //   if(ack_seq_num>base)
                  //     cwnd+=(MSS_DATA);

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

void udp_send(unsigned char* send_buf, int sockfd, int size,sock_addr_len* sockDescriptor)
{
    //cout<<"SENDING SENDING "<<send_buf<<endl;
    if(sendto(sockfd,send_buf, size, 0,(struct sockaddr*) &sockDescriptor->addr,  sockDescriptor->len)<0)
      error("ERROR in sending ACK\n");
  //  cout<<"SENT IT"<<endl;
}

void send_ack(int ack_num, int broadcast_window,sock_addr_len* sockDescriptor)
{
    unsigned char ack[BUFSIZE];
    memset(ack,'\0',sizeof(ack));
    sprintf((char*)ack,"%s,%d,%d","ACK",ack_num,broadcast_window);
    //if(sendto(sockfd,ack, BUFSIZE, 0, &clientaddr, clientlen)<0)
      //  error("ERROR in sending ACK\n");
    udp_send(ack,sockfd,  BUFSIZE,sockDescriptor);
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

rec_data_node changedappRecv(int bytesReqd)
{
  pthread_mutex_lock(&cond_mutex);
  while(bytesReqd>recv_vec.size())
  {
    pthread_cond_wait(&cond_var,&cond_mutex);
  }
  rec_data_node* ret=(rec_data_node*)(malloc(sizeof(rec_data_node)));
  ret->data=new unsigned char[bytesReqd];
  ret->bytes=bytesReqd;
  std::copy(recv_vec.begin(),recv_vec.begin()+bytesReqd,ret->data);
  recv_vec.erase(recv_vec.begin(),recv_vec.begin()+bytesReqd);

  pthread_mutex_unlock(&cond_mutex);
  return (*ret);
}
void recvbuffer_handle(unsigned char* recv_buf,sock_addr_len* sockDescriptor)
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

      if(recv_seq_num==exp_seq_num )
      {
          pthread_mutex_lock(&cond_mutex);   // change
          if(recv_vec.size()+bytes_received<=RECV_Q_LIMIT*MSS_DATA)
          {
            printf("packet received with sequence number = %d and bytes received = %d \n",recv_seq_num,bytes_received);

            vector<unsigned char> tmp_vec(recv_buf+8,recv_buf+8+bytes_received);
            recv_vec.insert(recv_vec.begin()+recv_vec.size(),tmp_vec.begin(),tmp_vec.end());



            printf("REC q size: %d\n",recv_vec.size() );
            last_in_order=recv_seq_num+bytes_received-1;
            exp_seq_num+=bytes_received;
          }
          send_ack(recv_seq_num+bytes_received-1,RECV_Q_LIMIT*MSS_DATA-recv_vec.size(),sockDescriptor);
          pthread_cond_signal(&cond_var);

          pthread_mutex_unlock(&cond_mutex);     // change
      }
      else if(recv_seq_num!=exp_seq_num )
      {
          pthread_mutex_lock(&cond_mutex);   // change
          send_ack(last_in_order,RECV_Q_LIMIT*MSS_DATA-recv_vec.size(),sockDescriptor);
          printf("received sequence number (%d) doesn't match with expected sequence number (%d) , continuing \n",recv_seq_num,exp_seq_num);
          printf("sending ACK for sequence number %d again\n",last_in_order);
          pthread_mutex_unlock(&cond_mutex);     // change
      }
      else
      {

          pthread_mutex_lock(&cond_mutex);     // change
          printf("in else, received sequence number (%d) doesn't match with expected sequence number (%d) , continuing \n",recv_seq_num,exp_seq_num);
          send_ack(last_in_order,RECV_Q_LIMIT*MSS_DATA-recv_vec.size(),sockDescriptor);
          printf("received sequence number (%d) doesn't match with expected sequence number (%d) , continuing \n",recv_seq_num,exp_seq_num);
          printf("sending ACK for sequence number %d again\n",last_in_order);
          pthread_mutex_unlock(&cond_mutex);     // change
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

              n = recvfrom(sockfd, recv_buf, BUFSIZE, 0,(struct sockaddr*) &sockDescriptor->addr,(socklen_t*) &sockDescriptor->len);
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

                    printf("ACK NUM: %d, curr: %d, cwnd: %d, base: %d\n",ack_seq_num, curr, (int)cwnd,base);

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
                    recvbuffer_handle(recv_buf,sockDescriptor);
                  }
              }



  }


  printf("file received \n");

}
void init_send_modules(int to_send,struct sockaddr_in sockaddr, int socklen)
{
  send_vec.clear();
  (void) signal(SIGALRM, mysig);
  pthread_mutex_init(&send_cond_mutex,NULL);
  pthread_cond_init(&send_cond_var,NULL);
  pthread_mutex_init(&send_Q_mutex, NULL);           // sender
  pthread_mutex_init(&send_global_mutex, NULL);      // sender
  sem_init(&send_full,0,0);                         // sender
  sem_init(&send_empty,0,SEND_Q_LIMIT);             // sender
  data_to_be_sent=to_send;                         // sender
  sock_addr_len* sockDescriptor=(sock_addr_len*)(malloc(sizeof(sock_addr_len)));    // part of recieve
  sockDescriptor->addr=sockaddr;
  sockDescriptor->len=socklen;
  pthread_create(&rate_control_thread,NULL,rate_control,sockDescriptor);             // sender
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
  last_ack=0;
  last_one_ack=-1;
  last_two_ack=-2;
}

void init_receiver_modules(struct sockaddr_in sockaddr, int socklen)
{
  alarm_fired = 0;
  alarm_is_on=1;
  recv_vec.clear();
  exp_seq_num=1;
  last_in_order=0;
  sock_addr_len* sockDescriptor=(sock_addr_len*)(malloc(sizeof(sock_addr_len)));    // part of recieve
  sockDescriptor->addr=sockaddr;
  sockDescriptor->len=socklen;
  pthread_create(&udp_receive_thread,NULL,udp_receive,sockDescriptor);     // recieve (both have to include)
  pthread_mutex_init(&cond_mutex,NULL);
  pthread_cond_init(&cond_var,NULL);
}

void wait_till_data_sent()
{
  pthread_join(rate_control_thread,NULL);
  printf("Rate thread joined\n" );
  alarm_fired=0;
  alarm_is_on=0;
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
void setup_at(int port_number)
{
        portno=port_number;

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
}

int send_filename_and_size(string filename_str,struct sockaddr_in addr, int len)
{
  unsigned char hello_message[BUFSIZE],buf[BUFSIZE];
  int filesize;

  memset(buf,'\0',sizeof(buf));


  struct stat st;
  stat(filename_str.c_str(), &st);
  filesize = st.st_size;

  sprintf((char*)hello_message,"%s,%s,%d,%d","hello",filename_str.c_str(),filesize,(int)ceil(filesize/1016));
  memset(buf,'\0',sizeof(buf));
  printf("hello_message: %s\n",hello_message );


  init_send_modules(BUFSIZE,addr,len);
  init_receiver_modules(addr,len);
  app_send(hello_message,BUFSIZE);
  wait_till_data_sent();
  close_instance();

  printf("waiting for hello_ACK\n");
  memset(buf,'\0',sizeof(buf));

  return filesize;


}

filedata getfilename_and_size(struct sockaddr_in addr, int  len)
{
    filedata metadata;
  char hello_message[BUFSIZE],hello[BUFSIZE];

  char msg[BUFSIZE];
  char code[BUFSIZE];
  char rec_filesize_string[BUFSIZE];
  string filename;
  char ack[BUFSIZE];
  int number_of_packets;

    //

    init_receiver_modules(addr,len);
  //
  // while(1)
  // {


  bzero(msg, sizeof(msg));
  cout<<"waiting for filename"<<endl;
  //
  // if(recvfrom(sockfd, msg, sizeof(msg) , 0, (struct sockaddr *) &clientaddr,(socklen_t*) &clientlen) < 0)    // recieve
  //   error("ERROR on receiving hello");
 rec_data_node dataRec=changedappRecv(BUFSIZE);
  printf("%s\n",dataRec.data);
  memcpy(msg,dataRec.data,BUFSIZE);
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
      filename=string(tokens);
      cout<<"filename is : "<<filename<<endl;
    }
    else if(i==2)
    {
      // printf("rec_filesize decoded\n" );
      metadata.filesize=atoi(tokens);
    }
    else if(i==3)
    {
      number_of_packets=atoi(tokens);
    }

    tokens = strtok (NULL, ",");
    i++;
  }

  if(strcmp(code,"hello")!=0)
  {
    printf("\n Not a new Connection Request, restarting\n");
    // continue;
  }

  bzero(ack,sizeof(ack));
  sprintf((char*)ack,"%s","hello_ACK");
  printf(" \n sending %s  \n%d\n ",ack,tmp_client_len);

   close_instance();

  metadata.filename=filename;
  metadata.number_of_packets=number_of_packets;
  return metadata;
}
