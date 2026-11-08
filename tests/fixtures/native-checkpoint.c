// SPDX-License-Identifier: GPL-3.0-or-later
#define _GNU_SOURCE
#include <assert.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/auxv.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <time.h>
#include <unistd.h>
extern void *native_copy(void *, const void *, size_t);
extern void *native_fill(void *, int, size_t);
extern int64_t gpu_call(uint64_t, const uint64_t *, uint64_t);
static pthread_barrier_t start;
static atomic_int done;
static unsigned long progress[3];
static void together(void) { int r=pthread_barrier_wait(&start); assert(!r || r==PTHREAD_BARRIER_SERIAL_THREAD); }
static void *peer(void *argument) {
    size_t id=(size_t)argument; together();
    while(!atomic_load_explicit(&done,memory_order_relaxed)) { assert(syscall(SYS_getpid)>0); progress[id]++; }
    return NULL;
}
static uint64_t ns(void) { struct timespec t; assert(!clock_gettime(CLOCK_MONOTONIC,&t)); return (uint64_t)t.tv_sec*1000000000+t.tv_nsec; }
#define P(x) ((uint64_t)(uintptr_t)(x))
static void gpu(uint64_t operation,const uint64_t *a,size_t n) {
    uint64_t packet[16]={0};assert(n<=16);memcpy(packet,a,n*8);
    int64_t result=gpu_call(operation,packet,sizeof packet);
    if(result)fprintf(stderr,"gpu op=%lu result=%ld\n",(unsigned long)operation,(long)result);
    assert(!result);
}
#define G(op,...) do {const uint64_t args[]={__VA_ARGS__};gpu(op,args,sizeof args/8);} while(0)
static void shared_gpu(unsigned rounds) {
    assert(getauxval(0x54520008)==0x10006);
    uint64_t instance=0,physical=0,device=0,buffer=0,allocation=0;uint32_t count=1;
    G(1,P(&instance));G(3,instance,P(&count),P(&physical));assert(count==1);G(8,physical,0,P(&device));
    uint64_t capabilities=0;G(200,device,P(&capabilities));assert(capabilities&1);
    G(11,device,16384,2,P(&buffer));uint64_t req[3]={0};G(13,device,buffer,P(req));
    uint64_t types[65]={0};G(5,physical,P(types));const uint32_t *words=(const uint32_t *)types;
    uint32_t ty=0;for(;ty<words[0];ty++)if((req[2]&(1ull<<ty)) && (words[1+2*ty]&6)==6)break;assert(ty<words[0]);
    G(14,device,req[0],ty,P(&allocation));G(16,device,buffer,allocation,0);
    unsigned char *mapped=mmap(NULL,16384,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);assert(mapped!=MAP_FAILED);
    for(unsigned i=0;i<rounds;i++) {
        // The original host allocation replaces the existing guest mapping.
        G(17,device,allocation,0,16384,P(mapped),0x534841524544ull);
        assert((getauxval(0x54520001)==0x10001?native_fill(mapped,0x31+i,16384):memset(mapped,0x31+i,16384))==mapped);
        for(size_t j=0;j<16384;j++)assert(mapped[j]==(unsigned char)(0x31+i));
        G(18,device,allocation);
    }
    G(12,device,buffer);G(15,device,allocation);G(9,device);G(2,instance);assert(!munmap(mapped,16384));
}
static void mapped_file(unsigned rounds,int writing,int accelerated) {
    char name[]="/tmp/native-checkpoint-XXXXXX";int fd=mkstemp(name);assert(fd>=0);assert(!unlink(name));
    unsigned char bytes[4096],out[4096];for(size_t i=0;i<sizeof bytes;i++)bytes[i]=(unsigned char)(i*31+7);
    assert(write(fd,bytes,sizeof bytes)==sizeof bytes);
    for(unsigned i=0;i<rounds;i++) {
        unsigned char *map=mmap(NULL,sizeof bytes,PROT_READ|PROT_WRITE,writing?MAP_SHARED:MAP_PRIVATE,fd,0);assert(map!=MAP_FAILED);
        if(writing) {
            void *r=accelerated?native_fill(map,0x31+i,sizeof bytes):memset(map,0x31+i,sizeof bytes);assert(r==map);
            assert(!msync(map,sizeof bytes,MS_SYNC));assert(pread(fd,out,sizeof out,0)==sizeof out);
            for(size_t j=0;j<sizeof out;j++)assert(out[j]==(unsigned char)(0x31+i));
        } else {
            void *r=accelerated?native_copy(out,map,sizeof out):memcpy(out,map,sizeof out);assert(r==out);assert(!memcmp(out,bytes,sizeof out));
            // A private COW write must not modify the original file.
            assert((accelerated?native_fill(map,9,sizeof bytes):memset(map,9,sizeof bytes))==map);
            assert(pread(fd,out,sizeof out,0)==sizeof out);assert(!memcmp(out,bytes,sizeof out));
        }
        assert(!munmap(map,sizeof bytes));
    }
    assert(!close(fd));
}
int main(int argc,char **argv) {
    assert(argc==3);unsigned rounds=(unsigned)strtoul(argv[2],NULL,10);assert(rounds && rounds<=10000);
    int gpu_mode=!strcmp(argv[1],"gpu"),writing=!strcmp(argv[1],"write");assert(gpu_mode || writing || !strcmp(argv[1],"read"));
    assert(!mkdir("/tmp",0777) || errno==EEXIST);
    int accelerated=getauxval(0x54520001)==0x10001;
    assert(!pthread_barrier_init(&start,NULL,4));pthread_t threads[3];
    for(size_t i=0;i<3;i++)assert(!pthread_create(&threads[i],NULL,peer,(void *)i));
    together();uint64_t began=ns();if(gpu_mode)shared_gpu(rounds);else mapped_file(rounds,writing,accelerated);uint64_t elapsed=ns()-began;
    atomic_store_explicit(&done,1,memory_order_relaxed);unsigned long calls=0;
    for(unsigned i=0;i<3;i++){assert(!pthread_join(threads[i],NULL));calls+=progress[i];}assert(calls);
    assert(!pthread_barrier_destroy(&start));
    printf("{\"mode\":\"%s\",\"rounds\":%u,\"elapsed_ns\":%llu,\"peer_calls\":%lu,\"accelerated\":%d}\n",argv[1],rounds,(unsigned long long)elapsed,calls,accelerated);return 0;
}
