// SPDX-License-Identifier: GPL-3.0-or-later
#define _GNU_SOURCE
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <stdint.h>
#include <locale.h>
#define PRIVATE __attribute__((visibility("hidden")))
extern PRIVATE unsigned __trames_native_features;
#define available(id) (__trames_native_features & (1u << ((id)-1)))
struct result { uintptr_t value, fallback; };
#define CALL(id) extern PRIVATE struct result trames_call_##id(uintptr_t,uintptr_t,size_t)
CALL(20); CALL(21); CALL(22); CALL(23); CALL(24); CALL(25); CALL(26); CALL(27); CALL(28); CALL(29); CALL(30); CALL(31);
#define ORIGINAL(name) extern PRIVATE __typeof__(name) trames_orig_##name
ORIGINAL(memmem); ORIGINAL(strstr); ORIGINAL(strcasestr); ORIGINAL(strspn); ORIGINAL(strcspn); ORIGINAL(strpbrk);
ORIGINAL(strcasecmp); ORIGINAL(strncasecmp); ORIGINAL(mempcpy); ORIGINAL(strlcpy); ORIGINAL(strlcat);
ORIGINAL(atoi); ORIGINAL(atol); ORIGINAL(atoll);
// Local work for short strings avoids a native boundary and stops before any
// unreached guard page. This is size dispatch, never architecture dispatch.
static int short_string(const char *s) {
    for (unsigned i=0;i<16;++i) if (!s[i]) return 1;
    return 0;
}
void *memmem(const void *h,size_t len,const void *n,size_t count) {
    if (!count) return (void *)h;
    if (count>len) return 0;
    if (available(20) && len>=128) {
        uintptr_t request[4]={(uintptr_t)h,len,(uintptr_t)n,count};
        struct result r=trames_call_20((uintptr_t)request,0,0); if(!r.fallback)return (void *)r.value;
    }
    return trames_orig_memmem(h,len,n,count);
}
#define SUBSTRING(name,id) \
char *name(const char *s,const char *needle) { \
    if (!*needle) return (char *)s; \
    if (available(id) && !short_string(s)) { \
        struct result r=trames_call_##id((uintptr_t)s,(uintptr_t)needle,0); if(!r.fallback)return (char *)r.value; \
    } \
    return trames_orig_##name(s,needle); \
}
SUBSTRING(strstr,21)
SUBSTRING(strcasestr,22)
#define SET_SCAN(type,name,id) \
type name(const char *s,const char *set) { \
    if (available(id) && !short_string(s)) { \
        struct result r=trames_call_##id((uintptr_t)s,(uintptr_t)set,0); if(!r.fallback)return (type)r.value; \
    } \
    return trames_orig_##name(s,set); \
}
SET_SCAN(size_t,strspn,23)
SET_SCAN(size_t,strcspn,24)
SET_SCAN(char *,strpbrk,25)
int strcasecmp(const char *a,const char *b) {
    if(available(26)) { struct result r=trames_call_26((uintptr_t)a,(uintptr_t)b,0); if(!r.fallback)return (int)r.value; }
    return trames_orig_strcasecmp(a,b);
}
int strncasecmp(const char *a,const char *b,size_t n) {
    if(available(27) && n>=128) { struct result r=trames_call_27((uintptr_t)a,(uintptr_t)b,n); if(!r.fallback)return (int)r.value; }
    return trames_orig_strncasecmp(a,b,n);
}
int __strcasecmp_l(const char *a,const char *b,locale_t locale) { (void)locale; return strcasecmp(a,b); }
int __strncasecmp_l(const char *a,const char *b,size_t n,locale_t locale) { (void)locale; return strncasecmp(a,b,n); }
weak_alias(__strcasecmp_l,strcasecmp_l);
weak_alias(__strncasecmp_l,strncasecmp_l);
void *mempcpy(void *d,const void *s,size_t n) {
    if(available(28) && n>=128) { struct result r=trames_call_28((uintptr_t)d,(uintptr_t)s,n); if(!r.fallback)return (void *)r.value; }
    return trames_orig_mempcpy(d,s,n);
}
size_t strlcpy(char *d,const char *s,size_t n) {
    if(available(29) && !short_string(s)) { struct result r=trames_call_29((uintptr_t)d,(uintptr_t)s,n); if(!r.fallback)return r.value; }
    return trames_orig_strlcpy(d,s,n);
}
size_t strlcat(char *d,const char *s,size_t n) {
    if(available(30) && n>=128) { struct result r=trames_call_30((uintptr_t)d,(uintptr_t)s,n); if(!r.fallback)return r.value; }
    return trames_orig_strlcat(d,s,n);
}
#define PLAIN_INT(type,name,bits) \
type name(const char *s) { \
    if(available(31)) { struct result r=trames_call_31((uintptr_t)s,bits,0); if(!r.fallback)return (type)r.value; } \
    return trames_orig_##name(s); \
}
PLAIN_INT(int,atoi,32)
PLAIN_INT(long,atol,64)
PLAIN_INT(long long,atoll,64)
