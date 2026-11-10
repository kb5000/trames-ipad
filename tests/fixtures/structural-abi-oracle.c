#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/uio.h>
static long raw(uint64_t n,uint64_t a,uint64_t b,uint64_t c,uint64_t d,uint64_t e,uint64_t f){
 register uint64_t x8 __asm__("x8")=n,x0 __asm__("x0")=a,x1 __asm__("x1")=b,x2 __asm__("x2")=c,x3 __asm__("x3")=d,x4 __asm__("x4")=e,x5 __asm__("x5")=f;
 __asm__ volatile("svc 0":"+r"(x0):"r"(x8),"r"(x1),"r"(x2),"r"(x3),"r"(x4),"r"(x5):"memory","cc");return (long)x0;
}
int main(void){
 uint64_t upper=1ULL<<32;long pid=raw(172,0,0,0,0,0,0),wide=raw(upper|172,0,0,0,0,0,0);unsigned char params[120]={0},probe[1024]={0};
 long fd=raw(upper|425,4,(uintptr_t)params,0,0,0,0);printf("wide_number_same_pid=%d io_uring_setup=%ld\n",pid==wide,fd);
 if(fd<0)return pid==wide?0:1;
 void *page=0;if(posix_memalign(&page,4096,4096))return 2;memset(page,0,4096);struct iovec vec={page,4096};
 long a=raw(upper|427,fd,upper|0,(uintptr_t)&vec,upper|1,0,0);
 long b=raw(upper|427,fd,upper|1,0,upper|0,0,0);
 long c=raw(upper|427,fd,upper|8,(uintptr_t)probe,upper|64,0,0);
 printf("wide_buffer_registration=%ld wide_unregistration=%ld wide_probe=%ld\n",a,b,c);raw(57,fd,0,0,0,0,0);free(page);return pid==wide&&a==0&&b==0&&c==0?0:1;
}
