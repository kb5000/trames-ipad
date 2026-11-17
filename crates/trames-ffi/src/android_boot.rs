// SPDX-License-Identifier: GPL-3.0-or-later
//! Android policy for this interpreter embedding, separate from the Mac HVF product.
use trames_vfs::{Vfs, AT_FDCWD};

pub(crate) fn interpreter_timeouts(vfs: &mut Vfs) -> Result<(), String> {
    const PATH: &[u8] = b"/run/trames-android/waydroid.prop";
    let result = (|| {
        let fd = vfs.open_at(AT_FDCWD, PATH, 0, 0)?;
        let mut properties = Vec::new();
        let read = (|| {
            loop {
                let bytes = vfs.read(fd, 4096)?;
                if bytes.is_empty() {
                    break;
                }
                properties.extend(bytes);
                if properties.len() > 65536 {
                    return Err(trames_vfs::Errno(27));
                }
            }
            Ok(())
        })();
        let closed = vfs.close(fd);
        read?;
        closed?;
        if properties
            .split(|b| *b == b'\n')
            .any(|line| line.split(|b| *b == b'=').next() == Some(b"ro.hw_timeout_multiplier"))
        {
            return Ok(());
        }
        // Original Android 13 ActiveServices uses 20s * this multiplier. Real
        // iPad traces show runnable SystemUI initialization exceeding 20s.
        // Use Android's own slow-hardware policy, never scaled guest clocks,
        // synthetic responsiveness or a host scheduler/coordinator workaround.
        let fd = vfs.open_at(AT_FDCWD, PATH, 1 | 0x400, 0)?;
        let bytes = b"\nro.hw_timeout_multiplier=5\n";
        let written = vfs.write(fd, bytes);
        let closed = vfs.close(fd);
        if written? != bytes.len() {
            return Err(trames_vfs::Errno(5));
        }
        closed
    })();
    result.map_err(|e| format!("configure Android interpreter timeouts: {e}"))
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn interpreter_policy_preserves_explicit_image_timeout_and_other_properties() {
        let mut vfs = Vfs::default();
        let path = b"/run/trames-android/waydroid.prop";
        vfs.import_file(path, b"custom=value\n".to_vec()).unwrap();
        vfs.import_file(b"/vendor/waydroid.prop", Vec::new())
            .unwrap();
        vfs.mount(Some(path), b"/vendor/waydroid.prop", None, 4096, None)
            .unwrap();
        interpreter_timeouts(&mut vfs).unwrap();
        interpreter_timeouts(&mut vfs).unwrap();
        let fd = vfs
            .open_at(AT_FDCWD, b"/vendor/waydroid.prop", 0, 0)
            .unwrap();
        assert_eq!(
            vfs.read(fd, 4096).unwrap(),
            b"custom=value\n\nro.hw_timeout_multiplier=5\n"
        );
        vfs.close(fd).unwrap();
        let explicit = b"custom=value\nro.hw_timeout_multiplier=9\n";
        let fd = vfs.open_at(AT_FDCWD, path, 1 | 0x200, 0).unwrap();
        vfs.write(fd, explicit).unwrap();
        vfs.close(fd).unwrap();
        interpreter_timeouts(&mut vfs).unwrap();
        let fd = vfs.open_at(AT_FDCWD, path, 0, 0).unwrap();
        assert_eq!(vfs.read(fd, 4096).unwrap(), explicit);
    }
}
