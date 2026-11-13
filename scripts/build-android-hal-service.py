#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Build only an ARM64 HIDL HAL launcher using the selected image's libraries."""
import argparse
import base64
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tarfile
import urllib.request

ROOT = Path(__file__).resolve().parents[1]


def sha(data):
    return hashlib.sha256(data).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rootfs", type=Path, required=True)
    parser.add_argument("--cache", type=Path, default=ROOT / "target/android-toolchain")
    parser.add_argument("--output", type=Path, default=ROOT / "target/vulkan-android")
    parser.add_argument("--offline", action="store_true")
    parser.add_argument("--cc", default="clang")
    args = parser.parse_args()
    cache, output = args.cache.resolve(), args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    prepare = [sys.executable, str(ROOT / "scripts/prepare-android.py"), "--cache", str(cache)]
    if args.offline:
        prepare.append("--offline")
    subprocess.run(prepare, check=True)
    manifest = json.loads((ROOT / "scripts/android-upstream.json").read_text())
    pin = manifest["platform_cxx"]
    for name, url, expected, encoded in [
        (pin["archive"], pin["url"], pin["sha256"], False),
        (pin["license_archive"], pin["license_url"], pin["license_sha256"], True),
    ]:
        path = cache / name
        if not path.exists() or sha(path.read_bytes()) != expected:
            if args.offline:
                raise RuntimeError("missing or changed platform libc++ input: " + name)
            with urllib.request.urlopen(url, timeout=30) as response:
                data = response.read()
            if encoded:
                data = base64.b64decode(data, validate=True)
            if sha(data) != expected:
                raise RuntimeError("platform libc++ input digest mismatch: " + name)
            path.write_bytes(data)
    headers = cache / "platform-libcxx"
    with tarfile.open(cache / pin["archive"]) as archive:
        for item in archive:
            name = Path(item.name)
            if name.is_absolute() or ".." in name.parts:
                raise RuntimeError("unsafe libc++ header path")
            if item.isfile():
                path = headers / name
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_bytes(archive.extractfile(item).read())
    inputs = output / "hal-link-inputs"
    inputs.mkdir(exist_ok=True)
    libraries = {}
    with tarfile.open(args.rootfs) as archive:
        members = {item.name.removeprefix("./"): item for item in archive.getmembers()}
        for name in ["libhidlbase.so", "libc++.so"]:
            member = members["system/lib64/" + name]
            data = archive.extractfile(member).read()
            if data[:5] != b"\x7fELF\x02" or data[18:20] != b"\xb7\x00":
                raise RuntimeError("selected image library is not ARM64: " + name)
            (inputs / name).write_bytes(data)
            libraries[name] = sha(data)
    sysroot = cache / "sysroot"
    lib = sysroot / f"usr/lib/aarch64-linux-android/{manifest['api']}"
    rust = subprocess.check_output(["rustc", "-vV"], text=True)
    host = next(line.split(": ", 1)[1] for line in rust.splitlines() if line.startswith("host: "))
    rustroot = Path(subprocess.check_output(["rustc", "--print", "sysroot"], text=True).strip())
    linker = rustroot / "lib/rustlib" / host / "bin/rust-lld"
    # Keep the ELF linker name so Clang selects the original lld driver mode.
    linkname = inputs / "ld.lld"
    if linkname.is_symlink():
        linkname.unlink()
    if linkname.exists():
        raise RuntimeError("refusing to replace a non-symlink linker")
    linkname.symlink_to(linker)
    source = ROOT / "accelerator/android/hal-service.cpp"
    command = [args.cc, f"--target=aarch64-linux-android{manifest['api']}",
               f"--sysroot={sysroot}", f"--ld-path={linkname}", "-O2", "-mno-outline-atomics",
               "-Wall", "-Wextra", "-Werror", "-std=c++17", "-fno-exceptions", "-fno-rtti",
               "-nostdinc++", "-nostdlib++", "-isystem", str(headers), "-fPIE", "-pie", "-nostdlib",
               str(lib / "crtbegin_dynamic.o"), str(source), str(inputs / "libhidlbase.so"),
               str(inputs / "libc++.so"), "-L", str(lib), "-Wl,--no-undefined",
               "-Wl,--allow-shlib-undefined", "-Wl,--build-id=sha1", "-Wl,-z,noexecstack",
               "-Wl,--dynamic-linker,/system/bin/linker64", "-lc", "-lm", "-ldl",
               str(lib / "crtend_android.o"), "-o", str(output / "trames-hal-service")]
    subprocess.run(command, check=True)
    record = dict(api=manifest["api"], abi="arm64-v8a", platform_cxx=pin,
                  source_sha256=sha(source.read_bytes()), libraries_sha256=libraries,
                  command=command, output_sha256=sha((output / "trames-hal-service").read_bytes()))
    (output / "hal-service-manifest.json").write_text(json.dumps(record, indent=2) + "\n")
    shutil.copyfile(cache / pin["license_archive"], output / "libcxx-LICENSE.TXT")
    print(json.dumps(dict(output=str(output / "trames-hal-service"), sha256=record["output_sha256"])))


if __name__ == "__main__":
    main()
