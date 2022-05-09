/*
    Group - 42

    Seemant G. Achari (19CS10055)
    Rajas Bhatt (19CS30037)

    ftpS.c
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <dirent.h>

#define MY_PORT 20000
#define DEF_SUCCESS 200
#define DEF_FAILURE 500
#define MAX_CONNECTIONS 5


#define BUFFER_SIZE 1024
#define MAX_DIRECTORY_SIZE 1024
#define SEND_BLOCK 128

/* Debug mode to get extra information */
#define __DEBUG 0

/* Print a string of strings */
/* "abc""bcd""" is printed as abc\0bcd\0\0 */
void printString(const char *buff) {
    char prev = 1;
    for (int i = 0; ; i++) {
        if (buff[i] == '\0') printf("\\0");
        else printf("%c", buff[i]);
        if (prev == buff[i] && buff[i] == '\0') break;
        prev = buff[i];
    }
}

/* Customized min function */
int cmin(int a, int b) {
    return a > b ? b : a;
}

/* Send a particular code to the client */
int sendCode(int newsockfd, int code) {
    char buf[4];
    int r;
    sprintf(buf, "%d", code);
    buf[3] = '\0';
    r = send(newsockfd, buf, 4, 0);
    if (r < 0) {
        printf("myFTP: Error in sending code\n");
        return -1;
    }
    return 0;
}

/* Get a dynamic entry (variable size) in the form 
of a NULL terminated string from the client */
int getEntry(int newsockfd, char **buff) {
    int size = BUFFER_SIZE;
    free (*buff);
    *buff = (char *)calloc(size, sizeof(char));
    if (*buff == NULL) {
        printf("myFTP: calloc failed\n");
        return -1;
    }
    *buff[0] = '\0';
    char c = 0;
    int k = 0, r, over = 0;
    char temp[BUFFER_SIZE];
    while (!over) {
        if (k >= size / 2) { // Increase alotted memory if needed
            size *= 2;
            *buff = (char *)realloc(*buff, size * sizeof(char));
            if (*buff == NULL) {
                printf("myFTP: realloc failed\n");
                return -1;
            }
        }
        r = recv(newsockfd, temp, BUFFER_SIZE, 0);
        if (r == 0) { // In this case the client has closed connection
            if (__DEBUG) printf("myFTP: Connection Closed\n");
            if (*buff != NULL) free (*buff);
            close(newsockfd); // Close the socket from server side
            exit(EXIT_SUCCESS);
        }
        if (r < 0) {
            printf("myFTP: Error in receiving data\n");
            return -1;
        }
        for (int i = 0; i < r; i++) {
            (*buff)[k++] = temp[i];
            if (temp[i] == '\0') { // If \0 detected, stop accepting
                over = 1;
                break;
            }
        }
    }
    if (__DEBUG) printf("receiveEntry: %s\n", *buff);
    return 0;
}

/* Search for user entry in file */
/* Return the line no if entry is matched and -1 for not matched */
int searchName(const char *name, char pass[]) {
    pass[0] = '\0';
    // Open the file in read only mode
    int fd = open("user.txt", O_RDONLY, 0666);
    if (fd < 0) {
        perror("myFTP ");
        return -1;
    }
    int r;
    char buf[BUFFER_SIZE], temp[BUFFER_SIZE];
    char uname[BUFFER_SIZE], passw[BUFFER_SIZE];
    int lc = 0;
    int k = 0;
    while(r = read(fd, buf, BUFFER_SIZE)) {
        if (r < 0) {
            perror("myFTP ");
            return -1;
        }
        buf[r] = '\0';
        for (int i = 0; i < r; i++) {
            if (buf[i] == '\n') { // If newline detected, search the complete line
                temp[k] = '\0';
                // Find if the username matches with first string
                // If it does, fetch the password
                sscanf(temp, "%s %s", uname, passw);
                if (!strcmp(name, uname)) {
                    strcpy(pass, passw);
                    return lc;
                }
                lc++;
                k = 0;
            }         
            else temp[k++] = buf[i];               
        }
        if (k == 0) return -1;
        temp[k] = '\0';
        sscanf(temp, "%s %s", uname, passw);
        if (!strcmp(name, uname)) {
            strcpy(pass, passw);
            return lc;
        }
        return -1;
    }
}

/* Get command from user input */
int getCMD(const char *buff, char cmd[5]) {
    int l = strlen(buff), k = 0;
    for (int i = 0; i < l; i++) {
        if (buff[i] == ' ') break;
        cmd[k++] = buff[i];
        if (k > 4) return -1;   // Error condition detected
    }
    cmd[k] = '\0';
    if (k == 0) return -1;  // Errror condition
    if (__DEBUG) printf("getCMD: %s\n", cmd);
    return 0;
}  

/* Get the arguments for a particular space 
separated command and return their number */
int getArgs(const char *buff, char **arg1, char **arg2) {
    int N = strlen(buff), i = 0;
    int size1 = BUFFER_SIZE, size2 = BUFFER_SIZE;
    int k = 0;
    while (i < N && buff[i] == ' ') i++;
    while (i < N && buff[i] != ' ') i++;        // The command is first
    while (i < N && buff[i] == ' ') i++;
    if (i >= N) return 0;       // O arguments

    // Try to get the first argument
    if (*arg1 != NULL) free (*arg1);
    *arg1 = (char *)calloc(size1, sizeof(char));
    if (*arg1 == NULL) {
        printf("myFTP: calloc failed\n");
        return -1;
    }
    (*arg1)[0] = '\0';
    while (i < N && buff[i] != ' ') {
        if (k >= size1 / 2) {
            size1 *= 2;
            *arg1 = (char *)realloc(*arg1, size1 * sizeof(char));
            if (*arg1 == NULL) {
                printf("myFTP: realloc failed\n");
                return -1;
            }
        }
        (*arg1)[k++] = buff[i++];
    }   
    (*arg1)[k] = '\0';

    if (__DEBUG) printf("getArgs 1: %s\n", *arg1);  

    while (i < N && buff[i] == ' ') i++;    // Eat up any empty spaces
    if (i >= N) return 1;   // 1 argument

    if (*arg2 != NULL) free (*arg2);
    *arg2 = (char *)calloc(size2, sizeof(char));
    if (*arg2 == NULL) {
        printf("myFTP: calloc failed\n");
        return -1;
    }
    (*arg2)[0] = '\0';
    k = 0;
    while (i < N && buff[i] != ' ') {
        if (k >= size2 / 2) {
            size2 *= 2;
            *arg2 = (char *)realloc(*arg2, size2 * sizeof(char));
            if (*arg2 == NULL) {
                printf("myFTP: realloc failed\n");
                return -1;
            }
        }
        (*arg2)[k++] = buff[i++];
    }   
    (*arg2)[k] = '\0';

    if (__DEBUG) printf("getArgs 2: %s\n",*arg2);
    return 2;       // 2 arguments
}

/* Send a null terminated character string to socket newsockfd along with the \0 character*/
int sendData(int newsockfd, const char * buff) {
    int len = strlen(buff);
    if (send(newsockfd, buff, len + 1, 0) < 0) {
        printf("myFTP: Error in sending data\n");
        return -1;
    }
    if (__DEBUG) printf("sendData: %s\n", buff);
    return 0;
}

/* Send a file opened by descriptor fd to socket newsockfd */
int sendFile(int fd, int newsockfd) {
    char buf[SEND_BLOCK], temp[SEND_BLOCK];
    int first = 1;
    int r, q, prelen = 0;
    char flag[1], preflag[1];
    int16_t sent; // Number of bytes in block
    while (1) {
        r = read(fd, buf, SEND_BLOCK);
        if (r < 0) {
            printf("myFTP: Error in sending file");
            return -1;
        }
        if (r == 0) flag[0] = 'L';
        else flag[0] = 'M';
        if (first && r == 0) {
            // Empty file
            q = send(newsockfd, flag, 1, 0);
            if (q < 0) {
                printf("myFTP: Error in sending flag");
                return -1;
            }    
            sent = htons(0);
            q = send(newsockfd, (void *)&sent, 2, 0);
            if (q < 0) {
                printf("myFTP: Error in sending number of bytes");
                return -1;
            }
        }
        if (!first) {
            q = send(newsockfd, flag, 1, 0);
            if (q < 0) {
                printf("myFTP: Error in sending flag");
                return -1;
            }    
            sent = htons(prelen);
            q = send(newsockfd, (void *)&sent, 2, 0);
            if (q < 0) {
                printf("myFTP: Error in sending number of bytes");
                return -1;
            }
            q = send(newsockfd, temp, prelen, 0);
                if (q < 0) {
                printf("myFTP: Error in sending block");
                return -1;
            }
        }

        for (int i = 0; i < SEND_BLOCK; i++) temp[i] = buf[i];
        prelen = r;
        first = 0;
        preflag[0] = flag[0];
        if (r == 0) break; // If end of file is detected
    }
    return 0;
}

/* Get a file from newsockfd and write it to fd */
int getFile(int fd, int newsockfd) {
    char flag[1];
    char data[3];
    int16_t no, len;
    int r;
    char temp[1000];
    char buf[BUFFER_SIZE];
    char f;
    int k = 0;
    // Wait for code    
    while (1) {
        // Get flag
        int r = recv(newsockfd, temp, 1, 0);
        if (r < 0) {
            printf("myFTP: Error in getting flag\n");
            return -1;
        }
        f = temp[0];
        // Get the number of bytes in the block
        // By getting one byte at a time
        char *temp2 = (char *)&no;
        r = recv(newsockfd, temp2, 1, 0);
        if (r < 0) {
            printf("myFTP: Error in getting file size\n");
            return -1;
        }
        temp2++;
        r = recv(newsockfd, temp2, 1, 0);
        if (r < 0) {
            printf("myFTP: Error in getting file size\n");
            return -1;
        }
        temp2++;
        len = ntohs(no);    // Number of bytes in block
        if (__DEBUG) printf("%c %d\n", f, len);
        while (len > 0) {
            r = recv(newsockfd, buf, cmin(len, BUFFER_SIZE), 0);
            if (r < 0) {
                printf("myFTP: Error in getting data\n");
                return -1;
            }
            int rlen = len;
            // Write the the file
            while (rlen > 0) {
                int sent = write(fd, buf, cmin(rlen, BUFFER_SIZE));
                if (sent < 0) {
                    printf("myFTP: Error in writing data\n");
                    return -1;
                }
                rlen -= sent;
            }
            len -= r;
        }        
        if (f == 'L') break; // Last block detected
    }
    return 0;
}

/* Main function */
int main() {
    int freq, auth, eline;   

    // Server details
    int sockfd, newsockfd, err;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        printf("myFTP: Error in opening server socket\n");
        exit(EXIT_FAILURE);
    }
    struct sockaddr_in serv_addr, cli_addr;
    memset((void *)&serv_addr, 0, sizeof(serv_addr));
    memset((void *)&cli_addr, 0, sizeof(cli_addr));
    socklen_t cli_size;
    // Set server address
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(MY_PORT);

    // Bind
    err = bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    if (err < 0) {
        printf("myFTP: Error in binding\n");
        exit(EXIT_FAILURE);
    }
    
    // Listen
    listen(sockfd, MAX_CONNECTIONS);

    while (1) {
        freq = 0;
        auth = 0;
        // Try to accept connections
        cli_size = sizeof(cli_addr);
        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &cli_size);
        if (newsockfd < 0) {
            printf("myFTP: Error in accept\n");
            exit(EXIT_FAILURE);
        }
        // Fork to create a concurrent server
        int x = fork();        
        if (x == 0) { // Child process
            char pwd[MAX_DIRECTORY_SIZE];
            getcwd(pwd, MAX_DIRECTORY_SIZE);         

            while (1) {                
                char pass[BUFFER_SIZE];
                char cmd[5];
                char *entry = NULL;   // Get user entry
                char *arg1 = NULL, *arg2 = NULL;
                // Get commands from the client
                if (getEntry(newsockfd, &entry) < 0) exit(EXIT_FAILURE);
                if (getCMD(entry, cmd) < 0) exit(EXIT_FAILURE);
                if (getArgs(entry, &arg1, &arg2) < 0) exit(EXIT_FAILURE);
                if (!auth) { // Authentication first
                    if (!strcmp(cmd, "user")) {
                        // Find the argument
                        if (freq == 0) {
                            eline = searchName(arg1, pass);
                            if (eline < 0) sendCode(newsockfd, 500);
                            else {
                                sendCode(newsockfd, 200);
                                freq++;
                            }
                        }
                        else sendCode(newsockfd, 600);
                    }
                    else if (!strcmp(cmd, "pass")) {
                        // Find the argument
                        if (freq == 1) {
                            if (!strcmp(pass, arg1)) {
                                sendCode(newsockfd, 200);
                                freq++; 
                                auth = 1;   // Authentication Complete
                            }
                            else {
                                sendCode(newsockfd, 500);
                                freq = 0;
                            }
                        }
                        else {
                            sendCode(newsockfd, 600);
                            freq = 0;
                        }
                    }
                }                   
                else {
                    if (!strcmp(cmd, "cd")) {
                        if (chdir(arg1) < 0) sendCode(newsockfd, 500);
                        else {
                            getcwd(pwd, MAX_DIRECTORY_SIZE);
                            sendCode(newsockfd, 200);
                        }
                    }   
                    else if (!strcmp(cmd, "dir")) {
                        // if (sendCode(newsockfd, 200) < 0) exit(EXIT_FAILURE);
                        // Send directory information as null terminated strings
                        // followed by a ""
                        if (sendData(newsockfd, pwd) < 0) exit(EXIT_FAILURE);
                        DIR *d;
                        struct dirent *dir;
                        d = opendir(pwd);
                        if (d) {
                            while ((dir = readdir(d)) != NULL) {
                                if (sendData(newsockfd, dir->d_name) < 0) exit(EXIT_FAILURE);
                            }
                            closedir(d);
                        }
                        if (sendData(newsockfd, "") < 0) exit(EXIT_FAILURE);
                    } 
                    else if (!strcmp(cmd, "get")) {
                        int fd = open(arg1, O_RDONLY);
                        if (fd < 0 || ((arg1[0] == '.') && (arg1[1] == '/')) || (((arg1[0] == '.') && (arg1[1] == '.')) && (arg1[2] == '/'))) {
                            sendCode(newsockfd, 500);
                        }
                        else {
                            sendCode(newsockfd, 200);
                            sendFile(fd, newsockfd);
                        }                        
                    } 
                    else if (!strcmp(cmd, "put")) {
                        int fd = open(arg2, O_WRONLY|O_CREAT|O_TRUNC, 0666);
                        if (fd < 0 || ((arg2[0] == '.') && (arg2[1] == '/')) || (((arg2[0] == '.') && (arg2[1] == '.')) && (arg2[2] == '/'))) {
                            sendCode(newsockfd, 500);
                        }
                        else {
                            sendCode(newsockfd, 200);
                            getFile(fd, newsockfd);
                        }
                    }
                    else {
                        printf("myFTP: No such command\n");
                    }
                }        
                if (entry != NULL) free (entry);
                if (arg1 != NULL) free (arg1);
                if (arg2 != NULL) free (arg2);    
            }   
            close(newsockfd);
        }
        else { // Parent process
            close(newsockfd);
        }
    }         
    // Close the connection
    close(sockfd);
    return 0;
}