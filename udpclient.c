 #include "quick.c"


int main(int argc, char **argv)
{





    int read_count=0;


    char buf[BUFSIZE];
    int retransmitted = 0;
    /* check command line arguments */
    if (argc != 3)
    {
       fprintf(stderr,"usage: %s <hostname> <port>\n", argv[0]);
       exit(0);
    }
    // hostname = argv[1];
    // portno = atoi(argv[2]);
    //
    // /* socket: create the socket */
    // sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    // if (sockfd < 0)
    //     error("ERROR opening socket");
    //
    // /* gethostbyname: get the server's DNS entry */
    // server = gethostbyname(hostname);
    // if (server == NULL)
    // {
    //     fprintf(stderr,"ERROR, no such host as %s\n", hostname);
    //     exit(-1);
    // }
    //
    // /* build the server's Internet address */
    // memset((char *) &serveraddr,0, sizeof(serveraddr));
    // serveraddr.sin_family = AF_INET;
    // bcopy((char *)server->h_addr,
	  // (char *)&serveraddr.sin_addr.s_addr, server->h_length);
    // serveraddr.sin_port = htons(portno);
    // serverlen = sizeof(serveraddr);
    // struct timeval tv;
    //
    // tv.tv_sec = 1;                       // TIMEOUT IN SECONDS
    // tv.tv_usec = 0;                      // DEFAULT
    //
    //
    // if(setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
    //     printf("Cannot Set SO_RCVTIMEO for socket\n");

    set_connection_to(argv[1],atoi(argv[2]));

    char hello_message[3*BUFSIZE],hello[BUFSIZE];

    strcpy(hello,"hello\0");



    memset(buf,'\0',sizeof(buf));

    int filesize;
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




    int ack_seq_num=0, to_trans=(filesize/1016)+1;
    while(1)
    {
        if( sendto (sockfd, hello_message, strlen(hello_message), 0,(struct sockaddr*) &serveraddr, serverlen) < 0 )
            error("ERROR in hello");
        printf("waiting for hello_ACK\n");

            memset(buf,'\0',sizeof(buf));

        if(recvfrom(sockfd, buf, sizeof(buf),0,(struct sockaddr*)&serveraddr,(socklen_t*) &serverlen) < 0)
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

    int  remain_data = filesize,sent_bytes,nread,dont_read=0;


    init_send_modules(filesize);
    init_receiver_modules(serveraddr,serverlen);


    unsigned char packet_buf[2*BUFSIZE-8]={0};
    while(1)
        {

            while(dont_read!=1)
            {
                memset(packet_buf,0,sizeof(packet_buf));
                nread = fread(packet_buf,1,2*BUFSIZE-8,fp);
                read_count++;
                if(nread<=0)
                    {
                        dont_read=1;
                        break;
                     }
                remain_data-=nread;
                app_send(packet_buf,nread);
            }

            if (remain_data==0)
            {
                printf("Finished reading the file\n");
                if (feof(fp))
                {
                    printf("File Sent\n");
                    printf("Number of retransmitted packets = %d\n",retransmitted);
                }
                if (ferror(fp))
                    printf("Error Sending file\n");
                break;
            }

        }
        fclose(fp);

        wait_till_data_sent();
        close_instance();

        close(sockfd);

        return 0;
}
