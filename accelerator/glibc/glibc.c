// SPDX-License-Identifier: GPL-3.0-or-later
// Cold-selected Debian 13 libc derivative. No interposition, startup hooks,
// CPU feature queries, host pointers, allocator replacement or new global state.
#include <stddef.h>
#include <stdint.h>
struct result { uintptr_t value, fallback; };
#define HIDDEN __attribute__((visibility("hidden")))
#define CALL(id) extern HIDDEN struct result trames_call_##id(uintptr_t,uintptr_t,size_t)
CALL(1);CALL(2);CALL(3);CALL(4);CALL(5);CALL(6);CALL(7);CALL(8);CALL(9);CALL(10);CALL(11);CALL(12);CALL(13);CALL(14);CALL(15);CALL(16);CALL(17);CALL(18);CALL(19);CALL(20);CALL(21);CALL(22);CALL(23);CALL(24);CALL(25);CALL(26);CALL(27);CALL(28);CALL(29);CALL(30);
extern HIDDEN struct result trames_glibc_call_1(uintptr_t,uintptr_t,size_t);
extern HIDDEN struct result trames_glibc_call_2(uintptr_t,uintptr_t,size_t);
extern HIDDEN int *trames_errno_location(void);
extern HIDDEN void *trames_uselocale(void *);
extern HIDDEN unsigned char trames_C_tolower[],trames_global_locale[];
struct locale_abi { const void *categories[13]; const unsigned short *ctype_b; const int *tolower,*toupper; const char *names[13]; };
static int c_name(const char *s){
    if(s[0]=='C' && !s[1])return 1;
    if(s[0]=='P'&&s[1]=='O'&&s[2]=='S'&&s[3]=='I'&&s[4]=='X'&&!s[5])return 1;
    return s[0]=='C'&&s[1]=='.'&&(s[2]=='U'||s[2]=='u')&&(s[3]=='T'||s[3]=='t')&&(s[4]=='F'||s[4]=='f')&&((s[5]=='8'&&!s[6])||(s[5]=='-'&&s[6]=='8'&&!s[7]));
}
static int c_locale(void *locale,int numeric){
    struct locale_abi *loc=locale;
    if(!loc){loc=trames_uselocale(0);if((intptr_t)loc==-1)loc=(void*)trames_global_locale;}
    else if((intptr_t)loc==-1)return 0; // invalid explicit _l argument: original owns its behavior
    if(!numeric && loc->tolower==(const int*)trames_C_tolower)return 1;
    return c_name(loc->names[0])&&(!numeric||c_name(loc->names[1]));
}
static inline __attribute__((always_inline)) size_t prefix(const char*s){
    // Same aligned-word prefix as Bionic: no word crosses a guest page, and
    // short strlen returns its known result without scanning the string twice.
    typedef size_t __attribute__((__may_alias__)) word;
    const unsigned char*p=(const unsigned char*)s,*start=p;
    while((uintptr_t)p&(sizeof(word)-1)){if(!*p)return(size_t)(p-start);++p;}
    const size_t ones=(size_t)-1/255,highs=ones*128;
    while((size_t)(p-start)+sizeof(word)<=16){
        size_t value=*(const word*)p,zero=(value-ones)&~value&highs;
        if(zero)return(size_t)(p-start)+(size_t)__builtin_ctzl(zero)/8;
        p+=sizeof(word);
    }
    while((size_t)(p-start)<16){if(!*p)return(size_t)(p-start);++p;}
    return 16;
}
static int short_string(const char*s){return prefix(s)<16;}
#define ORIGINAL(type,name,args) extern HIDDEN type trames_orig_##name args
#define MEMORY(name,id) \
ORIGINAL(void*,name,(void*,const void*,size_t)); \
void *trames_glibc_##name(void*d,const void*s,size_t n){if(n>=128){struct result r=trames_call_##id((uintptr_t)d,(uintptr_t)s,n);if(!r.fallback)return(void*)r.value;}return trames_orig_##name(d,s,n);}
MEMORY(memcpy,1) MEMORY(memmove,2) MEMORY(mempcpy,28)
ORIGINAL(void*,memset,(void*,int,size_t));
void *trames_glibc_memset(void*d,int c,size_t n){if(n>=128){struct result r=trames_call_3((uintptr_t)d,(uintptr_t)c,n);if(!r.fallback)return(void*)r.value;}return trames_orig_memset(d,c,n);}
ORIGINAL(int,memcmp,(const void*,const void*,size_t));
int trames_glibc_memcmp(const void*a,const void*b,size_t n){if(n>=128){struct result r=trames_call_4((uintptr_t)a,(uintptr_t)b,n);if(!r.fallback)return(int)r.value;}return trames_orig_memcmp(a,b,n);}
#define MEMSEARCH(name,id) \
ORIGINAL(void*,name,(const void*,int,size_t)); \
void *trames_glibc_##name(const void*s,int c,size_t n){if(n>=128){struct result r=trames_call_##id((uintptr_t)s,(uintptr_t)c,n);if(!r.fallback)return(void*)r.value;}return trames_orig_##name(s,c,n);}
MEMSEARCH(memchr,6) MEMSEARCH(memrchr,7)
ORIGINAL(size_t,strlen,(const char*)); ORIGINAL(size_t,strnlen,(const char*,size_t));
size_t trames_glibc_strlen(const char*s){size_t n=prefix(s);if(n<16)return n;struct result r=trames_call_5((uintptr_t)s,0,0);return r.fallback?trames_orig_strlen(s):r.value;}
size_t trames_glibc_strnlen(const char*s,size_t n){if(n>=128&&!short_string(s)){struct result r=trames_call_8((uintptr_t)s,n,0);if(!r.fallback)return r.value;}return trames_orig_strnlen(s,n);}
ORIGINAL(int,strcmp,(const char*,const char*)); ORIGINAL(int,strncmp,(const char*,const char*,size_t));
int trames_glibc_strcmp(const char*a,const char*b){if(!short_string(a)){struct result r=trames_call_9((uintptr_t)a,(uintptr_t)b,0);if(!r.fallback)return(int)r.value;}return trames_orig_strcmp(a,b);}
int trames_glibc_strncmp(const char*a,const char*b,size_t n){if(n>=128){struct result r=trames_call_10((uintptr_t)a,(uintptr_t)b,n);if(!r.fallback)return(int)r.value;}return trames_orig_strncmp(a,b,n);}
#define CHARSEARCH(name,id) \
ORIGINAL(char*,name,(const char*,int)); \
char *trames_glibc_##name(const char*s,int c){if(!short_string(s)){struct result r=trames_call_##id((uintptr_t)s,(uintptr_t)c,0);if(!r.fallback)return(char*)r.value;}return trames_orig_##name(s,c);}
CHARSEARCH(strchr,11) CHARSEARCH(strrchr,12) CHARSEARCH(strchrnul,13)
#define STRINGCOPY(name,id) \
ORIGINAL(char*,name,(char*,const char*)); \
char *trames_glibc_##name(char*d,const char*s){if(!short_string(s)){struct result r=trames_call_##id((uintptr_t)d,(uintptr_t)s,0);if(!r.fallback)return(char*)r.value;}return trames_orig_##name(d,s);}
STRINGCOPY(strcpy,14) STRINGCOPY(stpcpy,15) STRINGCOPY(strcat,18)
#define STRINGBOUNDED(type,name,id) \
ORIGINAL(type,name,(char*,const char*,size_t)); \
type trames_glibc_##name(char*d,const char*s,size_t n){if(n>=128){struct result r=trames_call_##id((uintptr_t)d,(uintptr_t)s,n);if(!r.fallback)return(type)r.value;}return trames_orig_##name(d,s,n);}
STRINGBOUNDED(char*,strncpy,16) STRINGBOUNDED(char*,stpncpy,17) STRINGBOUNDED(char*,strncat,19) STRINGBOUNDED(size_t,strlcat,30)
ORIGINAL(size_t,strlcpy,(char*,const char*,size_t));
size_t trames_glibc_strlcpy(char*d,const char*s,size_t n){if(!short_string(s)){struct result r=trames_call_29((uintptr_t)d,(uintptr_t)s,n);if(!r.fallback)return r.value;}return trames_orig_strlcpy(d,s,n);}
ORIGINAL(void*,memmem,(const void*,size_t,const void*,size_t));
void *trames_glibc_memmem(const void*h,size_t len,const void*n,size_t count){if(!count)return(void*)h;if(count>len)return 0;if(len>=128){uintptr_t a[4]={(uintptr_t)h,len,(uintptr_t)n,count};struct result r=trames_call_20((uintptr_t)a,0,0);if(!r.fallback)return(void*)r.value;}return trames_orig_memmem(h,len,n,count);}
#define SUBSTRING(name,id,locale) \
ORIGINAL(char*,name,(const char*,const char*)); \
char *trames_glibc_##name(const char*s,const char*n){if(!*n)return(char*)s;if(!short_string(s)&&(!(locale)||c_locale(0,0))){struct result r=trames_call_##id((uintptr_t)s,(uintptr_t)n,0);if(!r.fallback)return(char*)r.value;}return trames_orig_##name(s,n);}
SUBSTRING(strstr,21,0) SUBSTRING(strcasestr,22,1)
#define SET(type,name,id) \
ORIGINAL(type,name,(const char*,const char*)); \
type trames_glibc_##name(const char*s,const char*set){if(!short_string(s)){struct result r=trames_call_##id((uintptr_t)s,(uintptr_t)set,0);if(!r.fallback)return(type)r.value;}return trames_orig_##name(s,set);}
SET(size_t,strspn,23) SET(size_t,strcspn,24) SET(char*,strpbrk,25)
ORIGINAL(int,strcasecmp,(const char*,const char*)); ORIGINAL(int,strncasecmp,(const char*,const char*,size_t));
ORIGINAL(int,strcasecmp_l,(const char*,const char*,void*)); ORIGINAL(int,strncasecmp_l,(const char*,const char*,size_t,void*));
int trames_glibc_strcasecmp(const char*a,const char*b){if(c_locale(0,0)){struct result r=trames_call_26((uintptr_t)a,(uintptr_t)b,0);if(!r.fallback)return(int)r.value;}return trames_orig_strcasecmp(a,b);}
int trames_glibc_strncasecmp(const char*a,const char*b,size_t n){if(n>=128&&c_locale(0,0)){struct result r=trames_call_27((uintptr_t)a,(uintptr_t)b,n);if(!r.fallback)return(int)r.value;}return trames_orig_strncasecmp(a,b,n);}
int trames_glibc_strcasecmp_l(const char*a,const char*b,void*l){if(c_locale(l,0)){struct result r=trames_call_26((uintptr_t)a,(uintptr_t)b,0);if(!r.fallback)return(int)r.value;}return trames_orig_strcasecmp_l(a,b,l);}
int trames_glibc_strncasecmp_l(const char*a,const char*b,size_t n,void*l){if(n>=128&&c_locale(l,0)){struct result r=trames_call_27((uintptr_t)a,(uintptr_t)b,n);if(!r.fallback)return(int)r.value;}return trames_orig_strncasecmp_l(a,b,n,l);}
// Cursor state remains caller-owned, matching the original tokenizer algorithm.
char *trames_glibc_strsep(char**s,const char*d){char*start=*s;if(!start)return 0;size_t n=trames_glibc_strcspn(start,d);if(start[n]){start[n]=0;*s=start+n+1;}else *s=0;return start;}
char *trames_glibc_strtok_r(char*s,const char*d,char**save){if(!s)s=*save;s+=trames_glibc_strspn(s,d);if(!*s){*save=s;return 0;}char*start=s;s+=trames_glibc_strcspn(s,d);if(*s)*s++=0;*save=s;return start;}
struct numeric_output{uintptr_t end,error,fpsr;};
static struct result integer(const char*s,char**end,int base,int is_unsigned,int binary,void*l){
    struct result fallback={0,1};if((base!=0&&(base<2||base>36))||!c_locale(l,0))return fallback;
    struct numeric_output out;
    struct result r=trames_glibc_call_1((uintptr_t)s,(uintptr_t)&out,(uint32_t)base|((uintptr_t)is_unsigned<<32)|((uintptr_t)binary<<33));
    if(!r.fallback){if(out.error)*trames_errno_location()=(int)out.error;if(end)*end=(char*)out.end;}return r;
}
#define INTEGER(type,name,u,binary) \
ORIGINAL(type,name,(const char*,char**,int)); \
type trames_glibc_##name(const char*s,char**end,int base){struct result r=integer(s,end,base,u,binary,0);return r.fallback?trames_orig_##name(s,end,base):(type)r.value;}
#define INTEGER_L(type,name,u,binary) \
ORIGINAL(type,name,(const char*,char**,int,void*)); \
type trames_glibc_##name(const char*s,char**end,int base,void*l){struct result r=integer(s,end,base,u,binary,l);return r.fallback?trames_orig_##name(s,end,base,l):(type)r.value;}
#define INTEGER_INTERNAL(type,name,u) \
ORIGINAL(type,name,(const char*,char**,int,int)); \
type trames_glibc_##name(const char*s,char**end,int base,int group){if(!group){struct result r=integer(s,end,base,u,0,0);if(!r.fallback)return(type)r.value;}return trames_orig_##name(s,end,base,group);}
INTEGER(long,strtol,0,0) INTEGER(unsigned long,strtoul,1,0)
INTEGER(long,__isoc23_strtol,0,1) INTEGER(unsigned long,__isoc23_strtoul,1,1)
INTEGER_L(long,strtol_l,0,0) INTEGER_L(unsigned long,strtoul_l,1,0)
INTEGER_L(long,__isoc23_strtol_l,0,1) INTEGER_L(unsigned long,__isoc23_strtoul_l,1,1)
INTEGER_INTERNAL(long,__strtol_internal,0) INTEGER_INTERNAL(unsigned long,__strtoul_internal,1)
int trames_glibc_atoi(const char*s){return(int)trames_glibc_strtol(s,0,10);}
long trames_glibc_atol(const char*s){return trames_glibc_strtol(s,0,10);}
long long trames_glibc_atoll(const char*s){return trames_glibc_strtol(s,0,10);}
static struct result floating(const char*s,char**end,int single,void*l){
    struct result fallback={0,1};uintptr_t fpcr;__asm__ volatile("mrs %0, fpcr":"=r"(fpcr));
    if(fpcr||!c_locale(l,1))return fallback;
    struct numeric_output out;struct result r=trames_glibc_call_2((uintptr_t)s,(uintptr_t)&out,single);
    if(!r.fallback){if(out.error)*trames_errno_location()=(int)out.error;if(end)*end=(char*)out.end;uintptr_t fpsr;__asm__ volatile("mrs %0, fpsr":"=r"(fpsr));fpsr|=out.fpsr;__asm__ volatile("msr fpsr, %0"::"r"(fpsr));}return r;
}
#define FLOATING(type,bittype,name,single) \
ORIGINAL(type,name,(const char*,char**)); \
type trames_glibc_##name(const char*s,char**end){struct result r=floating(s,end,single,0);if(r.fallback)return trames_orig_##name(s,end);union{bittype bits;type value;}v={.bits=(bittype)r.value};return v.value;}
#define FLOATING_L(type,bittype,name,single) \
ORIGINAL(type,name,(const char*,char**,void*)); \
type trames_glibc_##name(const char*s,char**end,void*l){struct result r=floating(s,end,single,l);if(r.fallback)return trames_orig_##name(s,end,l);union{bittype bits;type value;}v={.bits=(bittype)r.value};return v.value;}
#define FLOATING_INTERNAL(type,bittype,name,single) \
ORIGINAL(type,name,(const char*,char**,int)); \
type trames_glibc_##name(const char*s,char**end,int group){if(!group){struct result r=floating(s,end,single,0);if(!r.fallback){union{bittype bits;type value;}v={.bits=(bittype)r.value};return v.value;}}return trames_orig_##name(s,end,group);}
FLOATING(double,uintptr_t,strtod,0) FLOATING(float,uint32_t,strtof,1)
FLOATING_L(double,uintptr_t,strtod_l,0) FLOATING_L(float,uint32_t,strtof_l,1)
FLOATING_INTERNAL(double,uintptr_t,__strtod_internal,0) FLOATING_INTERNAL(float,uint32_t,__strtof_internal,1)
double trames_glibc_atof(const char*s){return trames_glibc_strtod(s,0);}
