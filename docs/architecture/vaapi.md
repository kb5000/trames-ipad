# VA-API over VideoToolbox

Status: experimental H.264, HEVC and AV1 decode, updated 2026-11-12. The shared runtime,
standard libva driver, retained image FDs, native Vulkan import and Mesa EGL
adapter are implemented. Stock Firefox 142 selects VA-API in its RDD process
with its normal sandbox. Foreground 720p30 and near-source-rate 1080p60 playback
are validated below. The recommended Firefox 151/native-video profile passes cold short-clip
autoplay/looping without the old 15-second buffer pause; Firefox 142 retains that
limitation. HEVC Main and AV1 Profile 0 hardware decode now work with the paired
FFmpeg adapter below, limited to 8-bit 4:2:0 output. Encoding, Main10/P010, video
processing, physical A/V synchronization and physical iPad playback remain unvalidated.
Mac audio output and virtual input are covered in the native-audio follow-up below.

## Original-bitstream HEVC / AV1 adapter (2026-11-12)

The runtime now accepts HEVC and AV1 complete compressed samples through the
same asynchronous decoder, retained frame FDs and native GPU plane imports as
H.264. It requires a hardware VideoToolbox session and checks the actual
`UsingHardwareAcceleratedVideoDecoder` property; it never silently substitutes a
software VideoToolbox decoder. Compressed input is owned until native completion.
Callbacks signal the original eventfd; waiting releases the guest execution slot.
There is no new coordinator or video service thread.

The standard stateless VA structures omit original codec fields. Rather than
inventing those fields, a small
[FFmpeg 7.1.1 patch](https://github.com/kb5000/trames-runtime/blob/main/patches/ffmpeg/0001-vaapi-original-bitstream-v1.patch)
negotiates `trames_vaapi_bitstream_v1` with `vaGetLibFunc`. It forwards the original
escaped HEVC VPS/SPS/PPS and slice NALs, or AV1 `av1C`, sequence and complete frame
OBUs, including hidden reference pictures and all tile groups. Ordinary AV1
`show_existing_frame` reuses the retained output surface; KEY-reference display
also updates the native decoder's reference state. The unchanged H.264 adapter
remains available to ordinary libva clients. HEVC/AV1 profiles are exposed only
on an opted-in display and only when the host reports the corresponding hardware.

Build the driver with the existing `runtime/scripts/build-video-guest.sh`. On
native ARM64 Linux with libva/libdrm development headers, Bash, make, pkg-config,
patch, xz, curl and Python installed, build the paired FFmpeg overlay with:

```sh
cd runtime
TRAMES_FFMPEG_OUTPUT=/absolute/path/to/ffmpeg-output \
  bash scripts/build-video-ffmpeg.sh
```

The recipe pins the official source archive and patch hash, preserves the LGPL
license, and emits an install manifest with library hashes. Use a musl build for
Alpine and a glibc build for Debian. The Mac Firefox launcher accepts
`--video-driver /path/to/trames_drv_video.so --video-ffmpeg /path/to/ffmpeg-output/install`.
It verifies the protocol, patch and library hashes, then imports the overlay into
a private content-addressed directory without replacing distribution libraries.
Only that paired launch sets `TRAMES_VIDEO_BITSTREAM_V1=1` for Gecko's separate
VA profile probe; do not set it globally for unpatched VA clients. FFmpeg CLI
clients negotiate directly and do not need that environment switch. Select the
`av1` decoder when requesting AV1 hardware acceleration; `libdav1d` and `libaom`
are software decoder implementations.

On the local M6, full-frame hashes exactly match independent stock software
decoders for 60 HEVC 1080p30 frames, 240 HEVC 1080p60 frames, 240 AV1 1080p30
frames with hidden/reference reuse, and 60 AV1 720p30 frames with 2×2 tiles.
Tests cover invalid configuration, valid configuration with corrupt packets,
callback completion and context destruction. The existing H.264 multi-slice
fork/SCM_RIGHTS/EGL native-pixel import oracle still passes. The old constant
H.264 import fixture fails on both the published baseline and candidate; this
work does not expand the original H.264 reconstruction boundary.

Firefox 151 confirms actual RDD hardware decode for both codecs. The local
AV1 1080p30 interval accepts 600/600 frames; HEVC 1080p60 accepts 888/901 frames
over repeated four-second loops. These are browser counters, not physical scanout
measurements. The requested Bilibili AV1 page initially accepts about 24 fps after
reference reuse; final installed Dev intervals reach 28.99–29.77 fps, dropping
20 and 4 frames respectively out of 600. Both are visible, focused and buffered;
startup and page-state variation remain. Hardware decoding does not eliminate
the remaining complex-page execution and presentation overhead. Filtered
[Mac evidence](https://github.com/kb5000/trames-mac/blob/main/docs/validation/mac-bilibili-BV1FRHV68EUX.json)
includes independent native VideoToolbox throughput controls and their differing
FFmpeg versions. ARM64 iPad FFI compilation passes; no iPad hardware result is
claimed. Hardware encoding needs a separate input-surface, compressed-output and
rate-control interface and is deferred.

## Architecture

```text
Linux Firefox / FFmpeg / other VA-API clients
  → distribution libva + trames_drv_video.so (guest Linux ELF)
  → existing NativeCall transport
  → shared runtime video backend
  → VideoToolbox → CVPixelBuffer / IOSurface
  → CVMetalTextureCache → native Metal plane textures
  → guest frame FDs → EGL image import → Mesa Zink → Vulkan / Metal
  → existing Wayland window presentation
```

Keep the libva frontend and its driver ABI. Implement `trames_drv_video.so`
against pinned libva headers, initially selecting it with
`LIBVA_DRIVER_NAME=trames`. A library override alone is insufficient:
`vaGetDisplayDRM()` checks the FD's DRM node type, and Firefox opens a render
node through `DMABufDevice` before initializing VA-API. Provide a Trames virtual
render-device identity and the device/GBM discovery operations actually needed
by these clients. This is a user-kernel ABI adapter; it does not require a
Linux kernel, KMS implementation, or hardware DRM command submission.

Put the guest driver, codec adaptation and a new `trames-video` backend in
`trames-runtime`. Use a small shared frame-resource interface for VFS/video/
Vulkan interoperation; keep Apple types behind the Apple backend. The Mac and
iPad products only select capabilities and link the native frameworks. A fourth
repository or a separate video service process is unnecessary.

## Two integration gaps

### Compressed input adaptation

VA-API VLD is a parsed-picture interface. For example, FFmpeg's H.264 VA backend
submits picture parameters, quantization matrices, slice parameters and slice
data. Its `start_frame` does not submit the original SPS/PPS NAL units.
VideoToolbox's H.264 path constructs a CoreMedia format description from
parameter sets and submits complete compressed samples. A direct dispatch of
the VA vtable to VideoToolbox cannot bridge this difference.

Start with progressive H.264 8-bit 4:2:0. Reconstruct the decode-essential
SPS/PPS, preserve or adapt the slice headers as needed, and form length-prefixed
access units with correct RBSP escaping. Infer parameter-set IDs and any
recoverable defaults from slice data and effective VA parameters. In particular,
the original PPS default reference counts are absent from the picture struct;
setting them unconditionally to zero misinterprets some P/B slices. VA metadata
also omits some original SPS information, including POC type-1 cycle offsets.
Unsupported cases need an honest capability/error boundary, rather than invented
headers. Test ordinary Firefox/FFmpeg decode order, B frames, IDR/seek, parameter
set changes and reference reuse before expanding the advertised profile.

VideoToolbox owns its decoder reference state; VA-API exposes explicit reference
surfaces. Arbitrary reference-picture injection or a decoder resumed in the
middle of a GOP is not automatically equivalent. HEVC VPS/SPS/PPS and AV1
sequence/frame/OBU adaptation require separate proofs. Existing upstream codec
parsers/writers may help, but they do not recover fields the client never sends.
A targeted FFmpeg extension preserving original parameter sets is a possible
compatibility escape hatch if reconstruction blocks real streams; it is not a
prerequisite for every Linux client.

Own the small compressed input until the asynchronous decode finishes. One
bounded compressed-data copy avoids retaining mutable guest pointers after the
client can reuse its buffers. It does not require copying decoded pictures.

### Decoded-frame import into Firefox

The deployed Firefox 142 VA path calls:

1. `vaExportSurfaceHandle(DRM_PRIME_2, READ_ONLY | SEPARATE_LAYERS)`;
2. `vaSyncSurface()`;
3. duplicates and transfers the resulting plane FDs to the compositor;
4. creates per-plane `EGL_LINUX_DMA_BUF_EXT` images and GL textures.

The [native EGL/Wayland adapter](zink.md) and Vulkan bridge now provide this
resource import path through two small additional Mesa patches. Vulkan 1.2 YCbCr conversion alone does not
make a VA surface consumable by Firefox. Its existing YUV compositor can consume
separate R8/RG8 planes; P010 needs the corresponding 16-bit plane handling.

Represent exported objects as real guest FD resources retaining an immutable
decoded-frame version. `dup`, fork, `SCM_RIGHTS`, close and outstanding GPU uses
must retain the same allocation. A reused `VASurfaceID` points at its new frame;
old exported FDs keep their previous frame alive. Extend the existing VFS
open-description and FD-transfer ownership path, without exposing a host pointer,
host FD number or globally discoverable IOSurface ID as a guest handle.

Prefer retaining Mesa's standard external-resource import flow and translating
the guest FD to the owned native texture inside the bridge. The remaining work
includes external-memory format/property queries, allocation/bind semantics,
plane offsets/pitches, image views and synchronization. Determine the smallest
EGL/Zink adapter needed with an actual import fixture; merely enumerating
external-memory extensions is insufficient. The implemented export contains separate owned R8/GR88 plane views of ordinary
uncompressed NV12 CoreVideo storage, with offset zero, the actual row pitch and
`DRM_FORMAT_MOD_LINEAR`. Compressed native pixel formats are rejected. This
layout describes each retained plane view; it does not expose an IOSurface ID or
promise CPU `mmap`. CPU access goes through explicit VA image readback.

`VK_EXT_metal_objects` already provides a verified host-side texture import
primitive. Native `VK_EXT_external_memory_metal` is also enumerated by the local
driver, but its memory-allocation/bind route has not been tested by this probe.
Keep Metal handles internal to the backend in either case.

## Ownership, memory and scheduling

- Create the texture cache on the same `MTLDevice` as the active Vulkan device.
  A frame owns its `CVPixelBuffer`, plane texture wrappers and GPU leases until
  all consumers finish. Reuse the existing GPU image lease/wakeup principles;
  the current single-RGBA Wayland presenter is not itself an NV12 importer.
- Allocate decoded storage through VideoToolbox's output pool as frames are
  produced. VA surface IDs can initially be logical slots. Avoid a second pool
  of full-size pictures or a copy into each preallocated VA surface. Retained
  decoder references and in-flight compositor frames still consume real memory.
- Submit native operations on the guest's current host thread. Synchronize
  shared context/frame state at object scope. Keep independent sessions
  concurrent; add no global video lock or coordinator thread.
- VideoToolbox callbacks retain completed native frames, publish status, and
  wake existing resumable waits. They never retain a guest byte borrow or write
  guest memory from the callback thread. `vaQuerySurfaceStatus` is nonblocking;
  waits yield the execution slot. No polling service or busy loop is needed.
- Because Firefox exports **before** synchronizing, export itself may need to
  suspend until VideoToolbox has produced the frame. Complete both operations
  through the same frame-completion state; do not require a different client
  call order or copy into a temporary surface to satisfy it.
- CPU image access (`vaGetImage`/mapping) is an explicit readback path. The
  Firefox rendering path keeps NV12/P010 on native resources and performs color
  conversion in the GPU compositor. Preserve color range, primaries, transfer,
  chroma placement and crop metadata.

## Local native evidence

The [standalone probe](../../tests/fixtures/vaapi-videotoolbox-metal.mm) uses
public Apple APIs and the existing patched static MoltenVK archive. It encodes
one synthetic frame to obtain a valid compressed sample, requires hardware
H.264 decoding, checks the actual decoder property, creates both Metal plane
textures, imports/re-exports them as native Vulkan images, and runs a Metal
compute shader against the decoded planes.

Measured on Apple M6, macOS 27.0.1 (26A434), Xcode/SDK 27.2:

| Check | Result |
|---|---|
| `VTIsHardwareDecodeSupported`: H.264 / HEVC / AV1 / VP9 | true / true / true / false |
| Actual H.264 hardware decoder property and decode | pass |
| Decoded pixel format | NV12 video-range, 128 × 96 |
| Luma / chroma dimensions and byte pitches | 128 × 96 / 64 × 48; both pitches 128 |
| Both textures retain the decoded IOSurface and plane identity | pass |
| Vulkan import/re-export returns each exact same Metal texture | pass |
| GPU sample Y / Cb / Cr (expected 96 / 64 / 192) | 96 / 64 / 192 |
| CPU lock or copy of the decoded picture | none |
| iPadOS SDK compile, deployment target 17.4 | pass; syntax only |

This proves the native shared-plane primitive. It does **not** prove guest
DMA-BUF import, Vulkan sampling/layout transitions, full VA decoding, Firefox
playback, HEVC/AV1 decoding, profile/level coverage, throughput or iPad hardware.
Hardware queries are codec-level hints: advertise VA profiles only after actual
session creation and decoding validate the profile, depth and dimensions on the
current device. VP9 remains on the software path on this Mac unless a later
device/session probe establishes hardware support.

Reproduce with the existing prepared Vulkan headers and static MoltenVK archive:

```sh
export DEVELOPER_DIR=/Applications/Xcode.app/Contents/Developer
# Set these to the include directory and archive produced by prepare-vulkan.py.
export VULKAN_HEADERS=/absolute/path/to/Vulkan-Headers/include
export MVK_ARCHIVE=/absolute/path/to/libMoltenVK.a
xcrun --sdk macosx clang++ -std=c++17 -O2 -Werror \
  -Wno-missing-field-initializers -I "$VULKAN_HEADERS" \
  tests/fixtures/vaapi-videotoolbox-metal.mm "$MVK_ARCHIVE" \
  -framework VideoToolbox -framework CoreMedia -framework CoreVideo \
  -framework IOSurface -framework Metal -framework Foundation \
  -framework QuartzCore -framework CoreGraphics -framework MetalPerformanceShaders \
  -framework AppKit -framework IOKit -o /tmp/trames-vt-probe
/tmp/trames-vt-probe
```

The probe links MoltenVK directly, whose local build does not expose loader-only
portability enumeration. Its blocking completion calls are confined to this
standalone test, not the proposed guest execution path.

## Implementation order and acceptance

| Stage | Deliverable and acceptance |
|---|---|
| 1 | Shared video module + guest libva driver + minimal render-device discovery; `vainfo` reports only validated H.264 VLD capabilities. |
| 2 | H.264 adaptation and asynchronous lifecycle; stock FFmpeg VA decode matches software reference frames, including B frames, seek, resize and cancellation. Readback is acceptable as a correctness oracle. |
| 3 | Frame FD export/transfer and EGL/Zink import; two guest processes sample the same native planes with zero CPU frame copies, correct lifetime and nonblocking waits. |
| 4 | Stock Firefox RDD → compositor playback with its sandbox retained; verify actual hardware decoding, frame drops, A/V sync, seek, window resize and teardown. |
| 5 | HEVC Main/Main10 and AV1 where the device and adapter pass real samples; encoding and video processing follow separately. |

Measure decode submission/callback latency, native wait time, CPU frame-copy
bytes, end-to-end dropped frames, host CPU, and retained frame/pool memory.
A 4K60 NV12 full-frame copy alone moves about 746 MB/s of payload; compressed
input copies are far smaller. Avoiding raw-frame round trips is the main design
win. The implementation is opt-in. Unchanged acceptance results are reused; the
new video, FD, import and affected native graphics paths have separate checks.

## Implemented interface and local acceptance

`trames-video` and `trames-graphics` live in the common runtime. Module
`0x10007`, auxv `0x54520009`, uses the same validated NativeCall entry as the
other native modules. Compressed submissions run on the guest worker. A native
callback publishes an owned frame and signals the original retained eventfd;
guest `poll` yields its execution slot. There is no video service thread.

The virtual node `/dev/dri/renderD128` exists only when video is enabled. Its
canonical sysfs location is
`/sys/devices/platform/trames/drm/renderD128`; `/sys/dev/char` and `/sys/class/drm`
point there, and `device` points to the parent platform device. Complete directory
enumeration lets Firefox build its ordinary broker policy. An EGL native device
reports `EGL_EXT_device_drm_render_node` only when native image import is available
and the node exists. No PCI/KMS hardware identity is invented.

The Vulkan addition is an **import-only image profile**: dedicated 2D,
one-mip, one-layer, single-sample R8/RG8/RGBA8/BGRA8 planes, explicit LINEAR
modifier and retained guest FD ownership. Memory import consumes the supplied
FD on success. Ordinary Vulkan allocations retain their existing path. Generic
GPU memory FD export and external buffers are not supported. Mesa's adapter
honors import-only capabilities, does not request re-export for these imports,
and keeps explicit modifier/pitch metadata. This is not an external-memory
conformance claim.

| Check on Apple M6 / macOS 27.0.1 / SDK 27.2 | Result |
|---|---|
| Stock Ubuntu FFmpeg + libva H.264 Baseline/Main/High | 270/270 frame hashes match native Linux software decode |
| Cropped 126×94 and four-slice 320×240, B frames and custom quantization | 240/240 frame hashes match; 120 multislice frames repeated after handle reuse fix |
| Continuous read-only VA → DRM mapping | 120/120 frames exported; no decode error |
| Fork + SCM_RIGHTS + closed sender FDs + Vulkan import | Both planes match the VA readback oracle byte for byte |
| Same two-process import through EGL/GLES/Zink | Exact Y/UV pixels on MOLTENVK; no generic export extension advertised |
| Callback ownership, bad input and early decoder destruction | Every submitted owned job released |
| Shared native image FD lifetime | Worker creation, stat/seek, dup, SCM_RIGHTS, procfs and final import lease pass |
| VFS unit suite | 336 passed, 1 existing ignored; subsequent procfs/sysfs changes have targeted regressions |
| Shared runtime Linux and iPadOS compilation | Pass; physical iPad video not tested |
| Full shared Rust suites on macOS / ARM64 Linux | 862 passed, 12 ignored / 861 passed, 11 ignored |
| Interpreter process/scheduler conformance | 225 passed, 3 ignored |
| Strict shared workspace Clippy / repository boundaries | Pass on macOS and ARM64 Linux |
| Firefox 142 / stock FFmpeg 7 / normal RDD sandbox | VA-API selected, hardware-required decode, GPU picture shown; seek and loop work |
| Foreground 1280×720, 30 FPS, 10.034-second interval | 300 frame advances, 122 drops, 17.74 displayed FPS; performance work remains |

The GPU import fixtures use CPU readback only as an oracle. The production
export/import path retains the original native decoded allocation and does not
lock or copy decoded pixels. The small compressed access unit is copied into
owned submission memory once. VA handle slots are reused rather than growing a
metadata table on every decoded frame.

Build the guest driver on ARM64 Linux using
[`runtime/scripts/build-video-guest.sh`](../../runtime/scripts/build-video-guest.sh)
with libva development headers and a musl compiler. `TRAMES_VIDEO_CC=gcc` selects
the glibc variant. Select it with `LIBVA_DRIVER_NAME=trames` and
`LIBVA_DRIVERS_PATH` pointing at the library. The Mac CLI exposes `--video`;
its maintained `scripts/run-firefox-gpu.py --video-driver PATH` installs the
paired library into the selected guest graphics directory. Use the updated
paired EGL/Gallium build from `scripts/build-zink-gallium.py`, not a mixture of
old and new libraries. Browser hardware force preferences are unnecessary.

[`vaapi-native-import.c`](../../tests/fixtures/vaapi-native-import.c) is the
stock-API two-process pixel oracle. Compile on ARM64 Linux with `pkg-config`
`libavcodec libavformat libavutil libva-drm vulkan`; the `-DTEST_EGL` variant
links `egl glesv2` instead of Vulkan. Run the signed Mac guest via
[`vaapi-native.py`](../../tests/integration/vaapi-native.py), passing an isolated
store, the exact Linux libraries, driver, ICD and H.264 sample. Its JSON record
binds every imported artifact to a SHA-256. Do not use an active user's store for
this test. The local fixture does not prove audio output, A/V sync, 4K throughput,
web streaming, every H.264 stream or all advertised codec levels.

## Headless FFmpeg throughput (2026-10-31)

The performance fixture deliberately separates completed decode from export and
CPU readback. [`vaapi-ffmpeg-bench.c`](../../tests/fixtures/vaapi-ffmpeg-bench.c)
uses stock FFmpeg APIs, one codec thread and required hardware output. `sync`
waits for every returned VA surface; `export` exports before sync and closes the
native plane FDs; `download` calls `av_hwframe_transfer_data`. No frame is counted
before its selected operation completes. EOF is flushed and the exact expected
frame count is required. There is no renderer, frame pacing, hashing, audio or
network in the timed interval. The explicit download path is separate from the
production native-image export/import path.

The first run exposed a shared scheduler bug: an eventfd with a retained native
producer was not counted as an external event source. An idle joined scheduler
could return `Deadlock` before VideoToolbox completed, including with multiple
configured execution slots. Native producer leases now retain the original
event object and mark its live descriptors as external. Completion also unparks
the existing joined owner; a saved wake token covers completion immediately
before idle parking. Ordinary worker readiness subscriptions remain in use.
There is no new service thread or polling bus. Tests cover producer lifetime,
dup/close/FD reuse, wake-before-park and an actual joined guest read completed by
a native producer. A plain guest eventfd still permits deadlock detection.

Measurements use Apple M6, 12 host CPUs, 24 GiB RAM, macOS 27.0.1, SDK 27.2;
the guest has four execution slots and host-managed memory. Both builds use
FFmpeg 6.1.1: the Ubuntu ARM64 package and an unmodified native macOS source
build. The Linux build VM is stopped and all runs are sequential, without
metrics instrumentation or concurrent compilation. Reported throughput is the
median of three complete runs, excluding initial stream discovery/device
creation and final context teardown. The harness also records whole-process
wall time and host child user/system CPU time.

| Resolution | Guest completed decode FPS | Guest decode + export FPS | Guest decode + CPU download FPS | Native decode FPS | Native decode + CPU download FPS |
|---|---:|---:|---:|---:|---:|
| 1280×720 | 4,522.6 | 4,420.3 | 552.1 | 707.0 | 744.3 |
| 1920×1080 | 2,283.1 | 2,261.7 | 275.9 | 433.2 | 374.1 |
| 3840×2160 | 666.7 | 671.1 | 52.4 | 219.7 | 170.8 |

Export is within about 0–2.3% of completed decode throughput; the slight 4K
increase is run variation. CPU download changes the cost substantially:
its measured consumer time is 1.696 / 3.480 / 18.748 ms per guest frame,
versus 0.098 / 0.339 / 1.458 ms natively. Guest total host CPU cost for download
is 1.748 / 3.468 / 18.608 ms per frame, compared with 0.104 / 0.176 / 0.554 ms
for decode plus native FD export. Thus raw-frame download is an expensive path,
and this implementation cannot sustain 4K60 CPU download on the tested input.
Completed native-frame export has ample throughput on these inputs. These
results do not identify the cause of the earlier browser frame drops.

The [filtered machine-readable record](../validation/vaapi-ffmpeg.json) retains
artifact hashes, frame counts, median timings and the observed FPS ranges.
Detailed logs and individual process outputs remain in ignored build output.

The native FFmpeg VideoToolbox implementation synchronizes after every decode;
the VA path can pipeline asynchronous submissions. Both require actual hardware
H.264 decoding. Their throughput therefore compares these two application paths,
not an isolated adapter overhead or a hardware-speed advantage. Native `sync`
already includes completion in the FFmpeg decode API; its near-zero consumer
time does not mean hardware decode is free.

Inputs are continuous Annex-B H.264 with complete IDR/SPS/PPS segments: 10,800
720p frames, 7,200 1080p frames and 2,700 4K frames. The 1080p/4K streams use
`testsrc2`, x264 High, 8-bit 4:2:0, CRF 23, preset fast, GOP 60, three B frames
and four references. The 720p stream reuses the earlier video acceptance input.
The first 30 frames at each resolution match Linux software NV12 frame hashes
exactly. Repeated compressed segments measure steady throughput, not a diverse
codec corpus. One separate FFmpeg CLI `-stream_loop` attempt failed during filter
reinitialization at the MP4 loop boundary; that remains undiagnosed and its
partial runs are excluded from these results. Continuous raw input avoids that
boundary without treating a failed run as a successful benchmark.

Stock FFmpeg CLI `hwmap=mode=read,format=drm_prime` also exports all
10,800 / 7,200 / 2,700 continuous frames successfully. Three-run 720p checks
with one and twelve execution slots finish every frame: completed decode is
4,533 / 4,527 FPS and export is 4,436 / 4,418 FPS respectively. Additional CPU
slots do not improve this single codec-thread workload. No GUI is created.

Scoped acceptance passes the three native-event VFS tests on macOS and ARM64
Linux, 225 existing interpreter scheduler tests plus the new joined native-event
regression, strict affected-package Clippy on both hosts, interpreter conformance
Clippy, iPad ARM64 FFI compilation, and the signed normal Mac bundle build.
Unchanged graphics acceptance is reused; no IPA or physical iPad video test is
performed in this pass.

Compile the fixture on ARM64 Linux with
`cc -O2 vaapi-ffmpeg-bench.c $(pkg-config --cflags --libs libavformat libavcodec libavutil libva)`.
The macOS variant uses the same source, FFmpeg static libraries and their Apple
framework dependencies. Enable H.264 VideoToolbox, the H.264 parser, MOV/H.264
demuxers and file protocol in the native build; preserve ARM64 assembly. The
[official FFmpeg 6.1.1 source archive](https://ffmpeg.org/releases/ffmpeg-6.1.1.tar.xz)
has SHA-256 `8684f4b00f94b85461884c3719382f1261f0d9eb3d59640a1f4ac0873616f968`.

[`vaapi-ffmpeg-perf.py`](../../tests/integration/vaapi-ffmpeg-perf.py) imports
content-addressed artifacts before timing, saves every raw run, checks exit and
frame counts, and writes hashes plus median timings. Use `--native-program` for
the paired macOS fixture, `--cpus 4 --repeats 3` for the comparison, and an
isolated prepared guest store. `--libraries` and `--rootfs` can provide a fresh
guest environment. `--modes sync export --cpus 1 12` tests scheduling without
unnecessary repeated CPU readback. Browser playback, GPU composition, audio and
end-to-end latency are outside this benchmark; Firefox is not relaunched here.

## End-to-end Firefox playback (2026-11-01)

Headless decoder/export throughput does not measure browser presentation. In a
foreground 720p H.264 test, Firefox supplied video at source rate but lost frames
in the compositor. A marker-only capture showed a 31 ms median vsync-notification
gap and a 104 ms 95th percentile. Full all-thread stack sampling disturbed one
run enough to end on software decoding; that run is excluded from the playback
comparison.

Two shared-runtime mechanisms were corrected:

- Seccomp no longer excludes every shared dup/close operation from the worker
  path. Allowed descriptor operations use the existing files_struct membership;
  an owner-only resource or denial carries its exact entry decision back to the
  owner without re-evaluating the filter. Four-worker and denial/deferral tests
  cover the change. This removes avoidable checkpoints but did not by itself
  improve this fixture's FPS.
- Requested Wayland frame callbacks follow the advertised 60 Hz clock even if
  the requesting surface has not replaced its own pixels. The old 100 ms
  callback-only interval restricted browser synchronization surfaces, including
  clients rendering new content into a child surface. Producer-dependent extra
  cooldown is also removed. Callback-only commits still do not publish/repaint
  an unchanged scene; unpresented commits still cannot be acknowledged, and
  clients without requests schedule no callbacks.

The same original 720p video track was repeated ten times through AVFoundation
passthrough, without re-encoding, with audio omitted. Each sequential run used a
fresh profile, metadata readiness plus ten seconds of startup settling, five
seconds of playback warmup, and a 20-second unprofiled interval. Guest execution
used 12 slots and host-managed memory. The build VM was stopped.

| Path | Accepted video FPS | Dropped / total frames in interval |
| --- | ---: | ---: |
| Trames before | 18.16 | 235 / 599 |
| Trames with both fixes | 29.61 | 4 / 597 |
| Native macOS Firefox | 29.83 | 0 / 597 |

These are individual source-paced samples, not maximum-throughput scores or
statistical medians. Guest Firefox is 142.0; native Firefox is 159.0, so this is
an application-path reference, not an isolated runtime-overhead measurement.
Playback-quality counters measure accepted/dropped frames, not physical scanout.
The final guest sample retained hardware decoding and reported no buffering. An additional
installed Trames Dev smoke test accepted 29.26 FPS (9/745 dropped) over
25.151 seconds, with click and typing succeeding after timing.
Removing only the adaptive cooldown gave 15.28 FPS; removing callback-only
downclocking produced the substantial gain.

Cold autoplay remains a separate limitation: the original 12-second AAC fixture
still enters Firefox's 15-second BUFFERING state during startup. A 25-second
window straddling that pause accepted 10.58 FPS (272 total, 7 dropped), then
resumed normal playback and looping. The steady-state result above does not
claim to fix that cold-start stall. A WebDriver click and text-input check after
that window completed correctly in 403 ms for the combined controller roundtrip.
Audio output/A-V synchronization, 4K browser playback and physical iPad
presentation are not established by this pass; the shared iPad compile passes.

Use the Mac product's `scripts/benchmark-firefox-video.py --port PORT --output
result.json --seconds 20 --check-input` with a separate visible Marionette test
profile already displaying the video. It reads playback-quality counters and
frame-callback notifications without changing decoder, graphics or sandbox
preferences. The input check runs after timing. [Machine-readable samples and
fixture hashes](../validation/firefox-video-playback.json) retain intermediate
controls as well as the native comparison.

## 1080p60 and worker sleep deadlines (2026-11-01)

A new 45-second 1920×1080/60 Hz H.264 High/AAC fixture uses a continuous encode,
not repeated remuxed segments. A foreground marker capture identified 21,396
whole-cohort returns for `nanosleep`, in addition to Unix message transfers.
Such short graphics-library backoffs must release the caller's execution slot
without pausing its peers. The shared runtime now parks nanosleep/clock_nanosleep
on the existing deadline queue. Original deadlines and remaining pointers survive
checkpoints; normal expiry returns zero, and the existing owner handles signal
interruption. Seccomp still evaluates each accepted/denied entry once. No extra
service thread, polling loop or guest preference is introduced.

Wayland callback deadlines also retain their fixed phase rather than accumulating
one dispatch delay per tick. The phase-only change did not improve this sample
by itself. Requested callbacks still require presented content, and missed ticks
are skipped without replaying a backlog.

| Path | Accepted video FPS | Dropped / total |
| --- | ---: | ---: |
| Previous runtime | 54.82 | 127 / 1465 |
| Fixed phase only | 54.26 | 140 / 1466 |
| Worker sleeps + fixed phase | 58.89 | 27 / 1467 |
| Native Firefox 159 | 59.97 | 0 / 1456 |

These sequential individual samples exclude five seconds of playback warmup
following metadata readiness and ten seconds of settling. The measured windows
are about 24 seconds (30 seconds sampled once per second). No profiler/build VM
ran during timing. The final guest retained hardware decoding, with zero decoder
drops and no buffering during this interval. Firefox versions differ (142 vs
159); window geometry is not pixel-identical. This is not a maximum-throughput
or physical-scanout score. See the paired counters and fixture SHA-256 in
[the machine-readable record](../validation/firefox-video-playback.json).

The original short AAC cold-autoplay case still stalls; neither a larger decoded
queue nor the scheduling fix removes it. Its buffered range ends at 12.000 s,
while Firefox 142 reports 12.021333 s duration. At the last five-second threshold,
a transient decoded-queue shortage enters BUFFERING even though the entire file
is local. The queue can refill with already late frames and wait for the fixed
15-second timeout repeatedly. JavaScript remains responsive. A similar duration/
buffered-end failure mode is described in [Mozilla bug 2083351](https://bugzilla.mozilla.org/show_bug.cgi?id=2083351),
but that later warm-seek regression is not identified as the exact Firefox 142
cause. This limitation requires a media-state-machine/client fix; changing guest
clocks or suppressing frame counters would not repair it. The 1080p interval above
does not cover its final five seconds or looping, and does not claim to fix it.

### Newer client and cold-autoplay acceptance

The isolated test environment was upgraded using official Alpine v3.23 signed
packages to Firefox 151.0.3. It reports 12.000 s for the original short clip,
matching the buffered end, and no longer triggers the old 15-second buffer wait.
The previous Firefox 142 environment was preserved for the paired timing above.

Two independent startup issues were then addressed:

- Mesa 25.1.9 wrote `zink_screen.total_mem` once and never read it. Its failed
  physical-memory query nevertheless rejected a working Vulkan device inside
  the RDD sandbox. [Patch 0005](../../patches/mesa/0005-drop-unused-host-memory-probe.patch)
  removes the dead query and field; existing resource/cache budgets continue to
  use Vulkan heaps. The new pixel fixture makes sysconf unavailable and denies
  sysinfo with seccomp: the old library fails EGL initialization, and the patched
  library matched the hardware frame's Y/UV pixels. No browser sandbox change
  is required.
- Firefox 151 counts wall-clock decode delays and may switch to software after
  cold graphics initialization/scheduling stalls. On this verified-fast
  VideoToolbox backend that switch is counterproductive. The Mac launcher seeds
  `media.ffmpeg.vaapi.enabled=true` and
  `media.ffmpeg.disable-software-fallback=true` for native-video profiles only
  when no explicit preference exists. `--video-policy browser` preserves browser
  defaults. The [upstream slow-decode branch](https://github.com/mozilla-firefox/firefox/blob/FIREFOX_151_0_3_RELEASE/dom/media/platforms/ffmpeg/FFmpegVideoDecoder.cpp#L1334)
  is separate from actual VA decode/image-import failures, which still use the
  ordinary error/recovery path. This keeps the hardware choice; it does not
  suppress frame-drop counters or reduce video quality.

A fresh cold short-clip run with that policy retained hardware decoding, crossed
two loop transitions and reported zero buffering time. Initial startup still
lost frames; this functional check is not a zero-latency or zero-drop claim.

A separate 55.018-second 1080p60 run, including the clip end and looping, accepted
56.05 FPS (218/3302 dropped), retained hardware decoding and reported zero
buffering time. Click and typing passed after timing (576 ms combined controller
roundtrip). This longer/newer-client result has a different scope from the
58.89 FPS paired steady interval above. Neither is a sustained zero-drop 60 FPS
acceptance. Audio was muted; output/A-V synchronization remains outside scope.

## Primary sources

- [Apple VideoToolbox hardware decoder query](https://developer.apple.com/documentation/videotoolbox/vtishardwaredecodesupported(_:)) and [actual hardware decoder property](https://developer.apple.com/documentation/videotoolbox/kvtdecompressionpropertykey_usinghardwareacceleratedvideodecoder).
- [Apple CoreVideo Metal texture cache](https://developer.apple.com/documentation/CoreVideo/cvmetaltexturecache-q3j).
- [libva 2.22 driver ABI](https://github.com/intel/libva/blob/2.22.0/va/va_backend.h), [DRM display creation](https://github.com/intel/libva/blob/2.22.0/va/drm/va_drm.c), and [PRIME surface descriptor](https://github.com/intel/libva/blob/2.22.0/va/va_drmcommon.h).
- [FFmpeg 7.1 VA H.264 input](https://github.com/FFmpeg/FFmpeg/blob/n7.1/libavcodec/vaapi_h264.c) and [VideoToolbox input](https://github.com/FFmpeg/FFmpeg/blob/n7.1/libavcodec/videotoolbox.c).
- [Firefox 142 VA export/sync](https://github.com/mozilla-firefox/firefox/blob/FIREFOX_142_0_RELEASE/dom/media/platforms/ffmpeg/FFmpegVideoDecoder.cpp), [display initialization](https://github.com/mozilla-firefox/firefox/blob/FIREFOX_142_0_RELEASE/dom/media/platforms/ffmpeg/VALibWrapper.cpp), and [plane import](https://github.com/mozilla-firefox/firefox/blob/FIREFOX_142_0_RELEASE/widget/gtk/DMABufSurface.cpp).
- [Khronos native Metal texture import](https://docs.vulkan.org/refpages/latest/refpages/source/VkImportMetalTextureInfoEXT.html) and [Metal external-memory import](https://docs.vulkan.org/refpages/latest/refpages/source/VkImportMemoryMetalHandleInfoEXT.html).

## Native audio follow-up (2026-11-01)

Standard ALSA output and capture now connect to the shared native audio backend.
The final Mac output path uses AVAudioSourceNode device pull, rather than
prefilling Audio Queue buffers with silence. This removes the persistent
discontinuities heard in the first integration. Capture is accepted with the
system virtual microphone, as requested; physical USB microphone quality is not
part of this acceptance.

The ALSA fixture plays 144,000 stereo/48 kHz frames in 3.067 s. Its continuous
clock check observes at most 517 frames of progress over a maximum 10.767 ms
poll gap; simultaneous capture reads 48,000 mono frames in 1.053 s. Original
FFmpeg S16/44.1 kHz output also completes. Firefox 151 plays with the normal
sandbox and native audio; the owner confirmed the persistent crackle is gone.

This does not close browser smoothness acceptance. A 25-second foreground
1080p60 audio/video interval accepts 53.62 FPS, but video callbacks have a
97.3 ms p99 gap and a 1.565 s maximum gap. The media clock advances at 0.958
of wall time. Four webpage volume changes complete in 12.7–40.6 ms controller
roundtrips; their observed two-second window has no callback gap over 100 ms.
Long stalls elsewhere still need correlation with scheduling/rendering data.

[Shared architecture and setup](https://github.com/kb5000/trames-runtime/blob/main/docs/architecture/audio.md)
and [machine-readable acceptance](../validation/native-audio.json) describe the
capability opt-ins, callback ownership, artifacts and limitations.

### Elapsed-time stall attribution

Starting the clip at 18 seconds crossed the original 22-second media position
without the severe stall. Later isolated runs aligned frame gaps with Firefox's
Glean local database replacement burst, rather than with a particular video
sample. The private namespace manifest cost remains a separate runtime storage
limitation. The Mac launcher now accepts a dedicated `--host-profile` directory,
using the existing persistent host-directory backend without disabling Glean or
profile saving. Paired counters and methodology are in
[the audio validation record](../validation/native-audio.json). Early playback
still has a shorter cold-start gap; this does not claim sustained zero-drop 60 FPS
or physical A/V synchronization acceptance.
