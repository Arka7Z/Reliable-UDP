
#ifndef _insertion_sort_h
#define _insertion_sort_h
#include <bits/stdc++.h>
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
#include <semaphore.h>


#define BUFSIZE 1024
#define SLEEP_VAL 2
#define MSS 1024
#define MSS_DATA 1016
#define SEND_Q_LIMIT 50
#define RECV_Q_LIMIT 30

using namespace std;

typedef struct{
  struct sockaddr_in addr;
  int len;

} sock_addr_len;
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
typedef struct {
  char code[10];
  int isData;
  int ack_seq_num;
}response;
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

typedef struct {
  string filename;
  int filesize;
  int number_of_packets;
}filedata;


int sockfd; /* socket file descriptor - an ID to uniquely identify a socket by the application program */
int portno; /* port to listen on */
socklen_t clientlen,tmp_client_len; /* byte size of client's address */
struct sockaddr_in serveraddr; /* server's addr */
struct sockaddr_in clientaddr; /* client addr */
struct hostent *hostp; /* client host info */

char *hostaddrp; /* dotted decimal host addr string */
struct hostent *server;
int serverlen, tmp_server_len;
char *hostname;
int optval; /* flag value for setsockopt */
int n; /* message byte size */
double drop_prob=0.003;


int recv_seq_num,exp_seq_num=1,last_in_order=0;
rec_data_node* rec_Q_head=NULL;
int rec_Q_size=0;
pthread_mutex_t rec_Q_mutex,remain_data_mutex;
sem_t rec_full,rec_empty;

pthread_mutex_t send_Q_mutex, send_global_mutex, first_run_mutex, one_buff_present;
pthread_t rate_control_thread;
pthread_t udp_receive_thread;

pthread_mutex_t cond_mutex;
pthread_cond_t cond_var;

pthread_mutex_t send_cond_mutex;
pthread_cond_t send_cond_var;


data_node* send_Q_head=NULL;
int send_Q_size=0;
int last_ack=0,last_one_ack=-1,last_two_ack=-2;
static int alarm_fired = 0,alarm_is_on=1;
int base=0,curr=0;
double cwnd=3*MSS_DATA;
int fwnd=1000*MSS_DATA;
int bytes_running=1;
int ack_seq_num;
int SS_Thresh=1000*MSS_DATA;
sem_t send_full,send_empty;

int data_to_be_sent;

std::vector<unsigned char> recv_vec;
std::vector<unsigned char> send_vec;
int retransmitted=0;

// client
void createPacketAndSend(unsigned char* packet_buf, int bytes,int to_send_seq_num,  sock_addr_len* sockDescriptor);
void shift();
int check_for_triple_duplicate();
void mysig(int sig);
void* rate_control(void* param);
int app_send(unsigned char* packet_buf, int bytes);
response parse_packets(unsigned char* buf);
void update_window(char* code);
void init_send_modules(int to_send);
//void* udp_receive(void* param);


// server

void udp_send(unsigned char* send_buf, int sockfd, int size,sock_addr_len* sockDescriptor);
void send_ack(int ack_num, int broadcast_window,sock_addr_len* sockDescriptor);
rec_data_node appRecv();
//response parse_packets(unsigned char* buf);
void recvbuffer_handle(unsigned char* recv_buf,sock_addr_len* sockDescriptor);
void* udp_receive(void* param);
//void* udp_recieve(void* param);

// both
void init_receiver_modules();
void wait_till_data_sent();
void close_instance();
void set_connection_to(char* name, int port);

#endif
