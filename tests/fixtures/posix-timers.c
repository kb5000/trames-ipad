// SPDX-License-Identifier: GPL-3.0-or-later
#define _GNU_SOURCE
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#ifndef __APPLE__
#include <sys/syscall.h>
#include <poll.h>
#include <sys/wait.h>
#endif
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL line=%d errno=%d: %s\n",__LINE__,errno,#x);exit(1);}}while(0)
static double now(void){struct timespec t;CHECK(clock_gettime(CLOCK_MONOTONIC,&t)==0);return t.tv_sec+t.tv_nsec/1e9;}
#ifndef __APPLE__
static _Atomic unsigned seen;
static _Atomic int last_code,last_value,last_tid;
static _Thread_local volatile sig_atomic_t local_seen;
static void caught(int sig,siginfo_t *info,void *p){(void)sig;(void)p;local_seen++;last_code=info->si_code;last_value=info->si_value.sival_int;last_tid=(sig_atomic_t)syscall(SYS_gettid);atomic_fetch_add(&seen,1);}
static void pause_ms(long ms){struct timespec t={.tv_sec=ms/1000,.tv_nsec=ms%1000*1000000};while(nanosleep(&t,&t)<0)CHECK(errno==EINTR);}
static void wait_signal(unsigned count){double until=now()+3;while(atomic_load(&seen)<count&&now()<until)pause_ms(1);CHECK(atomic_load(&seen)>=count);}
static void *parallel_timer(void *p){
    (void)p;local_seen=0;sigset_t blocked,empty;sigemptyset(&blocked);sigaddset(&blocked,SIGUSR1);sigemptyset(&empty);CHECK(pthread_sigmask(SIG_BLOCK,&blocked,NULL)==0);
    struct sigevent event={.sigev_notify=SIGEV_THREAD_ID,.sigev_signo=SIGUSR1};pid_t tid=(pid_t)syscall(SYS_gettid);memcpy((char*)&event+16,&tid,sizeof(tid));timer_t timer;
    CHECK(timer_create(CLOCK_MONOTONIC,&event,&timer)==0);struct itimerspec arm={.it_value={.tv_nsec=5000000}};CHECK(timer_settime(timer,0,&arm,NULL)==0);
    struct timespec timeout={.tv_sec=2};errno=0;CHECK(ppoll(NULL,0,&timeout,&empty)==-1&&errno==EINTR);CHECK(local_seen==1);CHECK(timer_delete(timer)==0);return NULL;
}
static void abi(const char *self){
    struct sigevent none={.sigev_notify=SIGEV_NONE};
    for(int clock=0;clock<=1;clock++){
        timer_t timer;CHECK(timer_create(clock,&none,&timer)==0);struct itimerspec old,arm={.it_value={.tv_sec=60}};
        CHECK(timer_gettime(timer,&old)==0&&!old.it_value.tv_sec&&!old.it_value.tv_nsec);CHECK(timer_settime(timer,0,&arm,&old)==0&&!old.it_value.tv_sec&&!old.it_value.tv_nsec);
        CHECK(timer_gettime(timer,&old)==0&&old.it_value.tv_sec>=59);CHECK(timer_getoverrun(timer)==0);
        struct timespec time;CHECK(clock_gettime(clock,&time)==0);arm.it_value=time;arm.it_value.tv_sec+=60;CHECK(timer_settime(timer,TIMER_ABSTIME,&arm,NULL)==0);
        arm=(struct itimerspec){.it_value={.tv_sec=60}};CHECK(timer_settime(timer,2,&arm,NULL)==0);CHECK(timer_gettime(timer,&old)==0&&old.it_value.tv_sec>=59);
        pid_t pid=fork();CHECK(pid>=0);if(pid==0){errno=0;CHECK(timer_gettime(timer,&old)==-1&&errno==EINVAL);_exit(0);}int status;CHECK(waitpid(pid,&status,0)==pid&&status==0);
        arm=(struct itimerspec){.it_value={.tv_nsec=1000000},.it_interval={.tv_nsec=1000000}};CHECK(timer_settime(timer,0,&arm,NULL)==0);pause_ms(20);
        CHECK(timer_getoverrun(timer)==0);CHECK(timer_gettime(timer,&old)==0&&!old.it_value.tv_sec&&old.it_value.tv_nsec>0&&old.it_value.tv_nsec<=1000000);
        CHECK(timer_delete(timer)==0);errno=0;CHECK(timer_gettime(timer,&old)==-1&&errno==EINVAL);
    }
    puts("{\"case\":\"posix-clock-abi-ownership-and-delete\",\"passed\":true}");
    struct sigaction action={.sa_sigaction=caught,.sa_flags=SA_SIGINFO|SA_RESTART};sigemptyset(&action.sa_mask);CHECK(sigaction(SIGUSR1,&action,NULL)==0);
    sigset_t set;sigemptyset(&set);sigaddset(&set,SIGUSR1);CHECK(pthread_sigmask(SIG_BLOCK,&set,NULL)==0);
    struct sigevent event={.sigev_notify=SIGEV_SIGNAL,.sigev_signo=SIGUSR1,.sigev_value.sival_int=0x1234};timer_t timer;CHECK(timer_create(CLOCK_MONOTONIC,&event,&timer)==0);
    struct itimerspec periodic={.it_interval={.tv_nsec=1000000},.it_value={.tv_nsec=1000000}};CHECK(timer_settime(timer,0,&periodic,NULL)==0);pause_ms(30);siginfo_t info;struct timespec zero={0};
    CHECK(sigtimedwait(&set,&info,&zero)==SIGUSR1&&info.si_code==SI_TIMER&&info.si_value.sival_int==0x1234&&info.si_overrun>=1);CHECK(timer_delete(timer)==0);
    CHECK(pthread_sigmask(SIG_UNBLOCK,&set,NULL)==0);puts("{\"case\":\"queued-timer-payload-and-overrun\",\"passed\":true}");
    atomic_store(&seen,0);event.sigev_notify=SIGEV_THREAD_ID;pid_t tid=(pid_t)syscall(SYS_gettid);memcpy((char*)&event+16,&tid,sizeof(tid));CHECK(timer_create(CLOCK_MONOTONIC,&event,&timer)==0);
    struct itimerspec once={.it_value={.tv_nsec=10000000}};CHECK(timer_settime(timer,0,&once,NULL)==0);wait_signal(1);CHECK(last_code==SI_TIMER&&last_value==0x1234&&last_tid==(pid_t)syscall(SYS_gettid));CHECK(timer_delete(timer)==0);
    puts("{\"case\":\"thread-target-handler-and-sigreturn\",\"passed\":true}");
    pthread_t threads[4];for(int i=0;i<4;i++)CHECK(pthread_create(&threads[i],NULL,parallel_timer,NULL)==0);for(int i=0;i<4;i++)CHECK(pthread_join(threads[i],NULL)==0);
    puts("{\"case\":\"four-thread-targeted-timers-and-masked-waits\",\"passed\":true}");
    CHECK(signal(SIGALRM,SIG_IGN)!=SIG_ERR);struct itimerval alarm={.it_value={.tv_sec=60}},out;CHECK(setitimer(ITIMER_REAL,&alarm,&out)==0);CHECK(getitimer(ITIMER_REAL,&out)==0&&out.it_value.tv_sec>=59);
    pid_t pid=fork();CHECK(pid>=0);if(pid==0){CHECK(getitimer(ITIMER_REAL,&out)==0&&!out.it_value.tv_sec&&!out.it_value.tv_usec);CHECK(setitimer(ITIMER_REAL,&alarm,NULL)==0);execl(self,self,"--after-exec",NULL);_exit(2);}int status;CHECK(waitpid(pid,&status,0)==pid&&status==0);
    alarm=(struct itimerval){0};CHECK(setitimer(ITIMER_REAL,&alarm,NULL)==0);puts("{\"case\":\"interval-timer-fork-and-exec\",\"passed\":true}");
}
#endif
struct Barrier{pthread_mutex_t lock;pthread_cond_t ready;unsigned count,total;};static struct Barrier start;
static void barrier(void){pthread_mutex_lock(&start.lock);if(++start.count==start.total)pthread_cond_broadcast(&start.ready);else while(start.count<start.total)pthread_cond_wait(&start.ready,&start.lock);pthread_mutex_unlock(&start.lock);}
struct Job{unsigned iterations;int kind;double seconds;};
static void *bench(void *p){struct Job *j=p;
#ifndef __APPLE__
    struct sigevent none={.sigev_notify=SIGEV_NONE};
    if(j->kind==4){none.sigev_notify=SIGEV_THREAD_ID;none.sigev_signo=SIGUSR1;pid_t tid=(pid_t)syscall(SYS_gettid);memcpy((char*)&none+16,&tid,sizeof(tid));}
    timer_t timer=0;struct itimerspec value={.it_value={.tv_sec=60}},out;if(j->kind!=3)CHECK(timer_create(CLOCK_MONOTONIC,&none,&timer)==0);
#else
    CHECK(j->kind==3);
#endif
    struct itimerval it;barrier();double began=now();
    for(unsigned i=0;i<j->iterations;i++){
        if(j->kind==3){CHECK(getitimer(ITIMER_REAL,&it)==0);}
#ifndef __APPLE__
        else if(j->kind==0){CHECK(timer_gettime(timer,&out)==0);}
        else if(j->kind==1||j->kind==4){CHECK(timer_settime(timer,0,&value,NULL)==0);}
        else{timer_t t;CHECK(timer_create(CLOCK_MONOTONIC,&none,&t)==0);CHECK(timer_delete(t)==0);}
#endif
    }
    j->seconds=now()-began;
#ifndef __APPLE__
    if(j->kind!=3)CHECK(timer_delete(timer)==0);
#endif
    return NULL;
}
int main(int argc,char **argv){
    if(argc==5&&!strcmp(argv[1],"--bench")){
        int kind=atoi(argv[2]);unsigned workers=(unsigned)atoi(argv[3]),iterations=(unsigned)atoi(argv[4]);CHECK(workers&&workers<=32&&iterations&&kind>=0&&kind<=4);
        start=(struct Barrier){.lock=PTHREAD_MUTEX_INITIALIZER,.ready=PTHREAD_COND_INITIALIZER,.total=workers};pthread_t threads[32];struct Job jobs[32];
        for(unsigned i=0;i<workers;i++){jobs[i]=(struct Job){.kind=kind,.iterations=iterations};CHECK(pthread_create(&threads[i],NULL,bench,&jobs[i])==0);}
        double maximum=0;for(unsigned i=0;i<workers;i++){CHECK(pthread_join(threads[i],NULL)==0);if(jobs[i].seconds>maximum)maximum=jobs[i].seconds;}
        printf("{\"kind\":%d,\"workers\":%u,\"iterations\":%u,\"longest_worker_ms\":%.6f,\"passed\":true}\n",kind,workers,iterations,maximum*1000);return 0;
    }
#ifndef __APPLE__
    if(argc==2&&!strcmp(argv[1],"--after-exec")){struct itimerval out;CHECK(getitimer(ITIMER_REAL,&out)==0&&out.it_value.tv_sec>=59);return 0;}
    abi(argv[0]);
#endif
    return 0;
}
