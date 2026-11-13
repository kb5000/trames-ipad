// SPDX-License-Identifier: GPL-3.0-or-later
#define _GNU_SOURCE
#include <errno.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/auxv.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>
static void check(int okay,const char *step){if(!okay){perror(step);exit(1);}}
static void packet(int fd,int flags,unsigned char *control,size_t *size){
    unsigned char bytes[65536];struct iovec iov={bytes,sizeof(bytes)};struct sockaddr_nl peer;
    struct msghdr msg={.msg_name=&peer,.msg_namelen=sizeof(peer),.msg_iov=&iov,.msg_iovlen=1,.msg_control=control,.msg_controllen=*size};
    check(recvmsg(fd,&msg,flags)>0,"recvmsg");check(!(msg.msg_flags&MSG_CTRUNC),"control capacity");check(peer.nl_pid==0,"original kernel sender");*size=msg.msg_controllen;
    int credentials=0,stamp=0;
    for(struct cmsghdr *c=CMSG_FIRSTHDR(&msg);c;c=CMSG_NXTHDR(&msg,c)){
        if(c->cmsg_level!=SOL_SOCKET)continue;
        if(c->cmsg_type==SCM_CREDENTIALS){struct ucred value;memcpy(&value,CMSG_DATA(c),sizeof(value));check(!value.pid&&!value.uid&&!value.gid,"kernel credentials");credentials++;}
        if(c->cmsg_type==SCM_TIMESTAMP){struct timeval value;memcpy(&value,CMSG_DATA(c),sizeof(value));time_t now=time(NULL);check(value.tv_sec<=now&&value.tv_sec>=now-3&&value.tv_usec>=0&&value.tv_usec<1000000,"actual receive timestamp");stamp++;}
    }
    check(credentials==1&&stamp==0,"Linux netlink credentials without timestamp records");
}
int main(void){
    setbuf(stdout,NULL);check(getauxval(AT_CLKTCK)==100&&sysconf(_SC_CLK_TCK)==100,"Linux USER_HZ auxv");
    int fd=socket(AF_NETLINK,SOCK_DGRAM|SOCK_CLOEXEC,NETLINK_ROUTE);check(fd>=0,"route socket");int on=1;
    check(!setsockopt(fd,SOL_SOCKET,SO_PASSCRED,&on,sizeof(on)),"SO_PASSCRED");check(!setsockopt(fd,SOL_SOCKET,SO_TIMESTAMP,&on,sizeof(on)),"SO_TIMESTAMP");
    struct sockaddr_nl local={.nl_family=AF_NETLINK,.nl_groups=RTMGRP_LINK|(1u<<(RTNLGRP_ND_USEROPT-1))},kernel={.nl_family=AF_NETLINK};check(!bind(fd,(void*)&local,sizeof(local)),"namespace subscriptions");
    struct {struct nlmsghdr header;struct ifinfomsg link;} request={.header={.nlmsg_len=sizeof(request),.nlmsg_type=RTM_GETLINK,.nlmsg_flags=NLM_F_REQUEST|NLM_F_DUMP,.nlmsg_seq=42}};
    check(sendto(fd,&request,sizeof(request),0,(void*)&kernel,sizeof(kernel))==sizeof(request),"route query");
    unsigned char a[128]={0},b[128]={0};size_t as=sizeof(a),bs=sizeof(b);packet(fd,MSG_PEEK,a,&as);packet(fd,0,b,&bs);check(as==bs&&!memcmp(a,b,as),"peek preserves queued timestamp and credentials");close(fd);
    fd=socket(AF_NETLINK,SOCK_DGRAM|SOCK_CLOEXEC,NETLINK_NETFILTER);check(fd>=0,"netfilter socket");
    struct {struct nlmsghdr header;unsigned char body[4];} unknown={.header={.nlmsg_len=sizeof(unknown),.nlmsg_type=0xff01,.nlmsg_flags=NLM_F_REQUEST,.nlmsg_seq=99}};
    check(sendto(fd,&unknown,sizeof(unknown),0,(void*)&kernel,sizeof(kernel))==sizeof(unknown),"unsupported subsystem send admission");
    unsigned char response[256];ssize_t count=recv(fd,response,sizeof(response),0);check(count>=(ssize_t)(sizeof(struct nlmsghdr)+sizeof(struct nlmsgerr)),"error reply");
    struct nlmsghdr *head=(void*)response;struct nlmsgerr *error=NLMSG_DATA(head);check(head->nlmsg_type==NLMSG_ERROR&&head->nlmsg_seq==99&&error->error<0,"honest asynchronous unsupported protocol error");close(fd);
    puts("android-netlink: PASS kernel credentials, timestamp option/peek, subscriptions and error ACK");return 0;
}
