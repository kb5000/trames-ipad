# Vulkan guest recording batches and iPad update — 2026-11-28

This delivery accumulates recording commands **before HLT**, on the guest
command-buffer object. A synchronous publication carries up to 64 commands and
16 KiB of owned parameter bytes. End/begin/reset/free and affected pool lifetime
boundaries publish the remainder. An oversized input uses the original immediate
path after its predecessors; allocation failure and older hosts also retain the
original path. There is no new timer, service thread or batching UI control.
See the [architecture](../architecture/vulkan-compute.md#command-recording-before-hlt-2026-11-28)
and [shared backend validation](../../runtime/docs/validation/vulkan-command-batch-20261128.md).

Paired runtime: `7309727cf80298e80a7ae87e088e4c1585658f60`.
Paired CLI: `978a7151472299ca9793ba27954df1b3d18d1a1f`.
The final runtime revision only reorders module declarations for rustfmt after
feature validation at `8842c81`; the validated implementation is unchanged.
The original published JIT/AES changes through `9a69793` are retained by
fast-forward. The iPad CPU continues to use its existing interpreter backend.

## Validation

- Actual ICD with ASan/UBSan on Mac: 200 draws need four publications instead
  of 200, plus the unchanged end call. Delayed nested input lifetime, arena/full
  and oversized boundaries, thread handoff, pool isolation, custom allocation,
  scratch allocation failure and failed-publication non-replay pass.
- The same Linux C test passes with UBSan. ASan in this Linux VM fails during
  sanitizer initialization before the fixture; that limitation is recorded in
  the shared validation and is not counted as a pass.
- The existing descriptor batch regression still passes (72 writes: 72 → 3).
- Original native Metal and Linux Lavapipe both execute the new backend packet
  regression, including actual buffer readback and fault/non-replay checks.
- Final shared Mac Vulkan unit suite: 67 pass, 16 ignored; Linux: 68 pass,
  2 ignored. The new hardware test is explicitly exercised separately. Distro
  login/default tests: 47 pass on each host. CLI common contract tests: 4 pass.
  Mac/Linux Vulkan Clippy and iOS target compilation pass.
  Both paired Mac and Linux product consumers also pass `cargo check`; these
  are compile checks, not HVF/KVM hardware acceptance.
- Musl, Debian glibc and Android Bionic guest ICD/HAL builds pass. Wire revision
  22 manifests are bound to the current sources and recipe. The existing paired
  iOS Mesa, video, libc and native MoltenVK artifacts pass source/hash checks;
  their unchanged binaries are reused.

The real guest regression compares the previous immediate ICD and new packet
ICD on the same original Metal driver, at both one and four guest CPUs. All ten
pairs produce identical compute output or exact image bytes. Whole-run worker
GPU calls (including unchanged setup/query/submit/synchronization calls):

| Workload | Immediate | Packet |
| --- | ---: | ---: |
| Compute | 183 | 159 |
| Persistent coherent compute | 182 | 158 |
| Descriptor-template compute | 185 | 161 |
| Dynamic graphics | 149 | 104 |
| Clear-attachment graphics | 146 | 119 |

The count result holds at both CPU counts. These are transport call counts,
not end-to-end application speedups or Vulkan conformance claims. The Mac and
Linux products consume the same shared backend and guest ICD; installing an old
ICD keeps immediate recording until that guest library is updated. Product
HVF/KVM display acceptance is outside this scoped transport validation.

Reproduce the real GPU comparison after building the current fixtures and
retaining the prior immediate ICD:

```sh
python3 tests/integration/vulkan-command-batch.py
python3 tests/integration/vulkan-command-batch-gpu.py \
  --baseline target/vulkan-command-batch/baseline/libtrames_vulkan_icd.so \
  --inputs target/local-ci/io/vulkan-command-batch-guest \
  --loader target/local-ci/io/vaapi-build/alpine-libs/usr/lib/libvulkan.so.1 \
  --output target/vulkan-command-batch/gpu-final
```

## Combined interface and login changes

At the owner's request, this update also includes the adjacent environment
editor change: clear an explicit startup directory back to the login user's
home, and clear that directory when selecting “Follow Login User”. The existing
translations are paired. CLI/session startup resolves the selected user's
login shell/home and preserves explicit overrides. Both frontend and shared
runtime changes are committed with their paired pins. This interface change is
separate from the internal Vulkan batching policy.

## Device delivery

The source-checked iOS Rust archive and full signed Xcode Release build pass.
The stable application is updated in place on the connected iPad Air M3;
installation and launch are recorded in the [delivery receipt](vulkan-command-batch-20261128.json).
The existing stable bundle identifier/version are retained, and application
data is preserved. This verifies building, signing, installation and launch;
the GPU output measurements above were taken on the Mac host, not on the iPad.
