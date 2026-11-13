#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Build a Bionic Vulkan HAL and original-loader acceptance tools without AOSP.

Uses the existing native Clang/Rust ELF linker and a verified NDK C sysroot.
The output is a headless Android HAL, not a Linux ICD/JSON package. Android
native buffers, Gralloc and acquire/release sync-file interoperability remain
separate work; no such capabilities are advertised by this frontend.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
HEADERS = ROOT / "target/vulkan-upstream/headers/Vulkan-Headers-cfebfc96b2b0bce93da7d12f2c14cc01793ae25c/include"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cache", type=Path, default=ROOT / "target/android-toolchain")
    parser.add_argument("--output", type=Path, default=ROOT / "target/vulkan-android")
    parser.add_argument("--offline", action="store_true")
    parser.add_argument("--shader", type=Path)
    parser.add_argument("--cc", default=os.environ.get("CC", "clang"))
    args = parser.parse_args()
    cache, output = args.cache.resolve(), args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    prepare = [sys.executable, str(ROOT / "scripts/prepare-android.py"), "--cache", str(cache)]
    if args.offline:
        prepare.append("--offline")
    subprocess.run(prepare, check=True)
    if not HEADERS.exists():
        if args.offline:
            raise RuntimeError("pinned project Vulkan headers are not cached")
        subprocess.run([sys.executable, str(ROOT / "scripts/prepare-vulkan.py")], check=True)
    subprocess.run([sys.executable, str(ROOT / "scripts/generate-vulkan-compute.py"), "--check"], check=True)
    rust_sysroot = Path(subprocess.check_output(["rustc", "--print", "sysroot"], text=True).strip())
    rust_version = subprocess.check_output(["rustc", "-vV"], text=True)
    host = next(line.split(": ", 1)[1] for line in rust_version.splitlines() if line.startswith("host: "))
    linker = rust_sysroot / "lib/rustlib" / host / "bin/rust-lld"
    if not linker.is_file():
        raise RuntimeError("native Rust ELF linker is missing")
    (cache / "bin").mkdir(exist_ok=True)
    linker_name = cache / "bin/ld.lld"
    if linker_name.is_symlink():
        linker_name.unlink()
    if linker_name.exists():
        raise RuntimeError("refusing to replace a non-symlink linker in the Android cache")
    linker_name.symlink_to(linker)
    api = json.loads((ROOT / "scripts/android-upstream.json").read_text())["api"]
    lib = cache / f"sysroot/usr/lib/aarch64-linux-android/{api}"
    compiler = [args.cc, f"--target=aarch64-linux-android{api}", f"--sysroot={cache / 'sysroot'}", f"--ld-path={linker_name}", "-O2", "-mno-outline-atomics", "-Wall", "-Wextra", "-Werror", "-Wno-misleading-indentation", "-I", str(HEADERS), "-I", str(cache / "platform-headers"), "-I", str(ROOT / "accelerator/vulkan")]
    link = ["-L", str(lib), "-Wl,--no-undefined", "-Wl,--build-id=sha1", "-Wl,-z,noexecstack"]
    commands = [compiler + ["-fPIC", "-fvisibility=hidden", "-shared", "-nostdlib", str(ROOT / "accelerator/android/vulkan.c"), str(ROOT / "accelerator/vulkan/calls.S")] + link + ["-Wl,-soname,vulkan.trames.so", "-lc", "-ldl", "-lm", "-o", str(output / "vulkan.trames.so")]]
    for source, name, libraries in [("vulkan-compute.c", "vulkan-compute", ["-lvulkan"]), ("android-vulkan-hal.c", "vulkan-hal-probe", [])]:
        commands.append(compiler + ["-fPIE", "-pie", "-nostdlib", str(lib / "crtbegin_dynamic.o"), str(ROOT / "tests/fixtures" / source)] + link + ["-Wl,--dynamic-linker,/system/bin/linker64", "-lc", "-ldl", "-lm"] + libraries + [str(lib / "crtend_android.o"), "-o", str(output / name)])
    for command in commands:
        subprocess.run(command, check=True, cwd=ROOT)
    if args.shader:
        shutil.copyfile(args.shader, output / "vulkan-compute.spv")
    sources = [ROOT / "accelerator/android/vulkan.c", ROOT / "scripts/android-upstream.json", ROOT / "scripts/prepare-android.py", Path(__file__).resolve(), ROOT / "tests/fixtures/vulkan-compute.c", ROOT / "tests/fixtures/android-vulkan-hal.c"]
    sources += sorted(p for p in (ROOT / "accelerator/vulkan").iterdir() if p.suffix in (".c", ".inc", ".S"))
    manifest = dict(api=api, abi="arm64-v8a", hal="/vendor/lib64/hw/vulkan.trames.so", property="ro.hardware.vulkan=trames", wire_profile="0x10006", wire_revision=20, scope="headless Android Vulkan HAL; no Android native-buffer/surface advertisement", compiler=subprocess.check_output([args.cc, "--version"], text=True).splitlines()[0], rust=rust_version, commands=commands, sources_sha256={str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest() for p in sources}, outputs_sha256={p.name: hashlib.sha256(p.read_bytes()).hexdigest() for p in output.iterdir() if p.name in ["vulkan.trames.so", "vulkan-compute", "vulkan-hal-probe", "vulkan-compute.spv"]})
    (output / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    shutil.copyfile(ROOT / "LICENSE", output / "GPL-3.0.txt")
    print(json.dumps(dict(output=str(output), artifacts=manifest["outputs_sha256"]), indent=2))


if __name__ == "__main__":
    main()
