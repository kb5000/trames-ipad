// SPDX-License-Identifier: GPL-3.0-or-later
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#ifdef __linux__
#include <sys/statfs.h>
#include <sys/syscall.h>
#include <sys/xattr.h>
#endif
static void check(int ok, const char *what) { if (!ok) { perror(what); exit(1); } }
static uint64_t now(void) { struct timespec t; check(!clock_gettime(CLOCK_MONOTONIC, &t), "clock"); return (uint64_t)t.tv_sec * 1000000000 + t.tv_nsec; }
static struct { pthread_mutex_t lock; pthread_cond_t event; unsigned waiting, total; } start = { PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER, 0, 0 };
static void meet(void) {
    check(!pthread_mutex_lock(&start.lock), "barrier lock");
    if (++start.waiting == start.total) check(!pthread_cond_broadcast(&start.event), "barrier wake");
    else while (start.waiting != start.total) check(!pthread_cond_wait(&start.event, &start.lock), "barrier wait");
    check(!pthread_mutex_unlock(&start.lock), "barrier unlock");
}
struct actor { int directory; unsigned iterations; uint64_t elapsed; };
static void *lifecycle(void *argument) {
    struct actor *a = argument; meet(); uint64_t begin = now();
    for (unsigned i = 0; i < a->iterations; i++) {
        check(!mkdirat(a->directory, "child", 0700), "mkdirat");
        int d = openat(a->directory, "child", O_RDONLY | O_DIRECTORY); check(d >= 0, "directory open");
        int fd = openat(d, "old", O_CREAT | O_EXCL | O_RDWR, 0600); check(fd >= 0, "file create");
        check(write(fd, &i, sizeof(i)) == sizeof(i), "file bytes");
        check(!fchmod(fd, 0640), "fchmod");
        check(!linkat(d, "old", d, "hard", 0), "linkat");
        check(!symlinkat("hard", d, "soft"), "symlinkat");
        check(!renameat(d, "old", d, "new"), "renameat");
        struct stat st; check(!fstatat(d, "soft", &st, 0) && st.st_nlink == 2 && st.st_size == sizeof(i), "retained links");
        check(!close(fd), "file close");
        check(!unlinkat(d, "new", 0) && !unlinkat(d, "hard", 0) && !unlinkat(d, "soft", 0), "unlinkat");
        check(!close(d) && !unlinkat(a->directory, "child", AT_REMOVEDIR), "rmdir");
    }
    a->elapsed = now() - begin; return NULL;
}
#ifdef __linux__
static void semantic(void) {
    char original[4096], path[] = "namespace-worker-semantic"; check(getcwd(original, sizeof(original)) != NULL, "cwd");
    check(!mkdir(path, 0700), "mkdir"); int d = open(path, O_DIRECTORY | O_RDONLY); check(d >= 0, "open directory");
    check(!fchdir(d), "fchdir"); char current[4096]; check(getcwd(current, sizeof(current)) && strstr(current, path), "live cwd");
    int fd = open("file", O_CREAT | O_RDWR | O_EXCL, 0600); check(fd >= 0, "relative create");
    check(!fsetxattr(fd, "user.value", "hello", 5, XATTR_CREATE), "attribute create");
    char bytes[64]; check(fgetxattr(fd, "user.value", NULL, 0) == 5 && fgetxattr(fd, "user.value", bytes, sizeof(bytes)) == 5 && !memcmp(bytes, "hello", 5), "attribute bytes");
    check(flistxattr(fd, bytes, sizeof(bytes)) == 11 && !memcmp(bytes, "user.value", 11), "attribute names");
    check(!fchmod(fd, 0640) && !fchown(fd, (uid_t)-1, (gid_t)-1), "metadata");
    struct timespec times[2] = {{123, 456}, {789, 123}}; check(!futimens(fd, times), "timestamps");
    struct stat st; check(!fstat(fd, &st) && (st.st_mode & 0777) == 0640 && st.st_mtim.tv_sec == 789, "metadata stat");
    check(!fremovexattr(fd, "user.value"), "attribute removal");
    struct statfs fs; check(!fstatfs(d, &fs) && fs.f_bsize > 0, "fstatfs");
    check(!close(fd) && !unlink("file") && !chdir(original) && !close(d) && !rmdir(path), "relative cleanup");
    puts("namespace-worker semantic passed");
}
#endif
int main(int argc, char **argv) {
    setvbuf(stdout, NULL, _IONBF, 0); check(argc >= 2, "arguments");
#ifdef __linux__
    if (!strcmp(argv[1], "semantic")) { semantic(); return 0; }
#endif
    check(argc == 4 && !strcmp(argv[1], "lifecycle"), "mode"); unsigned workers = strtoul(argv[2], NULL, 10), iterations = strtoul(argv[3], NULL, 10);
    check(workers > 0 && workers <= 8 && iterations > 0, "counts"); start.total = workers + 1;
    pthread_t threads[8]; struct actor actors[8]; char paths[8][64];
    for (unsigned i = 0; i < workers; i++) {
        snprintf(paths[i], sizeof(paths[i]), "namespace-worker-%u", i); check(!mkdir(paths[i], 0700), "actor directory");
        actors[i] = (struct actor){open(paths[i], O_RDONLY | O_DIRECTORY), iterations, 0}; check(actors[i].directory >= 0, "actor open");
        check(!pthread_create(&threads[i], NULL, lifecycle, &actors[i]), "thread create");
    }
    meet(); uint64_t longest = 0;
    for (unsigned i = 0; i < workers; i++) {
        check(!pthread_join(threads[i], NULL), "thread join"); if (actors[i].elapsed > longest) longest = actors[i].elapsed;
        check(!close(actors[i].directory) && !rmdir(paths[i]), "actor cleanup");
    }
    printf("namespace-worker lifecycle threads=%u iterations=%u longest_ns=%llu\n", workers, iterations, (unsigned long long)longest);
    return 0;
}
