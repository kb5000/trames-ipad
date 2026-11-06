// SPDX-License-Identifier: GPL-3.0-or-later
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/fsuid.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"line %d: %s errno=%d\n",__LINE__,#x,errno); _exit(1); } } while (0)
#define DENIED(x,e) do { errno=0; CHECK((x)==-1 && errno==(e)); } while (0)
struct header { uint32_t version; int pid; };
struct word { uint32_t e,p,i; };
struct caps { uint64_t e,p,i; };
static char secret[128];
static unsigned threads=1, loops=16;
static int benchmark;
static uint64_t elapsed[12];
static uint64_t now(void) {
    struct timespec t; CHECK(clock_gettime(CLOCK_MONOTONIC,&t)==0);
    return (uint64_t)t.tv_sec*1000000000+t.tv_nsec;
}
static pthread_barrier_t barrier;
static struct caps getcaps(int pid) {
    struct header h={0x20080522,pid}; struct word d[2]; CHECK(syscall(SYS_capget,&h,d)==0);
    return (struct caps){d[0].e|((uint64_t)d[1].e<<32),d[0].p|((uint64_t)d[1].p<<32),d[0].i|((uint64_t)d[1].i<<32)};
}
static void putcaps(struct caps c) {
    struct header h={0x20080522,0}; struct word d[2]={{c.e,c.p,c.i},{c.e>>32,c.p>>32,c.i>>32}};
    CHECK(syscall(SYS_capset,&h,d)==0);
}
static long ctl(unsigned op, unsigned long a) { return prctl(op,a,0UL,0UL,0UL); }
static void run(void (*fn)(void)) {
    pid_t p=fork(); CHECK(p>=0); if (!p) { fn(); _exit(0); }
    int status; CHECK(waitpid(p,&status,0)==p); CHECK(WIFEXITED(status)&&WEXITSTATUS(status)==0);
}
static void ids(void) {
    CHECK(setfsuid(1000)==0); CHECK(setfsgid(1001)==0);
    CHECK(setfsuid(-1)==1000 && setfsuid(-1)==1000);
    CHECK(setfsgid(-1)==1001 && setfsgid(-1)==1001);
    CHECK(syscall(SYS_setresuid,-1,-1,-1)==0 && setfsuid(-1)==1000);
    CHECK(syscall(SYS_setresgid,-1,-1,-1)==0 && setfsgid(-1)==1001);
    CHECK(syscall(SYS_setreuid,-1,-1)==0 && setfsuid(-1)==0);
    CHECK(syscall(SYS_setregid,-1,-1)==0 && setfsgid(-1)==0);
    CHECK(syscall(SYS_setresgid,1000,1001,1002)==0);
    CHECK(syscall(SYS_setresuid,1000,1001,1002)==0);
    DENIED(syscall(SYS_setuid,1001),EPERM); DENIED(syscall(SYS_setgid,1001),EPERM);
    DENIED(syscall(SYS_setreuid,1002,-1),EPERM); DENIED(syscall(SYS_setregid,1002,-1),EPERM);
    CHECK(syscall(SYS_setreuid,-1,1000)==0 && syscall(SYS_setregid,-1,1000)==0);
    unsigned r,e,s; CHECK(getresuid(&r,&e,&s)==0 && r==1000 && e==1000 && s==1002);
    CHECK(getresgid(&r,&e,&s)==0 && r==1000 && e==1000 && s==1002);
}
static void groups(void) {
    gid_t in[]={11,7,11}, out[3]; CHECK(setgroups(3,in)==0);
    CHECK(getgroups(0,NULL)==3 && getgroups(3,out)==3);
    CHECK(out[0]==7 && out[1]==11 && out[2]==11);
    CHECK(syscall(SYS_getgroups,65537,out)==3);
    DENIED(syscall(SYS_getgroups,-1,out),EINVAL);
    gid_t bad=(gid_t)-1; DENIED(setgroups(1,&bad),EINVAL);
    long page=sysconf(_SC_PAGESIZE); char *p=mmap(NULL,page*2,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
    CHECK(p!=MAP_FAILED && mprotect(p+page,page,PROT_NONE)==0);
    *(gid_t *)(p+page-sizeof(gid_t))=25;
    DENIED(setgroups(2,(gid_t *)(p+page-sizeof(gid_t))),EFAULT);
    CHECK(getgroups(3,out)==3 && out[0]==7 && out[2]==11);
    CHECK(munmap(p,page*2)==0);
    int sockets[2]; CHECK(socketpair(AF_UNIX,SOCK_DGRAM,0,sockets)==0);
    int pass=1; CHECK(setsockopt(sockets[1],SOL_SOCKET,SO_PASSCRED,&pass,sizeof(pass))==0);
    CHECK(ctl(PR_SET_DUMPABLE,1)==0 && ctl(PR_SET_PDEATHSIG,SIGUSR1)==0);
    CHECK(syscall(SYS_setuid,1000)==0); int sig=-1;
    CHECK(ctl(PR_GET_DUMPABLE,0)!=1 && prctl(PR_GET_PDEATHSIG,&sig)==0 && sig==0);
    DENIED(open(secret,O_RDONLY),EACCES); DENIED(setgroups(0,NULL),EPERM);
    DENIED(syscall(SYS_setgroups,65537,NULL),EPERM);
    CHECK(send(sockets[0],"x",1,0)==1);
    char byte, control[CMSG_SPACE(sizeof(struct ucred))];
    struct iovec v={&byte,1}; struct msghdr msg={0};
    msg.msg_iov=&v;msg.msg_iovlen=1;msg.msg_control=control;msg.msg_controllen=sizeof(control);
    CHECK(recvmsg(sockets[1],&msg,0)==1);
    struct cmsghdr *h=CMSG_FIRSTHDR(&msg); CHECK(h && h->cmsg_level==SOL_SOCKET && h->cmsg_type==SCM_CREDENTIALS);
    struct ucred credentials;memcpy(&credentials,CMSG_DATA(h),sizeof(credentials));
    CHECK(credentials.pid==getpid() && credentials.uid==1000);
    close(sockets[0]);close(sockets[1]);
    CHECK(socketpair(AF_UNIX,SOCK_STREAM,0,sockets)==0);
    socklen_t length=sizeof(credentials);
    CHECK(getsockopt(sockets[0],SOL_SOCKET,SO_PEERCRED,&credentials,&length)==0 && credentials.uid==1000);
    close(sockets[0]);close(sockets[1]);
}
static void remote(void) {
    int ready[2], done[2]; CHECK(pipe(ready)==0 && pipe(done)==0);
    struct caps full=getcaps(0);
    pid_t p=fork(); CHECK(p>=0);
    if (!p) {
        CHECK(ctl(PR_SET_KEEPCAPS,1)==0 && syscall(SYS_setuid,1000)==0);
        char c='x'; CHECK(write(ready[1],&c,1)==1 && read(done[0],&c,1)==1); _exit(0);
    }
    char c; CHECK(read(ready[0],&c,1)==1);
    struct caps child=getcaps(p); CHECK(child.e==0 && child.p==full.p);
    struct header h={0x20080522,0x3fffffff};
    DENIED(syscall(SYS_capget,&h,(void *)1),ESRCH);
    CHECK(syscall(SYS_capget,&h,NULL)==0);
    struct caps drop=full; drop.e=0; putcaps(drop);
    DENIED(kill(p,0),EPERM);
    struct rlimit limit; DENIED(prlimit(p,RLIMIT_NOFILE,NULL,&limit),EPERM);
    putcaps(full); CHECK(kill(p,0)==0 && prlimit(p,RLIMIT_NOFILE,NULL,&limit)==0);
    CHECK(write(done[1],&c,1)==1);
    siginfo_t info; memset(&info,0,sizeof(info)); CHECK(waitid(P_PID,p,&info,WEXITED)==0);
    CHECK(info.si_pid==p && info.si_uid==1000 && info.si_code==CLD_EXITED && info.si_status==0);
    close(ready[0]);close(ready[1]);close(done[0]);close(done[1]);
}
static void *worker(void *arg) {
    unsigned id=(unsigned)(uintptr_t)arg; struct caps full=getcaps(0), drop=full; drop.e=0;
    int b=pthread_barrier_wait(&barrier); CHECK(b==0 || b==PTHREAD_BARRIER_SERIAL_THREAD);
    uint64_t began=now();
    for (unsigned i=0;i<loops;i++) {
        CHECK(syscall(SYS_setfsgid,2000+id)==0);
        if (!benchmark) CHECK(syscall(SYS_setfsgid,-1)==2000+id);
        CHECK(syscall(SYS_setfsgid,0)==2000+id);
        putcaps(drop); CHECK(getcaps(0).e==0);
        DENIED(syscall(SYS_setgid,3000),EPERM);
        putcaps(full); CHECK(getcaps(0).e==full.e);
        CHECK(getuid()==0 && getgid()==0);
    }
    elapsed[id]=now()-began;
    if (benchmark) return NULL;
    CHECK(ctl(PR_SET_KEEPCAPS,1)==0);
    CHECK(syscall(SYS_setgid,1000+id)==0 && syscall(SYS_setuid,1000+id)==0);
    CHECK(getuid()==1000+id && getgid()==1000+id);
    DENIED(open(secret,O_RDONLY),EACCES);
    return NULL;
}
static void parallel(void) {
    pthread_t t[12]; CHECK(pthread_barrier_init(&barrier,NULL,threads)==0);
    for(unsigned i=0;i<threads;i++) CHECK(pthread_create(&t[i],NULL,worker,(void *)(uintptr_t)i)==0);
    for(unsigned i=0;i<threads;i++) CHECK(pthread_join(t[i],NULL)==0);
    CHECK(getuid()==0 && getgid()==0 && getcaps(0).e!=0);
    CHECK(pthread_barrier_destroy(&barrier)==0);
}
struct instruction { uint16_t code; uint8_t yes,no; uint32_t value; };
struct program { unsigned short count; struct instruction *instructions; };
int main(int argc,char **argv) {
    if (argc>1 && !strcmp(argv[1],"bench")) { benchmark=1; argc--; argv++; }
    if (argc>1) threads=(unsigned)strtoul(argv[1],NULL,10);
    if (argc>2) loops=(unsigned)strtoul(argv[2],NULL,10);
    CHECK(threads>=1 && threads<=12 && loops>0 && getuid()==0);
    CHECK(mkdir("/tmp",01777)==0 || errno==EEXIST);
    snprintf(secret,sizeof(secret),"/tmp/trames-credentials-%d",getpid());
    int fd=open(secret,O_CREAT|O_EXCL|O_WRONLY,0600); CHECK(fd>=0); CHECK(close(fd)==0);
    if(argc>3) { struct instruction i={6,0,0,0x7fff0000};struct program p={1,&i};
        CHECK(ctl(PR_SET_NO_NEW_PRIVS,1)==0 && prctl(PR_SET_SECCOMP,2,&p)==0); }
    if (benchmark) parallel();
    else { run(ids); run(groups); run(remote); run(parallel); }
    CHECK(unlink(secret)==0);
    uint64_t longest=0; for(unsigned i=0;i<threads;i++) if(elapsed[i]>longest) longest=elapsed[i];
    printf("{\"threads\":%u,\"loops\":%u,\"cases\":4,\"status\":\"pass\",\"seconds\":%.9f}\n",threads,loops,(double)longest/1e9);
    return 0;
}
