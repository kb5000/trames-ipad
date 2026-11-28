// SPDX-License-Identifier: GPL-3.0-or-later
//! Loopback-only SOCKS5 proxy dialing through a running guest's root network
//! namespace (including virtual Docker bridge subnets without port mapping).
use std::{
    collections::BTreeSet,
    io::{self, Read, Write},
    net::{IpAddr, Ipv4Addr, Ipv6Addr, Shutdown, SocketAddr, TcpListener, TcpStream, ToSocketAddrs},
    os::fd::OwnedFd,
    sync::{
        atomic::{AtomicBool, AtomicUsize, Ordering},
        Arc,
    },
    thread::{self, JoinHandle},
    time::{Duration, Instant},
};
use serde::Serialize;
use trames_net::{NetError, Network, Socket, MSG_DONTWAIT};
use trames_shared::{HostWait, Readiness};

#[derive(Clone, Debug, PartialEq, Eq, PartialOrd, Ord, Serialize)]
pub struct ListeningEndpoint {
    pub port: u16,
    pub address: String,
    pub url: String,
}

/// Snapshot currently listening TCP endpoints in the guest's root network namespace.
/// Pure in-process read with no background polling or guest command execution.
pub fn listening_tcp_endpoints(network: &Network) -> Vec<ListeningEndpoint> {
    let mut seen = BTreeSet::new();
    for (_, socket) in network.inet_inodes() {
        let Some(info) = socket.info() else { continue };
        if info.kind != 1 || info.state != 10 {
            continue;
        }
        let Some(local) = info.local else { continue };
        let port = local.port();
        if port == 0 {
            continue;
        }
        let (address, url) = match local.ip() {
            IpAddr::V4(ip) if ip.is_unspecified() => (
                "127.0.0.1".to_string(),
                format!("http://127.0.0.1:{port}"),
            ),
            IpAddr::V6(ip) if ip.is_unspecified() => (
                "127.0.0.1".to_string(),
                format!("http://127.0.0.1:{port}"),
            ),
            IpAddr::V4(ip) => (ip.to_string(), format!("http://{ip}:{port}")),
            IpAddr::V6(ip) => (format!("[{ip}]"), format!("http://[{ip}]:{port}")),
        };
        seen.insert(ListeningEndpoint { port, address, url });
    }
    seen.into_iter().collect()
}

const MAX_ACTIVE_CONNS: usize = 64;
const BUF_LIMIT: usize = 16 * 1024;
const CONNECT_TIMEOUT: Duration = Duration::from_secs(15);
const HANDSHAKE_TIMEOUT: Duration = Duration::from_secs(10);

pub struct GuestProxy {
    port: u16,
    shutdown: Arc<AtomicBool>,
    wake: Arc<Readiness>,
    accept_thread: Option<JoinHandle<()>>,
}

impl GuestProxy {
    pub fn start(network: Network) -> Result<Self, String> {
        let listener = TcpListener::bind((Ipv4Addr::LOCALHOST, 0))
            .map_err(|e| format!("bind web proxy listener: {e}"))?;
        let port = listener
            .local_addr()
            .map_err(|e| format!("read web proxy address: {e}"))?
            .port();
        listener
            .set_nonblocking(true)
            .map_err(|e| format!("configure web proxy listener: {e}"))?;
        let shutdown = Arc::new(AtomicBool::new(false));
        let wake = Arc::new(Readiness::default());
        let active = Arc::new(AtomicUsize::new(0));
        let flag = shutdown.clone();
        let signal = wake.clone();
        let handle = thread::Builder::new()
            .name(format!("trames-web-proxy-{port}"))
            .spawn(move || accept_loop(listener, network, flag, signal, active))
            .map_err(|e| format!("spawn web proxy listener: {e}"))?;
        Ok(Self {
            port,
            shutdown,
            wake,
            accept_thread: Some(handle),
        })
    }

    pub fn port(&self) -> u16 {
        self.port
    }
}

impl Drop for GuestProxy {
    fn drop(&mut self) {
        self.shutdown.store(true, Ordering::Release);
        self.wake.notify_atomic();
        if let Some(handle) = self.accept_thread.take() {
            let _ = handle.join();
        }
    }
}

struct ConnGuard(Arc<AtomicUsize>);
impl Drop for ConnGuard {
    fn drop(&mut self) {
        self.0.fetch_sub(1, Ordering::AcqRel);
    }
}

fn accept_loop(
    listener: TcpListener,
    network: Network,
    shutdown: Arc<AtomicBool>,
    wake: Arc<Readiness>,
    active: Arc<AtomicUsize>,
) {
    let mut wait = match HostWait::new() {
        Ok(w) => w,
        Err(_) => return,
    };
    while !shutdown.load(Ordering::Acquire) {
        match listener.accept() {
            Ok((stream, _)) => {
                if active.fetch_add(1, Ordering::AcqRel) >= MAX_ACTIVE_CONNS {
                    active.fetch_sub(1, Ordering::AcqRel);
                    drop(stream);
                    continue;
                }
                let guard = ConnGuard(active.clone());
                let net = network.clone();
                let flag = shutdown.clone();
                let _ = thread::Builder::new()
                    .name("trames-web-proxy-conn".into())
                    .spawn(move || {
                        let _guard = guard;
                        let _ = handle_client(stream, net, flag);
                    });
            }
            Err(e) if e.kind() == io::ErrorKind::WouldBlock => {
                wait.begin();
                wait.retain([wake.subscribe(wait.signal())]);
                if let Ok(dup) = listener.try_clone() {
                    let _ = wait.watch_owned(OwnedFd::from(dup).into(), false);
                }
                if shutdown.load(Ordering::Acquire) {
                    break;
                }
                if wait.wait().is_err() {
                    break;
                }
            }
            Err(_) => break,
        }
    }
}

fn read_exact_or_err(stream: &mut TcpStream, buf: &mut [u8]) -> io::Result<()> {
    stream.read_exact(buf)
}

fn send_socks_reply(stream: &mut TcpStream, rep: u8) {
    let reply = [5u8, rep, 0, 1, 0, 0, 0, 0, 0, 0];
    let _ = stream.write_all(&reply);
}

fn handle_client(
    mut stream: TcpStream,
    network: Network,
    shutdown: Arc<AtomicBool>,
) -> io::Result<()> {
    stream.set_nonblocking(false)?;
    stream.set_nodelay(true)?;
    stream.set_read_timeout(Some(HANDSHAKE_TIMEOUT))?;
    stream.set_write_timeout(Some(HANDSHAKE_TIMEOUT))?;

    // 1. Greeting: VER=5, NMETHODS, METHODS...
    let mut head = [0u8; 2];
    read_exact_or_err(&mut stream, &mut head)?;
    if head[0] != 5 {
        return Ok(());
    }
    let mut methods = vec![0u8; head[1] as usize];
    read_exact_or_err(&mut stream, &mut methods)?;
    if !methods.contains(&0) {
        let _ = stream.write_all(&[5, 0xff]);
        return Ok(());
    }
    stream.write_all(&[5, 0])?;

    // 2. Request: VER=5, CMD=1 (CONNECT), RSV=0, ATYP
    let mut req = [0u8; 4];
    read_exact_or_err(&mut stream, &mut req)?;
    if req[0] != 5 || req[1] != 1 {
        send_socks_reply(&mut stream, 0x07);
        return Ok(());
    }
    let targets: Vec<SocketAddr> = match req[3] {
        1 => {
            let mut ip = [0u8; 4];
            let mut port = [0u8; 2];
            read_exact_or_err(&mut stream, &mut ip)?;
            read_exact_or_err(&mut stream, &mut port)?;
            vec![SocketAddr::new(
                IpAddr::V4(Ipv4Addr::from(ip)),
                u16::from_be_bytes(port),
            )]
        }
        4 => {
            let mut ip = [0u8; 16];
            let mut port = [0u8; 2];
            read_exact_or_err(&mut stream, &mut ip)?;
            read_exact_or_err(&mut stream, &mut port)?;
            vec![SocketAddr::new(
                IpAddr::V6(Ipv6Addr::from(ip)),
                u16::from_be_bytes(port),
            )]
        }
        3 => {
            let mut len = [0u8; 1];
            read_exact_or_err(&mut stream, &mut len)?;
            let mut host = vec![0u8; len[0] as usize];
            let mut port = [0u8; 2];
            read_exact_or_err(&mut stream, &mut host)?;
            read_exact_or_err(&mut stream, &mut port)?;
            let port = u16::from_be_bytes(port);
            let host = String::from_utf8_lossy(&host);
            if host.eq_ignore_ascii_case("localhost") {
                vec![
                    SocketAddr::new(IpAddr::V4(Ipv4Addr::LOCALHOST), port),
                    SocketAddr::new(IpAddr::V6(Ipv6Addr::LOCALHOST), port),
                ]
            } else if let Ok(ip) = host.parse::<IpAddr>() {
                vec![SocketAddr::new(ip, port)]
            } else {
                match (host.as_ref(), port).to_socket_addrs() {
                    Ok(iter) => iter.collect(),
                    Err(_) => {
                        send_socks_reply(&mut stream, 0x04);
                        return Ok(());
                    }
                }
            }
        }
        _ => {
            send_socks_reply(&mut stream, 0x08);
            return Ok(());
        }
    };
    if targets.is_empty() {
        send_socks_reply(&mut stream, 0x04);
        return Ok(());
    }

    // 3. Connect through guest's root network namespace (supports virtual bridge subnets).
    let mut connected = None;
    let mut last_err = 0x05u8;
    for addr in targets {
        match connect_guest(&network, addr, &shutdown) {
            Ok(sock) => {
                connected = Some(sock);
                break;
            }
            Err(errno) => {
                last_err = match errno {
                    111 => 0x05, // ECONNREFUSED
                    101 => 0x03, // ENETUNREACH
                    113 => 0x04, // EHOSTUNREACH
                    110 => 0x06, // ETIMEDOUT
                    _ => 0x01,
                };
            }
        }
    }
    let Some(guest) = connected else {
        send_socks_reply(&mut stream, last_err);
        return Ok(());
    };
    send_socks_reply(&mut stream, 0x00);

    // 4. Event-driven bidirectional relay.
    stream.set_read_timeout(None)?;
    stream.set_write_timeout(None)?;
    stream.set_nonblocking(true)?;
    relay(stream, guest, shutdown)
}

fn connect_guest(
    network: &Network,
    addr: SocketAddr,
    shutdown: &AtomicBool,
) -> Result<Arc<Socket>, u32> {
    let family = match addr {
        SocketAddr::V4(_) => 2,
        SocketAddr::V6(_) => 10,
    };
    let sock = network.socket(family, 1, 0).map_err(|e| e.0)?;
    match sock.connect(addr) {
        Ok(()) => return Ok(sock),
        Err(NetError(115)) => {} // EINPROGRESS
        Err(NetError(e)) => return Err(e),
    }
    let deadline = Instant::now() + CONNECT_TIMEOUT;
    let mut wait = HostWait::new().map_err(|_| 5u32)?;
    loop {
        if shutdown.load(Ordering::Acquire) {
            return Err(125);
        }
        match sock.resume_connect() {
            Ok(true) => return Ok(sock),
            Ok(false) => {}
            Err(NetError(e)) => return Err(e),
        }
        if Instant::now() >= deadline {
            return Err(110);
        }
        wait.begin();
        let mut subs = vec![sock.subscribe_changes(wait.signal())];
        if let Some(sub) = sock.subscribe_io_ready(wait.signal(), false) {
            subs.push(sub);
        }
        if let Some(sub) = sock.subscribe_io_ready(wait.signal(), true) {
            subs.push(sub);
        }
        wait.retain(subs);
        if let Ok((fds, delay)) = sock.wait_sources() {
            for fd in fds {
                let _ = wait.watch_owned(fd.into(), false);
            }
            if let Some(delay) = delay {
                wait.until(deadline.min(Instant::now() + delay));
            } else {
                wait.until(deadline);
            }
        } else {
            wait.until(deadline);
        }
        // Recheck after arming subscriptions to avoid missed wakeups.
        match sock.resume_connect() {
            Ok(true) => return Ok(sock),
            Ok(false) => {}
            Err(NetError(e)) => return Err(e),
        }
        let _ = wait.wait();
    }
}

fn relay(mut client: TcpStream, guest: Arc<Socket>, shutdown: Arc<AtomicBool>) -> io::Result<()> {
    let mut wait = HostWait::new()?;
    let mut c2g = Vec::<u8>::new();
    let mut g2c = Vec::<u8>::new();
    let mut client_eof = false;
    let mut guest_eof = false;
    let mut guest_write_closed = false;
    let mut client_write_closed = false;
    let mut tmp = [0u8; 8192];

    while !shutdown.load(Ordering::Acquire) {
        let mut progressed = false;

        // Read from host client into c2g.
        if !client_eof && c2g.len() < BUF_LIMIT {
            let room = (BUF_LIMIT - c2g.len()).min(tmp.len());
            match client.read(&mut tmp[..room]) {
                Ok(0) => {
                    client_eof = true;
                    progressed = true;
                }
                Ok(n) => {
                    c2g.extend_from_slice(&tmp[..n]);
                    progressed = true;
                }
                Err(e) if e.kind() == io::ErrorKind::WouldBlock => {}
                Err(_) => break,
            }
        }

        // Write c2g into guest socket.
        if !c2g.is_empty() {
            match guest.send(&c2g, MSG_DONTWAIT, None) {
                Ok(0) => {}
                Ok(n) => {
                    c2g.drain(..n);
                    progressed = true;
                }
                Err(NetError(11)) => {}
                Err(_) => break,
            }
        } else if client_eof && !guest_write_closed {
            let _ = guest.shutdown(1);
            guest_write_closed = true;
            progressed = true;
        }

        // Read from guest socket into g2c.
        if !guest_eof && g2c.len() < BUF_LIMIT {
            let room = BUF_LIMIT - g2c.len();
            match guest.recv(room, MSG_DONTWAIT) {
                Ok(received) if received.data.is_empty() => {
                    guest_eof = true;
                    progressed = true;
                }
                Ok(received) => {
                    g2c.extend_from_slice(&received.data);
                    progressed = true;
                }
                Err(NetError(11)) => {}
                Err(_) => break,
            }
        }

        // Write g2c into host client.
        if !g2c.is_empty() {
            match client.write(&g2c) {
                Ok(0) => break,
                Ok(n) => {
                    g2c.drain(..n);
                    progressed = true;
                }
                Err(e) if e.kind() == io::ErrorKind::WouldBlock => {}
                Err(_) => break,
            }
        } else if guest_eof && !client_write_closed {
            let _ = client.shutdown(Shutdown::Write);
            client_write_closed = true;
            progressed = true;
        }

        if client_eof && guest_eof && c2g.is_empty() && g2c.is_empty() {
            break;
        }
        if progressed {
            continue;
        }

        wait.begin();
        let mut subs = vec![guest.subscribe_changes(wait.signal())];
        if !guest_eof && g2c.len() < BUF_LIMIT {
            if let Some(sub) = guest.subscribe_io_ready(wait.signal(), true) {
                subs.push(sub);
            }
        }
        if !c2g.is_empty() {
            if let Some(sub) = guest.subscribe_io_ready(wait.signal(), false) {
                subs.push(sub);
            }
        }
        wait.retain(subs);
        if !client_eof && c2g.len() < BUF_LIMIT {
            if let Ok(dup) = client.try_clone() {
                let _ = wait.watch_owned(OwnedFd::from(dup).into(), false);
            }
        }
        if !g2c.is_empty() {
            if let Ok(dup) = client.try_clone() {
                let _ = wait.watch_owned(OwnedFd::from(dup).into(), true);
            }
        }
        if let Ok((fds, delay)) = guest.wait_sources() {
            for fd in fds {
                let _ = wait.watch_owned(fd.into(), false);
            }
            if let Some(delay) = delay {
                wait.until(Instant::now() + delay.min(Duration::from_millis(50)));
            }
        } else if let Some(delay) = guest.wait_delay() {
            wait.until(Instant::now() + delay.min(Duration::from_millis(50)));
        }
        wait.wait()?;
    }
    Ok(())
}

#[cfg(test)]
mod tests {
    use super::*;
    use trames_net::{Fabric, NetworkMode};

    #[test]
    fn socks5_proxy_reaches_virtual_guest_listener_without_port_mapping() {
        let fabric = Fabric::new().unwrap();
        let guest_net = Network::virtual_network(&fabric, 2, NetworkMode::Loopback).unwrap();
        let server_sock = guest_net.socket(2, 1, 0).unwrap();
        let listen_addr: SocketAddr = "127.0.0.1:18080".parse().unwrap();
        server_sock.bind(listen_addr).unwrap();
        server_sock.listen(8).unwrap();
        server_sock.publish_inode(1, (0, 0));

        let endpoints = listening_tcp_endpoints(&guest_net);
        assert!(
            endpoints.iter().any(|e| e.port == 18080 && e.url == "http://127.0.0.1:18080"),
            "expected listening endpoint 18080, got {endpoints:?}"
        );

        let proxy = GuestProxy::start(guest_net.clone()).unwrap();
        let proxy_port = proxy.port();

        let server_thread = thread::spawn(move || {
            let mut wait = HostWait::new().unwrap();
            let accepted = loop {
                match server_sock.accept() {
                    Ok((peer, _)) => break peer,
                    Err(NetError(11)) => {
                        wait.begin();
                        let mut subs = vec![server_sock.subscribe_changes(wait.signal())];
                        if let Some(sub) = server_sock.subscribe_io_ready(wait.signal(), true) {
                            subs.push(sub);
                        }
                        wait.retain(subs);
                        if let Some(delay) = server_sock.wait_delay() {
                            wait.until(Instant::now() + delay.min(Duration::from_millis(20)));
                        }
                        let _ = wait.wait();
                    }
                    Err(e) => panic!("accept failed: {e:?}"),
                }
            };
            let mut received = Vec::new();
            while !received.ends_with(b"\r\n\r\n") {
                match accepted.recv(4096, MSG_DONTWAIT) {
                    Ok(chunk) if !chunk.data.is_empty() => received.extend_from_slice(&chunk.data),
                    Ok(_) => break,
                    Err(NetError(11)) => {
                        wait.begin();
                        let mut subs = vec![accepted.subscribe_changes(wait.signal())];
                        if let Some(sub) = accepted.subscribe_io_ready(wait.signal(), true) {
                            subs.push(sub);
                        }
                        wait.retain(subs);
                        if let Some(delay) = accepted.wait_delay() {
                            wait.until(Instant::now() + delay.min(Duration::from_millis(20)));
                        }
                        let _ = wait.wait();
                    }
                    Err(e) => panic!("recv failed: {e:?}"),
                }
            }
            let response = b"HTTP/1.1 200 OK\r\nContent-Length: 5\r\n\r\nhello";
            let mut off = 0;
            while off < response.len() {
                match accepted.send(&response[off..], MSG_DONTWAIT, None) {
                    Ok(n) => off += n,
                    Err(NetError(11)) => {
                        wait.begin();
                        if let Some(delay) = accepted.wait_delay() {
                            wait.until(Instant::now() + delay.min(Duration::from_millis(20)));
                        }
                        let _ = wait.wait();
                    }
                    Err(e) => panic!("send failed: {e:?}"),
                }
            }
            let _ = accepted.shutdown(1);
        });

        let mut client = TcpStream::connect((Ipv4Addr::LOCALHOST, proxy_port)).unwrap();
        client.set_read_timeout(Some(Duration::from_secs(5))).unwrap();
        // SOCKS5 greeting
        client.write_all(&[5, 1, 0]).unwrap();
        let mut greeting = [0u8; 2];
        client.read_exact(&mut greeting).unwrap();
        assert_eq!(greeting, [5, 0]);
        // SOCKS5 CONNECT 127.0.0.1:18080
        client
            .write_all(&[5, 1, 0, 1, 127, 0, 0, 1, (18080 >> 8) as u8, (18080 & 0xff) as u8])
            .unwrap();
        let mut reply = [0u8; 10];
        client.read_exact(&mut reply).unwrap();
        assert_eq!(reply[1], 0);
        // Send HTTP request through the SOCKS5 tunnel
        client.write_all(b"GET / HTTP/1.1\r\nHost: local\r\n\r\n").unwrap();
        let mut body = String::new();
        client.read_to_string(&mut body).unwrap();
        assert!(body.ends_with("hello"), "unexpected response: {body:?}");

        server_thread.join().unwrap();
        drop(proxy);
    }
}
