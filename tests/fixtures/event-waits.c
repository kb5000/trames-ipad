// SPDX-License-Identifier: GPL-3.0-or-later
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#ifndef __APPLE__
#include <sys/epoll.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <ucontext.h>
#endif
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL line=%d errno=%d: %s\n",__LINE__,errno,#x);exit(1);}}while(0)
static double now(void){struct timespec t;CHECK(clock_gettime(CLOCK_MONOTONIC,&t)==0);return (double)t.tv_sec+(double)t.tv_nsec/1e9;}
#ifndef __APPLE__
static void nap(long ns){struct timespec t={.tv_nsec=ns};while(nanosleep(&t,&t)<0)CHECK(errno==EINTR);}
#endif
static void write_byte(int fd){CHECK(write(fd,"x",1)==1);}
static void read_byte(int fd){char c;CHECK(read(fd,&c,1)==1&&c=='x');}
struct Wait{int kind,fd,ep,masked;};
#ifndef __APPLE__
static long wait_raw(struct Wait w,struct timespec *timeout,uint64_t *mask){
    struct{uint64_t *address;size_t size;} pointer={mask,8};
    if(w.kind==0){fd_set set;FD_ZERO(&set);FD_SET(w.fd,&set);return syscall(SYS_pselect6,w.fd+1,&set,NULL,NULL,timeout,&pointer);}
    if(w.kind==1){struct pollfd{int fd;short events,revents;} row={w.fd,1,0};return syscall(SYS_ppoll,&row,1,timeout,mask,8);}
    struct epoll_event events[2];
    if(w.kind==2){int ms=timeout?(int)(timeout->tv_sec*1000+timeout->tv_nsec/1000000):-1;return syscall(SYS_epoll_pwait,w.ep,events,2,ms,mask,8);}
    return syscall(SYS_epoll_pwait2,w.ep,events,2,timeout,mask,8);
}
static void setup(struct Wait *w,int fd,int kind){
    *w=(struct Wait){.kind=kind,.fd=fd,.ep=-1};
    if(kind>=2){w->ep=epoll_create1(0);CHECK(w->ep>=0);struct epoll_event e={.events=EPOLLIN,.data.u64=42};CHECK(epoll_ctl(w->ep,EPOLL_CTL_ADD,fd,&e)==0);}
}
static _Atomic int seen;
static volatile sig_atomic_t handler_usr2,frame_usr1,handler_usr1;
static void caught(int sig,siginfo_t *info,void *context){
    (void)sig;(void)info;sigset_t mask;CHECK(sigprocmask(SIG_SETMASK,NULL,&mask)==0);
    handler_usr2=sigismember(&mask,SIGUSR2);handler_usr1=sigismember(&mask,SIGUSR1);
    frame_usr1=sigismember(&((ucontext_t*)context)->uc_sigmask,SIGUSR1);atomic_fetch_add_explicit(&seen,1,memory_order_relaxed);
}
static void masks(int restore){sigset_t old;sigemptyset(&old);if(!restore)sigaddset(&old,SIGUSR1);CHECK(sigprocmask(SIG_SETMASK,&old,NULL)==0);}
static void abi(void){
    CHECK(mkdir("/tmp",0777)==0||errno==EEXIST);
    struct sigaction action={0};action.sa_sigaction=caught;action.sa_flags=SA_SIGINFO|SA_RESTART;sigemptyset(&action.sa_mask);CHECK(sigaction(SIGUSR1,&action,NULL)==0);
    for(int kind=0;kind<4;kind++){
        int p[2];CHECK(pipe(p)==0);struct Wait w;setup(&w,p[0],kind);
        uint64_t mask=1ull<<(SIGUSR2-1);struct timespec t={0};CHECK(wait_raw(w,&t,&mask)==0);
        t=(struct timespec){.tv_nsec=10000000};CHECK(wait_raw(w,&t,&mask)==0);if(kind<2)CHECK(t.tv_sec==0&&t.tv_nsec==0);else if(kind==3)CHECK(t.tv_nsec==10000000);
        write_byte(p[1]);t=(struct timespec){.tv_sec=1};CHECK(wait_raw(w,&t,&mask)==1);read_byte(p[0]);
        masks(0);atomic_store(&seen,0);CHECK(raise(SIGUSR1)==0);CHECK(atomic_load(&seen)==0);
        t=(struct timespec){.tv_sec=1};errno=0;CHECK(wait_raw(w,&t,&mask)==-1&&errno==EINTR);CHECK(atomic_load(&seen)==1&&handler_usr1==1&&handler_usr2==1&&frame_usr1==1);
        sigset_t current;CHECK(sigprocmask(SIG_SETMASK,NULL,&current)==0);CHECK(sigismember(&current,SIGUSR1)==1&&sigismember(&current,SIGUSR2)==0);masks(1);
        if(w.ep>=0){close(w.ep);}close(p[0]);close(p[1]);
        printf("{\"case\":\"abi-mask-and-timeout\",\"kind\":%d,\"passed\":true}\n",kind);
    }
}
struct Producer{int fd;};
static void *produce(void *opaque){struct Producer *p=opaque;nap(30000000);write_byte(p->fd);return NULL;}
static void async_masks(void){
    for(int kind=0;kind<4;kind++){
        int p[2],target[2];CHECK(pipe(p)==0&&pipe(target)==0);struct Wait w;setup(&w,target[0],kind);
        struct f_owner_ex owner={.type=F_OWNER_TID,.pid=(pid_t)syscall(SYS_gettid)};CHECK(fcntl(p[0],F_SETOWN_EX,&owner)==0);CHECK(fcntl(p[0],F_SETSIG,SIGUSR1)==0);CHECK(fcntl(p[0],F_SETFL,O_ASYNC|O_NONBLOCK)==0);
        for(int pending=0;pending<2;pending++){
            masks(0);atomic_store(&seen,0);pthread_t thread;struct Producer producer={p[1]};
            if(pending)write_byte(p[1]);else CHECK(pthread_create(&thread,NULL,produce,&producer)==0);
            uint64_t mask=1ull<<(SIGUSR2-1);struct timespec t={.tv_sec=2};errno=0;
            CHECK(wait_raw(w,&t,&mask)==-1&&errno==EINTR);CHECK(atomic_load(&seen)==1&&handler_usr2==1&&frame_usr1==1);
            if(!pending){CHECK(pthread_join(thread,NULL)==0);}read_byte(p[0]);masks(1);
        }
        if(w.ep>=0){close(w.ep);}close(p[0]);close(p[1]);close(target[0]);close(target[1]);
        printf("{\"case\":\"async-signal-pending-and-arriving\",\"kind\":%d,\"passed\":true}\n",kind);
    }
}
static void timeout_copyout(void){
    /* Final raw remaining-time copyout is best effort on native Linux. */
    int p[2];CHECK(pipe(p)==0);
    for(int kind=0;kind<2;kind++){
        struct timespec *t=mmap(NULL,4096,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);CHECK(t!=MAP_FAILED);*t=(struct timespec){.tv_nsec=1000000};CHECK(mprotect(t,4096,PROT_READ)==0);
        struct Wait w;setup(&w,p[0],kind);write_byte(p[1]);CHECK(wait_raw(w,t,NULL)==1);read_byte(p[0]);CHECK(t->tv_nsec==1000000);
        CHECK(wait_raw(w,t,NULL)==0&&t->tv_nsec==1000000);
        masks(0);CHECK(raise(SIGUSR1)==0);uint64_t mask=1ull<<(SIGUSR2-1);errno=0;CHECK(wait_raw(w,t,&mask)==-1&&errno==EINTR);masks(1);
        CHECK(munmap(t,4096)==0);
    }
    close(p[0]);close(p[1]);puts("{\"case\":\"raw-timeout-copyout-is-best-effort\",\"passed\":true}");
}

#else
static long wait_raw(struct Wait w,struct timespec *timeout,uint64_t *bits){
    CHECK(w.kind==0);fd_set set;FD_ZERO(&set);FD_SET(w.fd,&set);sigset_t mask=0;
    if(bits&&(*bits&(1ull<<(SIGUSR2-1))))sigaddset(&mask,SIGUSR2);
    return pselect(w.fd+1,&set,NULL,NULL,timeout,bits?&mask:NULL);
}
static void setup(struct Wait *w,int fd,int kind){CHECK(kind==0);*w=(struct Wait){.kind=kind,.fd=fd,.ep=-1};}
#endif
struct Barrier{pthread_mutex_t lock;pthread_cond_t ready;unsigned count,total;};
static struct Barrier start;
static void barrier(void){pthread_mutex_lock(&start.lock);if(++start.count==start.total)pthread_cond_broadcast(&start.ready);else while(start.count<start.total)pthread_cond_wait(&start.ready,&start.lock);pthread_mutex_unlock(&start.lock);}
struct Job{int kind,masked;unsigned iterations;double seconds;};
static void *benchmark(void *opaque){
    struct Job *j=opaque;int p[2];CHECK(pipe(p)==0);struct Wait w;setup(&w,p[0],j->kind);uint64_t mask=1ull<<(SIGUSR2-1);struct timespec zero={0};barrier();double began=now();
    for(unsigned i=0;i<j->iterations;i++){write_byte(p[1]);CHECK(wait_raw(w,&zero,j->masked?&mask:NULL)==1);read_byte(p[0]);}
    j->seconds=now()-began;if(w.ep>=0){close(w.ep);}close(p[0]);close(p[1]);return NULL;
}
int main(int argc,char **argv){
    if(argc==6&&!strcmp(argv[1],"--bench")){
        int kind=atoi(argv[2]),masked=atoi(argv[3]);unsigned workers=(unsigned)atoi(argv[4]),iterations=(unsigned)atoi(argv[5]);CHECK(workers&&workers<=32&&iterations);
        start=(struct Barrier){.lock=PTHREAD_MUTEX_INITIALIZER,.ready=PTHREAD_COND_INITIALIZER,.total=workers};pthread_t threads[32];struct Job jobs[32];
        for(unsigned i=0;i<workers;i++){jobs[i]=(struct Job){.kind=kind,.masked=masked,.iterations=iterations};CHECK(pthread_create(&threads[i],NULL,benchmark,&jobs[i])==0);}
        double maximum=0;for(unsigned i=0;i<workers;i++){CHECK(pthread_join(threads[i],NULL)==0);if(jobs[i].seconds>maximum)maximum=jobs[i].seconds;}
        printf("{\"kind\":%d,\"masked\":%d,\"workers\":%u,\"iterations\":%u,\"longest_worker_ms\":%.6f,\"passed\":true}\n",kind,masked,workers,iterations,maximum*1000);return 0;
    }
#ifndef __APPLE__
    abi();async_masks();timeout_copyout();
#else
    puts("Darwin reference supports native pselect only; other families use native Linux.");
#endif
    return 0;
}
