/*
    Group - 42

    Seemant G. Achari (19CS10055)
    Rajas Bhatt (19CS30037)

    ftpC.c
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#define BUFFER_SIZE 2048
#define MAX_DIRECTORY_SIZE 10000
#define SEND_BLOCK 256

#define __DEBUG 0

/* Customized min function */
int cmin(int a, int b) {
    return a > b ? b : a;
}

/* Customized print string function */
void printString(const char *buff) {
    char prev = 1;
    for (int i = 0; ; i++) {
        if (buff[i] == '\0') printf("\\0");
        else printf("%c", buff[i]);
        if (prev == buff[i] && buff[i] == '\0') break;
        prev = buff[i];
    }
}

/* Send a null terminated string to sockfd */
int sendData(int sockfd, const char *buff) {
    int len = strlen(buff);
    int err = send(sockfd, buff, len + 1, 0);
    if (err < 0) {
        printf("myFTP: Error in sending data\n");
        return -1;
    }
    if (__DEBUG) printf("sendData: %s\n", buff);
    return 0;
}

/* Get input from recv till \0\0 occurs */
int receiveData(int sockfd, char **buff) {
    int size = BUFFER_SIZE;
    free (*buff);
    *buff = (char *)calloc(size, sizeof(char));
    if (*buff == NULL) {
        printf("myFTP: calloc failed\n");
        return -1;
    }
    *buff[0] = '\0';
    char c = 0, prev = 1;
    int k = 0, r, over = 0;
    char temp[BUFFER_SIZE];
    while (!over) {
        if (k >= size / 2) {
            size *= 2;
            *buff = (char *)realloc(*buff, size * sizeof(char));
            if (*buff == NULL) {
                printf("myFTP: realloc failed\n");
                return -1;
            }
        }
        r = recv(sockfd, temp, BUFFER_SIZE, 0);
        if (r < 0) {
            printf("myFTP: Error in receiving data\n");
            return -1;
        }
        for (int i = 0; i < r; i++) {
            (*buff)[k++] = temp[i];
            // Check if prev == \0 and temp[i] == \0, double null received
            if (prev == temp[i] && temp[i] == '\0') {
                over = 1;
                break;
            }
            prev = temp[i];        
        }
    }
    if (__DEBUG) {
        printf("receiveData: ");
        printString(*buff);
        printf("\n");
    }
    return 0;
}

/* Write to fd by reading in the specified format from sockfd */
int writeToFile(int fd, int sockfd) {
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
        int r = recv(sockfd, temp, 1, 0);
        if (r < 0) {
            printf("myFTP: Error in getting flag\n");
            return -1;
        }
        f = temp[0];
        // Get the number of bytes one by one
        char *temp2 = (char *)&no;
        r = recv(sockfd, temp2, 1, 0);
        if (r < 0) {
            printf("myFTP: Error in getting file size\n");
            return -1;
        }
        temp2++;
        r = recv(sockfd, temp2, 1, 0);
        if (r < 0) {
            printf("myFTP: Error in getting file size\n");
            return -1;
        }
        temp2++;
        len = ntohs(no); // the number of bytes in the block
        // printf("%d\n", len);
        // if (__DEBUG) printf("%c %d\n", f, len);
        while (len > 0) {
            r = recv(sockfd, buf, cmin(len, BUFFER_SIZE), 0);
            if (r < 0) {
                printf("myFTP: Error in getting data\n");
                return -1;
            }
            int rlen = len; // Get len characters from recv
            while (rlen > 0) {
                int sent = write(fd, buf,  cmin(rlen, BUFFER_SIZE));
                if (sent < 0) {
                    printf("myFTP: Error in writing data\n");
                    return -1;
                }
                rlen -= sent; // subtract by the number of characters actually written to file
            }
            len -= r; // Subtract by the number of characters actually received
        }        
        if (f == 'L') break;
    }
    if (__DEBUG) printf("writeToFile: File received successfully\n");
    return 0;
}

/* Get null terminated entry from stdin */
int getEntry(char **buff) {
    int size = BUFFER_SIZE;
    if (*buff != NULL) free (*buff);
    *buff = (char *)calloc(size, sizeof(char));
    if (*buff == NULL) {
        printf("myFTP: calloc failed\n");
        return -1;
    }
    *buff[0] = '\0';
    char c;
    int k = 0;
    int f = 0;
    while (1) {
        while((c = getchar()) == ' ' && !f);
        if (k >= size / 2) {
            size *= 2;
            *buff = (char *)realloc(*buff, size * sizeof(char));
            if (*buff == NULL) {
                printf("myFTP: realloc failed\n");
                return -1;
            }
        }
        if (c == '\n'|| c == EOF) {
            (*buff)[k++] = '\0';
            break;
        }
        if (c == ' ') f = 0;
        else f = 1;
        (*buff)[k++] = c; 
    }
    if (__DEBUG) printf("getEntry: %s\n", *buff);
    return 0;
}

/* Get the CMD from the buffer */
int getCMD(const char *buff, char cmd[5]) {
    int l = strlen(buff), k = 0;
    for (int i = 0; i < l; i++) {
        if (buff[i] == ' ') break;
        cmd[k++] = buff[i];
        if (k > 4) return -1;
    }
    cmd[k] = '\0';
    if (k == 0) return -1;
    if (__DEBUG) printf("getCMD: %s\n", cmd);
    return 0;
}  

/* Get the arguments from the user entry */
int getArgs(const char *buff, char **arg1, char **arg2) {
    int N = strlen(buff), i = 0;
    int size1 = BUFFER_SIZE, size2 = BUFFER_SIZE;
    int k = 0;
    while (i < N && buff[i] == ' ') i++;
    while (i < N && buff[i] != ' ') i++; // Eat up the command
    while (i < N && buff[i] == ' ') i++;
    if (i >= N) return 0;     // Only the command and no arguments
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
    if (i >= N) return 1;   // One argument only

    // Try to get the second argument
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

    if (__DEBUG) printf("getArgs 2: %s\n",*arg2); // Two arguments
    return 2;
}

/* Get the 3 digit error code sent by sockfd as null terminated string */
int getCode(int sockfd, char buf[4]) {
    int rem_len = 4, r, k = 0;
    char temp[4];
    while (rem_len > 0) {
        r = recv(sockfd, temp, rem_len, 0);
        if (r < 0) {
            printf("myFTP: Error in getting code\n");
            return -1;
        }
        for (int i = 0; i < r; i++) {
            buf[k++] = temp[i];
        }
        rem_len -= r;
    }
    if (__DEBUG) printf("getCode: %s\n", buf);
}

/* Send a file opened as fd to sockfd */
int sendToFile(int fd, int sockfd) {
    char buf[SEND_BLOCK], temp[SEND_BLOCK];
    int first = 1;
    int r, q, prelen = 0;
    char flag[1], preflag[1];
    int16_t sent;
    while (1) {
        // Read a specified number of bytes
        r = read(fd, buf, SEND_BLOCK);
        if (r < 0) {
            printf("myFTP: Error in sending file");
            return -1;
        }
        if (r == 0) flag[0] = 'L';
        else flag[0] = 'M';        
        if (first && r == 0) {
            // Empty file
            q = send(sockfd, flag, 1, 0);
            if (q < 0) {
                printf("myFTP: Error in sending flag");
                return -1;
            }    
            sent = htons(0);
            q = send(sockfd, (void *)&sent, 2, 0);
            if (q < 0) {
                printf("myFTP: Error in sending number of bytes");
                return -1;
            }
        }

        if (!first) {
            q = send(sockfd, flag, 1, 0);
            if (q < 0) {
                printf("myFTP: Error in sending flag");
                return -1;
            }    
            sent = htons(prelen);
            q = send(sockfd, (void *)&sent, 2, 0);
            if (q < 0) {
                printf("myFTP: Error in sending flag");
                return -1;
            }
            q = send(sockfd, temp, prelen, 0);
                if (q < 0) {
                printf("myFTP: Error in sending flag");
                return -1;
            }
        }

        for (int i = 0; i < SEND_BLOCK; i++) temp[i] = buf[i];
        prelen = r;
        first = 0;
        preflag[0] = flag[0];
        if (r == 0) break;
    }
    if (__DEBUG) printf("sendToFile: File sent successfully\n");
    return 0;
}

/* Used to implement the mget and mput commands */
int fileCommands(int sockfd, char *buff, char cmd[5]) {
    int N = strlen(buff);
    int i = 0;
    while (i < N && buff[i] != ' ') i++;
    i++;
    // Start finding files
    char *files = strtok(buff + i * sizeof(char), ",");
    char code[4];
    while (files != NULL) {
        while (*files == ' ') files++;
        int j = strlen(files) - 1;
        while (j >= 0 && files[j] == ' ') files[j--] = '\0';
        if (strlen(files) != 0) {
            char *data = (char *)calloc(5 + 2 * strlen(files) + 5, sizeof(char));
            sprintf(data, "%s %s %s", cmd + 1, files, files);
            if (!strcmp(cmd, "mget")) {
                // Try to open file
                int fd = open(files, O_WRONLY|O_CREAT|O_TRUNC, 0666);
                if (fd < 0 || ((files[0] == '.') && (files[1] == '/')) || (((files[0] == '.') && (files[1] == '.')) && (files[2] == '/'))) {
                    printf("myFTP: Cannot open %s\n", files);
                    free (data);
                    return -1;
                }
                // Send the complete user command
                if (sendData(sockfd, data) < 0) exit(EXIT_FAILURE);
                // Get code
                if (getCode(sockfd, code) < 0) exit(EXIT_FAILURE);
                if (!strcmp(code, "200")) {                    
                    if (writeToFile(fd, sockfd) < 0) return -1;
                    printf("myFTP %s: File %s transferred successfully\n", code, files);
                }
                else if (!strcmp(code, "500")) {
                    printf("myFTP %s: File %s can't be found on server side\n", code, files);
                    free (data);
                    return -1;
                }
            }   
            else {
                // Try to open file
                int fd = open(files, O_RDONLY);
                if (fd < 0 || ((files[0] == '.') && (files[1] == '/')) || (((files[0] == '.') && (files[1] == '.')) && (files[2] == '/'))) {
                    printf("myFTP: Cannot open %s\n", files);
                    free (data);
                    return -1;
                }
                // Send the complete user command
                if (sendData(sockfd, data) < 0) exit(EXIT_FAILURE);
                // Get code
                if (getCode(sockfd, code) < 0) exit(EXIT_FAILURE);
                if (!strcmp(code, "200")) {
                    if (sendToFile(fd, sockfd) < 0) return -1;
                    printf("myFTP %s: File %s transferred successfully\n", code, files);
                }
                else if (!strcmp(code, "500")) {
                    printf("myFTP %s: File can't be found on server side\n", code);
                    free (data);
                    return -1;
                }
            }
            free (data);
            if (__DEBUG) printf("%s\n", files); 
        }        
        files = strtok(NULL, ",");
    }
    printf("myFTP: Command executed successfully\n");
    return 0;
}

/* Main function */
int main() {
    int freq = 0;
    int openc = 0, auth = 0, user = 0, pass = 0;
    char *entry = NULL;     // Command entered by the user
    char cmd[5], code[4];   // Command and 3-digit code
    char *arg1 = NULL, *arg2 = NULL;    // Argument buffers
    char *buff = NULL;  // Buffer
    char *pwd = getcwd(NULL, MAX_DIRECTORY_SIZE);
    
    int sockfd, err;    // Socket descriptor
    sockfd = socket(AF_INET, SOCK_STREAM, 0);   // Open client socket
    if (sockfd < 0) {
        printf("myFTP: Error in opening client socket\n");
        exit(EXIT_FAILURE);
    }
    // Server address
    struct sockaddr_in serv_addr;
    memset((void *)&serv_addr, 0, sizeof(serv_addr));
    
    serv_addr.sin_family = AF_INET;

    while (1) {
        printf(" %s myFTP> ", pwd);
        getEntry(&entry);       // Get user entry
        if(getCMD(entry, cmd) < 0) continue;  // If no valid command found, skip
        if (__DEBUG) printf("%s\n", cmd);
        int _no = getArgs(entry, &arg1, &arg2);
        if(!strcmp(cmd, "quit")) {
            close(sockfd);      // Quit called, close socket
            break;
        }
        if (!openc) {
            // The first command has to be the open command
            if (!strcmp(cmd, "open")) {
                // Get IP address and port
                int port;
                sscanf(arg2, "%d", &port);
                if (port < 20000 || port > 65535) {
                    printf("myFTP: Bad port\n");
                    continue;
                }
                serv_addr.sin_port = htons(port);
                inet_aton(arg1, &(serv_addr.sin_addr));
                // Try to establish connection
                err = connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
                if (err < 0) { // Connection failed
                    printf("myFTP: Error in establishing connection\n");
                }
                else {  // Connection established
                    printf("myFTP: Connection established successfully\n");
                    openc = 1;
                    freq++;
                }
            }
            else {
                printf("myFTP: Please open a connection first\n");
            }
            continue;
        }
        if (!auth) {
            // Authorize
            if (!strcmp(cmd, "user")) {
                if (sendData(sockfd, entry) < 0) exit(EXIT_FAILURE);
                if (getCode(sockfd, code) < 0) exit(EXIT_FAILURE);

                if (!strcmp(code, "200")) {
                    printf("myFTP %s: Command Executed Successfully\n", code);
                    user = 1;
                    freq++;
                }
                else if (!strcmp(code, "500")) printf("myFTP %s: No such user exists\n", code);
                else printf("myFTP %s: Wrong Command sent\n", code);
            }
            else if (!strcmp(cmd, "pass")) {
                if (sendData(sockfd, entry) < 0) exit(EXIT_FAILURE);
                if (getCode(sockfd, code) < 0) exit(EXIT_FAILURE);
                if (!strcmp(code, "200")) {
                    printf("myFTP %s: Command Executed Successfully\n", code);
                    pass = 1;
                    freq++;
                }
                else if (!strcmp(code, "500")) printf("myFTP %s: Password does not match\n", code);
                else printf("myFTP %s: Wrong Command sent\n", code);
            }   
            else printf("myFTP: Please authorize the user first\n");
            if (user && pass) auth = 1;
            continue;
        }
        if (!strcmp(cmd, "cd")) {
            if (sendData(sockfd, entry) < 0) exit(EXIT_FAILURE);
            if (getCode(sockfd, code) < 0) exit(EXIT_FAILURE);
            if (!strcmp(code, "200")) printf("myFTP %s: Server directory change successful\n", code);
            else if (!strcmp(code, "500")) printf("myFTP %s: Error in Server Directory change\n", code);
        }
        else if (!strcmp(cmd, "lcd")) {
            // Try to change the directory to arg1 on the client side
            if (chdir(arg1) < 0) {
                printf("myFTP: Error in Client Directory change\n");
            }
            else {
                printf("myFTP: Client Directory changed to %s\n", arg1);
                pwd = getcwd(pwd, MAX_DIRECTORY_SIZE);
            }
        }
        else if (!strcmp(cmd, "dir")) {
            if (sendData(sockfd, entry) < 0) exit(EXIT_FAILURE);
            // if (getCode(sockfd, code) < 0) exit(EXIT_FAILURE);
            // if (!strcmp(code, "200")) {
                // printf("myFTP %s: Command executed successfully\n", code);
                // Get data
                if (receiveData(sockfd, &buff) < 0) exit(EXIT_FAILURE);
                int i = 0; 
                char prev = 1;
                printf("myFTP: ");
                while (1) {
                    if (buff[i] == '\0') printf("\n");
                    else printf("%c", buff[i]);
                    if (prev == buff[i] && prev == '\0') break;
                    prev = buff[i];
                    i++;
                }
            // }
            // if (!strcmp(code, "500")) {
            //     printf("myFTP %s: Error executing command\n", code);
            // }     
        }
        else if (!strcmp(cmd, "get")) {
            // Check if the local_file can be opened for writing
            int fd = open(arg2, O_WRONLY|O_CREAT|O_TRUNC, 0666);
            if (fd < 0 || ((arg2[0] == '.') && (arg2[1] == '/')) || (((arg2[0] == '.') && (arg2[1] == '.')) && (arg2[2] == '/'))) {
                printf("myFTP: Cannot open %s\n", arg2);
            }
            else {
                // Send command and get code
                if (sendData(sockfd, entry) < 0) exit(EXIT_FAILURE);
                if (getCode(sockfd, code) < 0) exit(EXIT_FAILURE);
                if (!strcmp(code, "200")) {
                    if (writeToFile(fd, sockfd) < 0) exit(EXIT_FAILURE);
                    printf("myFTP %s: Command executed successfully\n", code);
                }
                else if (!strcmp(code, "500")) {
                    printf("myFTP %s: File can't be found on server side\n", code);
                }
            }
        }
        else if (!strcmp(cmd, "put")) {
            // Check if the local file can be opened for reading
            int fd = open(arg1, O_RDONLY);
            if (fd < 0 || ((arg1[0] == '.') && (arg1[1] == '/')) || (((arg1[0] == '.') && (arg1[1] == '.')) && (arg1[2] == '/'))) {
                printf("myFTP: Cannot open %s\n", arg1);
            }
            else {
                // Send command and get code
                if (sendData(sockfd, entry) < 0) exit(EXIT_FAILURE);
                if (getCode(sockfd, code) < 0) exit(EXIT_FAILURE);
                if (!strcmp(code, "200")) {
                    if (sendToFile(fd, sockfd) < 0) exit(EXIT_FAILURE);
                    printf("myFTP %s: Command executed successfully\n", code);
                }
                else if (!strcmp(code, "500")) {
                    printf("myFTP %s: File can't be found on server side\n", code);
                }
            }
        }
        else if (!strcmp(cmd, "mget") || !strcmp(cmd, "mput")) {
            if (fileCommands(sockfd, entry, cmd) < 0) {
                printf("myFTP: Error in %s\n", cmd);
            }
        }
        else {
            printf("myFTP: Wrong Command\n");
        }
        printf("\n");
    }
    if (buff != NULL) free(buff);
    if (arg1 != NULL) free(arg1);
    if (arg2 != NULL) free(arg2);
    if (entry != NULL) free (entry);
    if (pwd != NULL) free(pwd);
    return 0;
}
