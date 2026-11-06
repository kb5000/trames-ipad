// SPDX-License-Identifier: GPL-3.0-or-later
#define _GNU_SOURCE
#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/prctl.h>
#include <sys/wait.h>
#include <linux/filter.h>
#include <linux/seccomp.h>
static pthread_barrier_t gate;
static unsigned threads,rounds,command,filtered;
static _Atomic unsigned progress[12];
static uint64_t now(void){struct timespec t;assert(!clock_gettime(CLOCK_MONOTONIC,&t));return (uint64_t)t.tv_sec*1000000000+t.tv_nsec;}
static void *worker(void *arg){unsigned id=(unsigned)(uintptr_t)arg;pthread_barrier_wait(&gate);pthread_barrier_wait(&gate);
 for(unsigned i=0;i<rounds;i++){atomic_store_explicit(&progress[id],i+1,memory_order_relaxed);long r=syscall(SYS_membarrier,command,0,0);assert(r==0);}
 pthread_barrier_wait(&gate);return NULL;}
int main(int argc,char **argv){
 threads=argc>1?atoi(argv[1]):4;rounds=argc>2?atoi(argv[2]):100;command=argc>3?atoi(argv[3]):8;filtered=argc>4?atoi(argv[4]):0;
 assert(threads&&threads<=12&&rounds&&(command==1||command==8)&&filtered<=1);
 assert(syscall(SYS_membarrier,0,0,0)&8);errno=0;assert(syscall(SYS_membarrier,8,0,0)==-1&&errno==EPERM);
 assert(!syscall(SYS_membarrier,16,0,0));assert(syscall(SYS_membarrier,512,0,0)&16);
 // The kernel ABI consumes 32-bit cmd/flags, regardless of register high bits.
 if(argc<=5)assert(syscall(SYS_membarrier,(1UL<<32)|512,1UL<<32,0)&16);
 errno=0;assert(syscall(SYS_membarrier,8,1,0)==-1&&errno==EINVAL);
 if(filtered){struct sock_filter code=BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_ALLOW);struct sock_fprog filter={1,&code};assert(!prctl(PR_SET_NO_NEW_PRIVS,1,0,0,0));assert(!syscall(SYS_seccomp,SECCOMP_SET_MODE_FILTER,1,&filter));}
 assert(!pthread_barrier_init(&gate,NULL,threads+1));pthread_t ids[12];
 for(unsigned i=0;i<threads;i++)assert(!pthread_create(&ids[i],NULL,worker,(void*)(uintptr_t)i));
 pthread_barrier_wait(&gate);uint64_t began=now();pthread_barrier_wait(&gate);pthread_barrier_wait(&gate);uint64_t elapsed=now()-began;
 for(unsigned i=0;i<threads;i++){assert(!pthread_join(ids[i],NULL));assert(atomic_load_explicit(&progress[i],memory_order_relaxed)==rounds);}
 assert(!pthread_barrier_destroy(&gate));
 pid_t child=fork();assert(child>=0);if(!child){assert(syscall(SYS_membarrier,512,0,0)&16);_exit(0);}int status;assert(waitpid(child,&status,0)==child&&status==0);
 printf("worker-membarrier: threads=%u rounds=%u command=%u filtered=%u elapsed_ns=%llu: ok\n",threads,rounds,command,filtered,(unsigned long long)elapsed);return 0;
}
