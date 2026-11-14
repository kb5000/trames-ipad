# Public Metal equivalents for Vulkan features

Trames uses public Apple APIs on both macOS and iPadOS. Missing Metal operations
may be implemented by GPU shader transformations and existing resource machinery.
Do not enable `MVK_USE_METAL_PRIVATE_API` or advertise a Vulkan capability merely
to silence a Zink warning. An equivalent must implement the observable Vulkan
semantics before the shared profile exposes it.

This registry separates shipped implementations from experiments. Keep it next to
the native patches so a future driver update can replace each workaround without
leaving duplicate shader work enabled. The shared guest ICD and runtime must use
the same capability gates on Mac and iPad; device/OS limits still come from the
actual native driver.

## Status as of 2026-11-14

| Feature | Current path | Mac / iPad coverage | Incremental cost | Replacement condition |
| --- | --- | --- | --- | --- |
| Independent depth clip and depth clamp (`VK_EXT_depth_clip_enable`) | Implemented by the public-API MoltenVK/SPIRV-Cross backport and shared wire-21 bridge; ordinary complementary clip/clamp settings retain native Metal behavior | 432 color/depth cases passed on Mac native Metal, 4-CPU interpreter and Mac HVF guests, Linux native reference, and physical iPad M3 | Extra shader state/arithmetic when a pipeline can use combinations Metal cannot represent directly, including dynamic state; fragment depth output may affect early depth testing | A pinned upstream release includes both the driver and compiler implementation, cache serialization and our rendering regressions pass |
| Color logic operations (`logicOp`) | Not exposed by the public-API driver; framebuffer-fetch experiment only | Mac RGBA8 UNORM experiment passed all 16 operations, including two overlapping primitives; iPad and remaining format/sample coverage pending | Fetch destination color and execute integer operations in the fragment shader; attachment format handling and ordering must remain correct | Public native Metal logic operations, or a complete upstream public-API implementation, pass the same format, sample and overlap tests |
| Arbitrary sampler border colors (`VK_EXT_custom_border_color`) | Not implemented or exposed; public Metal has three fixed border colors | Same missing capability on both products | An equivalent must preserve filtering weights; any mask texture, metadata or extra sample must be allocated/used only where needed | Public native custom border colors, or an upstream equivalent, pass filtered, integer, comparison and gather tests |

“Shared source” does not imply new physical-device acceptance. In particular,
the Mac path is not exempt from the public Metal restrictions affecting iPad.
Depth clipping now has focused acceptance on both platforms. The earlier Firefox/
WebRender acceptance does not establish logic-operation or custom-border support.

## Depth clip implementation

The backport combines upstream MoltenVK commit
[`a81425051d171ddd4fa683398b78e5c752738d59`](https://github.com/KhronosGroup/MoltenVK/commit/a81425051d171ddd4fa683398b78e5c752738d59)
with SPIRV-Cross commit
[`0706157ef54d37a653681d6a0ce6af05350b0e3f`](https://github.com/KhronosGroup/SPIRV-Cross/commit/0706157ef54d37a653681d6a0ce6af05350b0e3f).
It lives in
[`0011-public-depth-clip-enable.patch`](../../patches/moltenvk/0011-public-depth-clip-enable.patch)
and uses the normal command encoder and implicit shader buffers, without a new
coordinator or GPU service thread.

Metal provides one clip-or-clamp switch. Vulkan independently controls clipping
against the clip volume and clamping to the viewport depth range. The equivalent
moves the viewport depth transform into the vertex shader when both are disabled
and clamps fragment depth when both are enabled. Ordinary complementary settings
continue to select the native Metal operation. Dynamic state and reversed depth
ranges must use the same shader state; an absent extension keeps Vulkan's original
clip behavior rather than inventing a new default.

Trames additionally serializes the new compiler options and changes the pipeline
cache ABI. The regression in `tests/fixtures/vulkan-depth-clip.c`, run through
`tests/integration/vulkan-depth-clip.py`, checks 432 renders including cache
reload. Physical iPad acceptance uses the same guest ELF and shaders. Before
removing this patch, retain coverage of static and dynamic combinations,
reversed viewports, explicit shader depth, early fragment tests, pipeline changes
and cache reload. Run the guest bridge and original native-driver oracle; a
capability query alone is insufficient.

## Logic-operation experiment and remaining requirements

Public Metal fragment framebuffer fetch can read the destination attachment.
The prototype in `tests/fixtures/vulkan-logic-fetch-metal.mm` compares all 16 Vulkan bitwise operations against a CPU
oracle and draws overlapping primitives to check that the second primitive sees
the first result. This is feasibility evidence, not a complete Vulkan feature.

Before exposing `logicOp`, cover signed/unsigned integer and normalized formats,
component write masks, multiple attachments, MSAA and primitive ordering. Float
and sRGB attachments do not receive bitwise operations under Vulkan, while
blending is disabled when logic operations are enabled. In particular, simply
converting an SNORM framebuffer-fetch float back to an integer loses the distinct
raw encodings of its most negative values. Do not generalize the RGBA8 prototype
to all formats or substitute approximate arithmetic.

The acceptance contract is the Vulkan
[logical operation specification](https://docs.vulkan.org/spec/latest/chapters/framebuffer.html#framebuffer-logicop).

## Custom border-color investigation

Replacing only samples whose coordinates lie outside the texture is incorrect:
bilinear, trilinear, anisotropic, comparison and gather operations can combine
inside texels with border texels. Any equivalent must retain the native filter's
per-tap contributions, Vulkan format conversion and component swizzles, including
nonuniform descriptor selection.

An all-ones companion texture sampled with a zero border can measure outside
filter weight, but its resource cost and comparison/integer behavior still need
validation. A view that swizzles its channels to one is not a zero-storage
substitute: the Mac experiment returned one outside the border as well. Sparse
aliasing is only a candidate optimization, not an implemented dependency.

Use the Vulkan
[custom border color contract](https://docs.vulkan.org/refpages/latest/refpages/source/VK_EXT_custom_border_color.html)
for the eventual sampler limits, feature gates and acceptance matrix.

## Replacement and performance rules

For each completed equivalent, record the owning patch, native capability gate,
minimum tested device/OS, pixel or semantic oracle, cache effects, and measured
overhead. Preserve a reproducible test when replacing it with an upstream/native
path. Select one path per operation; do not leave the equivalent active behind a
native implementation. Avoid adding per-call global checks or synchronization to
ordinary rendering that does not use the feature. Unsupported cases remain
unadvertised until implemented and tested.

## Acceptance and remaining baseline issue

The 2026-11-14 focused batch passed source/patch verification, public-only Mac
and iPad native compilation, 66 shared Vulkan unit tests, strict Vulkan lints,
artifact mismatch rejection, and the 432-case native/guest/device pixel oracles.
The test checks depth values as well as draw visibility; it includes missing
extension state (core defaults), normal/reversed depth ranges, explicit fragment
depth, early fragment tests, static/dynamic commands and pipeline-cache reload.
This is focused regression coverage, not a full Vulkan conformance claim.

The existing broader transform-feedback test also hit a native timestamp-copy
comparison failure with both the old and new Mac drivers. An isolated run that
skips only that independent timestamp assertion passed all 24 TF topology/index/
indirect cases, raster capture/replay, six restart cases, two zero-divisor cases
and the indirect bounds-error case. Production timestamp features were not
changed or disabled. Keep that baseline failure visible rather than describing
the broader combined test as wholly passing. Machine-readable evidence is in
[the iPad graphics validation record](../validation/ipad-graphics-video-20261114.json).
