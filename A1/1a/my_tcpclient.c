/*
    Name    - Rajas Bhatt
    Roll No - 19CS30037
    Section - 1
*/

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define MAX_BUFFER_SIZE 100         // Maximum size of the buffer in bytes
#define CHUNK_SIZE 8                // Chunk size known only by client
#define MY_PORT 20000               // Fixed port size used

int main(int argc, char *argv[]) {
    int sockfd;                     // Socket Descriptor
    struct sockaddr_in serv_addr;   // Socket Address struct

    /* Set parameters */
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(MY_PORT);
    inet_aton("127.0.0.1", &serv_addr.sin_addr);

    /* Basic error handling */
    if (argc != 2) {
        printf("Please enter ONE argument!\n");
        exit(0);
    }

    char *filename = argv[1];               // Name of the text file
    int fileD = open(filename, O_RDONLY);    // Get the file descriptor
    
    /* Create the buffer array */
    char buffer[MAX_BUFFER_SIZE];       

    /* File is not found when descriptor < 0 */
    if (fileD < 0) {
        printf("File not found!\n");
        exit(0);
    }

    /* Create client socket */
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Error in creating socket!");
        exit(0);
    }  

    /* Connect call to connect to the local server */
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Unable to connect to server!\n");
        exit(0);
    }

    /* 
        Read the specified number of characters and then send()
        Note, that the number of characters read may be less than the chunk size
        If no, characters are read, we have reached the end of the file
    */
    int chars_read, flag;
    while (1) {
        chars_read = read(fileD, buffer, CHUNK_SIZE);
        if (chars_read < 0) {
            printf("Error while reading the file!\n");
            exit(0);
        }
        if (chars_read == 0) {
            buffer[0] = '\0';
            flag = send(sockfd, buffer, 1, 0);    // Send an extra '\0' at the end
            if (flag < 0) {
                printf("Error while sending file to server!\n");
                exit(0);
            }
            break;
        }
        flag = send(sockfd, buffer, chars_read, 0);
        if (flag < 0) {
            printf("Error while sending file to server!\n");
            exit(0);
        }
    }

    /* Recieve the counts of characters, words and sentences using recv() */
    int cc, wc, sc, dc;
    flag = recv(sockfd, &cc, sizeof(cc), 0);
    if (flag < 0) {
        printf("Error while receiving statistics!\n");
        exit(0);
    }
    printf("\nNumber of Characters (including <space> and <dot>)  : %d\n", ntohl(cc));
    flag = recv(sockfd, &dc, sizeof(dc), 0);
    if (flag < 0) {
        printf("Error while receiving statistics!\n");
        exit(0);
    }
    printf("Number of Characters (excluding <space> and <dot>)  : %d\n", ntohl(dc));
    flag = recv(sockfd, &wc, sizeof(wc), 0);
    if (flag < 0) {
        printf("Error while receiving statistics!\n");
        exit(0);
    }
    printf("Number of Words                                     : %d\n", ntohl(wc));
    flag = recv(sockfd, &sc, sizeof(sc), 0);
    if (flag < 0) {
        printf("Error while receiving statistics!\n");
        exit(0);
    }
    printf("Number of Sentences                                 : %d\n", ntohl(sc));    
    
    /* Close the .txt file */
    close(fileD);

    /* Close the socket */
    close(sockfd);

    return 0;
}