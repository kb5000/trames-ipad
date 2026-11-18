// SPDX-License-Identifier: GPL-3.0-or-later
// Exercise the complete replacement libc through the original Android linker.
#define _GNU_SOURCE
#include <dlfcn.h>
#include <errno.h>
#include <fenv.h>
#include <inttypes.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

static unsigned long checks;
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #c); exit(1); } ++checks; } while (0)
#define LOAD(name) __typeof__(&name) p_##name = (__typeof__(&name))dlsym(RTLD_DEFAULT, #name); CHECK(p_##name)

static void suite(void) {
    LOAD(memcpy); LOAD(memmove); LOAD(memset); LOAD(memcmp); LOAD(strlen);
    LOAD(memchr); LOAD(memrchr); LOAD(strnlen); LOAD(strcmp); LOAD(strncmp);
    LOAD(strchr); LOAD(strrchr); LOAD(strchrnul); LOAD(strcpy); LOAD(stpcpy);
    LOAD(strncpy); LOAD(stpncpy); LOAD(strcat); LOAD(strncat); LOAD(mempcpy);
    LOAD(strlcpy); LOAD(strlcat);
    const size_t capacity = (1 << 20) + 8192;
    char *a = malloc(capacity), *b = malloc(capacity), *c = malloc(capacity);
    CHECK(a && b && c);
    const size_t lengths[] = {0, 1, 15, 16, 17, 63, 95, 96, 97, 127, 128, 129, 255, 4095, 4096, 65535, 1048576, 1048608};
    for (size_t k = 0; k < sizeof(lengths) / sizeof(*lengths); ++k) {
        size_t n = lengths[k];
        for (size_t i = 0; i < n; ++i) a[i] = (char)(1 + i % 127);
        a[n] = 0;
        CHECK(p_memset(b, 0xa5, n + 1) == b);
        CHECK(p_memcpy(b, a, n + 1) == b);
        CHECK(p_memcmp(a, b, n + 1) == 0);
        CHECK(p_strlen(b) == n);
        CHECK(p_strnlen(b, n / 2) == n / 2);
        CHECK(p_strnlen(b, n + 1) == n);
        CHECK(p_strcmp(a, b) == 0 && p_strncmp(a, b, n + 1) == 0);
        CHECK(p_memchr(b, 0, n + 1) == b + n);
        CHECK(p_memrchr(b, 0, n + 1) == b + n);
        CHECK(p_strchr(b, 0) == b + n && p_strrchr(b, 0) == b + n);
        CHECK(p_strchrnul(b, 0xff) == b + n && p_strchr(b, 0xff) == NULL);
        CHECK(p_strcpy(c, a) == c && p_memcmp(c, a, n + 1) == 0);
        CHECK(p_stpcpy(c, a) == c + n);
        CHECK(p_strncpy(c, a, n + 2) == c && c[n + 1] == 0);
        CHECK(p_stpncpy(c, a, n + 2) == c + n && c[n + 1] == 0);
        CHECK(p_stpncpy(c, a, n) == c + n);
        CHECK(p_mempcpy(c, a, n + 1) == c + n + 1);
        b[0] = 'Q'; b[1] = 0;
        CHECK(p_strcat(b, a) == b && b[0] == 'Q' && p_strcmp(b + 1, a) == 0);
        b[0] = 'Q'; b[1] = 0;
        CHECK(p_strncat(b, a, n) == b && b[n + 1] == 0);
        CHECK(p_strlcpy(b, a, n / 2 + 1) == n && b[n / 2] == 0);
        b[0] = 'Q'; b[1] = 0;
        CHECK(p_strlcat(b, a, n + 3) == n + 1 && b[n + 1] == 0);
        b[0] = 'Q'; b[1] = 0;
        CHECK(p_strlcat(b, a, 1) == n + 1 && b[0] == 'Q');
        if (n) {
            b[0] = a[0] + 1; b[n] = 0;
            CHECK(p_memcmp(a, b, n) < 0 && p_strcmp(a, b) < 0 && p_strncmp(a, b, n) < 0);
        }
        for (size_t i = 0; i < n + 32; ++i) b[i] = (char)(i % 251);
        CHECK(p_memmove(b + 7, b, n) == b + 7);
        for (size_t i = 0; i < n; ++i) CHECK((unsigned char)b[i + 7] == i % 251);
        CHECK(p_memmove(b, b + 7, n) == b);
        for (size_t i = 0; i < n; ++i) CHECK((unsigned char)b[i] == i % 251);
        CHECK(p_memmove(b, b, n) == b);
    }
    a[0] = (char)0x80; a[1] = (char)0xff; a[2] = 0;
    b[0] = 0x7f; b[1] = 0;
    CHECK(p_strcmp(a, b) > 0 && p_strncmp(a, b, 2) > 0 && p_memcmp(a, b, 1) > 0);
    CHECK(p_strlen(a) == 2 && p_strchr(a, 0x17ff) == a + 1);
    CHECK(p_strncpy(b, "", 256) == b && p_memcmp(b, b + 1, 255) == 0);
    CHECK(p_stpncpy(b, "", 256) == b);
    size_t page = (size_t)sysconf(_SC_PAGESIZE);
    char *guard = mmap(NULL, page * 3, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    CHECK(guard != MAP_FAILED && mprotect(guard + page * 2, page, PROT_NONE) == 0);
    char *end = guard + page * 2, *s = end - 258;
    p_memset(s, 'x', 257); s[257] = 0;
    CHECK(p_strlen(s) == 257 && p_strnlen(s, 4096) == 257);
    CHECK(p_memchr(s, 0, 258) == end - 1 && p_memrchr(s, 'x', 258) == end - 2);
    CHECK(p_strchr(s, 'x') == s && p_strrchr(s, 'x') == end - 2);
    CHECK(p_strchrnul(s, 'y') == end - 1);
    CHECK(p_strcpy(a, s) == a && p_strcmp(a, s) == 0);
    CHECK(p_strncmp(a, s, 4096) == 0);
    CHECK(p_strlcpy(end, s, 0) == 257 && p_strlcat(end, s, 0) == 257);
    CHECK(p_strncat(a, end, 0) == a);
    CHECK(p_memcpy(end, end, 0) == end && p_memmove(end, end, 0) == end);
    CHECK(p_memcmp(end, end, 0) == 0 && p_strnlen(end, 0) == 0);
    // Exercise every alignment and terminator position before an unmapped page.
    for (size_t n = 0; n <= 257; ++n) {
        char *tail = end - n - 1;
        p_memset(tail, 'x', n); tail[n] = 0;
        CHECK(p_strlen(tail) == n && p_strnlen(tail, 4096) == n);
        CHECK(p_strchr(tail, 0) == end - 1 && p_strrchr(tail, 0) == end - 1);
    }
    CHECK(munmap(guard, page * 3) == 0);
    errno = E2BIG;
    CHECK(p_memcpy(a, b, 4096) == a && errno == E2BIG);
    free(a); free(b); free(c);
}

static void *worker(void *argument) {
    uintptr_t id = (uintptr_t)argument;
    char a[1024], b[1024];
    for (unsigned i = 0; i < 100; ++i) {
        memset(a, 'A' + (int)id, sizeof(a)); a[sizeof(a) - 1] = 0;
        errno = 100 + (int)id;
        if (memcpy(b, a, sizeof(a)) != b || strcmp(a, b) || strlen(b) != 1023 || errno != 100 + (int)id)
            return (void *)1;
    }
    return NULL;
}

#ifdef __ANDROID__
static void fortify(void) {
    typedef void *(*checked_copy)(void *, const void *, size_t, size_t);
    checked_copy copy = (checked_copy)dlsym(RTLD_DEFAULT, "__memcpy_chk");
    CHECK(copy);
    char a[512], b[512]; memset(a, 'F', sizeof(a));
    CHECK(copy(b, a, sizeof(a), sizeof(b)) == b && !memcmp(a, b, sizeof(a)));
    pid_t child = fork(); CHECK(child >= 0);
    if (!child) { copy(b, a, sizeof(a), sizeof(b) - 1); _exit(99); }
    int status = 0; CHECK(waitpid(child, &status, 0) == child);
    CHECK(WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT);
}

#endif

static uint64_t numeric_hash = UINT64_C(1469598103934665603);
static void mix(uint64_t value) { numeric_hash = (numeric_hash ^ value) * UINT64_C(1099511628211); }
static void extended_suite(void) {
    LOAD(memmem); LOAD(strstr); LOAD(strcasestr); LOAD(strspn); LOAD(strcspn);
    LOAD(strpbrk); LOAD(strcasecmp); LOAD(strncasecmp); LOAD(strsep); LOAD(strtok_r);
    const size_t size = (1 << 21) + 4096;
    char *a = malloc(size), *b = malloc(size); CHECK(a && b);
    memset(a, 'a', size - 1); a[size - 1] = 0;
    const char *needle = "bCdEfGhIjK";
    const size_t positions[] = {0, 127, 4093, (1 << 20) - 5, (1 << 20) + 15, (1 << 21)};
    for (unsigned i = 0; i < sizeof(positions) / sizeof(*positions); ++i) {
        size_t pos = positions[i]; memcpy(a + pos, needle, 10);
        CHECK(p_memmem(a, size - 1, needle, 10) == a + pos);
        CHECK(p_strstr(a, needle) == a + pos);
        CHECK(p_strcasestr(a, "BcDeFgHiJk") == a + pos);
        memset(a + pos, 'a', 10);
    }
    CHECK(p_memmem(a, size - 1, "z", 1) == NULL);
    CHECK(p_strstr(a, "") == a && p_memmem(b, 0, "", 0) == b);
    CHECK(p_strspn(a, "a") == size - 1 && p_strcspn(a, "b") == size - 1);
    a[size - 2] = 'B';
    CHECK(p_strpbrk(a, "BC") == a + size - 2 && p_strcspn(a, "BC") == size - 2);
    a[size - 2] = 'a';
    memset(b, 'A', size - 1); b[size - 1] = 0;
    CHECK(p_strcasecmp(a, b) == 0 && p_strncasecmp(a, b, size) == 0);
    b[size - 2] = 'Z'; CHECK(p_strcasecmp(a, b) < 0 && p_strncasecmp(a, b, size) < 0);
    for (size_t n = 0; n < 64; ++n) CHECK(p_strncasecmp(a, b, n) == 0);
    char text[] = ":alpha::beta:"; char *state = text;
    CHECK(!strcmp(p_strsep(&state, ":"), "")); CHECK(!strcmp(p_strsep(&state, ":"), "alpha"));
    CHECK(!strcmp(p_strsep(&state, ":"), "")); CHECK(!strcmp(p_strsep(&state, ":"), "beta"));
    CHECK(!strcmp(p_strsep(&state, ":"), "")); CHECK(p_strsep(&state, ":") == NULL);
    char tokens[] = "  alpha, beta,,gamma ";
    CHECK(!strcmp(p_strtok_r(tokens, " ,", &state), "alpha"));
    CHECK(!strcmp(p_strtok_r(NULL, " ,", &state), "beta"));
    CHECK(!strcmp(p_strtok_r(NULL, " ,", &state), "gamma"));
    CHECK(p_strtok_r(NULL, " ,", &state) == NULL);
    // A repeated prefix used to expose quadratic substring implementations.
    memset(a, 'a', 32768); a[32768] = 0; memset(b, 'a', 127); b[127] = 'b'; b[128] = 0;
    CHECK(p_strstr(a, b) == NULL && p_memmem(a, 32768, b, 128) == NULL);
    free(a); free(b);
    LOAD(strtol); LOAD(strtoll); LOAD(strtoul); LOAD(strtoull); LOAD(strtoimax); LOAD(strtoumax);
    LOAD(strtod); LOAD(strtof); LOAD(atoi); LOAD(atol); LOAD(atoll); LOAD(atof);
    // Patterns and character sets beyond the former 64 KiB scratch limit.
    size_t pn=65536+17;char *pattern=malloc(pn+1),*hay=malloc(pn*2+32);CHECK(pattern && hay);
    memset(pattern,'A',pn);pattern[0]='B';pattern[pn-1]='Z';pattern[pn]=0;
    memset(hay,'c',pn*2+31);hay[pn*2+31]=0;memcpy(hay+pn+7,pattern,pn);
    CHECK(p_memmem(hay,pn*2+31,pattern,pn)==hay+pn+7);
    CHECK(p_strstr(hay,pattern)==hay+pn+7);
    pattern[0]='b';pattern[pn-1]='z';CHECK(p_strcasestr(hay,pattern)==hay+pn+7);
    memset(pattern,'c',pn);pattern[pn]=0;CHECK(p_strspn(hay,pattern)==pn+7);
    free(pattern);free(hay);
    const char *integers[] = {"", "x", " +", "0", "-0", "0x", "0xG", "  -0x7f!", "09", "0b11", "z!",
        "9223372036854775807!", "9223372036854775808", "-9223372036854775808", "-9223372036854775809",
        "18446744073709551615", "18446744073709551616", "-18446744073709551616"};
    const int bases[] = {-1, 0, 1, 2, 8, 10, 16, 36, 37};
    for (unsigned i = 0; i < sizeof(integers) / sizeof(*integers); ++i) {
        const char *s = integers[i]; char *end;
        for (unsigned j = 0; j < sizeof(bases) / sizeof(*bases); ++j) {
#define INTEGER(function) do { errno = 123; end=(char *)s; uint64_t value = (uint64_t)p_##function(s, &end, bases[j]); mix(value); mix((uintptr_t)(end - s)); mix((unsigned)errno); } while (0)
            INTEGER(strtol); INTEGER(strtoll); INTEGER(strtoul); INTEGER(strtoull); INTEGER(strtoimax); INTEGER(strtoumax);
        }
        errno = 123; mix((uint32_t)p_atoi(s)); mix((unsigned)errno);
        errno = 123; mix((uint64_t)p_atol(s)); mix((unsigned)errno);
        errno = 123; mix((uint64_t)p_atoll(s)); mix((unsigned)errno);
    }
    const char *floats[] = {"", "x", " +", ".", "0", "-0", "1e", "1e+", "0x", "0x.", "123.5tail", "0.1",
        "0x1.8p+3!", "1.00000000000000011102230246251565404236316680908203125", "2.2250738585072014e-308",
        "4.9406564584124654e-324", "1e-5000", "-1e5000", "1.7976931348623157e308", "3.4028234663852886e38",
        "1.1754943508222875e-38", "1.401298464324817e-45", "inf", "-Infinity", "NaN", "nan(0x123)", "nan(foo)", "nan("};
    const int modes[] = {FE_TONEAREST, FE_UPWARD, FE_DOWNWARD, FE_TOWARDZERO};
    for (unsigned mode = 0; mode < 4; ++mode) {
        CHECK(fesetround(modes[mode]) == 0);
        for (unsigned i = 0; i < sizeof(floats) / sizeof(*floats); ++i) {
            const char *s = floats[i]; char *end;
            errno = 123; feclearexcept(FE_ALL_EXCEPT);
            union { double value; uint64_t bits; } d = {.value = p_strtod(s, &end)};
            size_t dend = (size_t)(end - s); int derror = errno, dflags = fetestexcept(FE_ALL_EXCEPT);
            mix(d.bits); mix(dend); mix((unsigned)derror); mix((unsigned)dflags);
            errno = 123; feclearexcept(FE_ALL_EXCEPT);
            union { float value; uint32_t bits; } f = {.value = p_strtof(s, &end)};
            size_t fend = (size_t)(end - s); int ferror = errno, fflags = fetestexcept(FE_ALL_EXCEPT);
            mix(f.bits); mix(fend); mix((unsigned)ferror); mix((unsigned)fflags);
            printf("FLOAT %u %u %016" PRIx64 " %zu %d %x %08" PRIx32 " %zu %d %x\n",
                mode, i, d.bits, dend, derror, dflags, f.bits, fend, ferror, fflags);
            errno = 123; d.value = p_atof(s); mix(d.bits); mix((unsigned)errno);
        }
    }
    // Deterministic varied mantissas/exponents, compared bit-for-bit by the
    // host harness against this exact original Android libc, including flags.
    uint64_t random = 0x14337a889122fd13;
    for (unsigned i = 0; i < 256; ++i) {
        random = random * 6364136223846793005ULL + 1;
        char token[96], *end;
        snprintf(token, sizeof(token), "%s%" PRIu64 ".%09u%s%d!", (random >> 32) & 1 ? "-" : "",
            random >> 7, (unsigned)(random % 1000000000), (random >> 33) & 1 ? "e+" : "e-", (int)(i * 17 % 500));
        CHECK(fesetround(modes[i % 4]) == 0);
        errno = 123; feclearexcept(FE_ALL_EXCEPT);
        union { double value; uint64_t bits; } d = {.value = p_strtod(token, &end)};
        mix(d.bits); mix((size_t)(end - token)); mix((unsigned)errno); mix((unsigned)fetestexcept(FE_ALL_EXCEPT));
        errno = 123; feclearexcept(FE_ALL_EXCEPT);
        union { float value; uint32_t bits; } f = {.value = p_strtof(token, &end)};
        mix(f.bits); mix((size_t)(end - token)); mix((unsigned)errno); mix((unsigned)fetestexcept(FE_ALL_EXCEPT));
    }
    CHECK(fesetround(FE_TONEAREST) == 0);
    size_t page = (size_t)sysconf(_SC_PAGESIZE);
    char *guard = mmap(NULL, page * 3, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    CHECK(guard != MAP_FAILED && mprotect(guard + page * 2, page, PROT_NONE) == 0);
    char *token = guard + page - 128, *end;
    memset(token, ' ', page + 120); memcpy(guard + page * 2 - 8, "1.25!", 6);
    errno = 123; CHECK(p_strtod(token, &end) == 1.25 && end == guard + page * 2 - 4 && errno == 123);
    token = guard + page * 2 - 2; token[0] = '7'; token[1] = 0;
    CHECK(p_strtod(token, &end) == 7 && end == token + 1);
    CHECK(munmap(guard, page * 3) == 0);
    printf("NUMERIC %016" PRIx64 "\n", numeric_hash);
    // Exercise the authoritative guest allocator through cache hits/misses,
    // zeroing and reallocation. Thread handoff and fork are tested separately.
    for (unsigned round = 0; round < 12; ++round) {
        void *pointers[128]; size_t sizes[128];
        for (unsigned i = 0; i < 128; ++i) {
            size_t n = 8 + ((i * 71 + round * 11) % 8192); sizes[i] = n;
            pointers[i] = calloc(1, n); CHECK(pointers[i]);
            unsigned char *p = pointers[i]; CHECK(p[0] == 0 && p[n - 1] == 0);
            memset(p, (int)(i % 251), n);
        }
        for (unsigned i = 0; i < 128; ++i) {
            unsigned char *p = realloc(pointers[i], sizes[i] + 127); CHECK(p);
            CHECK(p[0] == i % 251 && p[sizes[i] - 1] == i % 251); free(p);
        }
    }
}

static void *allocator_handoff(void *argument) {
    void **pointers = argument;
    for (unsigned i = 0; i < 128; ++i) {
        unsigned char *p = pointers[i];
        if (p[0] != i || p[511] != i) return (void *)1;
        free(p);
        pointers[i] = calloc(1, 1024);
        if (!pointers[i]) return (void *)1;
    }
    return NULL;
}
static void allocator_lifetimes(void) {
    unsigned char *sentinel = malloc(4096); CHECK(sentinel);
    memset(sentinel, 0x51, 4096);
    pid_t child = fork(); CHECK(child >= 0);
    if (child == 0) {
        sentinel[0] = 0;
        void *p = malloc(64); if (!p) _exit(2); free(p); free(sentinel); _exit(0);
    }
    int status; CHECK(waitpid(child, &status, 0) == child && WIFEXITED(status) && WEXITSTATUS(status) == 0);
    CHECK(sentinel[0] == 0x51); free(sentinel);
    void *pointers[128];
    for (unsigned i = 0; i < 128; ++i) { pointers[i] = malloc(512); CHECK(pointers[i]); memset(pointers[i], (int)i, 512); }
    pthread_t thread; CHECK(pthread_create(&thread, NULL, allocator_handoff, pointers) == 0);
    void *result; CHECK(pthread_join(thread, &result) == 0 && result == NULL);
    for (unsigned i = 0; i < 128; ++i) {
        unsigned char *p = pointers[i]; CHECK(p[0] == 0 && p[1023] == 0); free(p);
    }
}

static uint64_t now(void) {
    struct timespec t;
    if (clock_gettime(CLOCK_MONOTONIC, &t)) exit(2);
    return (uint64_t)t.tv_sec * 1000000000 + (uint64_t)t.tv_nsec;
}
static void benchmark(void) {
    LOAD(memcpy); LOAD(memmove); LOAD(memset); LOAD(memcmp); LOAD(strlen);
    LOAD(strcmp); LOAD(strchr); LOAD(strcpy); LOAD(strlcpy);
    LOAD(memmem); LOAD(strstr); LOAD(strcasestr); LOAD(strspn); LOAD(strcspn); LOAD(strcasecmp);
    LOAD(strtoll); LOAD(strtod); LOAD(malloc); LOAD(free);
    char *a = malloc(3 << 20), *b = malloc(3 << 20);
    CHECK(a && b);
    memset(a, 'a', 131071); a[131071] = 0;
    memset(b, 'a', 131071); b[131071] = 0;
    volatile uintptr_t sink = 0;
#define BENCH(label, bytes, count, expression) do { \
    uint64_t start = now(); \
    for (unsigned i = 0; i < (count); ++i) sink ^= (uintptr_t)(expression); \
    uint64_t elapsed = now() - start; \
    printf("BENCH %s %u %u %llu\n", label, (unsigned)(bytes), (unsigned)(count), (unsigned long long)elapsed); \
} while (0)
    BENCH("memcpy-short", 32, 20000, p_memcpy(b, a, 32));
    BENCH("memcpy-4k", 4096, 4000, p_memcpy(b, a, 4096));
    BENCH("memcpy-64k", 65536, 1000, p_memcpy(b, a, 65536));
    BENCH("memmove-4k", 4096, 4000, p_memmove(b + 3, b, 4096));
    BENCH("memset-4k", 4096, 4000, p_memset(b, 'a', 4096));
    BENCH("memcmp-4k", 4096, 4000, p_memcmp(b, a, 4096));
    a[1023] = 0; b[1023] = 0;
    BENCH("strlen-1k", 1023, 4000, p_strlen(a));
    BENCH("strcmp-1k", 1023, 4000, p_strcmp(a, b));
    BENCH("strchr-1k", 1023, 4000, p_strchr(a, 'z'));
    BENCH("strcpy-1k", 1023, 4000, p_strcpy(b, a));
    BENCH("strlcpy-1k", 1023, 4000, p_strlcpy(b, a, 1024));
    a[5] = 0;
    BENCH("strlen-short", 5, 20000, p_strlen(a));
    memset(a, 'a', 65536); a[65536] = 0; memset(b, 'A', 65536); b[65536] = 0;
    BENCH("memmem-64k", 65536, 100, p_memmem(a, 65536, "zzzzq", 5));
    BENCH("strstr-64k", 65536, 100, p_strstr(a, "zzzzq"));
    BENCH("strcasestr-64k", 65536, 100, p_strcasestr(a, "ZzZzQ"));
    BENCH("strspn-64k", 65536, 100, p_strspn(a, "a"));
    BENCH("strcspn-64k", 65536, 100, p_strcspn(a, "z"));
    BENCH("strcasecmp-64k", 65536, 100, p_strcasecmp(a, b));
    BENCH("strtoll", 19, 10000, p_strtoll("9223372036854775807!", NULL, 10));
    BENCH("strtod", 24, 4000, p_strtod("1.234567890123456789e100!", NULL) != 0);
    memset(a, 'a', 2 << 20); a[2 << 20] = 0;
    BENCH("memcpy-2m", 2 << 20, 20, p_memcpy(b, a, 2 << 20));
    BENCH("memmove-2m", 2 << 20, 20, p_memmove(b + 3, b, 2 << 20));
    BENCH("strlen-2m", 2 << 20, 20, p_strlen(a));
    BENCH("strcpy-2m", 2 << 20, 20, p_strcpy(b, a));
    uint64_t start = now();
    for (unsigned i = 0; i < 20000; ++i) { void *p = p_malloc(64); if (!p) exit(3); sink ^= (uintptr_t)p; p_free(p); }
    printf("BENCH malloc-free-64 64 20000 %llu\n", (unsigned long long)(now() - start));
    free(a); free(b);
    (void)sink;
}

#ifdef __ANDROID__
// Pinned-image diagnostics only. Public Android libc hides jemalloc mallctl;
// production adapters never change allocator configuration.
static void allocator_cache(void) {
    Dl_info info; CHECK(dladdr((void *)malloc, &info) != 0);
    typedef int (*control)(const char *, void *, size_t *, void *, size_t);
    control mallctl = (control)((uintptr_t)info.dli_fbase + 0x4f290);
    unsigned char enabled = 1;
    int error = mallctl("thread.tcache.enabled", NULL, NULL, &enabled, sizeof(enabled));
    printf("ALLOCATOR cache-enable=%d\n", error); CHECK(error == 0);
}

#endif

int main(int argc, char **argv) {
#ifdef __ANDROID__
    if (argc == 2 && !strcmp(argv[1], "allocator-cache-bench")) { allocator_cache(); benchmark(); return 0; }
    if (argc == 2 && !strcmp(argv[1], "allocator-cache-stress")) allocator_cache();
    if (argc == 2 && (!strcmp(argv[1], "allocator-info") || !strcmp(argv[1], "allocator-cache-info"))) {
        if (!strcmp(argv[1], "allocator-cache-info")) allocator_cache();
        void *allocation = malloc(64); CHECK(allocation);
        Dl_info info; CHECK(dladdr((void *)malloc, &info) != 0);
        unsigned char *base = info.dli_fbase;
        unsigned key = *(unsigned *)(base + 0x30d0d0);
        unsigned char *tsd = pthread_getspecific(key);
        printf("ALLOCATOR key=%u slow=%u slow-flags=%u opt-tcache=%u tsd=%p state=%u flags=%u,%u,%u\n", key,
            base[0x102888], base[0x313d34], base[0x313f54], tsd, tsd ? tsd[8] : 255,
            tsd ? tsd[9] : 255, tsd ? tsd[10] : 255, tsd ? tsd[11] : 255);
        free(allocation); return 0;
    }
#endif
    if (argc == 2 && !strcmp(argv[1], "bench")) { benchmark(); return 0; }
    if (argc == 2 && !strcmp(argv[1], "cleanup")) {
        return unlink("/system/lib64/bootstrap/libc.so.trames-new") != 0;
    }
    if (argc == 2 && !strcmp(argv[1], "forget-record")) {
        return unlink("/system/lib64/.trames-bionic.json") != 0;
    }
    suite();
#ifdef __ANDROID__
    fortify();
#endif
    extended_suite(); allocator_lifetimes();
    pthread_t threads[4];
    for (uintptr_t i = 0; i < 4; ++i) CHECK(pthread_create(&threads[i], NULL, worker, (void *)i) == 0);
    for (unsigned i = 0; i < 4; ++i) { void *result; CHECK(pthread_join(threads[i], &result) == 0 && result == NULL); }
#ifdef __ANDROID__
#define LIBC_NAME "Bionic"
#define EXTRA_CHECKS "original FORTIFY, "
#elif defined(__GLIBC__)
#define LIBC_NAME "glibc"
#define EXTRA_CHECKS ""
#else
#define LIBC_NAME "musl"
#define EXTRA_CHECKS ""
#endif
    printf("PASS " LIBC_NAME " 44 functions, %lu checks, guard pages, " EXTRA_CHECKS "parallel TLS\n", checks);
    return 0;
}
