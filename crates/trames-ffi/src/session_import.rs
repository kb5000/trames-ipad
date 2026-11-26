// SPDX-License-Identifier: GPL-3.0-or-later
//! Accepted native stream imports park on the existing file-layout driver.
use super::*;
use trames_linux::HostFileTask as _;

pub(super) struct Reader {
    file: File,
    session: Arc<Session>,
    cancelled: Arc<AtomicBool>,
    remaining: u64,
}
impl Reader {
    pub(super) fn new(
        file: File,
        session: Arc<Session>,
        cancelled: Arc<AtomicBool>,
        maximum: Option<u64>,
    ) -> Self {
        Self {
            file,
            session,
            cancelled,
            remaining: maximum.unwrap_or(u64::MAX),
        }
    }
}
pub(super) fn open_source(options: &TransferOptions) -> Result<File, String> {
    let file = File::open(&options.host).map_err(|e| e.to_string())?;
    let metadata = file.metadata().map_err(|e| e.to_string())?;
    if !metadata.is_file()
        || options
            .maximum_bytes
            .is_some_and(|limit| metadata.len() > limit)
    {
        return Err("source is not a regular file or exceeds transfer size limit".into());
    }
    Ok(file)
}
impl Read for Reader {
    fn read(&mut self, out: &mut [u8]) -> io::Result<usize> {
        checked_read(
            &mut self.file,
            &self.session.cancelled,
            &self.cancelled,
            &mut self.remaining,
            out,
        )
    }
}
pub(super) struct State {
    request: trames_vfs::HostImport<Reader>,
}
impl State {
    fn new(
        vfs: &Vfs,
        options: &TransferOptions,
        session: Arc<Session>,
        cancelled: Arc<AtomicBool>,
    ) -> Result<Self, String> {
        let reader = Reader::new(
            open_source(options)?,
            session,
            cancelled,
            options.maximum_bytes,
        );
        Ok(Self {
            request: vfs.host_import(options.guest.as_bytes(), reader, options.replace, false),
        })
    }
}
impl HostCommand {
    pub(super) fn unconditional_import(&self) -> bool {
        matches!(&self.command, Command::Transfer(transfer)
            if !transfer.options.export && transfer.options.expected.is_none())
    }
    pub(super) fn run_import(mut self: Box<Self>) {
        let _resources = self.resources.enter();
        let Command::Transfer(transfer) = &self.command else {
            unreachable!("accepted import");
        };
        let cancelled = transfer.cancelled.clone();
        let session = self.completion.session.clone();
        if cancelled.load(Ordering::Acquire) || session.cancelled.load(Ordering::Acquire) {
            self.finish_transfer(Err("transfer cancelled".into()));
            return;
        }
        if self.import.is_none() {
            match State::new(
                &self.vfs,
                &transfer.options,
                session.clone(),
                cancelled.clone(),
            ) {
                Ok(state) => self.import = Some(state),
                Err(error) => {
                    self.finish_transfer(Err(error));
                    return;
                }
            }
        }
        let state = self.import.as_mut().unwrap();
        let result = state.request.run_checked(|| {
            cancelled.load(Ordering::Acquire) || session.cancelled.load(Ordering::Acquire)
        });
        match result {
            Ok(Some(result)) => self.finish_transfer(Ok(result.inode)),
            Err(error) => self.finish_transfer(Err(if error.0 == 4 {
                "transfer cancelled".into()
            } else {
                error.to_string()
            })),
            Ok(None) => {
                let wait = state
                    .request
                    .wait()
                    .expect("original import resource wait")
                    .clone();
                let wait = resource_wait(wait, session, cancelled);
                drop(_resources);
                if let Some(layouts) = self.layouts.upgrade() {
                    layouts.wait_host(self, wait);
                } else {
                    self.fail();
                }
            }
        }
    }
    pub(super) fn finish_transfer(mut self: Box<Self>, result: Result<u64, String>) {
        drop(self.import.take());
        drop(self.export.take());
        drop(self.edit.take());
        Adapter::snapshot_session(&self.completion.session, &self.vfs);
        if let Command::Transfer(transfer) = &self.command {
            let _ = transfer.result.send(result);
        }
    }
}
pub(super) fn resource_wait(
    wait: Arc<trames_linux::HostFileWait>,
    session: Arc<Session>,
    cancelled: Arc<AtomicBool>,
) -> Arc<trames_linux::HostFileWait> {
    trames_linux::HostFileWait::resource(Arc::new(Wait {
        wait,
        session,
        cancelled,
    }))
}

#[cfg(test)]
#[path = "session_import_tests.rs"]
mod tests;
struct Wait {
    wait: Arc<trames_linux::HostFileWait>,
    session: Arc<Session>,
    cancelled: Arc<AtomicBool>,
}
impl trames_shared::ResourceWait for Wait {
    fn ready(&self) -> bool {
        self.cancelled.load(Ordering::Acquire)
            || self.session.cancelled.load(Ordering::Acquire)
            || self.wait.ready()
    }
    fn subscribe(
        &self,
        signal: &Arc<trames_shared::IoSignal>,
    ) -> Vec<trames_shared::IoSubscription> {
        let mut subscriptions = self.wait.subscribe(signal);
        subscriptions.push(self.session.console_readiness.subscribe(signal));
        if trames_shared::ResourceWait::ready(self) {
            signal.notify();
        }
        subscriptions
    }
    fn deadline(&self) -> Option<Instant> {
        if self.cancelled.load(Ordering::Acquire) || self.session.cancelled.load(Ordering::Acquire)
        {
            Some(Instant::now())
        } else {
            self.wait.deadline()
        }
    }
}

pub(super) fn checked_read(
    file: &mut impl Read,
    session: &AtomicBool,
    cancelled: &AtomicBool,
    remaining: &mut u64,
    out: &mut [u8],
) -> io::Result<usize> {
    if cancelled.load(Ordering::Acquire) || session.load(Ordering::Acquire) {
        return Err(io::ErrorKind::Interrupted.into());
    }
    if out.is_empty() {
        return Ok(0);
    }
    if *remaining == 0 {
        let mut extra = [0];
        return match file.read(&mut extra)? {
            0 => Ok(0),
            _ => Err(io::Error::new(
                io::ErrorKind::FileTooLarge,
                "source exceeds transfer size limit",
            )),
        };
    }
    let length = out.len().min((*remaining).min(usize::MAX as u64) as usize);
    let count = file.read(&mut out[..length])?;
    *remaining -= count as u64;
    Ok(count)
}
