// SPDX-License-Identifier: GPL-3.0-or-later
// Statically linked into the interpreter's replacement Bionic libc.so.
// No architecture, auxv, feature bitmap, TLS or constructor on this path.
#include <stddef.h>
#include <stdint.h>

struct result { uintptr_t value, fallback; };
#define CALL(n) extern __attribute__((visibility("hidden"))) struct result trames_call_##n(uintptr_t, uintptr_t, size_t)
CALL(1); CALL(2); CALL(3); CALL(4); CALL(5); CALL(6); CALL(7);
CALL(8); CALL(9); CALL(10); CALL(11); CALL(12); CALL(13);
CALL(14); CALL(15); CALL(16); CALL(17); CALL(18); CALL(19);
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

#define MEMORY(name, id, type) \
void *trames_bionic_large_##name(void *d, type s, size_t n) { \
    struct result r = trames_call_##id((uintptr_t)d, (uintptr_t)s, n); \
    if (!r.fallback) return (void *)r.value; \
    return trames_orig_##name(d, s, n); \
}
MEMORY(memcpy, 1, const void *)
MEMORY(memmove, 2, const void *)
MEMORY(memset, 3, int)

int trames_bionic_large_memcmp(const void *a, const void *b, size_t n) {
    struct result r = trames_call_4((uintptr_t)a, (uintptr_t)b, n);
    if (!r.fallback) return (int)r.value;
    return trames_orig_memcmp(a, b, n);
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
    if (!r.fallback) return (void *)r.value; \
    return trames_orig_##name(s, c, n); \
}
SEARCH(memchr, 6)
SEARCH(memrchr, 7)
size_t trames_bionic_large_strnlen(const char *s, size_t n) {
    size_t len = prefix(s);
    if (len < PREFIX) return len;
    struct result r = trames_call_8((uintptr_t)s, n, 0);
    if (!r.fallback) return r.value;
    return trames_orig_strnlen(s, n);
}
int trames_bionic_strcmp(const char *a, const char *b) {
    for (size_t i = 0; i < PREFIX; ++i) {
        unsigned char x = (unsigned char)a[i], y = (unsigned char)b[i];
        if (x != y || !x) return (int)x - (int)y;
    }
    struct result r = trames_call_9((uintptr_t)a, (uintptr_t)b, 0);
    return r.fallback ? trames_orig_strcmp(a, b) : (int)r.value;
}
int trames_bionic_large_strncmp(const char *a, const char *b, size_t n) {
    for (size_t i = 0; i < PREFIX; ++i) {
        unsigned char x = (unsigned char)a[i], y = (unsigned char)b[i];
        if (x != y || !x) return (int)x - (int)y;
    }
    struct result r = trames_call_10((uintptr_t)a, (uintptr_t)b, n);
    if (!r.fallback) return (int)r.value;
    return trames_orig_strncmp(a, b, n);
}
#define CHAR_SEARCH(name, id) \
char *trames_bionic_##name(const char *s, int c) { \
    for (size_t i = 0; i < PREFIX; ++i) { \
        unsigned char x = (unsigned char)s[i]; \
        if (x == (unsigned char)c) return (char *)s + i; \
        if (!x) return id == 13 ? (char *)s + i : NULL; \
    } \
    struct result r = trames_call_##id((uintptr_t)s, (uintptr_t)c, 0); \
    return r.fallback ? trames_orig_##name(s, c) : (char *)r.value; \
}
CHAR_SEARCH(strchr, 11)
CHAR_SEARCH(strchrnul, 13)
char *trames_bionic_strrchr(const char *s, int c) {
    if (prefix(s) < PREFIX) return trames_orig_strrchr(s, c);
    struct result r = trames_call_12((uintptr_t)s, (uintptr_t)c, 0);
    return r.fallback ? trames_orig_strrchr(s, c) : (char *)r.value;
}
#define COPY(name, id) \
char *trames_bionic_##name(char *d, const char *s) { \
    if (prefix(s) == PREFIX) { \
        struct result r = trames_call_##id((uintptr_t)d, (uintptr_t)s, 0); \
        if (!r.fallback) return (char *)r.value; \
    } \
    return trames_orig_##name(d, s); \
}
COPY(strcpy, 14)
COPY(stpcpy, 15)
#define SIZED_COPY(name, id) \
char *trames_bionic_large_##name(char *d, const char *s, size_t n) { \
    struct result r = trames_call_##id((uintptr_t)d, (uintptr_t)s, n); \
    if (!r.fallback) return (char *)r.value; \
    return trames_orig_##name(d, s, n); \
}
SIZED_COPY(strncpy, 16)
SIZED_COPY(stpncpy, 17)
char *trames_bionic_strcat(char *d, const char *s) {
    if (prefix(d) == PREFIX || prefix(s) == PREFIX) {
        struct result r = trames_call_18((uintptr_t)d, (uintptr_t)s, 0);
        if (!r.fallback) return (char *)r.value;
    }
    return trames_orig_strcat(d, s);
}
char *trames_bionic_strncat(char *d, const char *s, size_t n) {
    // n == 0 does not inspect s, including an inaccessible source pointer.
    if (n && (n >= BULK || prefix(d) == PREFIX)) {
        struct result r = trames_call_19((uintptr_t)d, (uintptr_t)s, n);
        if (!r.fallback) return (char *)r.value;
    }
    return trames_orig_strncat(d, s, n);
}
void *trames_bionic_large_mempcpy(void *d, const void *s, size_t n) {
    struct result r = trames_call_1((uintptr_t)d, (uintptr_t)s, n);
    if (!r.fallback) return (char *)d + n;
    return trames_orig_mempcpy(d, s, n);
}
size_t trames_bionic_strlcpy(char *d, const char *s, size_t n) {
    if (prefix(s) < PREFIX) return trames_orig_strlcpy(d, s, n);
    struct result len = trames_call_5((uintptr_t)s, 0, 0);
    if (len.fallback) return trames_orig_strlcpy(d, s, n);
    if (n) {
        size_t copy = len.value < n - 1 ? len.value : n - 1;
        struct result r = trames_call_1((uintptr_t)d, (uintptr_t)s, copy);
        if (r.fallback) return trames_orig_strlcpy(d, s, n);
        d[copy] = 0;
    }
    return len.value;
}
size_t trames_bionic_large_strlcat(char *d, const char *s, size_t n) {
    struct result dst = trames_call_8((uintptr_t)d, n, 0);
    if (dst.fallback) return trames_orig_strlcat(d, s, n);
    struct result src = trames_call_5((uintptr_t)s, 0, 0);
    if (src.fallback) return trames_orig_strlcat(d, s, n);
    if (dst.value < n) {
        size_t room = n - dst.value - 1;
        size_t copy = src.value < room ? src.value : room;
        struct result r = trames_call_1((uintptr_t)d + dst.value, (uintptr_t)s, copy);
        if (r.fallback) return trames_orig_strlcat(d, s, n);
        d[dst.value + copy] = 0;
    }
    return dst.value + src.value;
}
