#include "quick.c"



int main(int argc, char **argv)
{

     string filename;

     pthread_mutex_init(&rec_Q_mutex, NULL);

     sem_init(&rec_full,0,0);
     sem_init(&rec_empty,0,RECV_Q_LIMIT);

     if (argc != 2 && argc !=3) {
       printf("Arguments provided: %d\n",argc);
       fprintf(stderr, "usage: %s <port_for_server>\n", argv[0]);
       exit(1);
     }
     //portno = atoi(argv[1]);


     //  socket: create the socket
     if (argc==3)
      drop_prob=atof(argv[2]);

     setup_at(atoi(argv[1]));

     int rec_filesize, rec_remain_data;

     srand(time(0));


           /*
            * recvfrom: receive a UDP datagram from a client
            */

           filedata metadata=getfilename_and_size(clientaddr,clientlen);
           filename=metadata.filename;
           rec_filesize=metadata.filesize;


           printf("filename : %s , filesize: %d \n",filename.c_str(), rec_filesize);
           rec_remain_data = rec_filesize;


           FILE *received_file= fopen(filename.c_str(), "ab");

           init_receiver_modules(clientaddr,clientlen);

           while(1)
           {

             if(rec_remain_data>0)
             {
               rec_data_node data_received;
               if(rec_remain_data>BUFSIZE)
                 data_received=changedappRecv(BUFSIZE);
               else
                 data_received=changedappRecv(rec_remain_data);
               rec_remain_data -=data_received.bytes;


               fwrite(data_received.data,1,data_received.bytes,received_file);
               //printf("data RECIEVED: %s\n\n",data_received.data );
             }
             else
             {

               break;
             }

           }

          // sleep(2);

           close_instance();

           printf("Thread Joined\n" );

           fclose(received_file);
        //   sleep(5);



////
cout<<"ENTER HOST IP AND PORT"<<endl;
string ip,port;
cin>>ip;
cin>>port;
set_connection_to((char*)ip.c_str(),atoi((const char*)port.c_str()));

printf("Enter file name\n" );
string filename_str;
cin>>filename_str;

int filesize=send_filename_and_size(filename_str,serveraddr,serverlen);



FILE* fp = fopen(filename_str.c_str(), "rb");
int ack_seq_num=0, to_trans=(filesize/1016)+1;
int  remain_data = filesize,sent_bytes,nread,dont_read=0;


init_send_modules(filesize,serveraddr,serverlen);
init_receiver_modules(serveraddr,serverlen);


unsigned char packet_buf[2*BUFSIZE-8]={0};
while(1)
    {

        while(dont_read!=1)
        {
            memset(packet_buf,0,sizeof(packet_buf));
            nread = fread(packet_buf,1,2*BUFSIZE-8,fp);
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
}
