// SPDX-License-Identifier: GPL-3.0-or-later
//! Trusted native UI ABI. Scene handles retain buffers independently of guest
//! descriptors and session teardown; the UI releases only after GPU completion.
use super::*;
use serde::Deserialize;
use trames_wayland::{Server, WindowMode, WindowSnapshot};
struct Frame {
    session: u64,
    server: Server,
    window: WindowSnapshot,
}
fn frames() -> &'static Mutex<BTreeMap<u64, Frame>> {
    static FRAMES: OnceLock<Mutex<BTreeMap<u64, Frame>>> = OnceLock::new();
    FRAMES.get_or_init(Mutex::default)
}
static NEXT_FRAME: AtomicU64 = AtomicU64::new(1);
fn controller(handle: u64) -> Result<Server, String> {
    session(handle)
        .map_err(str::to_owned)?
        .wayland
        .lock()
        .map_err(|_| "Wayland session lock unavailable".to_owned())?
        .clone()
        .ok_or_else(|| "Wayland session not ready".to_owned())
}
#[derive(Deserialize)]
#[serde(tag = "op", rename_all = "snake_case", deny_unknown_fields)]
enum Command {
    ClipboardChanged,
    OutputScale {
        window: u64,
        scale: i32,
    },
    Resize {
        window: u64,
        width: u32,
        height: u32,
    },
    Mode {
        window: u64,
        mode: String,
    },
    Close {
        window: u64,
    },
    Focus {
        window: Option<u64>,
    },
    Pointer {
        window: u64,
        x: f64,
        y: f64,
        time: u32,
    },
    Button {
        button: u32,
        pressed: bool,
        time: u32,
    },
    Scroll {
        horizontal: f64,
        vertical: f64,
        time: u32,
    },
    Key {
        key: u32,
        pressed: bool,
        modifiers: u32,
        time: u32,
    },
    Text {
        serial: u32,
        context: u64,
        epoch: u64,
        #[serde(default)]
        preedit: Option<String>,
        #[serde(default)]
        cursor_begin: i32,
        #[serde(default)]
        cursor_end: i32,
        #[serde(default)]
        commit: Option<String>,
        #[serde(default)]
        delete_before: u32,
        #[serde(default)]
        delete_after: u32,
    },
}
/// # Safety
/// Same caller-owned bounded writable buffer contract as session_status.
#[no_mangle]
pub unsafe extern "C" fn trames_session_wayland_status(
    handle: u64,
    pointer: *mut u8,
    capacity: u32,
) -> i32 {
    boundary(|| {
        let result = (|| {
            let session = session(handle).map_err(str::to_owned)?;
            let server = session
                .wayland
                .lock()
                .map_err(|_| "Wayland lock unavailable")?
                .clone();
            let snapshot = server.map(|s| s.snapshot()).unwrap_or_default();
            let windows:Vec<_>=snapshot.windows.iter().map(|w|serde_json::json!({"id":w.id,"connection":w.connection,"title":w.title,"app_id":w.app_id,"width":w.width,"height":w.height,"bounds":w.bounds(),"parent":w.parent,"min_size":[w.min_size.0,w.min_size.1],"max_size":[w.max_size.0,w.max_size.1],"revision":w.revision,"mode":match w.mode{WindowMode::Floating=>"floating",WindowMode::Minimized=>"minimized",WindowMode::Maximized=>"maximized"}})).collect();
            let bytes = serde_json::to_vec(
                &serde_json::json!({"revision":snapshot.revision,"windows":windows,"text":snapshot.text.map(|t|serde_json::json!({"version":t.version,"id":t.id,"epoch":t.epoch,"surface":t.surface,"window":t.window,"serial":t.serial,"text":t.text,"cursor":t.cursor,"anchor":t.anchor,"caret":t.caret,"hint":t.hint,"purpose":t.purpose}))}),
            )
            .map_err(|e| e.to_string())?;
            // SAFETY: forwarded embedding caller buffer contract.
            unsafe { copy(pointer, capacity, &bytes) }.map_err(str::to_owned)
        })();
        result.unwrap_or_else(error)
    })
}
/// # Safety
/// `pointer` references `length` immutable JSON bytes for the call, max 64KiB.
#[no_mangle]
pub unsafe extern "C" fn trames_session_wayland_command(
    handle: u64,
    pointer: *const u8,
    length: u32,
) -> i32 {
    boundary(|| {
        let result = (|| {
            // SAFETY: bounded caller-owned bytes, consumed before return.
            let command: Command =
                serde_json::from_slice(unsafe { bytes(pointer, length) }.map_err(str::to_owned)?)
                    .map_err(|e| e.to_string())?;
            let server = controller(handle)?;
            match command {
                Command::OutputScale { window, scale } => server.output_scale(window, scale),
                Command::ClipboardChanged => server.clipboard_changed(),
                Command::Resize {
                    window,
                    width,
                    height,
                } => {
                    if width == 0 || height == 0 || width > 4096 || height > 4096 {
                        return Err("invalid Wayland size".into());
                    }
                    server.resize(window, width, height)
                }
                Command::Mode { window, mode } => server.set_mode(
                    window,
                    match mode.as_str() {
                        "floating" => WindowMode::Floating,
                        "minimized" => WindowMode::Minimized,
                        "maximized" => WindowMode::Maximized,
                        _ => return Err("invalid window mode".into()),
                    },
                ),
                Command::Close { window } => server.close(window),
                Command::Focus { window } => {
                    server.input(trames_wayland::InputEvent::Focus { window })
                }
                Command::Pointer { window, x, y, time } => {
                    server.input(trames_wayland::InputEvent::Pointer { window, x, y, time })
                }
                Command::Button {
                    button,
                    pressed,
                    time,
                } => server.input(trames_wayland::InputEvent::Button {
                    button,
                    pressed,
                    time,
                }),
                Command::Scroll {
                    horizontal,
                    vertical,
                    time,
                } => server.input(trames_wayland::InputEvent::Scroll {
                    horizontal,
                    vertical,
                    time,
                }),
                Command::Key {
                    key,
                    pressed,
                    modifiers,
                    time,
                } => server.input(trames_wayland::InputEvent::Key {
                    key,
                    pressed,
                    modifiers,
                    time,
                }),
                Command::Text {
                    serial,
                    context,
                    epoch,
                    preedit,
                    cursor_begin,
                    cursor_end,
                    commit,
                    delete_before,
                    delete_after,
                } => server.edit_text(trames_wayland::TextEdit {
                    serial,
                    context,
                    epoch,
                    preedit: preedit.map(|s| (s, cursor_begin, cursor_end)),
                    commit,
                    delete_before,
                    delete_after,
                }),
            }
            .map_err(|e| e.to_string())?;
            Ok::<_, String>(0)
        })();
        result.unwrap_or_else(error)
    })
}
#[repr(C)]
#[derive(Clone, Copy, Default)]
pub struct FrameInfo {
    pub width: u32,
    pub height: u32,
    pub images: u32,
    pub reserved: u32,
    pub revision: u64,
}
#[repr(C)]
#[derive(Clone, Copy, Default)]
pub struct ImageInfo {
    /// 1=shared bytes; 2=retained native Metal texture (GPU WSI).
    pub kind: u64,
    pub address: u64,
    pub length: u64,
    pub offset: u64,
    pub width: u32,
    pub height: u32,
    pub stride: u32,
    pub alpha: u32,
    pub x: i32,
    pub y: i32,
    pub scale: i32,
    pub transform: u32,
}
#[repr(C)]
pub struct ImageGeometry {
    pub width: u32,
    pub height: u32,
    pub source: [f32; 4],
}
#[no_mangle]
pub extern "C" fn trames_session_wayland_frame_acquire(handle: u64, window: u64) -> u64 {
    catch_unwind(AssertUnwindSafe(|| {
        let server = controller(handle).ok()?;
        let window = server
            .snapshot()
            .windows
            .into_iter()
            .find(|w| w.id == window)?;
        let mut frames = frames().lock().ok()?;
        if frames.values().filter(|f| f.session == handle).count() >= 192 {
            error("too many native Wayland frames");
            return None;
        }
        let id = NEXT_FRAME.fetch_add(1, Ordering::Relaxed);
        frames.insert(
            id,
            Frame {
                session: handle,
                server,
                window,
            },
        );
        Some(id)
    }))
    .ok()
    .flatten()
    .unwrap_or(0)
}
/// # Safety
/// `output` points to one writable TramesWaylandFrameInfo for this call.
#[no_mangle]
pub unsafe extern "C" fn trames_wayland_frame_info(handle: u64, output: *mut FrameInfo) -> i32 {
    boundary(|| {
        if output.is_null() {
            return error("null frame info pointer");
        }
        let Ok(frames) = frames().lock() else {
            return error("frame registry unavailable");
        };
        let Some(frame) = frames.get(&handle) else {
            return error("invalid frame handle");
        };
        let info = FrameInfo {
            width: frame.window.width,
            height: frame.window.height,
            images: frame.window.scene.len() as u32,
            reserved: 0,
            revision: frame.window.revision,
        };
        // SAFETY: validated non-null caller-owned record, no reference escapes.
        unsafe { output.write_unaligned(info) };
        0
    })
}
/// # Safety
/// `output` points to one writable TramesWaylandImageInfo. The frame must remain
/// retained until GPU completion; addresses are native capabilities, not guest VA.
#[no_mangle]
pub unsafe extern "C" fn trames_wayland_frame_image(
    handle: u64,
    index: u32,
    output: *mut ImageInfo,
) -> i32 {
    // SAFETY: preserve the original fixed-size ABI; only v2 writes geometry.
    unsafe { frame_image(handle, index, output, std::ptr::null_mut()) }
}
/// # Safety
/// Both outputs are writable native records; frame retention matches v1.
#[no_mangle]
pub unsafe extern "C" fn trames_wayland_frame_image_v2(
    handle: u64,
    index: u32,
    output: *mut ImageInfo,
    geometry: *mut ImageGeometry,
) -> i32 {
    if geometry.is_null() {
        return error("null image geometry pointer");
    }
    // SAFETY: caller supplies both bounded records; no references escape.
    unsafe { frame_image(handle, index, output, geometry) }
}
unsafe fn frame_image(
    handle: u64,
    index: u32,
    output: *mut ImageInfo,
    geometry: *mut ImageGeometry,
) -> i32 {
    boundary(|| {
        if output.is_null() {
            return error("null image info pointer");
        }
        let Ok(frames) = frames().lock() else {
            return error("frame registry unavailable");
        };
        let Some(image) = frames
            .get(&handle)
            .and_then(|f| f.window.scene.get(index as usize))
        else {
            return error("invalid frame image");
        };
        let info = match &image.buffer {
            trames_wayland::ImageBuffer::Shm(buffer) => {
                let pixels = &buffer.pixels;
                let map = &pixels.mapping;
                ImageInfo {
                    kind: 1,
                    address: map.allocation_pointer() as u64,
                    length: map.allocation_len() as u64,
                    offset: map.allocation_offset() as u64,
                    width: pixels.width,
                    height: pixels.height,
                    stride: pixels.stride,
                    alpha: u32::from(pixels.alpha),
                    x: image.x,
                    y: image.y,
                    scale: image.scale,
                    transform: image.transform,
                }
            }
            trames_wayland::ImageBuffer::Gpu(gpu) => {
                let (width, height) = gpu.dimensions();
                ImageInfo {
                    kind: 2,
                    address: gpu.metal_texture() as u64,
                    alpha: u32::from(image.buffer.has_alpha()),
                    width,
                    height,
                    x: image.x,
                    y: image.y,
                    scale: image.scale,
                    transform: image.transform,
                    ..Default::default()
                }
            }
        };
        // SAFETY: same scoped caller-owned native record contract.
        unsafe { output.write_unaligned(info) };
        if !geometry.is_null() {
            // SAFETY: v2 supplies a separate fixed-size caller-owned record.
            unsafe {
                geometry.write_unaligned(ImageGeometry {
                    width: image.destination.0,
                    height: image.destination.1,
                    source: image.source,
                })
            };
        }
        0
    })
}
#[no_mangle]
pub extern "C" fn trames_wayland_frame_release(
    handle: u64,
    presented: u32,
    milliseconds: u32,
) -> i32 {
    boundary(|| {
        let frame = frames().lock().ok().and_then(|mut f| f.remove(&handle));
        let Some(frame) = frame else {
            return error("invalid frame handle");
        };
        if presented != 0 {
            let _ = frame
                .server
                .presented(frame.window.id, frame.window.revision, milliseconds);
        }
        drop(frame);
        0
    })
}

#[cfg(test)]
mod tests {
    use super::*;
    struct Image;
    // SAFETY: test-only capability has no native texture or outstanding GPU use.
    unsafe impl trames_wayland::GpuImage for Image {
        fn dimensions(&self) -> (u32, u32) {
            (100, 50)
        }
        fn metal_texture(&self) -> usize {
            0
        }
        fn ready(&self) -> std::io::Result<bool> {
            Ok(true)
        }
        fn has_alpha(&self) -> bool {
            true
        }
    }
    #[test]
    fn image_v1_keeps_its_record_size_and_v2_returns_viewport_geometry() {
        assert_eq!(std::mem::size_of::<ImageInfo>(), 64);
        assert_eq!(std::mem::size_of::<ImageGeometry>(), 24);
        let handle = NEXT_FRAME.fetch_add(1, Ordering::Relaxed);
        frames().lock().unwrap().insert(
            handle,
            Frame {
                session: 0,
                server: Server::new().unwrap(),
                window: WindowSnapshot {
                    id: 1,
                    connection: 1,
                    title: String::new(),
                    app_id: String::new(),
                    width: 30,
                    height: 10,
                    mode: WindowMode::Floating,
                    interaction: None,
                    parent: None,
                    min_size: (0, 0),
                    max_size: (0, 0),
                    revision: 1,
                    scene: vec![trames_wayland::SurfaceImage {
                        surface: 1,
                        x: 0,
                        y: 0,
                        scale: 1,
                        transform: 0,
                        destination: (30, 10),
                        source: [0.1, 0.2, 0.5, 0.25],
                        buffer: trames_wayland::ImageBuffer::Gpu(Arc::new(Image)),
                    }],
                },
            },
        );
        #[repr(C)]
        struct Guarded {
            image: ImageInfo,
            guard: u64,
        }
        let mut record = Guarded {
            image: Default::default(),
            guard: 0x1234_5678_90ab_cdef,
        };
        let mut geometry = ImageGeometry {
            width: 0,
            height: 0,
            source: [0.0; 4],
        };
        // SAFETY: both versions receive their exact caller-owned native records.
        unsafe {
            assert_eq!(trames_wayland_frame_image(handle, 0, &mut record.image), 0);
            assert_eq!(record.image.alpha, 1);
            assert_eq!(record.guard, 0x1234_5678_90ab_cdef);
            assert_eq!(
                trames_wayland_frame_image_v2(handle, 0, &mut record.image, &mut geometry),
                0
            );
        }
        assert_eq!(record.image.width, 100);
        assert_eq!(record.image.alpha, 1);
        assert_eq!(record.guard, 0x1234_5678_90ab_cdef);
        assert_eq!((geometry.width, geometry.height), (30, 10));
        assert_eq!(geometry.source, [0.1, 0.2, 0.5, 0.25]);
        assert_eq!(trames_wayland_frame_release(handle, 0, 0), 0);
    }
}
