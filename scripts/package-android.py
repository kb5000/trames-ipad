#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Package a clean optional Android rootfs and thin HALs; never includes /data.

Streams the owner-preserving original rootfs rather than unpacking another tree.
The original Waydroid system/vendor archives must match the pinned provenance.
Geekbench and other separately installed applications are never bundled.
"""
import argparse
import copy
import gzip
import hashlib
import io
import json
from pathlib import Path, PurePosixPath
import subprocess
import tarfile

ROOT = Path(__file__).resolve().parents[1]
MUTABLE = {"data", "metadata", "dev", "proc", "sys", "run", "mnt", "storage", "cache"}
COMPONENTS = {
    "vendor/lib64/hw/vulkan.trames.so": "vulkan.trames.so",
    "vendor/lib64/hw/gralloc.trames.so": "gralloc.trames.so",
    "vendor/bin/hw/trames-hal-service": "trames-hal-service",
}


def digest(path):
    h = hashlib.sha256()
    with path.open("rb") as stream:
        for data in iter(lambda: stream.read(1024*1024), b""):
            h.update(data)
    return h.hexdigest()


def normalized(name):
    while name.startswith("./"):
        name = name[2:]
    name = name.rstrip("/")
    if not name or name == ".":
        return "."
    if name.startswith("/") or ".." in PurePosixPath(name).parts or "\0" in name:
        raise ValueError("unsafe original rootfs path: " + name)
    return name


def add_bytes(archive, name, data, mode=0o644):
    info = tarfile.TarInfo(name)
    info.uid = info.gid = 0
    info.mode = mode
    info.size = len(data)
    archive.addfile(info, io.BytesIO(data))


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--rootfs", type=Path, required=True)
    p.add_argument("--images", type=Path, required=True, help="directory containing the two original pinned ZIP archives")
    p.add_argument("--components", type=Path, default=ROOT / "target/vulkan-android")
    p.add_argument("--output", type=Path, required=True)
    args = p.parse_args()
    if args.output.exists():
        p.error("output exists; choose a new filename")
    pins = json.loads((ROOT / "scripts/android-images.json").read_text())
    for key in ["system", "vendor"]:
        pin = pins[key]
        image = args.images / pin["filename"]
        if image.stat().st_size != pin["size"] or digest(image) != pin["sha256"]:
            raise RuntimeError("original Waydroid image mismatch: " + key)
    components = {}
    for guest, name in COMPONENTS.items():
        path = args.components / name
        with path.open("rb") as stream:
            elf = stream.read(20)
        if elf[:5] != b"\x7fELF\x02" or elf[18:20] != b"\xb7\0":
            raise RuntimeError("component is not ARM64 ELF: " + name)
        components["/"+guest] = dict(sha256=digest(path), size=path.stat().st_size)
    revision = lambda repo: subprocess.check_output(["git", "-C", str(repo), "rev-parse", "HEAD"], text=True).strip()
    metadata = dict(schema=1, target="android", abi="arm64-v8a", android_api=33,
                    graphics_contract=1, images=pins, components=components,
                    source_revision=revision(ROOT), runtime_revision=revision(ROOT / "runtime"),
                    original_rootfs_sha256=digest(args.rootfs),
                    limitations=["experimental", "ARM64 only", "SELinux disabled", "no emulated external storage/FUSE", "no Android pressure-based app killing", "network/audio not accepted"])
    overrides = set(COMPONENTS) | {"vendor/waydroid.prop", ".trames-android.json"}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    partial = args.output.with_suffix(args.output.suffix + ".partial")
    try:
        with partial.open("xb") as raw, gzip.GzipFile(filename="", fileobj=raw, mode="wb", mtime=0, compresslevel=3) as compressed, tarfile.open(fileobj=compressed, mode="w|") as output, tarfile.open(args.rootfs) as source:
            seen = set()
            for original in source:
                name = normalized(original.name)
                if name in seen:
                    raise RuntimeError("duplicate original rootfs entry: " + name)
                seen.add(name)
                if name in overrides or name.startswith(".trames-android/"):
                    continue
                if name.split("/")[0] in MUTABLE and "/" in name:
                    continue
                info = copy.copy(original)
                info.name = name
                output.addfile(info, source.extractfile(original) if original.isfile() else None)
            required = {"system/bin/init", "system/bin/bootstrap/linker64", "vendor/lib64/egl/libEGL_angle.so"}
            if not required <= seen:
                raise RuntimeError("rootfs is missing original Waydroid executables")
            for guest, name in COMPONENTS.items():
                path = args.components / name
                info = tarfile.TarInfo(guest)
                info.mode = 0o755 if guest.startswith("vendor/bin/") else 0o644
                info.size = path.stat().st_size
                with path.open("rb") as stream:
                    output.addfile(info, stream)
            add_bytes(output, "vendor/waydroid.prop", b"ro.hardware.vulkan=trames\nro.hardware.egl=angle\nro.hardware.gralloc=trames\n")
            add_bytes(output, ".trames-android.json", (json.dumps(metadata, indent=2)+"\n").encode())
            add_bytes(output, ".trames-android/GPL-3.0.txt", (ROOT / "LICENSE").read_bytes())
            for repo, name in [(ROOT, "trames-ipad"), (ROOT / "runtime", "trames-runtime")]:
                # These are small source exports, not a duplicate binary tree.
                data = subprocess.check_output(["git", "-C", str(repo), "archive", "--format=tar.gz", "HEAD"])
                add_bytes(output, ".trames-android/sources/"+name+".tar.gz", data)
        partial.rename(args.output)
    except BaseException:
        partial.unlink(missing_ok=True)
        raise
    record = dict(filename=args.output.name, size=args.output.stat().st_size,
                  sha256=digest(args.output), metadata=metadata)
    args.output.with_suffix(args.output.suffix+".json").write_text(json.dumps(record, indent=2)+"\n")
    args.output.with_suffix(args.output.suffix+".sha256").write_text(record["sha256"]+"  "+args.output.name+"\n")
    print(json.dumps(dict(filename=record["filename"], size=record["size"], sha256=record["sha256"])), flush=True)


if __name__ == "__main__":
    main()
