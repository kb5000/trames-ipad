// SPDX-License-Identifier: GPL-3.0-or-later
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <pthread.h>
#include <sched.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/epoll.h>
#include <sys/uio.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <unistd.h>
enum { PAGE=4096, API=0xc018aa3fU, REGISTER=0xc020aa00U, UNREGISTER=0x8010aa01U,
       WAKE=0x8010aa02U, COPY=0xc028aa03U, ZERO=0xc020aa04U, CONTINUE=0xc020aa07U };
struct api { uint64_t api,features,ioctls; };
struct range { uint64_t start,len; };
struct reg { struct range range; uint64_t mode,ioctls; };
struct copy { uint64_t dst,src,len,mode; int64_t copied; };
struct zero { struct range range; uint64_t mode; int64_t copied; };
static void check(int ok,const char *what) { if(!ok) { fprintf(stderr,"FAIL %s errno=%d (%s)\n",what,errno,strerror(errno)); exit(1); } }
static unsigned char *map(size_t size) { void *p=mmap(NULL,size,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0); check(p!=MAP_FAILED,"mmap"); return p; }
static int uffd(void) {
    int fd=syscall(282,1|O_CLOEXEC|O_NONBLOCK); check(fd>=0,"userfaultfd");
    check((fcntl(fd,F_GETFD)&FD_CLOEXEC)&&(fcntl(fd,F_GETFL)&O_NONBLOCK),"userfaultfd flags");
    check((fcntl(fd,F_GETFL)&O_ACCMODE)==O_RDONLY,"read-only userfault OFD");
    errno=0;check(write(fd,"",0)==-1&&errno==EBADF,"read-only userfault write");
    check(lseek(fd,17,SEEK_SET)==0,"userfault noop seek");
    struct api api={.api=0xaa};check(ioctl(fd,API,&api)==0&&api.api==0xaa&&(api.ioctls&3)==3,"UFFDIO_API");
    return fd;
}
static void reg(int fd,void *p,size_t size) { struct reg r={{(uintptr_t)p,size},1,0};check(ioctl(fd,REGISTER,&r)==0&&(r.ioctls&28)==28,"REGISTER missing"); }
struct access { volatile unsigned char *page; int write; unsigned char value; atomic_int done; };
static void *access_page(void *raw) { struct access *a=raw;if(a->write)*a->page=0xcc;else a->value=*a->page;atomic_store(&a->done,1);return NULL; }
static void event(int fd,uintptr_t page,int write) {
    struct pollfd p={.fd=fd,.events=POLLIN};check(poll(&p,1,2000)==1&&(p.revents&POLLIN),"poll missing event");
    int ep=epoll_create1(EPOLL_CLOEXEC);check(ep>=0,"userfault epoll");struct epoll_event ev={.events=EPOLLIN|EPOLLET,.data.u64=77};
    check(!epoll_ctl(ep,EPOLL_CTL_ADD,fd,&ev)&&epoll_wait(ep,&ev,1,1000)==1&&(ev.events&EPOLLIN)&&ev.data.u64==77,"userfault edge epoll");close(ep);
    uint64_t msg[4];static unsigned reads;struct iovec split[]={{msg,16},{msg+2,16}};
    check((++reads&1 ? readv(fd,split,2) : read(fd,msg,sizeof(msg)))==32,"read/readv complete missing event");
    check((msg[0]&255)==0x12&&msg[1]==(uint64_t)write&&msg[2]==page,"original missing address and access flags");
}
static void probes(void) {
    errno=0;check(syscall(282,1|O_DIRECT)==-1&&errno==EINVAL,"invalid creation flags");
    int fd=syscall(282,1|O_NONBLOCK);check(fd>=0,"probe fd");
    struct pollfd pollfd={.fd=fd,.events=POLLIN};check(poll(&pollfd,1,0)==1&&(pollfd.revents&POLLERR),"uninitialized poll");
    struct api api={.api=0xaa,.features=1ULL<<63,.ioctls=77};
    errno=0;check(ioctl(fd,API,&api)==-1&&errno==EINVAL&&!api.api&&!api.features&&!api.ioctls,"unknown API feature clears response");
    api=(struct api){.api=0xaa};check(!ioctl(fd,API,&api),"probe API");
    struct reg r={{0,0},1,0};struct copy c={0};struct zero z={0};struct range range={0};
    unsigned requests[]={REGISTER,COPY,ZERO,CONTINUE,WAKE,UNREGISTER};
    void *args[]={&r,&c,&z,&z,&range,&range};
    for(int i=0;i<6;i++){errno=0;check(ioctl(fd,requests[i],args[i])==-1&&errno==EINVAL,"original Android 13 ART zero-range probe");}
    close(fd);
}
static void missing(void) {
    unsigned char *pages=map(PAGE*4),*source=map(PAGE*2);memset(source,0x5a,PAGE*2);
    int fd=uffd(),duplicate=dup(fd);check(duplicate>=0,"dup uffd");reg(fd,pages,PAGE*4);
    struct access read_access={.page=pages};pthread_t thread;
    check(!pthread_create(&thread,NULL,access_page,&read_access),"reader thread");event(duplicate,(uintptr_t)pages,0);
    struct copy c={(uintptr_t)pages,(uintptr_t)source+1,PAGE,0,0};check(!ioctl(duplicate,COPY,&c)&&c.copied==PAGE,"unaligned-source COPY");
    check(!pthread_join(thread,NULL)&&read_access.value==0x5a,"reader resumes copied bytes");
    struct access write_access={.page=pages+PAGE,.write=1};check(!pthread_create(&thread,NULL,access_page,&write_access),"writer thread");event(fd,(uintptr_t)pages+PAGE,1);
    struct zero z={{(uintptr_t)pages+PAGE,PAGE},1,0};check(!ioctl(fd,ZERO,&z)&&z.copied==PAGE,"ZEROPAGE DONTWAKE");
    usleep(20000);check(!atomic_load(&write_access.done),"DONTWAKE retains original waiter");
    struct range range={(uintptr_t)pages+PAGE,PAGE};check(!ioctl(fd,WAKE,&range),"WAKE");
    check(!pthread_join(thread,NULL)&&pages[PAGE]==0xcc,"writer resumes zero page");
    pid_t child=fork();check(child>=0,"fork");if(!child){pages[PAGE*2]=7;_exit(0);}int status;check(waitpid(child,&status,0)==child&&WIFEXITED(status)&&!WEXITSTATUS(status),"fork child is unregistered");
    close(fd);
    struct access close_access={.page=pages+PAGE*2};check(!pthread_create(&thread,NULL,access_page,&close_access),"close waiter");event(duplicate,(uintptr_t)pages+PAGE*2,0);
    close(duplicate);check(!pthread_join(thread,NULL)&&close_access.value==0,"last OFD close releases missing waiter");
    munmap(pages,PAGE*4);munmap(source,PAGE*2);
}
static void transfer(void) {
    unsigned char *page=map(PAGE);int fd=uffd(),pair[2];reg(fd,page,PAGE);check(!socketpair(AF_UNIX,SOCK_STREAM,0,pair),"SCM socketpair");
    pid_t child=fork();check(child>=0,"handler fork");
    if(!child){
        close(pair[0]);close(fd);char byte,control[CMSG_SPACE(sizeof(int))];struct iovec v={&byte,1};
        struct msghdr msg={.msg_iov=&v,.msg_iovlen=1,.msg_control=control,.msg_controllen=sizeof(control)};
        check(recvmsg(pair[1],&msg,0)==1,"receive original uffd");int received;memcpy(&received,CMSG_DATA(CMSG_FIRSTHDR(&msg)),sizeof(received));
        event(received,(uintptr_t)page,0);unsigned char *source=map(PAGE);memset(source,0x43,PAGE);
        struct copy c={(uintptr_t)page,(uintptr_t)source,PAGE,0,0};check(!ioctl(received,COPY,&c)&&c.copied==PAGE,"foreign process resolves original mm");close(received);_exit(0);
    }
    close(pair[1]);char byte='f',control[CMSG_SPACE(sizeof(int))]={0};struct iovec v={&byte,1};struct msghdr msg={.msg_iov=&v,.msg_iovlen=1,.msg_control=control,.msg_controllen=sizeof(control)};
    struct cmsghdr *h=CMSG_FIRSTHDR(&msg);h->cmsg_level=SOL_SOCKET;h->cmsg_type=SCM_RIGHTS;h->cmsg_len=CMSG_LEN(sizeof(int));memcpy(CMSG_DATA(h),&fd,sizeof(fd));check(sendmsg(pair[0],&msg,0)==1,"send original uffd");
    check(*(volatile unsigned char *)page==0x43,"foreign handler changed original bytes");int status;check(waitpid(child,&status,0)==child&&WIFEXITED(status)&&!WEXITSTATUS(status),"foreign handler exit");close(fd);close(pair[0]);munmap(page,PAGE);
}
static void same_page(void) {
    unsigned char *page=map(PAGE);int fd=uffd();reg(fd,page,PAGE);
    pthread_t first,second;struct access reader={.page=page},writer={.page=page,.write=1};
    check(!pthread_create(&first,NULL,access_page,&reader),"same-page reader");event(fd,(uintptr_t)page,0);
    check(!pthread_create(&second,NULL,access_page,&writer),"same-page writer");event(fd,(uintptr_t)page,1);
    struct zero z={{(uintptr_t)page,PAGE},1,0};check(!ioctl(fd,ZERO,&z),"same-page DONTWAKE");
    usleep(20000);check(!atomic_load(&reader.done)&&!atomic_load(&writer.done),"both same-page waiters stay parked");
    struct range r={(uintptr_t)page,PAGE};check(!ioctl(fd,WAKE,&r),"same-page WAKE");
    check(!pthread_join(first,NULL)&&!pthread_join(second,NULL)&&page[0]==0xcc,"both same-page waiters resume");
    close(fd);munmap(page,PAGE);
}
static void kernel_copy(void) {
    pid_t child=fork();check(child>=0,"kernel-copy fork");
    if(!child){int fd=uffd(),p[2];unsigned char *page=map(PAGE);reg(fd,page,PAGE);check(!pipe(p),"kernel-copy pipe");errno=0;check(write(p[1],page,1)==-1&&errno==EFAULT,"USER_MODE_ONLY kernel copy");_exit(0);}
    int status;check(waitpid(child,&status,0)==child&&WIFEXITED(status)&&!WEXITSTATUS(status),"USER_MODE_ONLY preserves kernel EFAULT");
}
struct shared_args { int ready; };
static int shared_child(void *raw) {struct shared_args *args=raw;check(write(args->ready,"r",1)==1,"shared child ready");for(;;)pause();return 0;}
static void shared_release(void) {
    unsigned char *stack=map(65536),*protected=map(PAGE);protected[0]=0xa7;int ready[2];check(!pipe(ready),"shared reaper pipe");
    struct shared_args args={ready[1]};pid_t child=clone(shared_child,stack+65536,CLONE_VM|SIGCHLD,&args);check(child>0,"CLONE_VM process");close(ready[1]);char byte;check(read(ready[0],&byte,1)==1,"shared mm ready");
    int fd=syscall(434,child,0);check(fd>=0,"shared pidfd");check(!syscall(424,fd,SIGKILL,NULL,0),"shared pidfd kill");errno=0;check(syscall(448,fd,0)==-1&&(errno==EINVAL||errno==ESRCH),"live peer prevents shared-mm reaping");
    int status;check(waitpid(child,&status,0)==child&&WIFSIGNALED(status),"shared child exits");check(protected[0]==0xa7,"live peer bytes survive");close(fd);close(ready[0]);munmap(protected,PAGE);munmap(stack,65536);
}
static void release(void) {
    errno=0;check(syscall(448,-1,0)==-1&&errno==EBADF,"lmkd reaper support probe");
    errno=0;check(syscall(448,-1,1)==-1&&errno==EINVAL,"reaper flags before fd lookup");
    int wrong=open("/dev/null",O_RDONLY);check(wrong>=0,"non-pidfd descriptor");errno=0;check(syscall(448,wrong,0)==-1&&errno==EBADF,"wrong OFD is not a pidfd");close(wrong);
    int fd=syscall(434,getpid(),0);check(fd>=0,"self pidfd");errno=0;check(syscall(448,fd,0)==-1&&errno==EINVAL,"live mm cannot be reaped");close(fd);
    int ready[2];check(!pipe(ready),"reaper pipe");pid_t child=fork();check(child>=0,"reaper fork");
    if(!child){close(ready[0]);unsigned char *pages=map(16*1024*1024);for(int i=0;i<16*1024*1024;i+=PAGE)pages[i]=1;check(write(ready[1],"r",1)==1,"child resident");for(;;)pause();}
    close(ready[1]);char byte;check(read(ready[0],&byte,1)==1,"child allocation ready");fd=syscall(434,child,0);check(fd>=0,"child pidfd");check(!syscall(424,fd,SIGKILL,NULL,0),"original lmkd pidfd SIGKILL");
    errno=0;int reaped=syscall(448,fd,0),error=errno;check(reaped==0||(reaped==-1&&error==ESRCH),"dying mm is reaped or already released");
    int status;check(waitpid(child,&status,0)==child&&WIFSIGNALED(status)&&WTERMSIG(status)==SIGKILL,"child killed");errno=0;check(syscall(448,fd,0)==-1&&errno==ESRCH,"reaped pidfd cannot select a new mm");
    close(fd);close(ready[0]);printf("process_mrelease: result=%d errno=%d\n",reaped,error);
}
static void pressure(void) {
    int fd=open("/proc/pressure/memory",O_RDWR|O_NONBLOCK);check(fd>=0,"memory PSI open");
    char stats[512]={0};check(read(fd,stats,sizeof(stats)-1)>0&&strstr(stats,"some avg10=")&&strstr(stats,"full avg10="),"memory PSI statistics");
    struct pollfd p={.fd=fd,.events=POLLPRI};check(poll(&p,1,0)==1&&(p.revents&(POLLPRI|POLLERR))==(POLLPRI|POLLERR),"unconfigured PSI poll");
    const char *invalid="some 0 2000000";errno=0;check(write(fd,invalid,strlen(invalid))==-1&&errno==EINVAL,"invalid PSI threshold");
    const char *trigger="some 2000000 2000000\n";check(write(fd,trigger,strlen(trigger))==(ssize_t)strlen(trigger),"original PSI trigger");
    int alias=dup(fd);check(alias>=0,"PSI alias");close(fd);errno=0;check(write(alias,trigger,strlen(trigger))==-1&&errno==EBUSY,"PSI trigger belongs to original OFD");
    check(lseek(alias,0,SEEK_SET)==0&&read(alias,stats,sizeof(stats))>0,"PSI seek and read through alias");
    int peer=open("/proc/pressure/memory",O_RDWR);check(peer>=0&&write(peer,trigger,strlen(trigger))==(ssize_t)strlen(trigger),"independent PSI monitor");close(peer);close(alias);
}
int main(void){setvbuf(stdout,NULL,_IONBF,0);alarm(20);pressure();puts("PASS PSI resource ABI");probes();puts("PASS ART probes");missing();same_page();puts("PASS missing, same-page waiters and close/fork");transfer();puts("PASS original-mm SCM_RIGHTS");kernel_copy();shared_release();puts("PASS kernel copy and live CLONE_VM peer");release();puts("PASS Android memory lifecycle: ART probes, anonymous missing faults, COPY/ZERO/WAKE, close/fork/SCM_RIGHTS, lmkd reaper");return 0;}
