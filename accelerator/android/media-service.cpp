// SPDX-License-Identifier: GPL-3.0-or-later
// ARM64 entry point for Android 13's original system OMX implementation.
// The selected Waydroid image supplies these libraries but a 32-bit launcher.
// Codec discovery, Binder objects and decoding remain upstream implementations.
#include <hidl/HidlTransportSupport.h>
#include <media/stagefright/omx/1.0/Omx.h>
#include <media/stagefright/omx/1.0/OmxStore.h>
#include "ResourceManagerService.h"
#include <android/binder_process.h>
#include <cstdio>
#include <csignal>

int main() {
    using namespace android;
    using namespace android::hardware::media::omx::V1_0;
    std::signal(SIGPIPE, SIG_IGN);
    ResourceManagerService::instantiate();
    ABinderProcess_startThreadPool();
    hardware::configureRpcThreadpool(16, true);
    sp<IOmx> omx = new implementation::Omx();
    if (omx->registerAsService() != 0) return 1;
    sp<IOmxStore> store = new implementation::OmxStore(omx);
    if (store->registerAsService() != 0) return 2;
    std::fprintf(stderr, "Trames: original ARM64 OMX services registered\n");
    hardware::joinRpcThreadpool();
    return 3;
}
