// SPDX-License-Identifier: GPL-3.0-or-later
// Exercise the complete replacement libc through the original Android linker.
#define _GNU_SOURCE
#include <dlfcn.h>
#include <errno.h>
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

static uint64_t now(void) {
    struct timespec t;
    if (clock_gettime(CLOCK_MONOTONIC, &t)) exit(2);
    return (uint64_t)t.tv_sec * 1000000000 + (uint64_t)t.tv_nsec;
}
static void benchmark(void) {
    LOAD(memcpy); LOAD(memmove); LOAD(memset); LOAD(memcmp); LOAD(strlen);
    LOAD(strcmp); LOAD(strchr); LOAD(strcpy); LOAD(strlcpy);
    char *a = malloc(131072), *b = malloc(131072);
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
    free(a); free(b);
    (void)sink;
}

int main(int argc, char **argv) {
    if (argc == 2 && !strcmp(argv[1], "bench")) { benchmark(); return 0; }
    if (argc == 2 && !strcmp(argv[1], "cleanup")) {
        return unlink("/system/lib64/bootstrap/libc.so.trames-new") != 0;
    }
    if (argc == 2 && !strcmp(argv[1], "forget-record")) {
        return unlink("/system/lib64/.trames-bionic.json") != 0;
    }
    suite(); fortify();
    pthread_t threads[4];
    for (uintptr_t i = 0; i < 4; ++i) CHECK(pthread_create(&threads[i], NULL, worker, (void *)i) == 0);
    for (unsigned i = 0; i < 4; ++i) { void *result; CHECK(pthread_join(threads[i], &result) == 0 && result == NULL); }
    printf("PASS Bionic 22 functions, %lu checks, guard pages, original FORTIFY, parallel TLS\n", checks);
    return 0;
}
