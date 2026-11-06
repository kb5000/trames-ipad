// SPDX-License-Identifier: GPL-3.0-or-later
#define _GNU_SOURCE
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#if defined(__APPLE__)
#include <sys/random.h>
#endif
#if defined(__linux__)
#include <linux/filter.h>
#include <linux/seccomp.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <sys/uio.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <poll.h>
#endif

static struct {
    pthread_mutex_t mutex;
    pthread_cond_t changed;
    unsigned arrived, generation;
} gate = {PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER, 0, 0};
static unsigned threads, rounds, policy, api;

static void rendezvous(void) {
    assert(!pthread_mutex_lock(&gate.mutex));
    unsigned generation = gate.generation;
    if (++gate.arrived == threads + 1) {
        gate.arrived = 0;
        gate.generation++;
        assert(!pthread_cond_broadcast(&gate.changed));
    } else {
        while (generation == gate.generation)
            assert(!pthread_cond_wait(&gate.changed, &gate.mutex));
    }
    assert(!pthread_mutex_unlock(&gate.mutex));
}
static uint64_t now(void) {
    struct timespec t;
    assert(!clock_gettime(CLOCK_MONOTONIC, &t));
    return (uint64_t)t.tv_sec * 1000000000 + t.tv_nsec;
}
static void preflight(int skip_wide) {
#if defined(__linux__)
    unsigned char bytes[64];
    for (unsigned flags = 0; flags <= 3; flags++)
        assert(syscall(SYS_getrandom, bytes, sizeof(bytes), flags) == sizeof(bytes));
    assert(syscall(SYS_getrandom, NULL, 0, 0) == 0);
    errno = 0;
    assert(syscall(SYS_getrandom, (void *)-1, 0, 0) == -1 && errno == EFAULT);
    if (!skip_wide) {
        assert(syscall(SYS_getrandom, bytes, sizeof(bytes), 1UL << 32) == sizeof(bytes));
    }
    errno = 0;
    assert(syscall(SYS_getrandom, (void *)-1, 1, 8) == -1 && errno == EINVAL);
    long page = sysconf(_SC_PAGESIZE);
    void *readonly = mmap(NULL, page, PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    assert(readonly != MAP_FAILED);
    errno = 0;
    assert(syscall(SYS_getrandom, readonly, 16, 0) == -1 && errno == EFAULT);
    assert(!munmap(readonly, page));
    for (unsigned index = 0; index < 2; index++) {
        const char *path = index ? "/dev/urandom" : "/dev/random";
        int fd = open(path, O_RDWR | O_CLOEXEC);
        assert(fd >= 0);
        struct stat info;
        assert(!fstat(fd, &info) && S_ISCHR(info.st_mode));
        assert(read(fd, bytes, sizeof(bytes)) == sizeof(bytes));
        struct iovec vectors[] = {{bytes, 16}, {bytes + 16, 48}};
        assert(readv(fd, vectors, 2) == sizeof(bytes));
        assert(write(fd, bytes, sizeof(bytes)) == sizeof(bytes));
        assert(writev(fd, vectors, 2) == sizeof(bytes));
        assert(pread(fd, bytes, sizeof(bytes), 0) == sizeof(bytes));
        assert(pwrite(fd, bytes, sizeof(bytes), 0) == sizeof(bytes));
        int nonblocking = 1;
        assert(!ioctl(fd, FIONBIO, &nonblocking));
        assert(fcntl(fd, F_GETFL) & O_NONBLOCK);
        struct pollfd event = {fd, POLLIN, 0};
        assert(poll(&event, 1, 0) == 1 && (event.revents & POLLIN));
        assert(!close(fd));
    }
    for (unsigned index = 0; index < 2; index++) {
        const char *path = index ? "/proc/sys/kernel/random/boot_id" : "/proc/sys/kernel/random/uuid";
        int fd = open(path, O_RDONLY | O_CLOEXEC);
        assert(fd >= 0);
        assert(read(fd, bytes, sizeof(bytes)) == 37);
        assert(bytes[8] == '-' && bytes[13] == '-' && bytes[18] == '-' && bytes[23] == '-' && bytes[36] == '\n');
        assert(!close(fd));
    }
    if (policy) {
        struct sock_filter code[] = {
            BPF_STMT(BPF_LD | BPF_W | BPF_ABS, 0),
            BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, SYS_getrandom, 0, 1),
            BPF_STMT(BPF_RET | BPF_K, policy == 2 ? SECCOMP_RET_ERRNO | 42 : SECCOMP_RET_ALLOW),
            BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ALLOW),
        };
        struct sock_fprog filter = {4, code};
        assert(!prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0));
        assert(!syscall(SYS_seccomp, SECCOMP_SET_MODE_FILTER, 1, &filter));
    }
#else
    (void)skip_wide;
    assert(!policy && !api);
#endif
}
static void *worker(void *unused) {
    (void)unused;
    unsigned char bytes[64];
    memset(bytes, 0x55, sizeof(bytes));
    int fd = -1;
    if (api) {
        fd = open(api == 1 ? "/dev/urandom" : "/dev/random", O_RDONLY);
        assert(fd >= 0);
    }
    rendezvous();
    rendezvous();
    for (unsigned i = 0; i < rounds; i++) {
#if defined(__linux__)
        if (!api) {
            long result = syscall(SYS_getrandom, bytes, sizeof(bytes), 0);
            if (policy == 2) assert(result == -1 && errno == 42);
            else assert(result == sizeof(bytes));
        } else if (api == 1) {
            assert(read(fd, bytes, sizeof(bytes)) == sizeof(bytes));
        } else {
            struct iovec vectors[] = {{bytes, 16}, {bytes + 16, 48}};
            assert(readv(fd, vectors, 2) == sizeof(bytes));
        }
#else
        assert(!getentropy(bytes, sizeof(bytes)));
#endif
    }
    rendezvous();
    if (!(policy == 2 && !api)) {
        unsigned char initial[64];
        memset(initial, 0x55, sizeof(initial));
        assert(memcmp(bytes, initial, sizeof(bytes)) != 0);
    }
    if (fd >= 0) assert(!close(fd));
    return NULL;
}
int main(int argc, char **argv) {
    threads = argc > 1 ? atoi(argv[1]) : 4;
    rounds = argc > 2 ? atoi(argv[2]) : 100;
    policy = argc > 3 ? atoi(argv[3]) : 0;
    api = argc > 4 ? atoi(argv[4]) : 0;
    assert(threads && threads <= 12 && rounds && policy <= 2 && api <= 2);
    preflight(argc > 5);
    pthread_t ids[12];
    for (unsigned i = 0; i < threads; i++) assert(!pthread_create(ids + i, NULL, worker, NULL));
    rendezvous();
    uint64_t began = now();
    rendezvous();
    rendezvous();
    uint64_t elapsed = now() - began;
    for (unsigned i = 0; i < threads; i++) assert(!pthread_join(ids[i], NULL));
    printf("worker-entropy: threads=%u rounds=%u policy=%u api=%u elapsed_ns=%llu: ok\n",
           threads, rounds, policy, api, (unsigned long long)elapsed);
    return 0;
}
