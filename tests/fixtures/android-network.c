// SPDX-License-Identifier: GPL-3.0-or-later
#define _GNU_SOURCE
#include <android/multinetwork.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <grp.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
int main(int argc, char **argv) {
    if (argc > 1) {
        gid_t inet=3003;
        if (setgroups(1,&inet)||setgid(atoi(argv[1]))||setuid(atoi(argv[1]))) { perror("identity");return 1; }
    }
    int failures=0;
    net_handle_t net = ((net_handle_t)100 << 32) | 0xcafed00dU;
    for (int mode=0;mode<4;mode++) {
        struct addrinfo hints={0},*result=NULL;
        hints.ai_family=AF_UNSPEC;hints.ai_socktype=SOCK_STREAM;hints.ai_flags=(mode&1)?AI_ADDRCONFIG:0;
        int r=(mode&2)?android_getaddrinfofornetwork(net,"www.google.com","443",&hints,&result):getaddrinfo("www.google.com","443",&hints,&result);
        if (r) failures++;
        printf("uid=%d mode=%d getaddrinfo=%d errno=%d (%s)\n",getuid(),mode,r,errno,gai_strerror(r));
        for(struct addrinfo *p=result;p;p=p->ai_next){char text[128];getnameinfo(p->ai_addr,p->ai_addrlen,text,sizeof(text),NULL,0,NI_NUMERICHOST);printf(" address=%s family=%d\n",text,p->ai_family);}
        freeaddrinfo(result);
    }
    for (int family=AF_INET;family<=AF_INET6;family+=AF_INET6-AF_INET) {
        int fd=socket(family,SOCK_DGRAM,0);int r=android_setsocknetwork(net,fd);if(r) failures++;printf("family=%d bindNetwork=%d errno=%d\n",family,r,errno);
        if(family==AF_INET){struct sockaddr_in a={.sin_family=AF_INET};inet_pton(AF_INET,"8.8.8.8",&a.sin_addr);r=connect(fd,(void*)&a,sizeof(a));}
        else{struct sockaddr_in6 a={.sin6_family=AF_INET6};inet_pton(AF_INET6,"::ffff:8.8.8.8",&a.sin6_addr);r=connect(fd,(void*)&a,sizeof(a));}
        if(r) failures++;
        printf(" connect-zero=%d errno=%d\n",r,errno);close(fd);
    }
    return failures?1:0;
}
