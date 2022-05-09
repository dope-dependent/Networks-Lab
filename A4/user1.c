#include "rsocket.h"

int main(){
    int sockfd=r_socket(AF_INET,SOCK_MRP,0);

    srand(time(NULL));

    struct sockaddr_in servaddr;

    memset(&servaddr, 0, sizeof(servaddr)); 

    servaddr.sin_family = AF_INET; 
    servaddr.sin_port = htons(50110); 
    inet_aton("127.0.0.1", &servaddr.sin_addr);

    socklen_t len=sizeof(servaddr);

    if(r_bind(sockfd,(struct sockaddr*)&servaddr,len)<0){
        perror("ERROR IN BIND\n");
        exit(0);
    }

    

    char hello[MAX_SIZE];

    scanf("%s",hello);

    struct sockaddr_in cliaddr;

    memset(&cliaddr, 0, sizeof(cliaddr)); 

    cliaddr.sin_family = AF_INET; 
    cliaddr.sin_port = htons(50111); 
    inet_aton("127.0.0.1", &cliaddr.sin_addr);

    socklen_t clilen=sizeof(cliaddr);
    
    for(int i=0;i<strlen(hello);i++){
        // printf("%c",hello[0]);
        r_sendto(sockfd,&hello[i],1,0,(struct sockaddr*)&cliaddr,clilen);
    }

    while(1);

    r_close(sockfd);

}