// SPDX-License-Identifier: GPL-3.0-or-later
// Fixed-work regular-file lifecycle and parallel append/growth reference.
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#ifdef __linux__
#include <sys/sendfile.h>
#include <sys/syscall.h>
#endif
static void check(int ok, const char *what) {
    if (!ok) { fprintf(stderr, "file-worker: %s: %s\n", what, strerror(errno)); exit(1); }
}
static uint64_t now(void) { struct timespec t; check(!clock_gettime(CLOCK_MONOTONIC, &t), "clock"); return (uint64_t)t.tv_sec * 1000000000 + t.tv_nsec; }
struct barrier { pthread_mutex_t lock; pthread_cond_t event; unsigned waiting, total; };
static struct barrier start = { PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER, 0, 0 };
static void meet(void) {
    check(!pthread_mutex_lock(&start.lock), "barrier lock");
    if (++start.waiting == start.total) check(!pthread_cond_broadcast(&start.event), "barrier wake");
    else while (start.waiting != start.total) check(!pthread_cond_wait(&start.event, &start.lock), "barrier wait");
    check(!pthread_mutex_unlock(&start.lock), "barrier unlock");
}
struct record { uint32_t id, sequence; uint64_t magic; unsigned char payload[16]; };
struct writer { int fd; unsigned id, iterations; uint64_t elapsed; };
static void *write_records(void *argument) {
    struct writer *w = argument; meet(); uint64_t begin = now();
    for (unsigned i = 0; i < w->iterations; i++) {
        struct record r = { w->id, i, 0x98127ab4c5d60ef3ULL, {0} }; memset(r.payload, w->id + i, sizeof(r.payload));
        check(write(w->fd, &r, sizeof(r)) == sizeof(r), "record write");
    }
    w->elapsed = now() - begin; return NULL;
}
static int temporary(void) { char name[] = "file-worker-XXXXXX"; int fd = mkstemp(name); check(fd >= 0 && !unlink(name), "temporary file"); return fd; }
static void records(int fd, unsigned writers, unsigned iterations, unsigned first_id) {
    struct stat st; check(!fstat(fd, &st) && st.st_size == (off_t)(writers * iterations * sizeof(struct record)), "record EOF");
    unsigned char *seen = calloc(writers * iterations, 1); check(seen != NULL, "record bitmap");
    for (unsigned i = 0; i < writers * iterations; i++) {
        struct record r; check(pread(fd, &r, sizeof(r), (off_t)i * sizeof(r)) == sizeof(r), "record read");
        check(r.id >= first_id && r.id < first_id + writers && r.sequence < iterations && r.magic == 0x98127ab4c5d60ef3ULL, "record identity");
        unsigned slot = (r.id - first_id) * iterations + r.sequence; check(!seen[slot]++, "record unique");
        for (unsigned j = 0; j < sizeof(r.payload); j++) check(r.payload[j] == (unsigned char)(r.id + r.sequence), "record bytes");
    }
    free(seen);
}
static void parallel(const char *mode, unsigned count, unsigned iterations) {
    pthread_t threads[8]; struct writer writers[8]; int shared = temporary(); int append = !strcmp(mode, "append");
    if (append) check(fcntl(shared, F_SETFL, O_APPEND) == 0, "append flag");
    start.total = count + 1;
    for (unsigned i = 0; i < count; i++) {
        writers[i] = (struct writer){ append ? shared : temporary(), i, iterations, 0 };
        check(!pthread_create(&threads[i], NULL, write_records, &writers[i]), "writer create");
    }
    meet(); uint64_t longest = 0;
    for (unsigned i = 0; i < count; i++) {
        check(!pthread_join(threads[i], NULL), "writer join"); if (writers[i].elapsed > longest) longest = writers[i].elapsed;
        if (!append) { records(writers[i].fd, 1, iterations, i); check(!close(writers[i].fd), "writer close"); }
    }
    if (append) records(shared, count, iterations, 0);
    check(!close(shared), "shared close");
    printf("file-worker %s threads=%u iterations=%u longest_ns=%llu\n", mode, count, iterations, (unsigned long long)longest);
}
#ifdef __linux__
static void semantic(void) {
    int fd = temporary(); unsigned char bytes[32768]; memset(bytes, 0xa5, sizeof(bytes));
    struct iovec vec[2] = {{bytes, 16384}, {bytes + 16384, 16384}};
    check(writev(fd, vec, 2) == sizeof(bytes), "writev growth"); memset(bytes, 0, sizeof(bytes));
    check(preadv(fd, vec, 2, 0) == sizeof(bytes) && bytes[0] == 0xa5 && bytes[32767] == 0xa5, "preadv bytes");
    check(lseek(fd, 0, SEEK_CUR) == sizeof(bytes), "positioned vector offset");
    check(!fallocate(fd, 0, 32768, 8192), "allocate"); struct stat st; check(!fstat(fd, &st) && st.st_size == 40960, "allocated EOF");
    check(!fsync(fd) && !fdatasync(fd), "durability");
    int copy = temporary(); off_t from = 1, to = 2;
    check(syscall(__NR_copy_file_range, fd, &from, copy, &to, 8192, 0) == 8192 && from == 8193 && to == 8194, "copy offsets");
    check(lseek(copy, 0, SEEK_CUR) == 0, "copy preserves implicit offset");
    from = 0; check(sendfile(copy, fd, &from, 8) == 8 && from == 8 && lseek(copy, 0, SEEK_CUR) == 8, "sendfile regular output");
    memset(bytes, 0x42, 16); vec[0].iov_len = 16;
    check(syscall(__NR_pwritev2, copy, vec, 1, 20, 0, 2) == 16, "pwritev2 DSYNC");
    memset(bytes, 0, 16); check(syscall(__NR_preadv2, copy, vec, 1, 20, 0, 0) == 16 && bytes[0] == 0x42, "preadv2 bytes");
    check(!close(copy), "copy close");
    check(!ftruncate(fd, 8192), "mapped initial EOF");
    unsigned char *map = mmap(NULL, 8192, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0); check(map != MAP_FAILED, "file mapping");
    map[0] = 0x11; map[4096] = 0x22; check(!msync(map, 8192, MS_SYNC), "mapped writeback");
    check(pread(fd, bytes, 1, 0) == 1 && bytes[0] == 0x11, "mapped source visibility");
    int ready[2], proceed[2]; check(!pipe(ready) && !pipe(proceed), "mapped peer pipes"); pid_t child = fork(); check(child >= 0, "mapped peer fork");
    if (!child) { check(write(ready[1], "r", 1) == 1 && read(proceed[0], bytes, 1) == 1, "mapped peer wait"); check(map[0] == 0x11, "mapped peer bytes after resize"); _exit(0); }
    check(read(ready[0], bytes, 1) == 1, "peer ready"); check(!ftruncate(fd, 12288), "mapped growth"); check(!ftruncate(fd, 32), "mapped shrink");
    check(write(proceed[1], "p", 1) == 1, "peer resume"); int status; check(waitpid(child, &status, 0) == child && WIFEXITED(status) && !WEXITSTATUS(status), "mapped peer result");
    check(!munmap(map, 8192) && !close(fd), "mapping release");
    for (int i = 0; i < 2; i++) { close(ready[i]); close(proceed[i]); }
    int named = open("truncate-open", O_CREAT | O_RDWR | O_TRUNC, 0600); check(named >= 0 && write(named, "original", 8) == 8, "named original");
    int truncated = open("truncate-open", O_RDWR | O_TRUNC); check(truncated >= 0 && !fstat(named, &st) && st.st_size == 0, "truncate original inode");
    check(!close(named) && !close(truncated) && !truncate("truncate-open", 8192), "path truncate growth");
    check(!stat("truncate-open", &st) && st.st_size == 8192 && !unlink("truncate-open"), "path truncate EOF");
    int directory = open(".", O_DIRECTORY | O_RDONLY); check(directory >= 0 && !syncfs(directory) && !close(directory), "directory syncfs");
    sync(); puts("file-worker semantic passed");
}
#endif
int main(int argc, char **argv) {
    setvbuf(stdout, NULL, _IONBF, 0); check(argc >= 2, "arguments");
#ifdef __linux__
    if (!strcmp(argv[1], "semantic")) { semantic(); return 0; }
#endif
    check(argc == 4 && (!strcmp(argv[1], "append") || !strcmp(argv[1], "grow")), "benchmark arguments");
    unsigned count = (unsigned)strtoul(argv[2], NULL, 10), iterations = (unsigned)strtoul(argv[3], NULL, 10);
    check(count > 0 && count <= 8 && iterations > 0 && iterations <= 100000, "work bounds"); parallel(argv[1], count, iterations); return 0;
}
