// SPDX-License-Identifier: GPL-3.0-or-later
#define _GNU_SOURCE
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <sys/eventfd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <sys/syscall.h>
#include <sys/prctl.h>
#include <linux/filter.h>
#include <linux/seccomp.h>
#include <time.h>
#include <unistd.h>
static unsigned threads,rounds,mode,policy;
static pthread_barrier_t barrier;
static unsigned signals;
static void caught(int signal) {assert(signal==SIGPIPE);__atomic_fetch_add(&signals,1,__ATOMIC_RELAXED);}
static uint64_t now(void) {struct timespec t;assert(!clock_gettime(CLOCK_MONOTONIC,&t));return (uint64_t)t.tv_sec*1000000000+t.tv_nsec;}
static socklen_t address(struct sockaddr_un *a,const char *name,int path) {
 memset(a,0,sizeof *a);a->sun_family=AF_UNIX;
 snprintf(a->sun_path+!path,sizeof(a->sun_path)-!path,"%s",name);
 return (socklen_t)(offsetof(struct sockaddr_un,sun_path)+strlen(name)+1);
}
static void *run(void *argument) {
 if(mode>=4) {
  int ends[2];assert(!socketpair(AF_UNIX,(mode<6?SOCK_STREAM:SOCK_SEQPACKET)|SOCK_CLOEXEC,0,ends));assert(!shutdown(ends[1],SHUT_RD));
  char byte='x';struct iovec vector={&byte,1};struct mmsghdr messages[2]={0};for(unsigned i=0;i<2;i++){messages[i].msg_hdr.msg_iov=&vector;messages[i].msg_hdr.msg_iovlen=1;}
  pthread_barrier_wait(&barrier);
  for(unsigned i=0;i<rounds;i++)assert(sendmmsg(ends[0],messages,2,(mode&1)?MSG_NOSIGNAL:0)==-1 && errno==EPIPE);
  assert(!close(ends[0]));assert(!close(ends[1]));return NULL;
 }
 unsigned id=(unsigned)(uintptr_t)argument;int tx=socket(AF_UNIX,SOCK_DGRAM|SOCK_CLOEXEC,0);
 int good=socket(AF_UNIX,SOCK_DGRAM|SOCK_CLOEXEC,0),bad=socket(AF_UNIX,SOCK_DGRAM|SOCK_CLOEXEC,0);assert(tx>=0 && good>=0 && bad>=0);
 struct sockaddr_un targets[2];char names[2][100];socklen_t sizes[2];
 for(unsigned i=0;i<2;i++) {
  snprintf(names[i],sizeof names[i],mode==3?"/tmp/tm-anc-%ld-%u-%u":"tm-anc-%ld-%u-%u",(long)getpid(),id,i);
  sizes[i]=address(&targets[i],names[i],mode==3);assert(!bind(i?bad:good,(struct sockaddr *)&targets[i],sizes[i]));
 }
 assert(!shutdown(bad,SHUT_RD));int directory=-1;
 if(mode==3) {
  directory=open("/tmp",O_RDONLY|O_DIRECTORY|O_CLOEXEC);assert(directory>=0);
  char alias[100];snprintf(alias,sizeof alias,"/proc/self/fd/%d/%s",directory,strrchr(names[0],'/')+1);
  sizes[0]=address(&targets[0],alias,1);
 }
 pthread_barrier_wait(&barrier);
 for(unsigned i=0;i<rounds;i++) {
  int carried=eventfd(i+1,EFD_CLOEXEC);assert(carried>=0);
  union {struct cmsghdr align;char bytes[CMSG_SPACE(sizeof(int))];} control={0};
  char byte='x';struct iovec vector={&byte,1};struct mmsghdr messages[2]={0};
  for(unsigned m=0;m<2;m++) {messages[m].msg_hdr.msg_name=&targets[m];messages[m].msg_hdr.msg_namelen=sizes[m];messages[m].msg_hdr.msg_iov=&vector;messages[m].msg_hdr.msg_iovlen=1;}
  messages[0].msg_hdr.msg_control=control.bytes;messages[0].msg_hdr.msg_controllen=sizeof control;
  struct cmsghdr *c=CMSG_FIRSTHDR(&messages[0].msg_hdr);c->cmsg_level=SOL_SOCKET;c->cmsg_type=SCM_RIGHTS;c->cmsg_len=CMSG_LEN(sizeof(int));memcpy(CMSG_DATA(c),&carried,sizeof carried);
  if(mode==1 || mode==2) {assert(sendmmsg(tx,messages,2,mode==2?MSG_NOSIGNAL:0)==1);assert(messages[0].msg_len==1 && messages[1].msg_len==0);}
  else assert(sendmsg(tx,&messages[0].msg_hdr,MSG_NOSIGNAL)==1);
  int replacement=eventfd(99,EFD_CLOEXEC);assert(replacement>=0);assert(dup3(replacement,carried,O_CLOEXEC)==carried);assert(!close(replacement));
  memset(&control,0,sizeof control);char received=0;struct iovec output={&received,1};
  struct msghdr result={.msg_iov=&output,.msg_iovlen=1,.msg_control=control.bytes,.msg_controllen=sizeof control};
  assert(recvmsg(good,&result,MSG_CMSG_CLOEXEC)==1 && received=='x' && !(result.msg_flags&MSG_CTRUNC));
  c=CMSG_FIRSTHDR(&result);assert(c && c->cmsg_level==SOL_SOCKET && c->cmsg_type==SCM_RIGHTS);int fd=-1;memcpy(&fd,CMSG_DATA(c),sizeof fd);
  uint64_t value=0;assert(fd>=0 && (fcntl(fd,F_GETFD)&FD_CLOEXEC));assert(read(fd,&value,8)==8 && value==i+1);assert(!close(fd));
  assert(read(carried,&value,8)==8 && value==99);assert(!close(carried));
 }
 if(directory>=0)assert(!close(directory));assert(!close(tx));assert(!close(good));assert(!close(bad));
 if(mode==3)for(unsigned i=0;i<2;i++)assert(!unlink(names[i]));return NULL;
}
int main(int argc,char **argv) {
 threads=argc>1?(unsigned)atoi(argv[1]):4;rounds=argc>2?(unsigned)atoi(argv[2]):100;mode=argc>3?(unsigned)atoi(argv[3]):0;policy=argc>4?(unsigned)atoi(argv[4]):0;
 assert(threads>0 && threads<=32 && rounds>0 && mode<=7);if(mkdir("/tmp",0777))assert(errno==EEXIST);
 struct sigaction action={.sa_handler=caught};assert(!sigemptyset(&action.sa_mask));assert(!sigaction(SIGPIPE,&action,NULL));
 if(policy) {struct sock_filter filter[]={BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_ALLOW)};struct sock_fprog program={1,filter};assert(!prctl(PR_SET_NO_NEW_PRIVS,1,0,0,0));assert(!prctl(PR_SET_SECCOMP,SECCOMP_MODE_FILTER,&program));}
 assert(!pthread_barrier_init(&barrier,NULL,threads+1));pthread_t tasks[32];for(unsigned i=0;i<threads;i++)assert(!pthread_create(&tasks[i],NULL,run,(void *)(uintptr_t)i));
 uint64_t began=now();pthread_barrier_wait(&barrier);for(unsigned i=0;i<threads;i++)assert(!pthread_join(tasks[i],NULL));uint64_t elapsed=now()-began;
 assert(signals==(mode==4?threads*rounds:0));
 printf("unix-ancillary threads=%u rounds=%u mode=%u policy=%u signals=%u elapsed_ns=%llu: ok\n",threads,rounds,mode,policy,signals,(unsigned long long)elapsed);return 0;
}
