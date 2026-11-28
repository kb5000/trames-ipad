# Running OCI Containers with `crun`

You can run unmodified OCI containers inside a Trames Linux environment using Alpine's `crun` with PID, mount, UTS, and cgroupfs isolation.

> For a full `dockerd` / `containerd` / `runc` workflow with `overlay2` and bridge networking, see [`guest-docker.md`](guest-docker.md).

---

## Quick Start (Alpine Linux)

Run the following inside your Trames terminal:

```sh
apk add crun
mkdir -p /root/containers/alpine/rootfs
cd /root/containers/alpine

wget https://dl-cdn.alpinelinux.org/alpine/v3.22/releases/aarch64/alpine-minirootfs-3.22.6-aarch64.tar.gz
printf '%s\n' '821565fa8f3953eefd12497b166b4b50add2f7c57fb312e75862f5867e06fefe  alpine-minirootfs-3.22.6-aarch64.tar.gz' | sha256sum -c -
tar -xzf alpine-minirootfs-3.22.6-aarch64.tar.gz -C rootfs

wget -O config.json https://raw.githubusercontent.com/project-trames/trames-ipad/main/examples/containers/alpine.json
cp /etc/resolv.conf rootfs/etc/resolv.conf

crun --root /run/crun --cgroup-manager cgroupfs run alpine
```

Type `exit` to leave the container; normal files in `rootfs/` remain persisted while `/dev`, `/dev/pts`, and `/dev/shm` mounts are cleaned up.

---

## Managing Running Containers

From another terminal tab in the same environment:

```sh
crun --root /run/crun list
crun --root /run/crun state alpine
crun --root /run/crun exec alpine /bin/sh -c 'echo inside; cat /proc/1/status'
crun --root /run/crun pause alpine
crun --root /run/crun resume alpine
crun --root /run/crun kill alpine TERM
crun --root /run/crun delete --force alpine
```

- For background containers, set `process.terminal` to `false` in `config.json`.
- Use distinct container IDs, `linux.cgroupsPath`, hostnames, and rootfs paths when running multiple bundles simultaneously.

---

## Notes & Boundaries

- The example `alpine.json` shares the guest's network namespace and sets a `pids.limit` of 8 tasks; adjust `linux.resources.pids.limit` as needed within your environment's `kernel.threads-max` limit.
- External `--console-socket` is not supported; foreground `crun run` uses the active Trames terminal directly.
- Guest containers are a userspace compatibility feature, not a hardware security isolation boundary for untrusted code. See [`docs/architecture/guest-containers.md`](../architecture/guest-containers.md).
