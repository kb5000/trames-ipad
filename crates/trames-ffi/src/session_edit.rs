// SPDX-License-Identifier: GPL-3.0-or-later
//! Conditional editor saves keep their original checksum/stage across waits.
use super::*;
use trames_linux::HostFileTask as _;

#[cfg(test)]
#[path = "session_edit_tests.rs"]
mod tests;

#[derive(Default)]
struct Checksum(Sha256);
impl Write for Checksum {
    fn write(&mut self, bytes: &[u8]) -> io::Result<usize> {
        self.0.update(bytes);
        Ok(bytes.len())
    }
    fn flush(&mut self) -> io::Result<()> {
        Ok(())
    }
}
pub(super) struct State {
    request: trames_vfs::HostEditWork<import::Reader, Checksum>,
}
impl State {
    fn new(
        original: trames_vfs::HostEdit,
        vfs: &Vfs,
        options: &TransferOptions,
        session: Arc<Session>,
        cancelled: Arc<AtomicBool>,
    ) -> Result<Self, String> {
        if !options.replace {
            return Err("conditional transfer requires replacement".into());
        }
        let mut file = import::open_source(options)?;
        verify_saved_edit(&mut file, options.expected.as_ref().unwrap(), || {
            cancelled.load(Ordering::Acquire) || session.cancelled.load(Ordering::Acquire)
        })?;
        let reader = import::Reader::new(file, session, cancelled, options.maximum_bytes);
        Ok(Self {
            request: original.into_work(vfs, reader),
        })
    }
    pub(super) fn layout(&self) -> Arc<trames_linux::HostFileLayout> {
        self.request.layout()
    }
}
impl HostCommand {
    pub(super) fn conditional_edit(&self) -> bool {
        matches!(&self.command, Command::Transfer(transfer)
            if !transfer.options.export && transfer.options.expected.is_some())
    }
    pub(super) fn run_edit(
        mut self: Box<Self>,
        admission: Option<trames_linux::HostFileAdmission>,
    ) {
        let _resources = self.resources.enter();
        let Command::Transfer(transfer) = &self.command else {
            unreachable!("accepted edit");
        };
        let session = self.completion.session.clone();
        let cancelled = transfer.cancelled.clone();
        if cancelled.load(Ordering::Acquire) || session.cancelled.load(Ordering::Acquire) {
            drop(admission);
            self.finish_transfer(Err("transfer cancelled".into()));
            return;
        }
        if self.edit.is_none() {
            let Some(original) = self.access.edit.take() else {
                drop(admission);
                self.finish_transfer(Err("missing original edit capability".into()));
                return;
            };
            match State::new(
                original,
                &self.vfs,
                &transfer.options,
                session.clone(),
                cancelled.clone(),
            ) {
                Ok(state) => self.edit = Some(state),
                Err(error) => {
                    drop(admission);
                    self.finish_transfer(Err(error));
                    return;
                }
            }
        }
        let Some(admission) = admission else {
            self.finish_transfer(Err("missing original edit admission".into()));
            return;
        };
        let expected = transfer.options.expected.as_ref().unwrap();
        let state = self.edit.as_mut().unwrap();
        let result = state.request.run(
            &admission,
            || cancelled.load(Ordering::Acquire) || session.cancelled.load(Ordering::Acquire),
            |inode, checksum| {
                let current = format!("{:x}", checksum.0.clone().finalize());
                if current == expected.replacement_sha256 {
                    return Ok(true);
                }
                if inode != expected.inode || current != expected.sha256 {
                    return Err(trames_vfs::Errno(116));
                }
                Ok(false)
            },
        );
        drop(admission);
        match result {
            Ok(Some(inode)) => self.finish_transfer(Ok(inode)),
            Err(error) => self.finish_transfer(Err(match error.0 {
                4 => "transfer cancelled".into(),
                116 => "Linux file changed since it was opened; edited copy retained".into(),
                _ => error.to_string(),
            })),
            Ok(None) => {
                let wait = state
                    .request
                    .wait()
                    .expect("original conditional edit wait")
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
