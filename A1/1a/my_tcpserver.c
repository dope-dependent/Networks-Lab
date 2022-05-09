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

#define MAX_BUFFER_SIZE 100       // Maximum size of the buffer in bytes
#define MY_PORT 20000             // Port Number     
#define QUEUE_CONNECTIONS 1       // Max Number of Connections on the queue

int main() {
    int sockfd, newsockfd;                   // Socket descriptors   
    int cli_len;                             // sizeof(cli_addr)
    struct sockaddr_in serv_addr, cli_addr;  // Address structs for server and client

    char buffer[MAX_BUFFER_SIZE];            // Buffer array

    /* Create the socket */
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Unable to create socket!\n");
        exit(0);
    }

    /* Set server parameters */
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(MY_PORT);
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    /* Associate the server to MY_PORT using bind() */
    if ((bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)) {
        perror("Unable to bind local address!\n");
        exit(0);
    }

    /* listen() call, allows atmost 5 parallel connections */
    listen(sockfd, QUEUE_CONNECTIONS);
    
    while (1) {

        cli_len = sizeof(cli_addr);
        /* Use accept() to get the pending connection */
        if ((newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &cli_len)) < 0) {
            perror("Accept fault!\n");
            exit(0);
        }
        
        int cc = 0;               // Count of characters including space and dot
        int dc = 0;               // Count of characters excluding space and dot
        int wc = 0;               // Count of words
        int sc = 0;               // Count of sentences
        int file_end = 0;         // Flag whether the file has been sent completely
        char prev = 0;            // Previous character
        int recv_value;           // The number of characters received
        int last_check = 0;       // Extra spaces after the final sentences
        int word_after_space = 0; // Words after the last space
        /* Receive the text */

        do {
            recv_value = recv(newsockfd, buffer, MAX_BUFFER_SIZE, 0);
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
                    word_after_space = 0;   // Word count has been incremented
                    cc++;
                    prev = buffer[i];
                    // Eat up additional spaces
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
                    // Eat up additional spaces
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
        int flag;
        flag = send(newsockfd, &cc, sizeof(cc), 0);
        if (flag < 0) {
            printf("Error in sending statistics!\n");
            exit(0);
        }
        flag = send(newsockfd, &dc, sizeof(dc), 0);
        if (flag < 0) {
            printf("Error in sending statistics!\n");
            exit(0);
        }
        flag = send(newsockfd, &wc, sizeof(wc), 0);
        if (flag < 0) {
            printf("Error in sending statistics!\n");
            exit(0);
        }
        flag = send(newsockfd, &sc, sizeof(sc), 0);
        if (flag < 0) {
            printf("Error in sending statistics!\n");
            exit(0);
        }

        /* When the client closes the connection (recv returns 0 value), print it on the Server */
        if (recv(newsockfd, buffer, MAX_BUFFER_SIZE, 0) == 0) {
            // printf("Client Connection Closed!\n");
        }
        close(newsockfd);
    }
 

    return 0;
}