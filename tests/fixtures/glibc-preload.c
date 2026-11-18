// SPDX-License-Identifier: GPL-3.0-or-later
#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
static long convert(const char*s,char**end,int base,const char*symbol,const char*version){
    if(!strcmp(s,"interposition-probe")){if(end)*end=(char*)s+strlen(s);return 424242;}
    long(*original)(const char*,char**,int)=dlvsym(RTLD_NEXT,symbol,version);
    return original(s,end,base);
}
// glibc 2.41 headers redirect the C spelling strtol to __isoc23_strtol.
// Explicit assembler names exercise interposition of both public ABI entries.
long classic_strtol(const char*,char**,int) __asm__("strtol");
long c23_strtol(const char*,char**,int) __asm__("__isoc23_strtol");
long classic_strtol(const char*s,char**end,int base){return convert(s,end,base,"strtol","GLIBC_2.17");}
long c23_strtol(const char*s,char**end,int base){return convert(s,end,base,"__isoc23_strtol","GLIBC_2.38");}
