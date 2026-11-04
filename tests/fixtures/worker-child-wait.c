// SPDX-License-Identifier: GPL-3.0-or-later
#define _GNU_SOURCE
#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
static pthread_barrier_t gate;
static pid_t target;
static unsigned rounds;
static int use_waitid;
static void barrier(void) { int rc=pthread_barrier_wait(&gate); assert(!rc || rc==PTHREAD_BARRIER_SERIAL_THREAD); }
static void *poll_child(void *unused) {
    (void)unused; barrier();
    for(unsigned i=0;i<rounds;i++) {
        if(use_waitid) { siginfo_t info; memset(&info,255,sizeof info); assert(!waitid(P_PID,target,&info,WEXITED|WNOHANG)); assert(info.si_pid==0); }
        else { int status=0x12345678; assert(waitpid(target,&status,WNOHANG)==0); assert(status==0x12345678); }
    }
    barrier(); return NULL;
}
static uint64_t ns(void) { struct timespec t; assert(!clock_gettime(CLOCK_MONOTONIC,&t)); return (uint64_t)t.tv_sec*1000000000+t.tv_nsec; }
static void benchmark(int threads) {
    int release[2]; assert(!pipe(release));
    target=fork(); assert(target>=0);
    if(!target) { char byte; close(release[1]); assert(read(release[0],&byte,1)==1); _exit(23); }
    close(release[0]);
    assert(!pthread_barrier_init(&gate,NULL,threads+1)); pthread_t workers[32];
    for(int i=0;i<threads;i++) assert(!pthread_create(workers+i,NULL,poll_child,NULL));
    uint64_t start=ns(); barrier(); barrier(); uint64_t elapsed=ns()-start;
    for(int i=0;i<threads;i++) assert(!pthread_join(workers[i],NULL));
    assert(write(release[1],"x",1)==1); close(release[1]);
    int status; assert(waitpid(target,&status,0)==target && WIFEXITED(status) && WEXITSTATUS(status)==23);
    errno=0; assert(kill(target,0)==-1 && errno==ESRCH);
    printf("{\"mode\":\"%s\",\"threads\":%d,\"calls\":%u,\"elapsed_ns\":%llu}\n",use_waitid?"waitid":"wait4",threads,rounds*threads,(unsigned long long)elapsed);
}
static void compatibility(void) {
    int release[2]; assert(!pipe(release));
    pid_t child=fork(); assert(child>=0);
    if(!child) { char byte; close(release[1]); raise(SIGSTOP); assert(read(release[0],&byte,1)==1); _exit(37); }
    close(release[0]); siginfo_t info={0}; int status;
    assert(!waitid(P_PID,child,&info,WSTOPPED|WNOWAIT)); assert(info.si_pid==child && info.si_code==CLD_STOPPED && info.si_status==SIGSTOP);
    assert(waitpid(child,&status,WUNTRACED)==child && WIFSTOPPED(status) && WSTOPSIG(status)==SIGSTOP);
    assert(!kill(child,SIGCONT));
    assert(!waitid(P_PID,child,&info,WCONTINUED)); assert(info.si_pid==child && info.si_code==CLD_CONTINUED && info.si_status==SIGCONT);
    memset(&info,255,sizeof info); assert(!waitid(P_PID,child,&info,WEXITED|WNOHANG) && !info.si_pid);
    assert(syscall(SYS_wait4,child,(void*)1,WNOHANG,0)==0);
    assert(write(release[1],"x",1)==1); close(release[1]);
    for(int i=0;i<2;i++) { assert(!waitid(P_PID,child,&info,WEXITED|WNOWAIT)); assert(info.si_pid==child && info.si_code==CLD_EXITED && info.si_status==37); }
    assert(waitpid(child,&status,0)==child && WIFEXITED(status) && WEXITSTATUS(status)==37);
    errno=0; assert(waitpid(child,&status,WNOHANG)==-1 && errno==ECHILD);
    errno=0; assert(kill(child,0)==-1 && errno==ESRCH);
    for(int mode=0;mode<2;mode++) {
        child=fork(); assert(child>=0); if(!child) _exit(7);
        assert(!waitid(P_PID,child,&info,WEXITED|WNOWAIT));
        errno=0;
        long result=mode?syscall(SYS_waitid,P_PID,child,(void*)1,WEXITED,0):syscall(SYS_wait4,child,(void*)1,0,0);
        assert(result==-1 && errno==EFAULT);
        errno=0; assert(waitpid(child,&status,WNOHANG)==-1 && errno==ECHILD);
    }
    puts("worker-child-wait compatibility passed");
}
int main(int argc,char **argv) {
    if(argc==2 && !strcmp(argv[1],"compat")) { compatibility(); return 0; }
    assert(argc==4); use_waitid=!strcmp(argv[1],"waitid"); assert(use_waitid || !strcmp(argv[1],"wait4"));
    int threads=atoi(argv[2]); rounds=(unsigned)strtoul(argv[3],NULL,10); assert(threads>0 && threads<=32 && rounds>0);
    benchmark(threads); return 0;
}
