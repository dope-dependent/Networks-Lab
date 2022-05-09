/*
    Name       - Rajas Bhatt
    Roll No    - 19CS30037
    Assignment - 2
    Problem    - (b)
    Filename   - newdnsclient.c
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h> 
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>

#define MY_PORT 8181
#define MAX_SIZE 10000
#define TCP_BUFFER 100

int main() {
    
    /* Get user entry */
    char ip[MAX_SIZE];
    printf("Enter DNS Name : ");
    scanf("%s", ip);

    if (strlen(ip) == 0) {
        printf("Please enter a valid name!\n");
        exit(1);
    }
  
    
    int sockfd;
    struct sockaddr_in serv_addr;
    socklen_t serv_size;
    /* Create new socket */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        printf("Error in creating TCP socket!\n");
        exit(1);
    }

    /* Fill server address details */
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(MY_PORT);
    inet_aton("127.0.0.1", &serv_addr.sin_addr);

    /* Connect to server */
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("Unable to connect to server!\n");
        exit(1);
    }   

    /* Send data to server */
    if (send(sockfd, ip, strlen(ip) + 1, 0) < 0) {
        printf("Unable to send DNS Name to server!\n");
        exit(1);
    }
    
    /* Set timeout to 2.0 secs */
    struct timeval time;
    time.tv_sec = 2;
    time.tv_usec = 0;

    /* Create FD_SET */
    fd_set myfd;
    FD_ZERO(&myfd);
    FD_SET(sockfd, &myfd);
    int err = select(sockfd + 1, &myfd, 0, 0, &time);
    if (err < 0) {
        printf("Error in select() call\n");
        exit(1);
    }

    /* Check if some data is recieved in 2 secs */
    int bytes;
    int done = 0;
    char recvip[TCP_BUFFER], prev = 0;
    char ipstream[TCP_BUFFER];  // Check for 0.0.0.0
    char zeroip[TCP_BUFFER];    // Store ip for 0.0.0.0
    strcpy(zeroip, "0.0.0.0");
    int countchar = 0;          // Add characters to ipstream

    printf("\n");
    if (FD_ISSET(sockfd, &myfd)) {
        while (1) {
            serv_size = sizeof(serv_addr);
            bytes = recv(sockfd, recvip, TCP_BUFFER, 0);
            if (bytes < 0) {
                printf("Error in receiving IP Addresses!\n");
                exit(1);
            }
            for (int i = 0; i < bytes; i++) {
                if (recvip[i] == '\0') {
                    if (prev == '\0') {
                        done = 1;
                        break;
                    }
                    else {
                        printf("\n");
                        ipstream[countchar++] = recvip[i];
                        if (strcmp(zeroip, ipstream) == 0) {
                            printf("Could not find any IP corresponding to hostname!\n");
                        }
                        ipstream[0] = '\0';
                    }
                }
                else {
                    ipstream[countchar++] = recvip[i];
                    printf("%c", recvip[i]);
                }
                prev = recvip[i];
            }
            if (done) break;
        }        
    }   
    else {
        /* Timeout and exit if nothing received after 2 seconds */
        printf("Client timeout!\n");
    }    

    return 0;
}