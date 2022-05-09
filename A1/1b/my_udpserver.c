/*
    Name    - Rajas Bhatt
    Roll No - 19CS30037
    Section - 1
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h> 
#include <netinet/in.h>
#include <arpa/inet.h>

#define MY_PORT 20000
#define MAX_BUFFER_SIZE 100


int main() {
    int sockfd;                             // Field descriptor
    struct sockaddr_in serv_addr, cli_addr; // Server and client socket
    socklen_t cli_size;
    char buffer[MAX_BUFFER_SIZE];           // Define a buffer

    memset(&cli_addr, 0, sizeof(cli_addr));
    memset(&serv_addr, 0, sizeof(serv_addr));
    
    /* Create UDP socket */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Error in creating socket!");
        exit(0);
    }

    /* Server socket address details */
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(MY_PORT);
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    /* Use bind() to associate socket with MY_PORT */    
    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Bind failed!");
        exit(0);
    }
    
    /* Iterative Code */

    while (1) {
        char prev = 0;            // Stores the previous character
        int cc = 0;               // Count of characters including space and dot
        int dc = 0;               // Count of characters excluding space and dot
        int wc = 0;               // Count of words
        int sc = 0;               // Count of sentences
        int file_end = 0;         // Flag whether the file has been sent completely
        int last_check = 0;       // Flag to check for extra spaces after the last sentence
        int recv_value;           // The number of characters received
        int word_after_space = 0; // Words after the last space present or not

        do {
            /* Recieve the data */
            cli_size = (socklen_t)sizeof(cli_addr);
            recv_value = recvfrom(sockfd, buffer, MAX_BUFFER_SIZE, 0, 
                        (struct sockaddr *)&cli_addr, &cli_size);

            if (recv_value < 0) {
                perror("Error in receiving data!");    // Error condition
                exit(0);
            } 
            /* 
                Iterate over all the characters received and change counters 
                If we encounter a full stop, we increment the number of sentences
                For each character received, we increment the number of characters
                For each space, we increment the number of words. 
                FILE ENDING MECHANISM: '\0' sent after the entire file signals end 
                of the file.
                If some buffer finishes with ' ', we will check for space on the next
                buffer in the beginning.
            */           
            int i = 0, j = 0;  /* Counter */   
            /* Extra spaces at the start */
            if (cc == 0 || prev == ' ' || prev == '\0' || prev == '\t' || prev == '\n') {
                while (i < recv_value) {
                    if (buffer[i] == ' ' || buffer[i] == '\n' || buffer[i] == '\t') {
                        prev = buffer[i];
                        cc++;i++;
                    }
                    else break;
                }
            }
            for (; i < recv_value; i++) {
                if (buffer[i] == '\0') {
                    if (word_after_space) wc++;
                    file_end = 1;
                    break;
                }
                if (buffer[i] == '.') {
                    // Eat all spaces after this
                    last_check = 0;
                    j = i + 1;
                    sc++;      // Increment the sentence count
                    if (word_after_space) wc++;
                    word_after_space = 0;
                    cc++;
                    prev = buffer[i];
                    while (j < recv_value) {
                        if (buffer[j] == ' ' || buffer[j] == '\n' || buffer[j] == '\t') {
                            prev = buffer[i];
                            cc++;
                            j++;
                        }
                        else break;
                    }
                    i = j - 1;
                }
                else if (buffer[i] == ' ' || buffer[i] == '\n' || buffer[i] == '\t') {
                    j = i + 1;
                    if (last_check) wc++; // Increment the word count only if this is not after the final sentence
                    cc++;
                    prev = buffer[i];
                    word_after_space = 0;
                    while (j < recv_value) {
                        if (buffer[j] == ' ' || buffer[j] == '\n' || buffer[j] == '\t') {
                            prev = buffer[i];
                            cc++;
                            j++;
                        }
                        else break;
                    }
                    i = j - 1;
                }
                else {
                    cc++;dc++;
                    prev = buffer[i];
                    last_check = 1;
                    word_after_space = 1;
                }
            }  
        } while (!file_end);

        /* Send the statistics */
        cc = htonl(cc);
        dc = htonl(dc);
        wc = htonl(wc);
        sc = htonl(sc);
        int flag;   // Report errors
        flag = sendto(sockfd, &cc, sizeof(cc), 0,
                (struct sockaddr *)&cli_addr, sizeof(cli_addr));
        if (flag < 0) {
            printf("Error in sending statistics!\n");
            exit(0);
        }
        flag = sendto(sockfd, &dc, sizeof(dc), 0,
                (struct sockaddr *)&cli_addr, sizeof(cli_addr));
        if (flag < 0) {
            printf("Error in sending statistics!\n");
            exit(0);
        }
        flag = sendto(sockfd, &wc, sizeof(wc), 0,
                (struct sockaddr *)&cli_addr, sizeof(cli_addr));
        if (flag < 0) {
            printf("Error in sending statistics!\n");
            exit(0);
        }
        flag = sendto(sockfd, &sc, sizeof(sc), 0,
                (struct sockaddr *)&cli_addr, sizeof(cli_addr));
        if (flag < 0) {
            printf("Error in sending statistics!\n");
            exit(0);
        }
    }       
    close(sockfd);
    return 0;
}