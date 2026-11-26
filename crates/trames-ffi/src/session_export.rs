// SPDX-License-Identifier: GPL-3.0-or-later
//! Live export keeps original guest and native handles across file waits.
use super::*;
use trames_linux::HostFileTask as _;

#[cfg(test)]
#[path = "session_export_tests.rs"]
mod tests;

struct Writer {
    file: Option<File>,
    path: PathBuf,
    session: Arc<Session>,
    cancelled: Arc<AtomicBool>,
    remaining: u64,
    keep: bool,
}
impl Write for Writer {
    fn write(&mut self, bytes: &[u8]) -> io::Result<usize> {
        checked_write(
            self.file.as_mut().unwrap(),
            &self.session.cancelled,
            &self.cancelled,
            &mut self.remaining,
            bytes,
        )
    }
    fn flush(&mut self) -> io::Result<()> {
        self.file.as_mut().unwrap().flush()
    }
}
impl Drop for Writer {
    fn drop(&mut self) {
        if self.keep {
            return;
        }
        // Check the retained handle before best-effort cleanup: an output name
        // replaced during a long resource wait may now identify another file.
        // Native writers retain their own filesystem synchronization duty.
        #[cfg(unix)]
        let original = {
            use std::os::unix::fs::MetadataExt;
            self.file
                .as_ref()
                .and_then(|file| file.metadata().ok())
                .zip(fs::symlink_metadata(&self.path).ok())
                .is_some_and(|(old, current)| {
                    old.dev() == current.dev() && old.ino() == current.ino()
                })
        };
        #[cfg(not(unix))]
        let original = false;
        drop(self.file.take());
        if original {
            let _ = fs::remove_file(&self.path);
        }
    }
}
pub(super) struct State {
    request: Option<trames_vfs::HostExportWork<Writer>>,
}
impl State {
    fn new(
        original: trames_vfs::HostExport,
        options: &TransferOptions,
        session: Arc<Session>,
        cancelled: Arc<AtomicBool>,
    ) -> Result<Self, String> {
        if options
            .maximum_bytes
            .is_some_and(|limit| original.len() > limit)
        {
            return Err("guest file exceeds transfer size limit".into());
        }
        let file = OpenOptions::new()
            .write(true)
            .create_new(true)
            .open(&options.host)
            .map_err(|e| e.to_string())?;
        let writer = Writer {
            file: Some(file),
            path: options.host.clone(),
            session,
            cancelled,
            remaining: options.maximum_bytes.unwrap_or(u64::MAX),
            keep: false,
        };
        Ok(Self {
            request: Some(original.into_work(writer)),
        })
    }
    pub(super) fn layout(&self) -> Arc<trames_linux::HostFileLayout> {
        self.request.as_ref().unwrap().layout()
    }
}
impl HostCommand {
    pub(super) fn is_export(&self) -> bool {
        matches!(&self.command, Command::Transfer(transfer) if transfer.options.export)
    }
    pub(super) fn run_export(
        mut self: Box<Self>,
        admission: Option<trames_linux::HostFileAdmission>,
    ) {
        let _resources = self.resources.enter();
        let Command::Transfer(transfer) = &self.command else {
            unreachable!("accepted export");
        };
        let session = self.completion.session.clone();
        let cancelled = transfer.cancelled.clone();
        if cancelled.load(Ordering::Acquire) || session.cancelled.load(Ordering::Acquire) {
            drop(admission);
            self.finish_transfer(Err("transfer cancelled".into()));
            return;
        }
        if self.export.is_none() {
            let Some(original) = self.access.export.take() else {
                drop(admission);
                self.finish_transfer(Err("missing original export capability".into()));
                return;
            };
            match State::new(
                original,
                &transfer.options,
                session.clone(),
                cancelled.clone(),
            ) {
                Ok(state) => self.export = Some(state),
                Err(error) => {
                    drop(admission);
                    self.finish_transfer(Err(error));
                    return;
                }
            }
        }
        let Some(admission) = admission else {
            self.finish_transfer(Err("missing original export admission".into()));
            return;
        };
        let request = self.export.as_mut().unwrap().request.as_mut().unwrap();
        let result = request.run(&admission, || {
            cancelled.load(Ordering::Acquire) || session.cancelled.load(Ordering::Acquire)
        });
        drop(admission);
        match result {
            Ok(Some(_)) => {
                let request = self.export.as_mut().unwrap().request.take().unwrap();
                let mut writer = request.into_writer();
                writer.keep = true;
                drop(writer);
                self.finish_transfer(Ok(0));
            }
            Err(error) => self.finish_transfer(Err(if error.0 == 4 {
                "transfer cancelled".into()
            } else {
                error.to_string()
            })),
            Ok(None) => {
                let wait = self
                    .export
                    .as_ref()
                    .unwrap()
                    .request
                    .as_ref()
                    .unwrap()
                    .wait()
                    .expect("original export resource wait")
                    .clone();
                let wait = import::resource_wait(wait, session, cancelled);
                drop(_resources);
                if let Some(layouts) = self.layouts.upgrade() {
                    layouts.wait_host(self, wait);
                } else {
                    self.fail();
                }
            }
        }
    }
}
pub(super) fn checked_write(
    file: &mut impl Write,
    session: &AtomicBool,
    cancelled: &AtomicBool,
    remaining: &mut u64,
    bytes: &[u8],
) -> io::Result<usize> {
    if cancelled.load(Ordering::Acquire) || session.load(Ordering::Acquire) {
        return Err(io::ErrorKind::Interrupted.into());
    }
    if !bytes.is_empty() && *remaining == 0 {
        return Err(io::Error::new(
            io::ErrorKind::FileTooLarge,
            "guest file exceeds transfer size limit",
        ));
    }
    let length = bytes
        .len()
        .min((*remaining).min(usize::MAX as u64) as usize);
    let count = file.write(&bytes[..length])?;
    *remaining -= count as u64;
    Ok(count)
}
