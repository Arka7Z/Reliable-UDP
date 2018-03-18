 #include "quick.c"



int main(int argc, char **argv)
{

      char filename[BUFSIZE];

      pthread_mutex_init(&rec_Q_mutex, NULL);

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

            if(recvfrom(sockfd, msg, sizeof(msg) , 0, (struct sockaddr *) &clientaddr, &clientlen) < 0)
              error("ERROR on receiving hello");
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

            rec_remain_data = rec_filesize;


            received_file = fopen(filename, "ab");

            init_receiver_modules(clientaddr,clientlen);

            while(1)
            {

              if(rec_remain_data>0)
              {

                rec_data_node data_received=appRecv();
                rec_remain_data -=data_received.bytes;


                fwrite(data_received.data,1,data_received.bytes,received_file);
                //printf("data RECIEVED: %s\n\n",data_received.data );
              }
              else
              {

                break;
              }

            }

            sleep(2);

            close_instance();
            
            printf("Thread Joined\n" );

            fclose(received_file);
            sleep(5);


    }
}
