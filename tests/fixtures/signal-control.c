// SPDX-License-Identifier: GPL-3.0-or-later
#define _GNU_SOURCE
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#ifndef __APPLE__
#include <sys/signalfd.h>
#endif
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"line %d: %s errno=%d\n",__LINE__,#x,errno); exit(1); } } while (0)
static pthread_mutex_t lock=PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t ready=PTHREAD_COND_INITIALIZER;
static unsigned arrived,total;
static double now(void) { struct timespec t; CHECK(clock_gettime(CLOCK_MONOTONIC,&t)==0); return t.tv_sec+t.tv_nsec/1e9; }
static void barrier(void) { CHECK(pthread_mutex_lock(&lock)==0); if(++arrived==total)CHECK(pthread_cond_broadcast(&ready)==0); else while(arrived<total)CHECK(pthread_cond_wait(&ready,&lock)==0); CHECK(pthread_mutex_unlock(&lock)==0); }
struct Job { unsigned iterations; int kind; double seconds; };
static void *bench(void *p) {
    struct Job *j=p;
    sigset_t set,alternate,old; sigemptyset(&set); sigaddset(&set,SIGUSR1); alternate=set; sigaddset(&alternate,SIGUSR2);
    CHECK(pthread_sigmask(SIG_BLOCK,&alternate,NULL)==0);
    struct sigaction action={.sa_handler=SIG_IGN},out; sigemptyset(&action.sa_mask);
    int fd=-1;
#ifndef __APPLE__
    if(j->kind==5) { fd=signalfd(-1,&set,0); CHECK(fd>=0); }
#else
    CHECK(j->kind<5);
#endif
    barrier(); double began=now();
    for(unsigned i=0;i<j->iterations;i++) {
        if(j->kind==0) { CHECK(pthread_sigmask(SIG_SETMASK,NULL,&old)==0); CHECK(sigismember(&old,SIGUSR1)==1); }
        else if(j->kind==1) { CHECK(pthread_sigmask(SIG_SETMASK,i&1?&set:&alternate,NULL)==0); }
        else if(j->kind==2) { CHECK(sigaction(SIGUSR2,NULL,&out)==0 && out.sa_handler==SIG_IGN); }
        else if(j->kind==3) { CHECK(sigaction(SIGUSR2,&action,NULL)==0); }
        else { CHECK(pthread_kill(pthread_self(),SIGUSR1)==0);
            if(j->kind==4) { int sig=0; CHECK(sigwait(&set,&sig)==0 && sig==SIGUSR1); }
#ifndef __APPLE__
            else { struct signalfd_siginfo info; CHECK(read(fd,&info,sizeof info)==(ssize_t)sizeof info && info.ssi_signo==SIGUSR1); }
#endif
        }
    }
    j->seconds=now()-began; if(fd>=0)CHECK(close(fd)==0); return NULL;
}
int main(int argc,char **argv) {
    CHECK(argc==5 && !strcmp(argv[1],"--bench"));
    int kind=atoi(argv[2]); unsigned workers=(unsigned)atoi(argv[3]),iterations=(unsigned)atoi(argv[4]);
    CHECK(kind>=0 && kind<=5 && workers>0 && workers<=32 && iterations>0);
    sigset_t blocked; sigemptyset(&blocked); sigaddset(&blocked,SIGUSR1); sigaddset(&blocked,SIGUSR2); CHECK(pthread_sigmask(SIG_BLOCK,&blocked,NULL)==0);
    struct sigaction action={.sa_handler=SIG_IGN}; sigemptyset(&action.sa_mask); CHECK(sigaction(SIGUSR2,&action,NULL)==0);
    pthread_t threads[32]; struct Job jobs[32]; total=workers;
    for(unsigned i=0;i<workers;i++) { jobs[i]=(struct Job){.iterations=iterations,.kind=kind}; CHECK(pthread_create(&threads[i],NULL,bench,&jobs[i])==0); }
    double longest=0; for(unsigned i=0;i<workers;i++) { CHECK(pthread_join(threads[i],NULL)==0); if(jobs[i].seconds>longest)longest=jobs[i].seconds; }
    printf("{\"kind\":%d,\"workers\":%u,\"iterations\":%u,\"longest_worker_ms\":%.6f,\"passed\":true}\n",kind,workers,iterations,longest*1000); return 0;
}
