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
#define CHUNK (1024 * 1024)
#define STRING_CHUNK CHUNK
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
static __attribute__((noinline)) size_t stream_length(const char *s, size_t bound) {
    size_t offset = 0;
    while (offset < bound) {
        size_t count = bound - offset < STRING_CHUNK ? bound - offset : STRING_CHUNK;
        struct result r = trames_call_8((uintptr_t)s + offset, count, 0);
        if (r.fallback) return offset + trames_orig_strnlen(s + offset, bound - offset);
        if (r.value < count) return offset + r.value;
        offset += count;
    }
    return offset;
}

static __attribute__((noinline)) void *stream_memcpy(void *d, const void *s, size_t n) {
    void *start = d;
    while (n) {
        size_t count = n < CHUNK ? n : CHUNK;
        struct result r = trames_call_1((uintptr_t)d, (uintptr_t)s, count);
        if (r.fallback) {
            if (n <= 96) trames_original_small_copy(d, s, n); else trames_orig_memcpy(d, s, n);
            break;
        }
        d = (char *)d + count; s = (const char *)s + count; n -= count;
    }
    return start;
}
static __attribute__((noinline)) void *stream_memmove(void *d, const void *s, size_t n) {
    void *start = d;
    if ((uintptr_t)d > (uintptr_t)s && (uintptr_t)d - (uintptr_t)s < n) {
        // The original backward-copy continuation is only selected here.
        while (n) {
            size_t count = n < CHUNK ? n : CHUNK, offset = n - count;
            struct result r = trames_call_2((uintptr_t)d + offset, (uintptr_t)s + offset, count);
            if (r.fallback) {
                if (n <= 96) trames_original_small_copy(d, s, n); else trames_orig_memmove(d, s, n);
                break;
            }
            n = offset;
        }
    } else {
        // Resume the original forward-copy continuation only for the remaining
        // suffix after a no-effect native fallback.
        while (n) {
            size_t count = n < CHUNK ? n : CHUNK;
            struct result r = trames_call_2((uintptr_t)d, (uintptr_t)s, count);
            if (r.fallback) {
                if (n <= 96) trames_original_small_copy(d, s, n); else trames_orig_memcpy(d, s, n);
                break;
            }
            d = (char *)d + count; s = (const char *)s + count; n -= count;
        }
    }
    return start;
}
static __attribute__((noinline)) void *stream_memset(void *d, int c, size_t n) {
    void *start = d;
    while (n) {
        size_t count = n < CHUNK ? n : CHUNK;
        struct result r = trames_call_3((uintptr_t)d, (uintptr_t)c, count);
        if (r.fallback) { trames_orig_memset(d, c, n); break; }
        d = (char *)d + count; n -= count;
    }
    return start;
}

void *trames_bionic_large_memcpy(void *d, const void *s, size_t n) {
    if (n > CHUNK) return stream_memcpy(d, s, n);
    struct result r = trames_call_1((uintptr_t)d, (uintptr_t)s, n);
    if (!r.fallback) return (void *)r.value;
    return n <= 96 ? trames_original_small_copy(d, s, n) : trames_orig_memcpy(d, s, n);
}
void *trames_bionic_large_memmove(void *d, const void *s, size_t n) {
    if (n > CHUNK) return stream_memmove(d, s, n);
    struct result r = trames_call_2((uintptr_t)d, (uintptr_t)s, n);
    if (!r.fallback) return (void *)r.value;
    return n <= 96 ? trames_original_small_copy(d, s, n) : trames_orig_memmove(d, s, n);
}
void *trames_bionic_large_memset(void *d, int c, size_t n) {
    if (n > CHUNK) return stream_memset(d, c, n);
    struct result r = trames_call_3((uintptr_t)d, (uintptr_t)c, n);
    if (!r.fallback) return (void *)r.value;
    return trames_orig_memset(d, c, n);
}
static __attribute__((noinline)) int compare_stream(const void *a, const void *b, size_t n) {
    while (n) {
        size_t count = n < CHUNK ? n : CHUNK;
        struct result r = trames_call_4((uintptr_t)a, (uintptr_t)b, count);
        if (r.fallback) return trames_orig_memcmp(a, b, n);
        if ((int)r.value) return (int)r.value;
        a = (const char *)a + count; b = (const char *)b + count; n -= count;
    }
    return 0;
}
int trames_bionic_large_memcmp(const void *a, const void *b, size_t n) {
    if (n > CHUNK) return compare_stream(a, b, n);
    struct result r = trames_call_4((uintptr_t)a, (uintptr_t)b, n);
    return r.fallback ? trames_orig_memcmp(a, b, n) : (int)r.value;
}
size_t trames_bionic_unaligned_strlen(const char *s) {
    size_t n = prefix(s);
    if (n < PREFIX) return n;
    struct result r = trames_call_5((uintptr_t)s, 0, 0);
    return r.fallback ? stream_length(s, SIZE_MAX) : r.value;
}
size_t trames_bionic_long_strlen(const char *s) {
    struct result r = trames_call_5((uintptr_t)s, 0, 0);
    return r.fallback ? stream_length(s, SIZE_MAX) : r.value;
}
#define SEARCH(name, id) \
void *trames_bionic_large_##name(const void *s, int c, size_t n) { \
    while (n) { \
        size_t count = n < CHUNK ? n : CHUNK, offset = id == 7 ? n - count : 0; \
        struct result r = trames_call_##id((uintptr_t)s + offset, (uintptr_t)c, count); \
        if (r.fallback) return trames_orig_##name(s, c, n); \
        if (r.value) return (void *)r.value; \
        n -= count; if (id != 7) s = (const char *)s + count; \
    } \
    return NULL; \
}
SEARCH(memchr, 6)
SEARCH(memrchr, 7)
size_t trames_bionic_large_strnlen(const char *s, size_t n) {
    size_t len = prefix(s);
    if (len < PREFIX) return len;
    struct result r = trames_call_8((uintptr_t)s, n, 0);
    if (!r.fallback) return r.value;
    return stream_length(s, n);
}
int trames_bionic_strcmp(const char *a, const char *b) {
    for (size_t i = 0; i < PREFIX; ++i) {
        unsigned char x = (unsigned char)a[i], y = (unsigned char)b[i];
        if (x != y || !x) return (int)x - (int)y;
    }
    struct result r = trames_call_9((uintptr_t)a, (uintptr_t)b, 0);
    if (!r.fallback) return (int)r.value;
    for (;;) {
        r = trames_bionic_call_10((uintptr_t)a, (uintptr_t)b, SIZE_MAX);
        if (r.fallback) return trames_orig_strcmp(a, b);
        if (r.value != UINTPTR_MAX) return (int)r.value;
        a += CHUNK; b += CHUNK;
    }
}
int trames_bionic_large_strncmp(const char *a, const char *b, size_t n) {
    for (size_t i = 0; i < PREFIX; ++i) {
        unsigned char x = (unsigned char)a[i], y = (unsigned char)b[i];
        if (x != y || !x) return (int)x - (int)y;
    }
    struct result r = trames_call_10((uintptr_t)a, (uintptr_t)b, n);
    if (!r.fallback) return (int)r.value;
    for (;;) {
        r = trames_bionic_call_10((uintptr_t)a, (uintptr_t)b, n);
        if (r.fallback) return trames_orig_strncmp(a, b, n);
        if (r.value != UINTPTR_MAX) return (int)r.value;
        a += CHUNK; b += CHUNK; n -= CHUNK;
    }
}
#define CHAR_SEARCH(name, id) \
char *trames_bionic_##name(const char *s, int c) { \
    for (size_t i = 0; i < PREFIX; ++i) { \
        unsigned char x = (unsigned char)s[i]; \
        if (x == (unsigned char)c) return (char *)s + i; \
        if (!x) return id == 13 ? (char *)s + i : NULL; \
    } \
    struct result r = trames_call_##id((uintptr_t)s, (uintptr_t)c, 0); \
    if (!r.fallback) return (char *)r.value; \
    for (;;) { \
        r = trames_bionic_call_13((uintptr_t)s, (uintptr_t)c, id == 13); \
        if (r.fallback) return trames_orig_##name(s, c); \
        if (r.value != UINTPTR_MAX) return (char *)r.value; \
        s += CHUNK; \
    } \
}
CHAR_SEARCH(strchr, 11)
CHAR_SEARCH(strchrnul, 13)
char *trames_bionic_strrchr(const char *s, int c) {
    if (prefix(s) < PREFIX) return trames_orig_strrchr(s, c);
    struct result r = trames_call_12((uintptr_t)s, (uintptr_t)c, 0);
    return r.fallback ? trames_bionic_large_memrchr(s, c, stream_length(s, SIZE_MAX) + 1) : (char *)r.value;
}
// Keep rare streaming state out of the original common-call frame.
static __attribute__((noinline)) char *copy_string_stream(char *d, const char *s, unsigned end) {
    size_t len = stream_length(s, SIZE_MAX);
    trames_bionic_large_memcpy(d, s, len + 1);
    return end ? d + len : d;
}
static __attribute__((noinline)) char *copy_sized_string_stream(char *d, const char *s, size_t n, unsigned end) {
    size_t len = stream_length(s, n);
    trames_bionic_large_memcpy(d, s, len);
    if (len < n) trames_bionic_large_memset(d + len, 0, n - len);
    return end ? d + len : d;
}
#define COPY(name, id) \
char *trames_bionic_##name(char *d, const char *s) { \
    if (prefix(s) == PREFIX) { \
        struct result r = trames_call_##id((uintptr_t)d, (uintptr_t)s, 0); \
        if (!r.fallback) return (char *)r.value; \
        return copy_string_stream(d, s, id == 15); \
    } \
    return trames_orig_##name(d, s); \
}
COPY(strcpy, 14)
COPY(stpcpy, 15)
#define SIZED_COPY(name, id) \
char *trames_bionic_large_##name(char *d, const char *s, size_t n) { \
    struct result r = trames_call_##id((uintptr_t)d, (uintptr_t)s, n); \
    if (!r.fallback) return (char *)r.value; \
    return copy_sized_string_stream(d, s, n, id == 17); \
}
SIZED_COPY(strncpy, 16)
SIZED_COPY(stpncpy, 17)
char *trames_bionic_strcat(char *d, const char *s) {
    if (prefix(d) == PREFIX || prefix(s) == PREFIX) {
        struct result r = trames_call_18((uintptr_t)d, (uintptr_t)s, 0);
        if (!r.fallback) return (char *)r.value;
        trames_bionic_strcpy(d + stream_length(d, SIZE_MAX), s);
        return d;
    }
    return trames_orig_strcat(d, s);
}
char *trames_bionic_strncat(char *d, const char *s, size_t n) {
    // n == 0 does not inspect s, including an inaccessible source pointer.
    if (n && (n >= BULK || prefix(d) == PREFIX)) {
        struct result r = trames_call_19((uintptr_t)d, (uintptr_t)s, n);
        if (!r.fallback) return (char *)r.value;
        size_t tail = stream_length(d, SIZE_MAX), len = stream_length(s, n);
        trames_bionic_large_memcpy(d + tail, s, len); d[tail + len] = 0;
        return d;
    }
    return trames_orig_strncat(d, s, n);
}
void *trames_bionic_large_mempcpy(void *d, const void *s, size_t n) {
    trames_bionic_large_memcpy(d, s, n);
    return (char *)d + n;
}
static __attribute__((noinline)) size_t bounded_copy_stream(char *d, const char *s, size_t n) {
    size_t length = stream_length(s, SIZE_MAX);
    if (n) { size_t copy = length < n - 1 ? length : n - 1; trames_bionic_large_memcpy(d, s, copy); d[copy] = 0; }
    return length;
}
size_t trames_bionic_strlcpy(char *d, const char *s, size_t n) {
    if (prefix(s) < PREFIX) return trames_orig_strlcpy(d, s, n);
    struct result len = trames_call_5((uintptr_t)s, 0, 0);
    if (len.fallback) return bounded_copy_stream(d, s, n);
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
    if (dst.fallback) dst.value = stream_length(d, n);
    struct result src = trames_call_5((uintptr_t)s, 0, 0);
    if (src.fallback) src.value = stream_length(s, SIZE_MAX);
    if (dst.value < n) {
        size_t room = n - dst.value - 1;
        size_t copy = src.value < room ? src.value : room;
        struct result r = trames_call_1((uintptr_t)d + dst.value, (uintptr_t)s, copy);
        if (r.fallback) trames_bionic_large_memcpy(d + dst.value, s, copy);
        d[dst.value + copy] = 0;
    }
    return dst.value + src.value;
}

ORIGINAL(void *, memmem, (const void *, size_t, const void *, size_t));
ORIGINAL(char *, strstr, (const char *, const char *));
ORIGINAL(char *, strcasestr, (const char *, const char *));
ORIGINAL(size_t, strspn, (const char *, const char *));
ORIGINAL(size_t, strcspn, (const char *, const char *));
ORIGINAL(char *, strpbrk, (const char *, const char *));
ORIGINAL(int, strcasecmp, (const char *, const char *));
ORIGINAL(int, strncasecmp, (const char *, const char *, size_t));
void *trames_bionic_memmem(const void *hay, size_t count, const void *needle, size_t n) {
    if (!n) return (void *)hay;
    if (n > count) return NULL;
    if (count < BULK || n > 65536) return trames_orig_memmem(hay, count, needle, n);
    uintptr_t offset = 0;
    while (count - offset >= n) {
        uintptr_t request[4] = {(uintptr_t)hay + offset, count - offset, (uintptr_t)needle, n};
        struct result r = trames_bionic_call_1((uintptr_t)request, 0, 0);
        if (r.fallback) return trames_orig_memmem((const char *)hay + offset, count - offset, needle, n);
        if (r.value != UINTPTR_MAX) return (void *)r.value;
        offset += CHUNK - n + 1;
    }
    return NULL;
}
#define SUBSTRING(name, id) \
char *trames_bionic_##name(const char *s, const char *needle) { \
    if (!*needle) return (char *)s; \
    if (prefix(s) < PREFIX) return trames_orig_##name(s, needle); \
    size_t n = trames_bionic_strlen(needle); \
    if (n > 65536) return trames_orig_##name(s, needle); \
    for (;;) { \
        struct result r = trames_bionic_call_##id((uintptr_t)s, (uintptr_t)needle, n); \
        if (r.fallback) return trames_orig_##name(s, needle); \
        if (r.value != UINTPTR_MAX) return (char *)r.value; \
        s += CHUNK - n + 1; \
    } \
}
SUBSTRING(strstr, 2)
SUBSTRING(strcasestr, 3)
#define SPAN(name, id) \
size_t trames_bionic_##name(const char *s, const char *set) { \
    if (prefix(s) < PREFIX) return trames_orig_##name(s, set); \
    size_t offset = 0; \
    for (;;) { \
        struct result r = trames_bionic_call_##id((uintptr_t)s + offset, (uintptr_t)set, 0); \
        if (r.fallback) return offset + trames_orig_##name(s + offset, set); \
        if (r.value != UINTPTR_MAX) return offset + r.value; \
        offset += CHUNK; \
    } \
}
SPAN(strspn, 4)
SPAN(strcspn, 5)
char *trames_bionic_strpbrk(const char *s, const char *set) {
    if (prefix(s) < PREFIX) return trames_orig_strpbrk(s, set);
    for (;;) {
        struct result r = trames_bionic_call_6((uintptr_t)s, (uintptr_t)set, 0);
        if (r.fallback) return trames_orig_strpbrk(s, set);
        if (r.value != UINTPTR_MAX) return (char *)r.value;
        s += CHUNK;
    }
}
static inline unsigned char lower(unsigned char c) { return c >= 'A' && c <= 'Z' ? c + 32 : c; }
int trames_bionic_strcasecmp(const char *a, const char *b) {
    for (size_t i = 0; i < PREFIX; ++i) {
        unsigned char x = lower((unsigned char)a[i]), y = lower((unsigned char)b[i]);
        if (x != y || !x) return (int)x - (int)y;
    }
    for (;;) {
        struct result r = trames_bionic_call_7((uintptr_t)a, (uintptr_t)b, 0);
        if (r.fallback) return trames_orig_strcasecmp(a, b);
        if (r.value != UINTPTR_MAX) return (int)r.value;
        a += CHUNK; b += CHUNK;
    }
}
int trames_bionic_strncasecmp(const char *a, const char *b, size_t n) {
    if (n < BULK) return trames_orig_strncasecmp(a, b, n);
    for (;;) {
        struct result r = trames_bionic_call_8((uintptr_t)a, (uintptr_t)b, n);
        if (r.fallback) return trames_orig_strncasecmp(a, b, n);
        if (r.value != UINTPTR_MAX) return (int)r.value;
        a += CHUNK; b += CHUNK; n -= CHUNK;
    }
}
char *trames_bionic_strsep(char **state, const char *set) {
    char *start = *state;
    if (!start) return NULL;
    size_t len = trames_bionic_strcspn(start, set);
    if (start[len]) { start[len] = 0; *state = start + len + 1; } else *state = NULL;
    return start;
}
char *trames_bionic_strtok_r(char *s, const char *set, char **state) {
    if (!s) s = *state;
    if (!s) return NULL;
    s += trames_bionic_strspn(s, set);
    if (!*s) { *state = NULL; return NULL; }
    char *start = s;
    s += trames_bionic_strcspn(s, set);
    if (*s) { *s++ = 0; *state = s; } else *state = NULL;
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
