// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef TRAMES_ANDROID_BUFFER_H
#define TRAMES_ANDROID_BUFFER_H
#include <cutils/native_handle.h>
#include <stdint.h>
#include <stddef.h>
#define TRAMES_BUFFER_MAGIC 0x54524742u
struct trames_buffer {
    native_handle_t base;
    int fd;
    uint32_t magic,width,height,format,stride;
    uint64_t size,address;
};
_Static_assert(sizeof(struct trames_buffer)==56,"Android arm64 native handle ABI");
static inline const struct trames_buffer *trames_buffer(buffer_handle_t handle) {
    const struct trames_buffer *b=(const void*)handle;
    return b && b->base.version==sizeof(native_handle_t) && b->base.numFds==1 &&
        b->base.numInts==(sizeof(*b)-sizeof(native_handle_t)-sizeof(int))/sizeof(int) && b->magic==TRAMES_BUFFER_MAGIC?b:NULL;
}
#endif
