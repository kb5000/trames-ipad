#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Prepare pinned public Android C/HAL inputs; extract no compiler or AOSP tree."""
import argparse
import base64
import hashlib
import json
from pathlib import Path
import shutil
import tempfile
import urllib.request
import zipfile

ROOT = Path(__file__).resolve().parents[1]


def digest(path, algorithm):
    result = hashlib.new(algorithm)
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            result.update(chunk)
    return result.hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cache", type=Path, default=ROOT / "target/android-toolchain")
    parser.add_argument("--offline", action="store_true")
    args = parser.parse_args()
    manifest = json.loads((ROOT / "scripts/android-upstream.json").read_text())
    cache = args.cache.resolve()
    cache.mkdir(parents=True, exist_ok=True)
    ndk = manifest["ndk"]
    archive = cache / ndk["archive"]
    if not archive.exists():
        if args.offline:
            raise RuntimeError("pinned NDK archive is not cached")
        with tempfile.NamedTemporaryFile(dir=cache, delete=False) as output:
            temporary = Path(output.name)
            try:
                with urllib.request.urlopen(ndk["url"], timeout=120) as source:
                    shutil.copyfileobj(source, output, 1024 * 1024)
            except BaseException:
                temporary.unlink(missing_ok=True)
                raise
        if temporary.stat().st_size != ndk["size"] or digest(temporary, "sha1") != ndk["sha1"]:
            temporary.unlink()
            raise RuntimeError("NDK archive digest mismatch")
        temporary.replace(archive)
    if archive.stat().st_size != ndk["size"] or digest(archive, "sha1") != ndk["sha1"]:
        raise RuntimeError("cached NDK archive digest mismatch")
    # Host-independent Bionic headers and API stubs only. The project already
    # has native Clang and the Rust toolchain's native ELF linker on Apple ARM.
    sysroot = cache / "sysroot"
    selected = {}
    with zipfile.ZipFile(archive) as source:
        for item in source.infolist():
            if item.is_dir() or not item.filename.startswith(ndk["sysroot_prefix"]):
                continue
            name = item.filename[len(ndk["sysroot_prefix"]):]
            if not (name.startswith("usr/include/") or name.startswith(f"usr/lib/aarch64-linux-android/{manifest['api']}/")):
                continue
            if Path(name).is_absolute() or ".." in Path(name).parts:
                raise RuntimeError("unsafe sysroot archive path")
            data = source.read(item)
            output = sysroot / name
            output.parent.mkdir(parents=True, exist_ok=True)
            if not output.exists() or output.read_bytes() != data:
                output.write_bytes(data)
            selected[name] = hashlib.sha256(data).hexdigest()
    for name, pin in manifest["headers"].items():
        output = cache / "platform-headers" / name
        if output.exists() and digest(output, "sha256") == pin["sha256"]:
            continue
        if args.offline:
            raise RuntimeError("missing or changed platform header: " + name)
        with urllib.request.urlopen(pin["url"], timeout=30) as source:
            data = base64.b64decode(source.read(), validate=True)
        if hashlib.sha256(data).hexdigest() != pin["sha256"]:
            raise RuntimeError("Android platform header digest mismatch: " + name)
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_bytes(data)
    (cache / "inputs.json").write_text(json.dumps(dict(ndk=ndk, api=manifest["api"], sysroot_sha256=selected, platform_headers=manifest["headers"]), indent=2) + "\n")
    print(json.dumps(dict(sysroot=str(sysroot), files=len(selected), platform_headers=len(manifest["headers"]))))


if __name__ == "__main__":
    main()
