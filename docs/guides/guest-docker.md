# Running Docker Inside a Trames Guest

Trames supports running unmodified `dockerd`, `containerd`, and `runc` directly inside a guest Linux environment—using guest IPC/cgroup/network namespaces, `overlay2`, seccomp/device filtering, and Docker bridge networking (not a host Docker socket).

---

## Install and Start Docker (Alpine Linux)

Install the standard signed Alpine packages and start `dockerd`:

```sh
apk add docker-engine docker-cli
dockerd >/tmp/dockerd.log 2>&1 &
```

`docker-engine` pulls in `containerd` and `runc` automatically. You do not need a custom `daemon.json`, `crun`, or `GOMAXPROCS=1`.

Wait until `/tmp/dockerd.log` reports `API listen on /var/run/docker.sock` before running `docker` commands.

### Minimal Shell / Standalone CLI Note
On iPad builds, `/run` and `/dev/shm` are mounted automatically as `tmpfs` before startup. If you are running a minimal standalone CLI environment without init, mount them first if not already mounted:

```sh
mkdir -p /run /dev/shm
mountpoint -q /run || mount -t tmpfs -o mode=755 tmpfs /run
mountpoint -q /dev/shm || mount -t tmpfs -o size=64m,mode=1777 tmpfs /dev/shm
```

---

## Running Containers and Networking

Example workflow importing an Alpine rootfs tarball and running containers on a custom bridge network:

```sh
docker import /tmp/alpine-rootfs.tar trames-alpine:local
docker run --rm trames-alpine:local \
  /bin/sh -ec 'ip a; ip route; nslookup dl-cdn.alpinelinux.org'

docker network create appnet
docker run --network appnet --name app -p 8080:8080 trames-alpine:local /path/to/server
```

- **Storage Driver**: `overlay2` is selected by default, sharing read-only image layers and copying files on first write.
- **DNS**: Managed guest DNS uses `169.254.0.53` (custom `/etc/resolv.conf` is preserved). Custom Docker bridge networks use Docker's embedded resolver at `127.0.0.11`.
- **Published Ports**: TCP (`-p 8080:8080`) and UDP (`-p 5353:5353/udp`) port proxies are supported within iPadOS network permissions and foreground/background lifecycle rules.

---

## Current Scope & Limits

- **Supported**: Default `runc` + managed `containerd`, `overlay2`, default/custom bridge networks, container DNS, NAT/firewall rules, `DOCKER-USER`, `--internal`/`--network none`, and TCP/UDP userland port proxies.
- **Not Yet Fully Covered**: Full SysV/POSIX IPC namespaces, rootless/user namespaces, full BuildKit/Compose workflows, `AF_PACKET`/ARP/STP/qdisc/`tc`, and `--userland-proxy=false`.
- **Architecture Details**: See [`docs/architecture/docker-network-runtime.md`](../architecture/docker-network-runtime.md) and [`docs/architecture/overlayfs.md`](../architecture/overlayfs.md).
