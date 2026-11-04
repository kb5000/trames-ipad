// SPDX-License-Identifier: GPL-3.0-or-later
#define _GNU_SOURCE
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#ifndef __APPLE__
#include <sys/sendfile.h>
#endif
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL line=%d errno=%d: %s\n",__LINE__,errno,#x);exit(1);}}while(0)
static const size_t block=4096;
static unsigned char payload[4096];
static void write_all(int fd,const void *buffer,size_t size){const char *p=buffer;while(size){ssize_t n=write(fd,p,size);if(n<0&&errno==EINTR)continue;CHECK(n>0);p+=n;size-=(size_t)n;}}
static void read_all(int fd,void *buffer,size_t size){char *p=buffer;while(size){ssize_t n=read(fd,p,size);if(n<0&&errno==EINTR)continue;CHECK(n>0);p+=n;size-=(size_t)n;}}
static int file(void){char name[]="/tmp/trames-transfer-XXXXXX";int fd=mkstemp(name);CHECK(fd>=0);CHECK(unlink(name)==0);return fd;}
static double now(void){struct timespec t;CHECK(clock_gettime(CLOCK_MONOTONIC,&t)==0);return (double)t.tv_sec+(double)t.tv_nsec/1e9;}
static ssize_t move_data(int in,off_t *from,int out,off_t *to,size_t count,unsigned flags){
#ifdef __APPLE__
    (void)flags;unsigned char buffer[4096];if(count>sizeof(buffer))count=sizeof(buffer);
    ssize_t n=from?pread(in,buffer,count,*from):read(in,buffer,count);if(n<=0)return n;
    ssize_t written=to?pwrite(out,buffer,(size_t)n,*to):write(out,buffer,(size_t)n);
    if(written>0){if(from)*from+=written;if(to)*to+=written;}return written;
#else
    return splice(in,from,out,to,count,flags);
#endif
}
static ssize_t send_file(int out,int in,off_t *offset,size_t count,int socket_out){
#ifdef __APPLE__
    if(socket_out){off_t pos=offset?*offset:lseek(in,0,SEEK_CUR),n=(off_t)count;int r=sendfile(in,out,pos,&n,NULL,0);if(n>0||r==0){if(offset)*offset+=n;else CHECK(lseek(in,pos+n,SEEK_SET)>=0);return (ssize_t)n;}return -1;}
    return move_data(in,offset,out,NULL,count,0);
#else
    (void)socket_out;return sendfile(out,in,offset,count);
#endif
}
#ifndef __APPLE__
static void tcp_pair(int pair[2]){
    int listener=socket(AF_INET,SOCK_STREAM,0);CHECK(listener>=0);
    struct sockaddr_in address;memset(&address,0,sizeof(address));address.sin_family=AF_INET;address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
    CHECK(bind(listener,(struct sockaddr*)&address,sizeof(address))==0);CHECK(listen(listener,4)==0);
    socklen_t size=sizeof(address);CHECK(getsockname(listener,(struct sockaddr*)&address,&size)==0);
    pair[0]=socket(AF_INET,SOCK_STREAM,0);CHECK(pair[0]>=0);CHECK(connect(pair[0],(struct sockaddr*)&address,size)==0);
    pair[1]=accept(listener,NULL,NULL);CHECK(pair[1]>=0);close(listener);
}
static _Atomic int seen;
static void caught(int signal){(void)signal;atomic_fetch_add_explicit(&seen,1,memory_order_relaxed);}
static void stream_tests(void){
    int source=file(),out=file(),a[2],b[2],u[2];unsigned char got[8192];CHECK(pipe(a)==0&&pipe(b)==0&&socketpair(AF_UNIX,SOCK_STREAM,0,u)==0);
    write_all(source,payload,block);CHECK(lseek(source,0,SEEK_SET)==0);
    off_t from=7,to=5;CHECK(move_data(source,&from,a[1],NULL,123,0)==123&&from==130);CHECK(lseek(source,0,SEEK_CUR)==0);
    CHECK(move_data(a[0],NULL,out,&to,123,0)==123&&to==128);CHECK(pread(out,got,123,5)==123&&!memcmp(got,payload+7,123));
    from=0;CHECK(send_file(a[1],source,&from,block,0)==(ssize_t)block&&from==(off_t)block);
    CHECK(move_data(a[0],NULL,b[1],NULL,block,0)==(ssize_t)block);read_all(b[0],got,block);CHECK(!memcmp(got,payload,block));
    CHECK(move_data(a[0],NULL,b[1],NULL,0,0)==0);
    errno=0;CHECK(move_data(a[0],(off_t*)1,b[1],NULL,1,0)==-1&&errno==ESPIPE);
    errno=0;CHECK(move_data(a[0],NULL,b[1],NULL,1,0x100)==-1&&errno==EINVAL);
    int flags=fcntl(b[1],F_GETFL);CHECK(fcntl(b[1],F_SETFL,flags|O_NONBLOCK)==0);size_t capacity=0;
    for(;;){ssize_t n=write(b[1],payload,block);if(n<0){CHECK(errno==EAGAIN);break;}capacity+=(size_t)n;}
    write_all(a[1],payload,block);errno=0;CHECK(move_data(a[0],NULL,b[1],NULL,block,SPLICE_F_NONBLOCK)==-1&&errno==EAGAIN);
    read_all(a[0],got,block);CHECK(!memcmp(got,payload,block));
    from=0;errno=0;CHECK(send_file(b[1],source,&from,block,0)==-1&&errno==EAGAIN&&from==0);CHECK(lseek(source,0,SEEK_CUR)==0);
    while(capacity){size_t n=capacity>sizeof(got)?sizeof(got):capacity;read_all(b[0],got,n);capacity-=n;}
    CHECK(fcntl(b[1],F_SETFL,flags)==0);
    for(int kind=0;kind<2;kind++){
        int sockets[2];if(kind)tcp_pair(sockets);else CHECK(socketpair(AF_UNIX,SOCK_STREAM,0,sockets)==0);
        from=0;CHECK(send_file(sockets[0],source,&from,block,1)==(ssize_t)block);read_all(sockets[1],got,block);CHECK(!memcmp(got,payload,block));
        write_all(a[1],payload,block);CHECK(move_data(a[0],NULL,sockets[0],NULL,block,0)==(ssize_t)block);read_all(sockets[1],got,block);CHECK(!memcmp(got,payload,block));
        write_all(sockets[0],payload,block);CHECK(move_data(sockets[1],NULL,b[1],NULL,block,0)==(ssize_t)block);read_all(b[0],got,block);CHECK(!memcmp(got,payload,block));
        CHECK(shutdown(sockets[0],SHUT_WR)==0);CHECK(move_data(sockets[1],NULL,b[1],NULL,block,0)==0);close(sockets[0]);close(sockets[1]);
    }
    /* Bulk disk work uses the existing pool; completion must wake a solo CPU. */
    for(size_t i=1;i<8;i++)CHECK(pwrite(source,payload,block,(off_t)(i*block))==(ssize_t)block);
    int bulk[2];CHECK(socketpair(AF_UNIX,SOCK_STREAM,0,bulk)==0);from=0;
    CHECK(send_file(bulk[0],source,&from,8*block,0)==(ssize_t)(8*block));
    for(size_t i=0;i<8;i++){read_all(bulk[1],got,block);CHECK(!memcmp(got,payload,block));}
    close(bulk[0]);close(bulk[1]);
    int append=fcntl(out,F_GETFL);CHECK(fcntl(out,F_SETFL,append|O_APPEND)==0);from=0;errno=0;CHECK(send_file(out,source,&from,1,0)==-1&&errno==EINVAL);CHECK(fcntl(out,F_SETFL,append)==0);
    struct sigaction action={0};action.sa_handler=caught;sigemptyset(&action.sa_mask);CHECK(sigaction(SIGPIPE,&action,NULL)==0);
    close(b[0]);seen=0;from=0;errno=0;CHECK(send_file(b[1],source,&from,1,0)==-1&&errno==EPIPE&&seen==1&&from==0);
    close(a[1]);CHECK(move_data(a[0],NULL,out,NULL,block,0)==0);
    close(source);close(out);close(a[0]);close(b[1]);close(u[0]);close(u[1]);
    puts("{\"case\":\"stream-abi\",\"passed\":true}");
}
struct Waiting{int in,out;ssize_t result;int error;pthread_mutex_t lock;pthread_cond_t ready;int started;};
static void *blocked(void *opaque){struct Waiting *w=opaque;pthread_mutex_lock(&w->lock);w->started=1;pthread_cond_signal(&w->ready);pthread_mutex_unlock(&w->lock);w->result=move_data(w->in,NULL,w->out,NULL,4096,0);w->error=errno;return NULL;}
static void disk_backpressure(void){
    int source=file(),a[2];CHECK(pipe(a)==0);write_all(source,payload,block);CHECK(lseek(source,0,SEEK_SET)==0);
    int flags=fcntl(a[1],F_GETFL);CHECK(fcntl(a[1],F_SETFL,flags|O_NONBLOCK)==0);size_t capacity=0;
    for(;;){ssize_t n=write(a[1],payload,block);if(n<0){CHECK(errno==EAGAIN);break;}capacity+=(size_t)n;}
    CHECK(fcntl(a[1],F_SETFL,flags)==0);
    struct Waiting w={.in=source,.out=a[1],.lock=PTHREAD_MUTEX_INITIALIZER,.ready=PTHREAD_COND_INITIALIZER};
    pthread_t thread;CHECK(pthread_create(&thread,NULL,blocked,&w)==0);
    pthread_mutex_lock(&w.lock);while(!w.started)pthread_cond_wait(&w.ready,&w.lock);pthread_mutex_unlock(&w.lock);
    struct timespec delay={.tv_nsec=30000000};nanosleep(&delay,NULL);
    unsigned char got[4096];while(capacity){size_t n=capacity>block?block:capacity;read_all(a[0],got,n);capacity-=n;}
    CHECK(pthread_join(thread,NULL)==0&&w.result==(ssize_t)block);read_all(a[0],got,block);CHECK(!memcmp(got,payload,block));
    CHECK(lseek(source,0,SEEK_CUR)==(off_t)block);close(source);close(a[0]);close(a[1]);pthread_cond_destroy(&w.ready);pthread_mutex_destroy(&w.lock);
    puts("{\"case\":\"disk-backpressure\",\"passed\":true}");
}
static void signal_tests(void){
    for(int restart=0;restart<2;restart++){
        struct sigaction action={0};action.sa_handler=caught;action.sa_flags=restart?SA_RESTART:0;sigemptyset(&action.sa_mask);CHECK(sigaction(SIGUSR1,&action,NULL)==0);atomic_store(&seen,0);
        int a[2],b[2];CHECK(pipe(a)==0&&pipe(b)==0);struct Waiting w={.in=a[0],.out=b[1],.lock=PTHREAD_MUTEX_INITIALIZER,.ready=PTHREAD_COND_INITIALIZER};
        pthread_t thread;CHECK(pthread_create(&thread,NULL,blocked,&w)==0);pthread_mutex_lock(&w.lock);while(!w.started)pthread_cond_wait(&w.ready,&w.lock);pthread_mutex_unlock(&w.lock);
        struct timespec delay={.tv_nsec=30000000};nanosleep(&delay,NULL);CHECK(pthread_kill(thread,SIGUSR1)==0);
        double deadline=now()+4;while(!atomic_load_explicit(&seen,memory_order_relaxed)){struct timespec tick={.tv_nsec=1000000};nanosleep(&tick,NULL);CHECK(now()<deadline);}
        if(restart)write_all(a[1],payload,block);
        CHECK(pthread_join(thread,NULL)==0);
        if(restart)CHECK(w.result==(ssize_t)block);else{CHECK(w.result==-1&&w.error==EINTR);write_all(a[1],payload,block);CHECK(move_data(a[0],NULL,b[1],NULL,block,0)==(ssize_t)block);}
        unsigned char got[4096];read_all(b[0],got,block);CHECK(!memcmp(got,payload,block));
        for(int i=0;i<2;i++){close(a[i]);close(b[i]);}pthread_cond_destroy(&w.ready);pthread_mutex_destroy(&w.lock);
    }
    puts("{\"case\":\"interruptible-wait-and-restart\",\"passed\":true}");
}

#endif
struct Barrier{pthread_mutex_t lock;pthread_cond_t condition;unsigned count,total;};
static struct Barrier start;
static void barrier(void){pthread_mutex_lock(&start.lock);if(++start.count==start.total)pthread_cond_broadcast(&start.condition);else while(start.count<start.total)pthread_cond_wait(&start.condition,&start.lock);pthread_mutex_unlock(&start.lock);}
struct Job{const char *mode;unsigned iterations;double seconds;};
static void *benchmark(void *opaque){
    struct Job *job=opaque;int source=file(),out=file(),a[2],b[2],s[2];CHECK(pipe(a)==0&&pipe(b)==0);CHECK(socketpair(AF_UNIX,SOCK_STREAM,0,s)==0);write_all(source,payload,block);write_all(out,payload,block);
    unsigned char got[4096];barrier();double began=now();
    for(unsigned i=0;i<job->iterations;i++){
        if(!strcmp(job->mode,"file-pipe")){off_t from=0,to=0;CHECK(send_file(a[1],source,&from,block,0)==(ssize_t)block);CHECK(move_data(a[0],NULL,out,&to,block,0)==(ssize_t)block);}
        else if(!strcmp(job->mode,"file-unix")){off_t from=0;CHECK(send_file(s[0],source,&from,block,1)==(ssize_t)block);read_all(s[1],got,block);CHECK(!memcmp(got,payload,block));}
        else if(!strcmp(job->mode,"socket-pipe")){write_all(s[0],payload,block);CHECK(move_data(s[1],NULL,a[1],NULL,block,0)==(ssize_t)block);read_all(a[0],got,block);CHECK(!memcmp(got,payload,block));}
        else{write_all(a[1],payload,block);CHECK(move_data(a[0],NULL,b[1],NULL,block,0)==(ssize_t)block);read_all(b[0],got,block);CHECK(!memcmp(got,payload,block));}
    }
    job->seconds=now()-began;
    if(!strcmp(job->mode,"file-pipe")){CHECK(pread(out,got,block,0)==(ssize_t)block&&!memcmp(got,payload,block));}
    close(source);close(out);for(int i=0;i<2;i++){close(a[i]);close(b[i]);close(s[i]);}return NULL;
}
int main(int argc,char **argv){
    mkdir("/tmp",0700);for(unsigned i=0;i<sizeof(payload);i++)payload[i]=(unsigned char)(i*37+11);
    if(argc==5&&!strcmp(argv[1],"--bench")){
        unsigned workers=(unsigned)strtoul(argv[3],NULL,10),iterations=(unsigned)strtoul(argv[4],NULL,10);CHECK(workers>=1&&workers<=32&&iterations>0);
        start=(struct Barrier){.lock=PTHREAD_MUTEX_INITIALIZER,.condition=PTHREAD_COND_INITIALIZER,.total=workers};struct Job jobs[32];pthread_t threads[32];
        for(unsigned i=0;i<workers;i++){jobs[i]=(struct Job){.mode=argv[2],.iterations=iterations};CHECK(pthread_create(&threads[i],NULL,benchmark,&jobs[i])==0);}
        double maximum=0;for(unsigned i=0;i<workers;i++){CHECK(pthread_join(threads[i],NULL)==0);if(jobs[i].seconds>maximum)maximum=jobs[i].seconds;}
        printf("{\"case\":\"%s\",\"workers\":%u,\"iterations\":%u,\"bytes_per_iteration\":%zu,\"longest_worker_ms\":%.6f,\"passed\":true}\n",argv[2],workers,iterations,block,maximum*1000);return 0;
    }
#ifndef __APPLE__
    stream_tests();disk_backpressure();signal_tests();
#else
    puts("Darwin runs the native transfer benchmark; Linux ABI checks use the unchanged Linux ELF.");
#endif
    return 0;
}
