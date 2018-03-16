 #include "quick.c"



int main(int argc, char **argv)
{

      char filename[BUFSIZE];
      pthread_mutex_init(&rec_Q_mutex, NULL);
      pthread_mutex_init(&remain_data_mutex, NULL);
      sem_init(&rec_full,0,0);
      sem_init(&rec_empty,0,RECV_Q_LIMIT);

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

      int rec_filesize, rec_remain_data;

      srand(time(0));
      while (1)
    {

            /*
             * recvfrom: receive a UDP datagram from a client
             */

            char hello_message[3*BUFSIZE],hello[BUFSIZE];

            char msg[3*BUFSIZE];
            char code[BUFSIZE];
            char rec_filesize_string[BUFSIZE];

            char ack[BUFSIZE];

            bzero(msg, sizeof(msg));
            printf("66\n" );

            if(recvfrom(sockfd, msg, sizeof(msg) , 0, (struct sockaddr *) &clientaddr, &clientlen) < 0)
              error("ERROR on receiving hello");
            printf("70\n" );
            //
            // hostaddrp = inet_ntoa(clientaddr.sin_addr);
            // if (hostaddrp == NULL)
            //   error("ERROR on inet_ntoa\n");
            //
            // printf("\nserver received datagram from (%s)\n", hostaddrp);
            //

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
                // printf("rec_filesize decoded\n" );
                strcpy(rec_filesize_string,tokens);
              }

              tokens = strtok (NULL, ",");
              i++;
            }

            rec_filesize=atoi(rec_filesize_string);

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


            printf("filename : %s , filesize: %d , code: %s \n",filename, rec_filesize, code);

            pthread_mutex_lock(&remain_data_mutex);
            rec_remain_data = rec_filesize;
            pthread_mutex_unlock(&remain_data_mutex);

            received_file = fopen(filename, "ab");
            sock_addr_len* sockDescriptor=(sock_addr_len*)(malloc(sizeof(sock_addr_len)));
            sockDescriptor->addr=clientaddr;
            sockDescriptor->len=clientlen;

            pthread_t receive_thread;
            pthread_create(&receive_thread,NULL,udp_receive,sockDescriptor);

            while(1)
            {
              //pthread_mutex_lock(&remain_data_mutex);
              if(rec_remain_data>0)
              {

                rec_data_node data_received=appRecv();
                rec_remain_data -=data_received.bytes;
                //pthread_mutex_unlock(&remain_data_mutex);

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
