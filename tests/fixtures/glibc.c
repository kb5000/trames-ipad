// SPDX-License-Identifier: GPL-3.0-or-later
// Same complete libc suite plus GNU ABI, C23, locale and exact FP differential.
#define main libc_common_main
#include "android-bionic.c"
#undef main
#include <locale.h>
static uint64_t bits64(double x){union{double f;uint64_t u;}v={.f=x};return v.u;}
static void glibc_specific(void){
    void *lib=dlopen("libc.so.6",RTLD_NOW);CHECK(lib);
    CHECK(dlvsym(lib,"memcpy","GLIBC_2.17")==dlsym(lib,"memcpy"));
    CHECK(dlvsym(lib,"strtod","GLIBC_2.17")==dlsym(lib,"strtod"));
    CHECK(dlvsym(lib,"strlcpy","GLIBC_2.38")==dlsym(lib,"strlcpy"));
    locale_t c=newlocale(LC_ALL_MASK,"C",0);CHECK(c);
    char *end;const char *tokens[]={"0b101!","-0B111!","0b!","0x!","x!","18446744073709551616!","-9223372036854775809!"};
    const char *signed_names[]={"strtol","strtoll","strtoimax","__isoc23_strtol","__isoc23_strtoll","__isoc23_strtoimax"};
    const char *unsigned_names[]={"strtoul","strtoull","strtoumax","__isoc23_strtoul","__isoc23_strtoull","__isoc23_strtoumax"};
    for(unsigned i=0;i<sizeof(tokens)/sizeof(*tokens);i++)for(int base=0;base<=36;base++){
        for(unsigned n=0;n<6;n++){
            long(*f)(const char*,char**,int)=dlsym(lib,signed_names[n]);CHECK(f);errno=123;end=(char*)tokens[i];uint64_t value=f(tokens[i],&end,base);printf("GLIBC_INT %u %u %d %016"PRIx64" %td %d\n",i,n,base,value,end-tokens[i],errno);
            unsigned long(*u)(const char*,char**,int)=dlsym(lib,unsigned_names[n]);CHECK(u);errno=123;end=(char*)tokens[i];value=u(tokens[i],&end,base);printf("GLIBC_UINT %u %u %d %016"PRIx64" %td %d\n",i,n,base,value,end-tokens[i],errno);
        }
    }
    const char*lnames[]={"strtol_l","strtoll_l","strtoul_l","strtoull_l","__isoc23_strtol_l","__isoc23_strtoll_l","__isoc23_strtoul_l","__isoc23_strtoull_l"};
    for(unsigned n=0;n<8;n++){long(*f)(const char*,char**,int,locale_t)=dlsym(lib,lnames[n]);CHECK(f);errno=123;long value=f("-0b101!",&end,0,c);printf("GLIBC_LOCALE_INT %u %ld %td %d\n",n,value,end-"-0b101!",errno);}
    const char*inames[]={"__strtol_internal","__strtoll_internal","__strtoul_internal","__strtoull_internal"};
    for(unsigned n=0;n<4;n++){long(*f)(const char*,char**,int,int)=dlsym(lib,inames[n]);CHECK(f);CHECK(f("0x7f!",&end,0,0)==127&&*end=='!');CHECK(f("127!",&end,10,1)==127&&*end=='!');}
    double(*dl)(const char*,char**,locale_t)=dlsym(lib,"strtod_l");float(*fl)(const char*,char**,locale_t)=dlsym(lib,"strtof_l");CHECK(dl&&fl);CHECK(dl("1.25!",&end,c)==1.25&&*end=='!');CHECK(fl("1.25!",&end,c)==1.25f&&*end=='!');
    int(*cl)(const char*,const char*,locale_t)=dlsym(lib,"strcasecmp_l");int(*cn)(const char*,const char*,size_t,locale_t)=dlsym(lib,"strncasecmp_l");CHECK(cl&&cn);CHECK(cl("AbCd","aBcD",c)==0);char a[300],b[300];memset(a,'A',299);memset(b,'a',299);a[299]=b[299]=0;CHECK(cn(a,b,300,c)==0);
    // The exposed GNU locale layout allows a controlled non-C byte table. No
    // registration/free or host state is involved; original _l functions own it.
    struct __locale_struct local=*c;int lower[384];for(int i=-128;i<256;i++)lower[i+128]=c->__ctype_tolower[i];lower['I'+128]=253;local.__ctype_tolower=lower+128;local.__names[LC_CTYPE]="tr_TR.test";
    CHECK(cl("I","i",&local)>0);a[298]='I';b[298]='i';CHECK(cn(a,b,300,&local)>0);
    locale_t tr=newlocale(LC_ALL_MASK,"tr_TR.ISO-8859-9",0);CHECK(tr);CHECK(cl("I","i",tr)>0);a[298]='I';b[298]='i';CHECK(cn(a,b,300,tr)>0);freelocale(tr);
    locale_t fr=newlocale(LC_ALL_MASK,"fr_FR.UTF-8",0);CHECK(fr);CHECK(dl("1,25!",&end,fr)==1.25&&*end=='!');freelocale(fr);
    CHECK(setlocale(LC_ALL,"C.UTF-8"));CHECK(strcasecmp("Mixed","mixed")==0);CHECK(strtod("1.25!",&end)==1.25);CHECK(setlocale(LC_ALL,"C"));
    locale_t utf=newlocale(LC_ALL_MASK,"C.UTF-8",0);
    CHECK(utf);if(utf){CHECK(dl("1.25!",&end,utf)==1.25&&*end=='!');CHECK(cl("ASCII","ascii",utf)==0);freelocale(utf);}
    size_t page=sysconf(_SC_PAGESIZE);char *guard=mmap(0,page*2,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);CHECK(guard!=MAP_FAILED);CHECK(mprotect(guard+page,page,PROT_NONE)==0);memcpy(guard+page-2,"0b",2);long(*classic)(const char*,char**,int)=dlsym(lib,"strtol");CHECK(classic(guard+page-2,&end,0)==0&&end==guard+page-1);CHECK(classic(guard+page-2,&end,10)==0&&end==guard+page-1);CHECK(munmap(guard,page*2)==0);
    CHECK(fesetround(FE_TONEAREST)==0);
    double(*d)(const char*,char**)=dlsym(lib,"strtod");float(*f)(const char*,char**)=dlsym(lib,"strtof");CHECK(d&&f);
    size_t long_n=65536+17;char *long_text=malloc(long_n+16);CHECK(long_text);memset(long_text,' ',long_n);memcpy(long_text+long_n,"1.25!",6);CHECK(d(long_text,&end)==1.25&&end==long_text+long_n+4);memset(long_text,'0',long_n);memcpy(long_text+long_n,"42!",4);CHECK(classic(long_text,&end,10)==42&&end==long_text+long_n+2);free(long_text);
    const char*nan[]={"nan(123)","-nan(077)","nan(0x123)","nan(0b101)","nan(18446744073709551616)","nan(_)","nan(bad%)","nan(0xffffffffffffffffffff)","-nan()"," NAN(1)_","nan(0x8000000000000)"};
    for(unsigned i=0;i<sizeof(nan)/sizeof(*nan);i++){errno=123;feclearexcept(FE_ALL_EXCEPT);uint64_t value=bits64(d(nan[i],&end));printf("GLIBC_NAN %u %016"PRIx64" %td %d %x\n",i,value,end-nan[i],errno,fetestexcept(FE_ALL_EXCEPT));}
    uint64_t random=0x994413779acd33;const int modes[]={FE_TONEAREST,FE_UPWARD,FE_DOWNWARD,FE_TOWARDZERO};
    for(unsigned i=0;i<4096;i++){
        random=random*6364136223846793005ULL+1;char token[160];snprintf(token,sizeof(token),"%s%"PRIu64".%019"PRIu64"e%d!",random&1?"-":"",random>>5,random,(int)(i*31%1000)-500);CHECK(fesetround(modes[i%4])==0);
        errno=123;feclearexcept(FE_ALL_EXCEPT);uint64_t value=bits64(d(token,&end));size_t de=end-token;int er=errno,flags=fetestexcept(FE_ALL_EXCEPT);errno=123;feclearexcept(FE_ALL_EXCEPT);union{float f;uint32_t u;}v={.f=f(token,&end)};
        printf("GLIBC_RANDOM %u %016"PRIx64" %zu %d %x %08"PRIx32" %td %d %x\n",i,value,de,er,flags,v.u,end-token,errno,fetestexcept(FE_ALL_EXCEPT));
    }
    CHECK(fesetround(FE_TONEAREST)==0);freelocale(c);CHECK(dlclose(lib)==0);puts("PASS glibc GNU aliases, C23, locale and exact FP status");
}
int main(int argc,char**argv){if(argc==2&&!strcmp(argv[1],"preload")){const char*symbols[]={"strtol","__isoc23_strtol"};for(unsigned i=0;i<2;i++){long(*f)(const char*,char**,int)=dlsym(RTLD_DEFAULT,symbols[i]);char*end;CHECK(f&&f("interposition-probe",&end,10)==424242&&!*end);CHECK(f("123!",&end,10)==123&&*end=='!');}puts("PASS glibc interposition");return 0;}if(argc==2&&!strcmp(argv[1],"cleanup-glibc"))return unlink("/usr/lib/aarch64-linux-gnu/libc.so.6.trames-new")!=0;int result=libc_common_main(argc,argv);if(argc==1)glibc_specific();return result;}
