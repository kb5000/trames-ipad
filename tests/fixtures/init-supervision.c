// SPDX-License-Identifier: GPL-3.0-or-later
#define _GNU_SOURCE
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

static atomic_int stop;
static atomic_ulong progress;
static volatile sig_atomic_t received;
static volatile sig_atomic_t signal_count;
static void handler(int number) { received = number; ++signal_count; }
static void pause_ms(long ms) {
    struct timespec delay = { .tv_sec = ms / 1000, .tv_nsec = ms % 1000 * 1000000 };
    while (nanosleep(&delay, &delay) && errno == EINTR) {}
}
static void *worker(void *unused) {
    (void)unused;
    uint64_t value = 1;
    while (!atomic_load_explicit(&stop, memory_order_relaxed)) {
        for (unsigned i = 0; i < 10000; ++i) value = value * 6364136223846793005ULL + 1;
        // Preserve CPU work without introducing another guest system call.
        __asm__ volatile("" : "+r"(value));
        atomic_fetch_add_explicit(&progress, 1, memory_order_relaxed);
    }
    return NULL;
}
static int notify(const char *message) {
    int kind = SOCK_DGRAM;
#ifdef SOCK_CLOEXEC
    kind |= SOCK_CLOEXEC;
#endif
    int fd = socket(AF_UNIX, kind, 0);
    if (fd < 0) return 1;
    struct sockaddr_un address;
    memset(&address, 0, sizeof(address));
    address.sun_family = AF_UNIX;
    strcpy(address.sun_path, "/run/host/notify");
    int failed = sendto(fd, message, strlen(message), 0,
                        (struct sockaddr *)&address, sizeof(address)) != (ssize_t)strlen(message);
    close(fd);
    return failed;
}
int main(int argc, char **argv) {
    int native = argc > 1 && !strcmp(argv[1], "native");
    int signal_number = native ? SIGUSR1 : 37;
    struct sigaction action;
    memset(&action, 0, sizeof(action));
    action.sa_handler = handler;
    sigemptyset(&action.sa_mask);
    if (sigaction(signal_number, &action, NULL)) return 90;
    setbuf(stdout, NULL);
    pthread_t threads[3];
    for (int i = 0; i < 3; ++i)
        if (pthread_create(&threads[i], NULL, worker, NULL)) return 91;
    if (!native) {
        pid_t child = fork();
        if (child == 0) _exit(notify("X_SYSTEMD_SIGNALS_LEVEL=2\nREADY=1"));
        int status;
        if (child < 0 || waitpid(child, &status, 0) != child || status != 0) return 92;
    }
    puts("SUPERVISOR_START");
    unsigned long before = atomic_load(&progress);
    for (int i = 0; i < 80; ++i) {
        if (!native && notify("READY=1")) return 93;
        pause_ms(3);
    }
    // Host stop arrives during the notifications, before PID 1 authenticates
    // its shutdown signal level. The non-init child's message cannot arm it.
    if (received || atomic_load(&progress) <= before) return 94;
    puts("SUPERVISOR_UNARMED_OK");
    if (!native && notify("X_SYSTEMD_SIGNALS_LEVEL=2\nREADY=1")) return 95;
    puts("SUPERVISOR_ARMED");
    for (int i = 0; i < 3000 && !received; ++i) pause_ms(1);
    if (received != signal_number) return 96;
    atomic_store(&stop, 1);
    for (int i = 0; i < 3; ++i)
        if (pthread_join(threads[i], NULL)) return 97;
    if (signal_count != 1) return 98;
    printf("{\"passed\":true,\"pid\":%ld,\"signal\":%d,\"signal_count\":%d,\"progress\":%lu}\n",
           (long)getpid(), (int)received, (int)signal_count, atomic_load(&progress));
    return 0;
}
