// SPDX-License-Identifier: GPL-3.0-or-later
// Whole-workflow comparison: identical static ARM64 ELF on Trames and Linux.
#define _GNU_SOURCE
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/io_uring.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <time.h>
#include <unistd.h>

static pthread_barrier_t ready;
static unsigned iterations;
static const char *mode;
struct job { unsigned index; uint64_t ns; int fd; };
static uint64_t now(void) {
    struct timespec t; assert(!clock_gettime(CLOCK_MONOTONIC,&t));
    return (uint64_t)t.tv_sec*1000000000+t.tv_nsec;
}
static void *run(void *opaque) {
    struct job *job=opaque;
    struct io_uring_params p={0};
    int fd=(int)syscall(SYS_io_uring_setup,8,&p);
    assert(fd>=0 && (p.features&IORING_FEAT_SINGLE_MMAP));
    size_t size=p.cq_off.cqes+p.cq_entries*sizeof(struct io_uring_cqe);
    char *q=mmap(NULL,size,PROT_READ|PROT_WRITE,MAP_SHARED,fd,IORING_OFF_SQ_RING);
    struct io_uring_sqe *sq=mmap(NULL,p.sq_entries*sizeof *sq,PROT_READ|PROT_WRITE,MAP_SHARED,fd,IORING_OFF_SQES);
    assert(q!=MAP_FAILED && sq!=MAP_FAILED);
    unsigned *tail=(void *)(q+p.sq_off.tail),*head=(void *)(q+p.cq_off.head);
    unsigned *cqtail=(void *)(q+p.cq_off.tail),*array=(void *)(q+p.sq_off.array);
    struct io_uring_cqe *cq=(void *)(q+p.cq_off.cqes);
    struct statx st={0}; uint32_t bytes=0x61746164;
    int rc=pthread_barrier_wait(&ready); assert(!rc || rc==PTHREAD_BARRIER_SERIAL_THREAD);
    uint64_t began=now();
    for(unsigned i=0;i<iterations;i++) {
        if(!strcmp(mode,"register")) {
            assert(!syscall(SYS_io_uring_register,fd,IORING_REGISTER_FILES,&job->fd,1));
            assert(!syscall(SYS_io_uring_register,fd,IORING_UNREGISTER_FILES,NULL,0));
            continue;
        }
        unsigned t=__atomic_load_n(tail,__ATOMIC_RELAXED),slot=t&(p.sq_entries-1);
        struct io_uring_sqe *s=&sq[slot]; memset(s,0,sizeof *s);
        s->user_data=i+1; s->fd=-1;
        if(!strcmp(mode,"statx")) {
            s->opcode=IORING_OP_STATX; s->fd=job->fd;
            s->addr=(uintptr_t)""; s->off=(uintptr_t)&st;
            s->len=STATX_BASIC_STATS; s->statx_flags=AT_EMPTY_PATH;
        }
        int expected=0;
        if(!strcmp(mode,"read") || !strcmp(mode,"write")) {
            s->opcode=!strcmp(mode,"read")?IORING_OP_READ:IORING_OP_WRITE;
            s->fd=job->fd; s->addr=(uintptr_t)&bytes; s->len=4; expected=4;
        }
        if(!strcmp(mode,"truncate")) {
            s->opcode=55; s->fd=job->fd; s->off=(i&1)?4:1;
        }
        array[slot]=slot; __atomic_store_n(tail,t+1,__ATOMIC_RELEASE);
        assert(syscall(SYS_io_uring_enter,fd,1,1,IORING_ENTER_GETEVENTS,NULL,0)==1);
        unsigned h=__atomic_load_n(head,__ATOMIC_RELAXED);
        assert(h!=__atomic_load_n(cqtail,__ATOMIC_ACQUIRE));
        struct io_uring_cqe c=cq[h&(p.cq_entries-1)];
        assert(c.user_data==i+1 && c.res==expected && !c.flags);
        if(s->opcode==IORING_OP_STATX) assert(st.stx_size==4 && (st.stx_mode&S_IFMT)==S_IFREG);
        __atomic_store_n(head,h+1,__ATOMIC_RELEASE);
    }
    job->ns=now()-began;
    assert(!close(fd) && !munmap(sq,p.sq_entries*sizeof *sq) && !munmap(q,size));
    return NULL;
}
int main(int argc,char **argv) {
    assert(argc==4); mode=argv[1]; unsigned workers=(unsigned)strtoul(argv[2],NULL,10);
    iterations=(unsigned)strtoul(argv[3],NULL,10);
    assert(workers>=1 && workers<=12 && iterations && iterations<=100000);
    assert(!strcmp(mode,"nop") || !strcmp(mode,"register") || !strcmp(mode,"statx") || !strcmp(mode,"read") || !strcmp(mode,"write") || !strcmp(mode,"truncate"));
    struct io_uring_params probe={0}; int check=(int)syscall(SYS_io_uring_setup,2,&probe);
    if(check<0) { fprintf(stderr,"io_uring unavailable: %s\n",strerror(errno)); return 77; }
    assert(!close(check));
    assert(!mkdir("/tmp",0777) || errno==EEXIST);
    pthread_t threads[12]; struct job jobs[12]={0};
    assert(!pthread_barrier_init(&ready,NULL,workers+1));
    for(unsigned i=0;i<workers;i++) {
        char path[96]; snprintf(path,sizeof path,"/tmp/uring-bench-%ld-%u",(long)getpid(),i);
        jobs[i].index=i; jobs[i].fd=open(path,O_CREAT|O_EXCL|O_RDWR,0600);
        assert(jobs[i].fd>=0 && write(jobs[i].fd,"data",4)==4 && !unlink(path));
        assert(!pthread_create(&threads[i],NULL,run,&jobs[i]));
    }
    int rc=pthread_barrier_wait(&ready); assert(!rc || rc==PTHREAD_BARRIER_SERIAL_THREAD);
    uint64_t longest=0;
    for(unsigned i=0;i<workers;i++) {
        assert(!pthread_join(threads[i],NULL) && !close(jobs[i].fd));
        if(jobs[i].ns>longest)longest=jobs[i].ns;
    }
    assert(!pthread_barrier_destroy(&ready));
    printf("{\"mode\":\"%s\",\"workers\":%u,\"iterations\":%u,\"longest_worker_ns\":%llu}\n",mode,workers,iterations,(unsigned long long)longest);
    return 0;
}
