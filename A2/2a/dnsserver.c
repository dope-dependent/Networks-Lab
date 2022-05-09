/*
    Name       - Rajas Bhatt
    Roll No    - 19CS30037
    Assignment - 2
    Problem    - (a)
    Filename   - dnsserver.c
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
    char buf[MAX_SIZE];            // Get DNS Name
    char ipbuf[MAX_IP_SIZE];       // Send IP address
    int sockfd;                   
    struct sockaddr_in servaddr, clientaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    memset(&clientaddr, 0, sizeof(clientaddr));

    socklen_t cli_len;

    /* Create UDP socket */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        printf("Error in socket creation!\n");
        exit(1);
    }

    /* Set server address parameters */
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(MY_PORT);
    servaddr.sin_addr.s_addr = INADDR_ANY;
    
    /* Bind port with server */
    if (bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        printf("Error in binding!\n");
        exit(1);
    }  

    struct hostent *host;  // Get the hostent struct containing the details of the host
    int flag;              // For errors
    /* Strategy - Keep sending IP addresses */
    /* Client automatically times out when no address recieved for two seconds */
    listen (sockfd, 5);
    while (1) {        
        cli_len = sizeof(clientaddr);
        int rx = recvfrom(sockfd, buf, MAX_SIZE, 0, (struct sockaddr *)&clientaddr, &cli_len);
        if (rx < 0) {
            printf("Error in receiving DNS Name!\n");
            exit(1);
        }
        buf[rx] = '\0';     
        host = gethostbyname(buf);
        /* Nothing found, return 0.0.0.0 */
        if (host == NULL) {     
            strcpy(ipbuf, "0.0.0.0");
            flag = sendto(sockfd, ipbuf, strlen(ipbuf) + 1, 0, (struct sockaddr *)&clientaddr, sizeof(clientaddr));
            if (flag < 0) {
                printf("Error in sending IP Address!\n");
                exit(1);
            } 
        }
        else {
            int i = 0;  // Counter
            while (host->h_addr_list[i] != NULL) {
                struct in_addr *caddr = (struct in_addr *)host->h_addr_list[i];
                const char *cip = inet_ntoa(*caddr);
                strcpy(ipbuf, cip);
                // Send to client each time an ip discovered
                flag = sendto(sockfd, ipbuf, strlen(ipbuf) + 1, 0, (struct sockaddr *)&clientaddr, sizeof(clientaddr));
                if (flag < 0) {
                    printf("Error in sending IP Address!\n");
                    exit(1);
                } 
                i++;
            }
        } 
    }   

    close(sockfd);   

    return 0;
}