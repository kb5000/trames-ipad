// SPDX-License-Identifier: GPL-3.0-or-later
#define _GNU_SOURCE
#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
struct scheduling_filter { uint16_t code; uint8_t yes, no; uint32_t value; };
struct scheduling_program { uint16_t count; struct scheduling_filter *instructions; };
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"line %d: %s errno=%d\n",__LINE__,#x,errno);exit(1); } } while(0)
static pthread_barrier_t barrier;
static _Atomic unsigned phase;
static pid_t tids[8];
static uint64_t allowed, low, high;
static unsigned threads=4, loops=256;
static const char *mode;
static double durations[8];
static double now(void) { struct timespec t; CHECK(clock_gettime(CLOCK_MONOTONIC,&t)==0); return t.tv_sec+t.tv_nsec/1e9; }
static uint64_t affinity(pid_t tid) { uint64_t mask=0; CHECK(syscall(SYS_sched_getaffinity,tid,sizeof(mask),&mask)==sizeof(mask)); return mask; }
static void pin(pid_t tid,uint64_t mask) { CHECK(syscall(SYS_sched_setaffinity,tid,sizeof(mask),&mask)==0); }
static unsigned cpu(void) { unsigned value=64; CHECK(syscall(SYS_getcpu,&value,NULL,NULL)==0); return value; }
static void barrier_wait(void) { int r=pthread_barrier_wait(&barrier); CHECK(r==0 || r==PTHREAD_BARRIER_SERIAL_THREAD); }
static void *worker(void *arg) {
 unsigned id=(unsigned)(uintptr_t)arg; tids[id]=(pid_t)syscall(SYS_gettid);
 barrier_wait();
 if(!strcmp(mode,"compat")) {
  if(id==0) {
   while(!atomic_load_explicit(&phase,memory_order_acquire)) __asm__ volatile("yield");
   CHECK(high & (UINT64_C(1)<<cpu()));
   for(unsigned n=0;n<16;n++) {
    pin(0,low); CHECK(low & (UINT64_C(1)<<cpu()));
    pin(0,high); CHECK(high & (UINT64_C(1)<<cpu()));
   }
   pin(0,allowed);
   atomic_store_explicit(&phase,2,memory_order_release);
  } else { while(atomic_load_explicit(&phase,memory_order_acquire)!=2) CHECK(syscall(SYS_sched_yield)==0); }
 } else {
  pid_t target=!strcmp(mode,"target")?tids[(id+1)%threads]:0;
  double began=now();
  for(unsigned n=0;n<loops;n++) {
   if(!strcmp(mode,"query") || !strcmp(mode,"target")) {
    CHECK(affinity(target)==allowed);
    CHECK(syscall(SYS_sched_getscheduler,target)==SCHED_OTHER);
    struct sched_param p={.sched_priority=-1}; CHECK(syscall(SYS_sched_getparam,target,&p)==0 && p.sched_priority==0);
    CHECK(syscall(SYS_getpriority,PRIO_PROCESS,target)>0);
   } else {
    CHECK(!strcmp(mode,"update")); pin(0,allowed);
    struct sched_param p={0}; CHECK(syscall(SYS_sched_setscheduler,0,SCHED_OTHER,&p)==0);
    CHECK(syscall(SYS_sched_setparam,0,&p)==0); CHECK(setpriority(PRIO_PROCESS,0,7)==0);
   }
  }
  durations[id]=now()-began;
 }
 barrier_wait();
 return NULL;
}
int main(int argc,char **argv) {
 if(argc>1 && !strcmp(argv[1],"exec-check")) {
  CHECK(syscall(SYS_sched_getscheduler,0)==SCHED_BATCH); CHECK(getpriority(PRIO_PROCESS,0)==7);
  CHECK(affinity(0)==strtoull(argv[2],NULL,10)); return 0;
 }
 mode=argc>1?argv[1]:"compat"; if(argc>2) threads=(unsigned)atoi(argv[2]); if(argc>3) loops=(unsigned)atoi(argv[3]);
 CHECK(threads>1 && threads<=8 && loops>0); allowed=affinity(0); CHECK(allowed!=0);
 low=allowed & -allowed; high=UINT64_C(1)<<(63-__builtin_clzll(allowed));
 if(argc>4 && !strcmp(argv[4],"filtered")) {
  struct scheduling_filter insn={.code=6,.value=0x7fff0000};
  struct scheduling_program filter={.count=1,.instructions=&insn};
  CHECK(prctl(PR_SET_NO_NEW_PRIVS,1,0,0,0)==0);
  CHECK(syscall(SYS_seccomp,1,0,&filter)==0);
 }
 if(!strcmp(mode,"compat")) {
  uint64_t zero=0;
  CHECK(syscall(SYS_sched_setaffinity,0,8,&zero)==-1 && errno==EINVAL);
  CHECK(affinity(0)==allowed);
  CHECK(syscall(SYS_sched_setaffinity,0,8,NULL)==-1 && errno==EFAULT);
  CHECK(syscall(SYS_sched_getaffinity,0,7,&zero)==-1 && errno==EINVAL);
  CHECK(sched_get_priority_max(SCHED_FIFO)==99 && sched_get_priority_min(SCHED_RR)==1);
 }
 CHECK(pthread_barrier_init(&barrier,NULL,threads+1)==0); pthread_t ids[8];
 for(unsigned id=0;id<threads;id++) CHECK(pthread_create(&ids[id],NULL,worker,(void*)(uintptr_t)id)==0);
 barrier_wait();
 if(!strcmp(mode,"compat")) { pin(tids[0],high); atomic_store_explicit(&phase,1,memory_order_release); }
 barrier_wait();
 for(unsigned id=0;id<threads;id++) CHECK(pthread_join(ids[id],NULL)==0);
 CHECK(pthread_barrier_destroy(&barrier)==0);
 if(!strcmp(mode,"compat")) {
  CHECK(affinity(0)==allowed); CHECK(syscall(SYS_sched_getscheduler,tids[0])==-1 && errno==ESRCH);
  struct sched_param param={0}; CHECK(syscall(SYS_sched_setscheduler,0,SCHED_BATCH,&param)==0);
  CHECK(setpriority(PRIO_PROCESS,0,7)==0); pin(0,high);
  pid_t child=fork(); CHECK(child>=0);
  if(!child) {
   CHECK(affinity(0)==high && syscall(SYS_sched_getscheduler,0)==SCHED_BATCH && getpriority(PRIO_PROCESS,0)==7);
   char mask[32]; snprintf(mask,sizeof(mask),"%llu",(unsigned long long)high);
   execl(argv[0],argv[0],"exec-check",mask,(char*)NULL); CHECK(0);
  }
  int status; CHECK(waitpid(child,&status,0)==child && WIFEXITED(status) && WEXITSTATUS(status)==0);
  puts("worker-scheduling-compatibility-ok");
 } else {
  double maximum=0; for(unsigned id=0;id<threads;id++) if(durations[id]>maximum) maximum=durations[id];
  printf("{\"mode\":\"%s\",\"threads\":%u,\"loops\":%u,\"calls\":%u,\"seconds\":%.9f}\n",mode,threads,loops,threads*loops*4,maximum);
 }
 return 0;
}
