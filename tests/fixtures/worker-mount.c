// SPDX-License-Identifier: GPL-3.0-or-later
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <sched.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s errno=%d\n", __LINE__, #x, errno); exit(1); } } while (0)
struct instruction { uint16_t code; uint8_t yes, no; uint32_t value; };
struct program { uint16_t count; struct instruction *instructions; };
static pthread_barrier_t barrier;
static char root[128];
static unsigned threads, loops;
static uint64_t elapsed[12];
static uint64_t now(void) {
    struct timespec t; CHECK(clock_gettime(CLOCK_MONOTONIC, &t) == 0);
    return (uint64_t)t.tv_sec * 1000000000 + t.tv_nsec;
}
static void rendezvous(void) {
    int r = pthread_barrier_wait(&barrier);
    CHECK(r == 0 || r == PTHREAD_BARRIER_SERIAL_THREAD);
}
static void *worker(void *arg) {
    unsigned id = (unsigned)(uintptr_t)arg;
    char path[160]; snprintf(path, sizeof path, "%s/%u", root, id);
    CHECK(mkdir(path, 0700) == 0);
    rendezvous(); uint64_t began = now();
    for (unsigned i = 0; i < loops; i++) {
        CHECK(mount("tmpfs", path, "tmpfs", MS_NOSUID | MS_NODEV, "size=4096") == 0);
        CHECK(umount2(path, 0) == 0);
    }
    elapsed[id] = now() - began; rendezvous();
    CHECK(rmdir(path) == 0); return NULL;
}
static void compat(void) {
    CHECK(chdir(root) == 0);
    CHECK(mkdir("a", 0700) == 0 && mkdir("b", 0700) == 0 && mkdir("c", 0700) == 0);
    CHECK(mount("tmpfs", "a", "tmpfs", 0, NULL) == 0);
    int file = open("a/file", O_CREAT | O_RDWR, 0600); CHECK(file >= 0);
    CHECK(write(file, "test", 4) == 4 && close(file) == 0);
    CHECK(mount(NULL, "a", NULL, MS_REMOUNT | MS_RDONLY, NULL) == 0);
    CHECK(open("a/file", O_WRONLY) == -1 && errno == EROFS);
    CHECK(mount(NULL, "a", NULL, MS_REMOUNT, NULL) == 0);
    CHECK(mount("a", "b", NULL, MS_MOVE, NULL) == 0);
    CHECK(mount("b", "c", NULL, MS_BIND, NULL) == 0);
    struct stat a, b;
    CHECK(stat("b/file", &a) == 0 && stat("c/file", &b) == 0);
    CHECK(a.st_ino == b.st_ino && a.st_dev == b.st_dev);
    int dir = open("b", O_DIRECTORY | O_RDONLY); CHECK(dir >= 0);
    CHECK(umount2("b", 0) == -1 && errno == EBUSY);
    CHECK(umount2("b", MNT_DETACH) == 0);
    file = openat(dir, "file", O_RDONLY); CHECK(file >= 0);
    char bytes[4]; CHECK(read(file, bytes, sizeof bytes) == 4 && !memcmp(bytes, "test", 4));
    CHECK(close(file) == 0 && close(dir) == 0 && umount2("c", 0) == 0);
    CHECK(chdir("/") == 0);
    pid_t child = fork(); CHECK(child >= 0);
    if (!child) {
        CHECK(unshare(CLONE_NEWNS) == 0 && chdir(root) == 0);
        CHECK(mount("tmpfs", "a", "tmpfs", 0, NULL) == 0);
        CHECK(mkdir("a/old", 0700) == 0 && chdir("/") == 0);
        char newroot[160], oldroot[180];
        snprintf(newroot, sizeof newroot, "%s/a", root);
        snprintf(oldroot, sizeof oldroot, "%s/old", newroot);
        CHECK(syscall(SYS_pivot_root, newroot, oldroot) == 0);
        char cwd[32]; CHECK(getcwd(cwd, sizeof cwd) && !strcmp(cwd, "/"));
        CHECK(umount2("/old", MNT_DETACH) == 0); _exit(0);
    }
    int status; CHECK(waitpid(child, &status, 0) == child && WIFEXITED(status) && WEXITSTATUS(status) == 0);
}
int main(int argc, char **argv) {
    const char *mode = argc > 1 ? argv[1] : "compat";
    threads = argc > 2 ? (unsigned)atoi(argv[2]) : 2;
    loops = argc > 3 ? (unsigned)atoi(argv[3]) : 32;
    CHECK(threads > 0 && threads <= 12 && loops > 0);
    // Native validation is root inside its own mount namespace, never the host
    // namespace. Trames uses the identical guest ABI and private filesystem.
    CHECK(unshare(CLONE_NEWNS) == 0);
    CHECK(mount(NULL, "/", NULL, MS_REC | MS_PRIVATE, NULL) == 0);
    if (argc > 4 && !strcmp(argv[4], "filtered")) {
        struct instruction instruction = {.code = 6, .value = 0x7fff0000};
        struct program filter = {.count = 1, .instructions = &instruction};
        CHECK(prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0) == 0);
        CHECK(syscall(SYS_seccomp, 1, 0, &filter) == 0);
    }
    CHECK(mkdir("/tmp", 0777) == 0 || errno == EEXIST);
    snprintf(root, sizeof root, "/tmp/trames-mount-%ld", (long)getpid());
    CHECK(mkdir(root, 0700) == 0);
    if (!strcmp(mode, "compat")) compat(); else CHECK(!strcmp(mode, "loops"));
    CHECK(pthread_barrier_init(&barrier, NULL, threads + 1) == 0);
    pthread_t ids[12];
    for (unsigned i = 0; i < threads; i++) CHECK(pthread_create(&ids[i], NULL, worker, (void *)(uintptr_t)i) == 0);
    rendezvous(); rendezvous(); uint64_t longest = 0;
    for (unsigned i = 0; i < threads; i++) {
        CHECK(pthread_join(ids[i], NULL) == 0); if (elapsed[i] > longest) longest = elapsed[i];
    }
    CHECK(pthread_barrier_destroy(&barrier) == 0);
    CHECK(chdir(root) == 0);
    if (!strcmp(mode, "compat")) CHECK(rmdir("a") == 0 && rmdir("b") == 0 && rmdir("c") == 0);
    CHECK(chdir("/") == 0 && rmdir(root) == 0);
    printf("{\"mode\":\"%s\",\"threads\":%u,\"loops\":%u,\"elapsed_ns\":%llu}\n", mode, threads, loops, (unsigned long long)longest);
    return 0;
}
