#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define SOCK_MRP 13
#define MAX_SIZE 100
#define RECV_WAIT_TIME 1
#define TABLE_SIZE 50
#define TIMEOUT 2
#define PROBABILITY 0.50


pthread_mutex_t lock;
pthread_t r;
pthread_t s;

int _sockfd;



typedef struct unack_msg{
    char buff[MAX_SIZE+sizeof(int)+sizeof(struct sockaddr)];
    struct sockaddr addr;
    int occupied;
    int16_t id;
    time_t time;
    int sockfd;
}unack_msg;

typedef struct unread_msg{
    struct sockaddr addr;
    socklen_t addrlen;
    char buff[MAX_SIZE];
    int ret;
    struct unread_msg* next;
    
}unread_msg;

typedef struct unread_table{
    struct unread_msg *head;
    struct unread_msg *tail;
}unread_table;

unread_table unread_pointers;
unack_msg *unack_table;


// struct 

int r_socket(int domain, int type, int protocol);
int r_bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
int r_sendto(int sockfd, const void *buf, size_t len, int flags, 
                const struct sockaddr *dest_addr, socklen_t addrlen);
int r_recvfrom(int sockfd, void *buf, size_t len, int flags,
                struct sockaddr *src_addr, socklen_t *addrlen);
int r_close(int fd);

