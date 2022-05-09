#include "rsocket.h"

int count=0;

void * R(void*);
void * S(void*);

int _drop(double x){
    double r=rand()%1000+1;
    r=r/1000;
    if(r>x) return 0;
    return 1;
}

int r_socket(int domain, int type, int protocol){
    if(type!=SOCK_MRP)
        return -1;
    int ret=socket(domain, SOCK_DGRAM, protocol);

    if(ret<0){
        perror("ERROR IN SOCKET");
        exit(0);
    }

    if(pthread_mutex_init(&lock,NULL)<0)
    {
        perror("ERROR IN MUTEX INIT\n");
        exit(0);
    }
    // printf("Actual sockfd: %d\n",ret);
    // printf("Pointer: %ld\n",(long)&ret);
    _sockfd=ret;

    unack_table= (unack_msg*)malloc(TABLE_SIZE*sizeof(unread_msg));


    pthread_create(&r,NULL,&R,NULL);
    pthread_create(&s,NULL,&S,NULL);
    return ret;
}

int r_bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen){
    int ret=bind(sockfd, addr, addrlen);
}

int r_sendto(int sockfd, const void *buf, size_t len, int flags, 
                const struct sockaddr *dest_addr, socklen_t addrlen)
{
    int16_t i;
    for(i=0;i<TABLE_SIZE;i++)
    if(unack_table[i].occupied==-1) break;

    pthread_mutex_lock(&lock);
    unack_table[i].occupied=1;
    unack_table[i].time=time(NULL);
    pthread_mutex_unlock(&lock);

    char buffer[MAX_SIZE+7+sizeof(struct sockaddr)];

    char msg[]="MSG";

    memcpy((void*)buffer,(void*) msg, 3);
    memcpy((void*)(buffer+3),(void*)&i,2);
    int16_t id=rand()%(1<<16-1)+1;
    memcpy((void*)(buffer+5),(void*)&id,2);
    memcpy((void*)(buffer+7),(void*)dest_addr,sizeof(struct sockaddr));
    memcpy((void*)(buffer+7+sizeof(struct sockaddr)),(void*)buf,MAX_SIZE);

    memcpy(unack_table[i].buff,buffer, MAX_SIZE);


    unack_table[i].sockfd=sockfd;
    unack_table[i].addr= *dest_addr;
    unack_table[i].id=id;


    int ret=sendto(sockfd, buffer, len+7+sizeof(struct sockaddr), flags, dest_addr, addrlen);

    pthread_mutex_lock(&lock);
    count++;
    pthread_mutex_unlock(&lock);
}


int r_recvfrom(int sockfd, void *buf, size_t len, int flags,
                struct sockaddr *src_addr, socklen_t *addrlen)
{

    while(1){
         pthread_mutex_lock(&lock);
        if(unread_pointers.head) 
        {
            memcpy(buf,unread_pointers.head->buff+7+sizeof(struct sockaddr),MAX_SIZE);
            int ret = unread_pointers.head->ret;
            unread_msg *pres=unread_pointers.head;
            
           
            if(unread_pointers.head==unread_pointers.tail){
                unread_pointers.tail=NULL;
            }
            unread_pointers.head=pres->next;
            

            free(pres); 

            pthread_mutex_unlock(&lock);         
            return ret;
        }
        else
        {
            pthread_mutex_unlock(&lock); 
            sleep(RECV_WAIT_TIME);
        }
    }
}

int r_close(int fd){
    int ret=close(fd);
    free(unack_table);
    while(unread_pointers.head!=NULL){
        unread_msg *temp=unread_pointers.head;
        unread_pointers.head=unread_pointers.head->next;
        free(temp);
    }
    return ret;
}


void* R(void* arg){

    int sockfd= _sockfd;    
    char buff[MAX_SIZE+7+sizeof(struct sockaddr)];
    struct sockaddr src_addr; 
    socklen_t addrlen;
    while(1){
        addrlen= sizeof(src_addr);
        int ret= recvfrom(sockfd,buff,MAX_SIZE,0,&src_addr,&addrlen);
        
        if(ret<0){
            perror("ERROR IN RECVFROM");
            exit(0);
        }
        if(_drop(PROBABILITY))
        {
            continue;
        }    
        else{
            //
            if(buff[0]=='M'){
                unread_msg *new = (unread_msg *)malloc(sizeof(unread_msg));
                
                for(int i=0;i<MAX_SIZE;i++)
                new->buff[i]=buff[i];
                new->ret=ret-7-sizeof(struct sockaddr);
                new->addr= src_addr;
                new->addrlen= addrlen;
                new->next=NULL;


                pthread_mutex_lock(&lock);
                if(unread_pointers.tail!=NULL)
                unread_pointers.tail->next=new;
                unread_pointers.tail=new;
                if(unread_pointers.head==NULL) unread_pointers.head=new;
                pthread_mutex_unlock(&lock);
                
                char ack_msg[8];
                ack_msg[0]='A'; ack_msg[1]='C'; ack_msg[2]='K';

                memcpy(ack_msg+3,buff+3,2);
                memcpy(ack_msg+5,buff+5,2);
                struct sockaddr_in *addr_in = (struct sockaddr_in *)&src_addr;

                int x=sendto(sockfd,ack_msg,8,0,&src_addr,addrlen);
            }
            else{
                                

                int16_t seq= *(int16_t*)(buff+3);
                int16_t id=  *(int16_t*)(buff+5);
                pthread_mutex_lock(&lock);
                if(unack_table[seq].id==id&&unack_table[seq].occupied==1)
                unack_table[seq].occupied=-1;
                
                pthread_mutex_unlock(&lock);
            }
        }
    }
}

void* S(void* arg){
    
    
    for(int i=0;i<TABLE_SIZE;i++)
    unack_table[i].occupied=-1;


    while(1){
        time_t start=time(NULL);

        for(int i=0;i<TABLE_SIZE;i++)
        {
            pthread_mutex_lock(&lock);
            if(unack_table[i].occupied!=-1 && start-unack_table[i].time>=TIMEOUT)
            {
                unack_table[i].time=start;
                int16_t id=rand()%(1<<15-1)+1;
                unack_table[i].id=id;
                memcpy(unack_table[i].buff+5,&id,2);
                unack_table[i].occupied=1;
                pthread_mutex_unlock(&lock);
                unack_msg a= unack_table[i];


                int ret= sendto(a.sockfd,a.buff,MAX_SIZE+4+sizeof(struct sockaddr),0,&a.addr,sizeof(a.addr));
                if(ret<0){
                    perror("Error in SENDTO\n");
                    exit(0);
                } 
                count++;
                // printf("count: %d\n",count);
                
            }
            pthread_mutex_unlock(&lock);
        }

        sleep(TIMEOUT);
    }
}
