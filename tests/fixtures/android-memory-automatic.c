// SPDX-License-Identifier: GPL-3.0-or-later
#define _GNU_SOURCE
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
static void check(int ok,const char*step){if(!ok){fprintf(stderr,"FAIL %s: %s\n",step,strerror(errno));exit(1);}}
static void priority(int value){char b[32];int n=snprintf(b,sizeof(b),"%d",value);int fd=open("/proc/self/oom_score_adj",O_WRONLY);check(fd>=0&&write(fd,b,n)==n,"oom_score_adj");close(fd);}
static void fill(size_t size){volatile unsigned char*p=mmap(NULL,size,3,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);check(p!=MAP_FAILED,"anonymous allocation");for(size_t a=0;a<size;a+=4096)p[a]=0x5a;for(size_t a=0;a<size;a+=4096)check(p[a]==0x5a,"original bytes after pressure");}
static void *allocate(void *unused){(void)unused;fill(80UL<<20);return NULL;}
static void register_process(int fd,pid_t pid,int adj){uint32_t packet[]={htonl(1),htonl(pid),htonl(getuid()),htonl(adj)};check(send(fd,packet,sizeof(packet),0)==sizeof(packet),"original LMK_PROCPRIO");}
static void barrier(int fd){uint32_t query[]={htonl(4),htonl(0),htonl(1000)},reply[2];check(send(fd,query,sizeof(query),0)==sizeof(query),"LMK_GETKILLCNT");check(recv(fd,reply,sizeof(reply),0)==sizeof(reply)&&ntohl(reply[0])==4,"original lmkd reply");}
static unsigned long available(void){FILE*f=fopen("/proc/meminfo","r");check(f!=NULL,"meminfo");char line[160];unsigned long value=0;while(fgets(line,sizeof(line),f))if(sscanf(line,"MemAvailable: %lu kB",&value)==1)break;fclose(f);return value;}
static void dump(const char*path){char b[4096];int fd=open(path,O_RDONLY);if(fd<0)return;ssize_t n=read(fd,b,sizeof(b));if(n>0){printf("%s:\n",path);check(write(STDOUT_FILENO,b,n)==n,"pressure diagnostics");}close(fd);}
int main(void){
 setvbuf(stdout,NULL,_IONBF,0);alarm(30);priority(-1000);
 int control=socket(AF_UNIX,SOCK_SEQPACKET,0);check(control>=0,"lmkd socket");struct sockaddr_un addr={.sun_family=AF_UNIX};strcpy(addr.sun_path,"/dev/socket/lmkd");check(!connect(control,(void*)&addr,sizeof(addr)),"original lmkd connect");
 printf("LMKD connected; initial_available_kib=%lu\n",available());int ready[2],start[2];check(!pipe(ready)&&!pipe(start),"pipes");pid_t victim=fork();check(victim>=0,"victim fork");
 if(!victim){close(STDOUT_FILENO);close(STDERR_FILENO);close(ready[0]);close(control);priority(900);fill(64UL<<20);check(write(ready[1],"r",1)==1,"victim resident");for(;;)pause();}
 close(ready[1]);char byte;check(read(ready[0],&byte,1)==1,"victim readiness");register_process(control,victim,900);barrier(control);unsigned long before=available();printf("LMKD registered cached victim %d; available_kib=%lu\n",victim,before);
 pid_t allocator=fork();check(allocator>=0,"allocator fork");if(!allocator){close(STDOUT_FILENO);close(STDERR_FILENO);close(start[1]);close(control);priority(0);check(read(start[0],&byte,1)==1,"allocator start");pthread_t threads[4];for(int i=0;i<4;i++)check(!pthread_create(&threads[i],NULL,allocate,NULL),"pressure thread");for(int i=0;i<4;i++)check(!pthread_join(threads[i],NULL),"pressure join");_exit(0);}
 close(start[0]);register_process(control,allocator,0);barrier(control);printf("LMKD registered allocator %d; beginning pressure\n",allocator);check(write(start[1],"g",1)==1,"release allocator");close(start[1]);
 int status;pid_t waited=0;for(int i=0;i<800;i++){waited=waitpid(allocator,&status,WNOHANG);if(waited)break;struct timespec delay={0,10000000};nanosleep(&delay,NULL);}dump("/proc/pressure/memory");dump("/proc/vmstat");if(!waited){char path[80];snprintf(path,sizeof(path),"/proc/%d/status",allocator);dump(path);snprintf(path,sizeof(path),"/proc/%d/status",victim);dump(path);}check(waited==allocator,"allocator bounded completion");printf("allocator wait status=%d\n",status);check(WIFEXITED(status)&&WEXITSTATUS(status)==0,"allocator resumes after automatic reclaim");
 check(waitpid(victim,&status,0)==victim&&WIFSIGNALED(status)&&WTERMSIG(status)==SIGKILL,"original lmkd selected cached victim");unsigned long after=available();
 check(after>=before+48*1024,"actual private memory returned");printf("PASS original lmkd automatic PSI reclaim: victim=%d allocator=%d available_kib=%lu->%lu\n",victim,allocator,before,after);close(control);return 0;
}
