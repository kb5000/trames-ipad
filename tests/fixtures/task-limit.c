// SPDX-License-Identifier: GPL-3.0-or-later
// Product acceptance for Trames' per-environment task ceiling. This changes
// the guest resource, never the Linux host's global threads-max.
#define _GNU_SOURCE
#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

static unsigned get_limit(int fd) {
    char text[64] = {0};
    ssize_t n = pread(fd, text, sizeof(text) - 1, 0);
    assert(n > 0);
    return (unsigned)strtoul(text, NULL, 10);
}
static void set_limit(int fd, unsigned limit) {
    char text[32];
    int n = snprintf(text, sizeof(text), "%u\n", limit);
    assert(n > 0 && (size_t)n < sizeof(text));
    assert(pwrite(fd, text, (size_t)n, 0) == n);
    assert(get_limit(fd) == limit);
}
static void reap(pid_t pid) {
    int status;
    assert(waitpid(pid, &status, 0) == pid);
    assert(WIFEXITED(status) && WEXITSTATUS(status) == 0);
}
static void admitted(void) {
    pid_t pid = fork();
    assert(pid >= 0);
    if (pid == 0) _exit(0);
    reap(pid);
}
static void denied(void) {
    errno = 0;
    pid_t pid = fork();
    if (pid == 0) _exit(99);
    if (pid > 0) { int status; (void)waitpid(pid, &status, 0); }
    assert(pid == -1 && errno == EAGAIN);
}
static int numeric(const char *name) {
    return *name && strspn(name, "0123456789") == strlen(name);
}
static unsigned task_count(void) {
    DIR *proc = opendir("/proc");
    assert(proc);
    unsigned count = 0;
    struct dirent *entry;
    while ((entry = readdir(proc))) {
        if (!numeric(entry->d_name)) continue;
        char path[512];
        assert(snprintf(path, sizeof(path), "/proc/%s/task", entry->d_name) > 0);
        DIR *tasks = opendir(path);
        assert(tasks);
        struct dirent *task;
        while ((task = readdir(tasks))) count += numeric(task->d_name);
        assert(closedir(tasks) == 0);
    }
    assert(closedir(proc) == 0);
    return count;
}
int main(int argc, char **argv) {
    assert(argc == 3);
    alarm(45);
    unsigned boot = (unsigned)strtoul(argv[2], NULL, 10);
    int fd = open("/proc/sys/kernel/threads-max", O_RDWR | O_CLOEXEC);
    assert(fd >= 0 && get_limit(fd) == boot);
    printf("TASK_LIMIT_BOOT:%u\n", boot);
    if (!strcmp(argv[1], "read")) {
        puts("TASK_LIMIT_READ_OK");
        return 0;
    }
    assert(!strcmp(argv[1], "live") && boot >= 32 && boot < 1073741823);
    for (unsigned i = 0; i < 2; i++) {
        const char *bad = i ? "1073741824" : "0";
        errno = 0;
        assert(pwrite(fd, bad, strlen(bad), 0) == -1 && errno == EINVAL);
        assert(get_limit(fd) == boot);
    }
    // Permissions are checked with the caller's current credentials even when
    // the proc OFD was originally opened by root.
    pid_t restricted = fork();
    assert(restricted >= 0);
    if (!restricted) {
        assert(setuid(65534) == 0);
        errno = 0;
        assert(pwrite(fd, "32", 2, 0) == -1 && errno == EACCES);
        errno = 0;
        assert(open("/proc/sys/kernel/threads-max", O_WRONLY) == -1 && errno == EACCES);
        assert(get_limit(fd) == boot);
        _exit(0);
    }
    reap(restricted);
    unsigned baseline = task_count();
    assert(baseline > 0 && baseline + 2 < boot);
    set_limit(fd, baseline + 1);
    int ready[2], release[2];
    assert(pipe(ready) == 0 && pipe(release) == 0);
    pid_t child = fork();
    assert(child >= 0);
    if (!child) {
        char byte = 'r';
        close(ready[0]); close(release[1]);
        assert(write(ready[1], &byte, 1) == 1);
        assert(read(release[0], &byte, 1) == 1);
        _exit(0);
    }
    close(ready[1]); close(release[0]);
    char byte;
    assert(read(ready[0], &byte, 1) == 1);
    // Lowering a ceiling keeps all existing tasks alive; future births fail.
    set_limit(fd, 1);
    assert(kill(child, 0) == 0);
    denied();
    set_limit(fd, baseline + 1);
    denied();
    set_limit(fd, baseline + 2);
    admitted();
    set_limit(fd, baseline + 1);
    assert(write(release[1], "x", 1) == 1);
    siginfo_t info;
    assert(waitid(P_PID, (id_t)child, &info, WEXITED | WNOWAIT) == 0);
    assert(info.si_pid == child && info.si_code == CLD_EXITED && info.si_status == 0);
    denied(); // An unreaped zombie still owns its admission slot.
    reap(child);
    admitted();
    close(ready[0]); close(release[1]);
    set_limit(fd, boot + 1);
    assert(task_count() == baseline); // Also forces fresh proc snapshots.
    assert(get_limit(fd) == boot + 1);
    assert(close(fd) == 0);
    printf("TASK_LIMIT_LIVE_OK:baseline=%u final=%u\n", baseline, boot + 1);
    return 0;
}
