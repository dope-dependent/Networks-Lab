/*
    Name       - Rajas Bhatt
    Roll No    - 19CS30037
    Assignment - 2
    Problem    - (a) and (b)
    Filename   - dnsclient.c
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
#define MAX_IP_SIZE 50

int main() {
    /* Get user input */
    char ip[MAX_SIZE];          // Send DNS Name
    char recvip[MAX_IP_SIZE];   // Get IP address
    char zeroip[MAX_IP_SIZE];   // For 0.0.0.0

    strcpy(zeroip, "0.0.0.0");


    printf("Enter DNS Name : ");
    scanf("%s", ip);

    
    if (strlen(ip) == 0) {
        printf("Enter a DNS Name!\n");
        exit(1);
    }

    /* Socket descriptors and server address */
    int sockfd;
    struct sockaddr_in servaddr;
    socklen_t sizes;
    memset(&servaddr, 0, sizeof(servaddr));

    /* Create UDP socket */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    if (sockfd < 0) {
        printf("Error in socket creation!\n");
        exit(1);
    }

    /* Set server parameters */
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(MY_PORT);
    inet_aton("127.0.0.1", &servaddr.sin_addr);

    /* Send the DNS Name to the Server */

    int bytes = sendto(sockfd, ip, strlen(ip), 0, (struct sockaddr *)&servaddr, sizeof(servaddr));
    if (bytes < 0) {
        printf("Error in sending DNS Name!\n");
        exit(1);
    } 

    /* Timeout of 2 secs*/
    struct timeval time;
    time.tv_sec = 2;
    time.tv_usec = 0;     

    int received = 0;   // Whether client has recieved something from the server
    printf("\n");
    /* Recieve Data until server stops sending IP addresses*/
    while (1) {
        /* FD_SET type */
        fd_set myfd;    
        FD_ZERO(&myfd);
        FD_SET(sockfd, &myfd);
        int err = select(sockfd + 1, &myfd, 0, 0, &time);
        if (err < 0) {
            printf("Error in select() call!\n");
            exit(1);
        }

        if (FD_ISSET(sockfd, &myfd)) {
            sizes = sizeof(servaddr);
            bytes = recvfrom(sockfd, recvip, MAX_SIZE, 0, (struct sockaddr *)&servaddr, &sizes);
            if (bytes < 0) {
                printf("Error in receiving IP Addresses!\n");
                exit(1);
            }
            recvip[bytes] = '\0';
            if (strcmp(recvip, zeroip) == 0) {
                printf("%s\n", recvip);
                printf("Could not find any IP corresponding to hostname!\n");
                exit(0);
            }
            printf("%s\n", recvip);
            received = 1;         // Something is received from the server
            FD_CLR(sockfd, &myfd);// Zero out the bit
        }   
        else {
            /* Timeout and exit if nothing received after 2 seconds */
            if (received == 0) printf("\nClient timeout!\n");
            break;
        }    
    }  
    printf("\n");
    

    /* Close the socket */
    close(sockfd);

    return 0;
}

