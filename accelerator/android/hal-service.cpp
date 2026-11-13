// SPDX-License-Identifier: GPL-3.0-or-later
// ARM64 launcher for the image's original 64-bit HIDL passthrough HALs.
// Function declarations match Android 13 libhidl's public LegacySupport /
// HidlTransportSupport interfaces; no generated HIDL objects are reimplemented.
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>
namespace android { namespace hardware {
void configureRpcThreadpool(std::size_t, bool);
void joinRpcThreadpool();
std::int32_t registerPassthroughServiceImplementation(
    const std::string&, const std::string&, const std::string&);
} }
int main(int argc, char **argv) {
    if (argc < 2) { std::fprintf(stderr,"usage: trames-hal-service INTERFACE...\n"); return 2; }
    android::hardware::configureRpcThreadpool(16,true);
    const std::string instance("default");
    for (int i=1;i<argc;i++) {
        const std::string descriptor(argv[i]);
        auto status=android::hardware::registerPassthroughServiceImplementation(descriptor,descriptor,instance);
        if(status) { std::fprintf(stderr,"HAL registration failed: %s (%d)\n",argv[i],status); return 1; }
    }
    android::hardware::joinRpcThreadpool();
    return 1;
}
