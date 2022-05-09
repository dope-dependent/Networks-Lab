/*
    Group 42
    Seemant G. Achari (19CS10055)
    Rajas Bhatt (19CS30037)
    
    Running instructions

    * Put terminal into sudo mode (pre-requisite) using
        sudo su
    * Compile using
        gcc -o mytraceroute <file_name>
    * Run executable using
        ./mytraceroute <domain_name>

*/

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define SOURCE_PORT 52523
#define DEST_PORT 32164
#define BUFFER_SIZE 128
#define UDP_PAYLOAD_SIZE 52
#define MAX_TTL 16
#define MAX_TRIES 3

void bitprint(const char *data, int size) {
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < 8; j++) {
            if ((1 << (8 - j - 1) & (data[i]))) printf("1");
            else printf("0");
        }
    }
    printf("\n");
}

long double timediff(struct timeval st, struct timeval rt) {
    time_t sec, msec;
    if (rt.tv_usec < st.tv_sec) {
        msec = 1000000 + rt.tv_usec - st.tv_usec;
        sec = rt.tv_sec - st.tv_sec - 1;
    }
    else {
        msec = rt.tv_usec - st.tv_usec;
        sec = rt.tv_sec - st.tv_sec;
    }
    long double diff = sec + (msec) / ((long double)1e6);
    return diff;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Format: mytraceroute <domain name>\n");
        exit(EXIT_FAILURE);
    }

    const char *domain = argv[1];
    struct hostent *host = gethostbyname(domain);
    if (host == NULL) {
        printf("gethostbyname: No IP Address corresponding to the domain name\n");
        exit(EXIT_FAILURE);
    }

    int i = 0;
    char *ip;
    while (!i && host->h_addr_list[i] != NULL) {
        struct in_addr *caddr = (struct in_addr *)host->h_addr_list[i];
        ip = inet_ntoa(*caddr);
        i++;
    }

    printf("Destination IP: %s\n", ip);

    /* Create raw sockets */
    int sockfd_udp, sockfd_icmp;
    sockfd_udp = socket(AF_INET, SOCK_RAW, IPPROTO_UDP);
    if (sockfd_udp < 0) {
        perror("sockfd_udp");
        exit(EXIT_FAILURE);
    }
    sockfd_icmp = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sockfd_icmp < 0) {
        perror("sockfd_icmp");
        exit(EXIT_FAILURE);
    }
    struct sockaddr_in caddr, daddr, recvaddr;
    memset(&caddr, 0, sizeof(caddr));
    memset(&daddr, 0, sizeof(daddr));
    memset(&recvaddr, 0, sizeof(recvaddr));
    
    caddr.sin_family = AF_INET;
    caddr.sin_port = htons(SOURCE_PORT);
    caddr.sin_addr.s_addr = INADDR_ANY;

    daddr.sin_family = AF_INET;
    daddr.sin_port = htons(DEST_PORT);
    inet_aton(ip, &daddr.sin_addr);

    

    if (bind(sockfd_udp, (struct sockaddr *)&caddr, sizeof(caddr)) < 0) {
        perror("sockfd_udp: bind");
        close(sockfd_icmp);
        close(sockfd_udp);
        exit(EXIT_FAILURE);
    }
    if (bind(sockfd_icmp, (struct sockaddr *)&caddr, sizeof(caddr)) < 0) {
        perror("sockfd_icmp: bind");
        close(sockfd_icmp);
        close(sockfd_udp);
        exit(EXIT_FAILURE);
    }

    int opts = 1;
    if (setsockopt(sockfd_udp, IPPROTO_IP, IP_HDRINCL, &opts, sizeof(opts)) < 0) {
        perror("setsockopt");
        close(sockfd_icmp);
        close(sockfd_udp); 
        exit(EXIT_FAILURE);
    }

    uint8_t TTL = 1;
    int repeat = 0;
    char buff[BUFFER_SIZE], buff2[BUFFER_SIZE];
    int reached = 0;
    srand(42);
    while (!reached && TTL <= MAX_TTL) {
        if (repeat >= MAX_TRIES) {
            printf("%3d\t*\t*\t*\t*\n", TTL);
            TTL++;
            repeat = 0;
            continue;
        }
        struct iphdr *iph  = (struct iphdr *)&buff;
        struct udphdr *udph = (struct udphdr *)(buff + sizeof(struct iphdr));
        char *data = buff + sizeof(struct iphdr) + sizeof(struct udphdr);

        /* UDP header */
        udph->dest    = htons(DEST_PORT);
        udph->source  = htons(SOURCE_PORT);
        udph->check   = 0;
        udph->len     = htons(UDP_PAYLOAD_SIZE + sizeof(struct udphdr));

        /* IP Header */
        iph->daddr    = daddr.sin_addr.s_addr;
        iph->saddr    = caddr.sin_addr.s_addr;
        iph->ttl      = TTL;
        iph->protocol = IPPROTO_UDP;
        iph->version  = 4U;
        iph->ihl      = (unsigned int)(sizeof(struct iphdr) / 4);
        iph->frag_off = 0; // Fragment offset is 0, very small packet
        iph->id       = 0; // ID not required as packet is going to be dropped
        iph->tos      = 0; // Type of service is normal service
        iph->check    = 0; // Don't really need the checksum, packet getting dropped anyway
        u_int16_t len     = UDP_PAYLOAD_SIZE + sizeof(struct udphdr) + sizeof(struct iphdr);
        iph->tot_len  = htons(len); 


        for (int i = 0; i < UDP_PAYLOAD_SIZE - 1; i++) data[i] = rand() % 255 - 128;
        data[UDP_PAYLOAD_SIZE - 1] = '\0';
        
        // bitprint(data, UDP_PAYLOAD_SIZE);

        int sd = sendto(sockfd_udp, buff, len, 0, (struct sockaddr *)&daddr, sizeof(daddr));
        struct timeval st;
        gettimeofday(&st, NULL);
        if (sd < 0) {
            perror("sendto");
            close(sockfd_icmp);
            close(sockfd_udp); 
            exit(EXIT_FAILURE);
        }  
        
        
        struct timeval t, tt, ttt;
        time_t wsec = 1;
        suseconds_t wusec = 0;        
        gettimeofday(&tt, NULL);

        while (!reached) {
            fd_set myFd;
            FD_ZERO(&myFd);
            FD_SET(sockfd_icmp, &myFd);
            t.tv_sec = wsec;
            t.tv_usec = wusec;
            if (select(sockfd_icmp + 1, &myFd, 0, 0, &t) < 0) {
                perror("select");
                close(sockfd_icmp);
                close(sockfd_udp);  
                exit(EXIT_FAILURE);
            }
            if (FD_ISSET(sockfd_icmp, &myFd)) {
                socklen_t dsize = sizeof(recvaddr);
                int r = recvfrom(sockfd_icmp, buff2, BUFFER_SIZE, 0, (struct sockaddr *)&recvaddr, &dsize);
                struct timeval rt;
                gettimeofday(&rt, NULL);
                if (r < 0) {
                    perror("recvfrom");
                    close(sockfd_icmp);
                    close(sockfd_udp);  
                    exit(EXIT_FAILURE);
                }
                struct iphdr *iphr = (struct iphdr *)buff2;
                if (iphr->protocol != 1) {
                    gettimeofday(&ttt, NULL);
                    wsec = 0;
                    wusec = ttt.tv_usec - tt.tv_usec;
                    continue;
                }
                struct icmphdr *icmphdr = (struct icmphdr *)(buff2 + sizeof(struct iphdr));
                char *datap = (char *)(buff2 + sizeof(struct iphdr) + sizeof(struct icmphdr));
                if (icmphdr->type == 3) {
                    if (iphr->saddr == daddr.sin_addr.s_addr) {
                        // Message from destination
                        char *ip = inet_ntoa(daddr.sin_addr);
                        printf("%3d   %-18s   %0.6Lfs\n", TTL, ip, timediff(st, rt));
                        TTL++;
                        repeat = 0;
                        reached = 1;
                        break;
                    }
                    else {
                        // Wait on select call
                        gettimeofday(&ttt, NULL);
                        wsec = 0;
                        wusec = ttt.tv_usec - tt.tv_usec;
                        continue;
                    }
                }
                else if (icmphdr->type == 11) {
                    int datasize = r - sizeof(struct iphdr) - sizeof(struct icmphdr);
                    // printf("%d\n", datasize);
                    // bitprint(datap, datasize);
                    struct in_addr _ip;
                    _ip.s_addr = iphr->saddr;
                    char *ip = inet_ntoa(_ip);
                    printf("%3d   %-18s   %0.6Lfs\n", TTL, ip, timediff(st, rt));
                    TTL++;
                    repeat = 0;
                    break;
                }
                else {
                    // Wait on select call
                    gettimeofday(&ttt, NULL);
                    wsec = 0;
                    wusec = ttt.tv_usec - tt.tv_usec;
                    continue;
                }
            }
            else {
                repeat++;
                break;
            } 
        }               
    }
    close(sockfd_icmp);
    close(sockfd_udp);    
    return 0;
}