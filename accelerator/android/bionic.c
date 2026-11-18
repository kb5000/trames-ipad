// SPDX-License-Identifier: GPL-3.0-or-later
// Statically linked into the interpreter's replacement Bionic libc.so.
// No architecture, auxv, feature bitmap, TLS or constructor on this path.
#include <stddef.h>
#include <stdint.h>

struct result { uintptr_t value, fallback; };
#define CALL(n) extern __attribute__((visibility("hidden"))) struct result trames_call_##n(uintptr_t, uintptr_t, size_t)
CALL(1); CALL(2); CALL(3); CALL(4); CALL(5); CALL(6); CALL(7);
CALL(8); CALL(9); CALL(10); CALL(11); CALL(12); CALL(13);
CALL(14); CALL(15); CALL(16); CALL(17); CALL(18); CALL(19); CALL(29); CALL(30);
#define BCALL(n) extern __attribute__((visibility("hidden"))) struct result trames_bionic_call_##n(uintptr_t, uintptr_t, size_t)
BCALL(1); BCALL(2); BCALL(3); BCALL(4); BCALL(5); BCALL(6); BCALL(7); BCALL(8); BCALL(9); BCALL(10); BCALL(13);
#define ORIGINAL(type, name, args) extern __attribute__((visibility("hidden"))) type trames_orig_##name args
ORIGINAL(void *, memcpy, (void *, const void *, size_t));
ORIGINAL(void *, memmove, (void *, const void *, size_t));
ORIGINAL(void *, memset, (void *, int, size_t));
ORIGINAL(int, memcmp, (const void *, const void *, size_t));
ORIGINAL(size_t, strlen, (const char *));
ORIGINAL(void *, memchr, (const void *, int, size_t));
ORIGINAL(void *, memrchr, (const void *, int, size_t));
ORIGINAL(size_t, strnlen, (const char *, size_t));
ORIGINAL(int, strcmp, (const char *, const char *));
ORIGINAL(int, strncmp, (const char *, const char *, size_t));
ORIGINAL(char *, strchr, (const char *, int));
ORIGINAL(char *, strrchr, (const char *, int));
ORIGINAL(char *, strchrnul, (const char *, int));
ORIGINAL(char *, strcpy, (char *, const char *));
ORIGINAL(char *, stpcpy, (char *, const char *));
ORIGINAL(char *, strncpy, (char *, const char *, size_t));
ORIGINAL(char *, stpncpy, (char *, const char *, size_t));
ORIGINAL(char *, strcat, (char *, const char *));
ORIGINAL(char *, strncat, (char *, const char *, size_t));
ORIGINAL(void *, mempcpy, (void *, const void *, size_t));
ORIGINAL(size_t, strlcpy, (char *, const char *, size_t));
ORIGINAL(size_t, strlcat, (char *, const char *, size_t));
extern __attribute__((visibility("hidden"))) void *trames_original_small_copy(void *, const void *, size_t);
extern __attribute__((visibility("hidden"))) size_t trames_bionic_strlen(const char *);

// Sized short calls stay in the original Bionic routines. Prefixes align before
// word loads, so a NUL beside a protected page never reads the following page.
#define BULK 128
#define PREFIX 16
static inline __attribute__((always_inline)) size_t prefix(const char *s) {
    typedef size_t __attribute__((__may_alias__)) word;
    const unsigned char *p = (const unsigned char *)s, *start = p;
    while ((uintptr_t)p & (sizeof(word) - 1)) {
        if (!*p) return (size_t)(p - start);
        ++p;
    }
    const size_t ones = (size_t)-1 / 255, highs = ones * 128;
    while ((size_t)(p - start) + sizeof(word) <= PREFIX) {
        size_t value = *(const word *)p;
        size_t zero = (value - ones) & ~value & highs;
        if (zero) return (size_t)(p - start) + (size_t)__builtin_ctzl(zero) / 8;
        p += sizeof(word);
    }
    while ((size_t)(p - start) < PREFIX) {
        if (!*p) return (size_t)(p - start);
        ++p;
    }
    return PREFIX;
}

// Every complete request crosses the native entry once. Backing admission may
// select original code before effects; byte count never selects a continuation.
#define BULK_MEMORY(name, id, original) \
void *trames_bionic_large_##name(void *d, const void *s, size_t n) { \
    struct result r = trames_call_##id((uintptr_t)d, (uintptr_t)s, n); \
    return r.fallback ? original(d, s, n) : (void *)r.value; \
}
BULK_MEMORY(memcpy, 1, trames_orig_memcpy)
BULK_MEMORY(memmove, 2, trames_orig_memmove)
void *trames_bionic_large_memset(void *d, int c, size_t n) {
    struct result r = trames_call_3((uintptr_t)d, (uintptr_t)c, n);
    return r.fallback ? trames_orig_memset(d, c, n) : (void *)r.value;
}
int trames_bionic_large_memcmp(const void *a, const void *b, size_t n) {
    struct result r = trames_call_4((uintptr_t)a, (uintptr_t)b, n);
    return r.fallback ? trames_orig_memcmp(a, b, n) : (int)r.value;
}
size_t trames_bionic_unaligned_strlen(const char *s) {
    size_t n = prefix(s);
    if (n < PREFIX) return n;
    struct result r = trames_call_5((uintptr_t)s, 0, 0);
    return r.fallback ? trames_orig_strlen(s) : r.value;
}
size_t trames_bionic_long_strlen(const char *s) {
    struct result r = trames_call_5((uintptr_t)s, 0, 0);
    return r.fallback ? trames_orig_strlen(s) : r.value;
}
#define SEARCH(name, id) \
void *trames_bionic_large_##name(const void *s, int c, size_t n) { \
    struct result r = trames_call_##id((uintptr_t)s, (uintptr_t)c, n); \
    return r.fallback ? trames_orig_##name(s, c, n) : (void *)r.value; \
}
SEARCH(memchr, 6)
SEARCH(memrchr, 7)
size_t trames_bionic_large_strnlen(const char *s, size_t n) {
    size_t len = prefix(s); if (len < PREFIX) return len;
    struct result r = trames_call_8((uintptr_t)s, n, 0);
    return r.fallback ? trames_orig_strnlen(s, n) : r.value;
}
int trames_bionic_strcmp(const char *a, const char *b) {
    for (size_t i=0; i<PREFIX; ++i) {
        unsigned char x=a[i], y=b[i]; if (x!=y || !x) return (int)x-(int)y;
    }
    struct result r=trames_call_9((uintptr_t)a,(uintptr_t)b,0);
    return r.fallback ? trames_orig_strcmp(a,b) : (int)r.value;
}
int trames_bionic_large_strncmp(const char *a, const char *b, size_t n) {
    for (size_t i=0; i<PREFIX; ++i) {
        unsigned char x=a[i], y=b[i]; if (x!=y || !x) return (int)x-(int)y;
    }
    struct result r=trames_call_10((uintptr_t)a,(uintptr_t)b,n);
    return r.fallback ? trames_orig_strncmp(a,b,n) : (int)r.value;
}
#define CHAR_SEARCH(name,id) \
char *trames_bionic_##name(const char *s,int c) { \
    for (size_t i=0;i<PREFIX;++i) { \
        unsigned char x=s[i]; if(x==(unsigned char)c) return (char *)s+i; \
        if(!x) return id==13 ? (char *)s+i : NULL; \
    } \
    struct result r=trames_call_##id((uintptr_t)s,(uintptr_t)c,0); \
    return r.fallback ? trames_orig_##name(s,c) : (char *)r.value; \
}
CHAR_SEARCH(strchr,11)
CHAR_SEARCH(strchrnul,13)
char *trames_bionic_strrchr(const char *s,int c) {
    if(prefix(s)<PREFIX) return trames_orig_strrchr(s,c);
    struct result r=trames_call_12((uintptr_t)s,(uintptr_t)c,0);
    return r.fallback ? trames_orig_strrchr(s,c) : (char *)r.value;
}
#define COPY(name,id) \
char *trames_bionic_##name(char *d,const char *s) { \
    if(prefix(s)<PREFIX) return trames_orig_##name(d,s); \
    struct result r=trames_call_##id((uintptr_t)d,(uintptr_t)s,0); \
    return r.fallback ? trames_orig_##name(d,s) : (char *)r.value; \
}
COPY(strcpy,14)
COPY(stpcpy,15)
#define SIZED_COPY(name,id) \
char *trames_bionic_large_##name(char *d,const char *s,size_t n) { \
    struct result r=trames_call_##id((uintptr_t)d,(uintptr_t)s,n); \
    return r.fallback ? trames_orig_##name(d,s,n) : (char *)r.value; \
}
SIZED_COPY(strncpy,16)
SIZED_COPY(stpncpy,17)
char *trames_bionic_strcat(char *d,const char *s) {
    if(prefix(d)<PREFIX && prefix(s)<PREFIX) return trames_orig_strcat(d,s);
    struct result r=trames_call_18((uintptr_t)d,(uintptr_t)s,0);
    return r.fallback ? trames_orig_strcat(d,s) : (char *)r.value;
}
char *trames_bionic_strncat(char *d,const char *s,size_t n) {
    if(!n) return d;
    if(n<BULK && prefix(d)<PREFIX) return trames_orig_strncat(d,s,n);
    struct result r=trames_call_19((uintptr_t)d,(uintptr_t)s,n);
    return r.fallback ? trames_orig_strncat(d,s,n) : (char *)r.value;
}
void *trames_bionic_large_mempcpy(void *d,const void *s,size_t n) {
    struct result r=trames_call_1((uintptr_t)d,(uintptr_t)s,n);
    return r.fallback ? trames_orig_mempcpy(d,s,n) : (char *)d+n;
}
size_t trames_bionic_strlcpy(char *d,const char *s,size_t n) {
    if(prefix(s)<PREFIX) return trames_orig_strlcpy(d,s,n);
    struct result r=trames_call_29((uintptr_t)d,(uintptr_t)s,n);
    return r.fallback ? trames_orig_strlcpy(d,s,n) : r.value;
}
size_t trames_bionic_large_strlcat(char *d,const char *s,size_t n) {
    struct result r=trames_call_30((uintptr_t)d,(uintptr_t)s,n);
    return r.fallback ? trames_orig_strlcat(d,s,n) : r.value;
}
#define BDECL(type,name,args) ORIGINAL(type,name,args)
BDECL(void *,memmem,(const void *,size_t,const void *,size_t));
BDECL(char *,strstr,(const char *,const char *));
BDECL(char *,strcasestr,(const char *,const char *));
BDECL(size_t,strspn,(const char *,const char *));
BDECL(size_t,strcspn,(const char *,const char *));
BDECL(char *,strpbrk,(const char *,const char *));
BDECL(int,strcasecmp,(const char *,const char *));
BDECL(int,strncasecmp,(const char *,const char *,size_t));
void *trames_bionic_memmem(const void *hay,size_t count,const void *needle,size_t n) {
    if(!n) return (void *)hay;
    if(n>count) return NULL;
    if(count<BULK) return trames_orig_memmem(hay,count,needle,n);
    uintptr_t request[4]={(uintptr_t)hay,count,(uintptr_t)needle,n};
    struct result r=trames_bionic_call_1((uintptr_t)request,0,0);
    return r.fallback ? trames_orig_memmem(hay,count,needle,n) : (void *)r.value;
}
#define SUBSTRING(name,id) \
char *trames_bionic_##name(const char *s,const char *needle) { \
    if(!*needle) return (char *)s; \
    if(prefix(s)<PREFIX) return trames_orig_##name(s,needle); \
    struct result r=trames_bionic_call_##id((uintptr_t)s,(uintptr_t)needle,0); \
    return r.fallback ? trames_orig_##name(s,needle) : (char *)r.value; \
}
SUBSTRING(strstr,2)
SUBSTRING(strcasestr,3)
#define SPAN(type,name,id) \
type trames_bionic_##name(const char *s,const char *set) { \
    if(prefix(s)<PREFIX) return trames_orig_##name(s,set); \
    struct result r=trames_bionic_call_##id((uintptr_t)s,(uintptr_t)set,0); \
    return r.fallback ? trames_orig_##name(s,set) : (type)r.value; \
}
SPAN(size_t,strspn,4)
SPAN(size_t,strcspn,5)
SPAN(char *,strpbrk,6)
static unsigned char lower(unsigned char c) { return c>='A' && c<='Z' ? c+32 : c; }
int trames_bionic_strcasecmp(const char *a,const char *b) {
    for(size_t i=0;i<PREFIX;++i) { unsigned char x=lower(a[i]),y=lower(b[i]); if(x!=y || !x) return (int)x-(int)y; }
    struct result r=trames_bionic_call_7((uintptr_t)a,(uintptr_t)b,0);
    return r.fallback ? trames_orig_strcasecmp(a,b) : (int)r.value;
}
int trames_bionic_strncasecmp(const char *a,const char *b,size_t n) {
    if(n<BULK) return trames_orig_strncasecmp(a,b,n);
    struct result r=trames_bionic_call_8((uintptr_t)a,(uintptr_t)b,n);
    return r.fallback ? trames_orig_strncasecmp(a,b,n) : (int)r.value;
}
char *trames_bionic_strsep(char **state,const char *set) {
    char *start=*state; if(!start) return NULL;
    size_t n=trames_bionic_strcspn(start,set);
    if(start[n]) { start[n]=0; *state=start+n+1; } else *state=NULL;
    return start;
}
char *trames_bionic_strtok_r(char *s,const char *set,char **state) {
    if(!s) s=*state; if(!s) return NULL;
    s+=trames_bionic_strspn(s,set); if(!*s) { *state=NULL; return NULL; }
    char *start=s; s+=trames_bionic_strcspn(s,set);
    if(*s) { *s++=0; *state=s; } else *state=NULL;
    return start;
}

struct numeric_output { uintptr_t end, error, fpsr; };
extern __attribute__((visibility("hidden"))) int *trames_bionic_errno(void);
extern __attribute__((visibility("hidden"))) struct result trames_integer_step(uintptr_t, uintptr_t, uintptr_t);
ORIGINAL(long, strtol, (const char *, char **, int));
ORIGINAL(unsigned long, strtoul, (const char *, char **, int));
ORIGINAL(long long, strtoll, (const char *, char **, int));
ORIGINAL(unsigned long long, strtoull, (const char *, char **, int));
ORIGINAL(int64_t, strtoimax, (const char *, char **, int));
ORIGINAL(uint64_t, strtoumax, (const char *, char **, int));
static uintptr_t integer(const char *s, char **end, int base, int sign) {
    struct numeric_output out;
    struct result r = trames_integer_step((uintptr_t)s, (uintptr_t)&out, (uint32_t)base | ((uintptr_t)sign << 32));
    if (r.fallback) return sign ? (uintptr_t)trames_orig_strtoll(s, end, base) : trames_orig_strtoull(s, end, base);
    // Bionic preserves errno on a valid-base conversion with no digits.
    if (out.error && !(out.error == 22 && base != 1 && (unsigned)base <= 36)) *trames_bionic_errno() = (int)out.error;
    if (end) *end = (char *)out.end;
    return r.value;
}
long trames_bionic_strtol(const char *s, char **end, int base) { return (long)integer(s, end, base, 1); }
long long trames_bionic_strtoll(const char *s, char **end, int base) { return (long long)integer(s, end, base, 1); }
unsigned long trames_bionic_strtoul(const char *s, char **end, int base) { return integer(s, end, base, 0); }
unsigned long long trames_bionic_strtoull(const char *s, char **end, int base) { return integer(s, end, base, 0); }
int64_t trames_bionic_strtoimax(const char *s, char **end, int base) { return (int64_t)integer(s, end, base, 1); }
uint64_t trames_bionic_strtoumax(const char *s, char **end, int base) { return integer(s, end, base, 0); }
int trames_bionic_atoi(const char *s) { return (int)integer(s, NULL, 10, 1); }
long trames_bionic_atol(const char *s) { return (long)integer(s, NULL, 10, 1); }
long long trames_bionic_atoll(const char *s) { return (long long)integer(s, NULL, 10, 1); }
ORIGINAL(double, strtod, (const char *, char **));
ORIGINAL(float, strtof, (const char *, char **));
static struct result floating(const char *s, char **end, unsigned single) {
    uintptr_t fpcr; __asm__ volatile("mrs %0, fpcr" : "=r"(fpcr));
    // Retain exact original-image behavior outside the normal nearest mode,
    // including its historical rounding behavior and nondefault FP controls.
    if (fpcr) return (struct result){0, 1};
    struct numeric_output out;
    struct result r = trames_bionic_call_9((uintptr_t)s, (uintptr_t)&out, ((fpcr >> 22) << 1) | single);
    if (!r.fallback) {
        if (out.error) *trames_bionic_errno() = (int)out.error;
        if (end) *end = (char *)out.end;
        uintptr_t fpsr; __asm__ volatile("mrs %0, fpsr" : "=r"(fpsr));
        fpsr |= out.fpsr; __asm__ volatile("msr fpsr, %0" :: "r"(fpsr));
    }
    return r;
}
double trames_bionic_strtod(const char *s, char **end) {
    struct result r = floating(s, end, 0);
    if (r.fallback) return trames_orig_strtod(s, end);
    union { uintptr_t bits; double value; } value = {.bits = r.value}; return value.value;
}
float trames_bionic_strtof(const char *s, char **end) {
    struct result r = floating(s, end, 1);
    if (r.fallback) return trames_orig_strtof(s, end);
    union { uint32_t bits; float value; } value = {.bits = (uint32_t)r.value}; return value.value;
}
double trames_bionic_atof(const char *s) { return trames_bionic_strtod(s, NULL); }
