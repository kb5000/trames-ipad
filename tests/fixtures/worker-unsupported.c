// SPDX-License-Identifier: GPL-3.0-or-later
#define _GNU_SOURCE
#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/prctl.h>
#include <linux/filter.h>
#include <linux/seccomp.h>
static pthread_barrier_t gate;
static unsigned threads,rounds,mode;
static uint64_t now(void) { struct timespec t;assert(!clock_gettime(CLOCK_MONOTONIC,&t));return (uint64_t)t.tv_sec*1000000000+t.tv_nsec; }
static void *worker(void *unused) {
 (void)unused;pthread_barrier_wait(&gate);pthread_barrier_wait(&gate);
 for(unsigned i=0;i<rounds;i++) {
  errno=0;long r=syscall(0xff00+(i&15),-1L,-1L,-1L,-1L,-1L,-1L);
  assert(r==-1 && errno==(mode==2?42:ENOSYS));
 }
 pthread_barrier_wait(&gate);return NULL;
}
int main(int argc,char **argv) {
 threads=argc>1?atoi(argv[1]):4;rounds=argc>2?atoi(argv[2]):1000;mode=argc>3?atoi(argv[3]):0;
 assert(threads&&threads<=12&&rounds&&mode<=2);
 assert(!pthread_barrier_init(&gate,NULL,threads+1));pthread_t ids[12];
 for(unsigned i=0;i<threads;i++) assert(!pthread_create(&ids[i],NULL,worker,NULL));
 pthread_barrier_wait(&gate);
 if(mode) {
  struct sock_filter code[]={BPF_STMT(BPF_LD|BPF_W|BPF_ABS,0),BPF_JUMP(BPF_JMP|BPF_JGE|BPF_K,0xff00,0,1),BPF_STMT(BPF_RET|BPF_K,mode==2?(SECCOMP_RET_ERRNO|42):SECCOMP_RET_ALLOW),BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_ALLOW)};
  struct sock_fprog filter={.len=4,.filter=code};assert(!prctl(PR_SET_NO_NEW_PRIVS,1,0,0,0));
  assert(!syscall(SYS_seccomp,SECCOMP_SET_MODE_FILTER,SECCOMP_FILTER_FLAG_TSYNC,&filter));
 }
 uint64_t began=now();pthread_barrier_wait(&gate);pthread_barrier_wait(&gate);uint64_t elapsed=now()-began;
 for(unsigned i=0;i<threads;i++) assert(!pthread_join(ids[i],NULL));assert(!pthread_barrier_destroy(&gate));
 printf("worker-unsupported: threads=%u rounds=%u mode=%u elapsed_ns=%llu: ok\n",threads,rounds,mode,(unsigned long long)elapsed);return 0;
}
