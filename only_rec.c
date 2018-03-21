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



           close_instance();

           printf("Thread Joined\n" );

           fclose(received_file);




}
