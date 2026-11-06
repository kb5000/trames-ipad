// SPDX-License-Identifier: GPL-3.0-or-later
// Reuse the complete serial contract and its ABI helpers for parallel control.
#define main serial_bpf_contract
#include "cgroup-bpf.c"
#undef main
#include <pthread.h>
#include <stdlib.h>
#include <time.h>
#include <linux/filter.h>
#include <linux/seccomp.h>
#include <sys/prctl.h>

static unsigned workers, rounds;
static int filtered;
static pthread_barrier_t gate;
static char base[128];
static uint64_t now_ns(void) {
 struct timespec t;assert(!clock_gettime(CLOCK_MONOTONIC,&t));
 return (uint64_t)t.tv_sec*1000000000+t.tv_nsec;
}
static void *control_worker(void *arg) {
 unsigned index=(unsigned)(uintptr_t)arg;
 char path[160];snprintf(path,sizeof path,"%s/%u",base,index);
 int target=open(path,O_PATH);assert(target>=0);
 // Separate readiness from timed release: target opens and thread startup
 // are part of functional acceptance, not the steady policy-control timer.
 pthread_barrier_wait(&gate);
 pthread_barrier_wait(&gate);
 const struct insn allow[]={{0xb7,0,0,1},{0x95,0,0,0}};
 for(unsigned i=0;i<rounds;i++) {
  int program=load(allow,2);assert(program>=0);
  unsigned id=identity(program,0);
  assert(attach(target,program,2,-1)==0);
  uint32_t ids[2];assert(query(target,0,ids,2,0)==1 && ids[0]==id);
  close(program);
  int original=getfd(id);assert(original>=0);
  int replacement=load(allow,2);assert(replacement>=0);
  unsigned next=identity(replacement,0);
  assert(attach(target,replacement,6,original)==0);
  close(original);
  DENIED(getfd(id),ENOENT);
  assert(query(target,1,ids,2,0)==1 && ids[0]==next);
  assert(detach(target,replacement)==0);
  close(replacement);
  DENIED(getfd(next),ENOENT);
 }
 pthread_barrier_wait(&gate);
 close(target);return NULL;
}
int main(int argc,char **argv) {
 workers=argc>1?(unsigned)strtoul(argv[1],NULL,10):4;
 rounds=argc>2?(unsigned)strtoul(argv[2],NULL,10):100;
 filtered=argc>3?atoi(argv[3]):0;
 assert(workers && workers<=12 && rounds && (filtered==0 || filtered==1));
 // Run only in an isolated test guest: native Linux mode installs real
 // device programs in that guest, never in the macOS host kernel.
 snprintf(base,sizeof base,CG "/bpf-workers-%ld",(long)getpid());
 assert(mkdir(base,0755)==0);
 for(unsigned i=0;i<workers;i++) { char path[160];snprintf(path,sizeof path,"%s/%u",base,i);assert(mkdir(path,0755)==0); }
 assert(!pthread_barrier_init(&gate,NULL,workers+1));
 pthread_t threads[12];
 for(unsigned i=0;i<workers;i++) assert(!pthread_create(&threads[i],NULL,control_worker,(void*)(uintptr_t)i));
 pthread_barrier_wait(&gate);
 if(filtered) {
  struct sock_filter insn=BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_ALLOW);
  struct sock_fprog filter={.len=1,.filter=&insn};
  assert(!prctl(PR_SET_NO_NEW_PRIVS,1,0,0,0));
  assert(!syscall(SYS_seccomp,SECCOMP_SET_MODE_FILTER,SECCOMP_FILTER_FLAG_TSYNC,&filter));
 }
 uint64_t began=now_ns();pthread_barrier_wait(&gate);pthread_barrier_wait(&gate);
 uint64_t elapsed=now_ns()-began;
 for(unsigned i=0;i<workers;i++) { assert(!pthread_join(threads[i],NULL));char path[160];snprintf(path,sizeof path,"%s/%u",base,i);assert(rmdir(path)==0); }
 assert(!pthread_barrier_destroy(&gate));assert(rmdir(base)==0);
 printf("worker-bpf: threads=%u rounds=%u filtered=%d elapsed_ns=%llu: ok\n",workers,rounds,filtered,(unsigned long long)elapsed);
 return 0;
}
