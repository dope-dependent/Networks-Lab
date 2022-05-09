/*
    Name       - Rajas Bhatt
    Roll No    - 19CS30037
    Assignment - 2
    Problem    - (b)
    Filename   - new_dnsserver.c
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
    int sockfd1, sockfd2, newsockfd;
    struct sockaddr_in serv_addr, cli_addr; 
    socklen_t cli_len;
    /* Open TCP Socket */
    sockfd1 = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd1 < 0) {
        printf("Error in creating TCP socket!\n");
        exit(1);
    }
    /* Open UDP socket */
    sockfd2 = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd2 < 0) {
        printf("Error in creating UDP socket!\n");
        exit(1);
    }

    /* Bind TCP and UDP sockets */
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(MY_PORT);

    if (bind(sockfd1, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("Error in Binding TCP Socket!\n");
        exit(1);
    }
    if (bind(sockfd2, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("Error in Binding UDP Socket!\n");
        exit(1);
    }
    listen (sockfd1, 5);
    listen (sockfd2, 5);
    /* Connect */   
    while (1) {
        /* Select to select the client */
        fd_set myfd;
        FD_ZERO(&myfd); 
        FD_SET(sockfd1, &myfd);
        FD_SET(sockfd2, &myfd);
        int maxval = (sockfd1 > sockfd2 ? sockfd1 : sockfd2) + 1;
        int err = select(maxval, &myfd, 0, 0, NULL);
        if (err < 0) {
            printf("Error in the select() call!\n");
            exit(1);
        }

        if (FD_ISSET(sockfd1, &myfd)) {
            /* TCP connection */           
            cli_len = sizeof(cli_addr);
            newsockfd = accept(sockfd1, (struct sockaddr *)&cli_addr, &cli_len);
            if (newsockfd < 0) {
                printf("Accept Error!\n");
                exit(1);
            }
            int x = fork(); 
            if (x == 0) {
                /* Variables required */
                struct hostent *host;
                char buff[MAX_SIZE], storage[MAX_SIZE];
                int rx, flag;
                /* Get the data using recv() */
                buff[0] = '\0';
                while (1) {
                    rx = recv(newsockfd, buff, MAX_SIZE, 0);
                    if (rx < 0) {
                        printf("Error in receiving DNS Name!\n");
                        exit(1);
                    }
                    if (buff[rx] == '\0') {
                        strcpy(storage, buff);
                        break;
                    }
                    else {
                        buff[rx] = '\0';
                        strcpy(storage, buff);
                    }
                }

                host = gethostbyname(storage);
                
                if (host == NULL) {     
                    strcpy(storage, "0.0.0.0");
                    storage[strlen(storage) + 1] = '\0';
                    flag = send(newsockfd, storage, strlen(storage) + 2, 0);  
                    if (flag < 0) {
                        printf("Error in sending IP Address!\n");
                        exit(1);
                    }   
                }
                else {
                    int i = 0;
                    while (host->h_addr_list[i] != NULL) {
                        struct in_addr *caddr = (struct in_addr *)host->h_addr_list[i];
                        const char *cip = inet_ntoa(*caddr);
                        strcpy(storage, cip);
                        flag = send(newsockfd, storage, strlen(storage) + 1, 0);  
                        if (flag < 0) {
                            printf("Error in sending IP Address!\n");
                            exit(1);
                        } 
                        i++;
                    }
                    storage[0] = '\0';
                    flag = send(newsockfd, storage, 1, 0);  
                    if (flag < 0) {
                        printf("Error in sending IP Address!\n");
                        exit(1);
                    } 
                }
                close(newsockfd);
            }
            else {
                /* The parent process branches out */
                close(newsockfd);
            }
        }
        if (FD_ISSET(sockfd2, &myfd)) {
            /* UDP */
            struct hostent *host;
            char ipbuf[MAX_IP_SIZE];
            char buf[MAX_SIZE];
            int flag;
        
            cli_len = sizeof(cli_addr);
            int rx = recvfrom(sockfd2, buf, MAX_SIZE, 0, (struct sockaddr *)&cli_addr, &cli_len);
            if (rx < 0) {
                printf("Error in receiving DNS Name!\n");
                exit(1);
            }
            buf[rx] = '\0';        
            host = gethostbyname(buf);
            /* Nothing found, return 0.0.0.0 */
            if (host == NULL) {     
                strcpy(ipbuf, "0.0.0.0");
                flag = sendto(sockfd2, ipbuf, strlen(ipbuf) + 1, 0, (struct sockaddr *)&cli_addr, sizeof(cli_addr));
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
                    flag = sendto(sockfd2, ipbuf, strlen(ipbuf) + 1, 0, (struct sockaddr *)&cli_addr, sizeof(cli_addr));
                    if (flag < 0) {
                        printf("Error in sending IP Address!\n");
                        exit(1);
                    } 
                    i++;
                }
            } 
        }
    }   

    close(sockfd1);  
    close(sockfd2);

    return 0;
}