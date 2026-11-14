#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Build the small public-ABI Waydroid HWC callback adapter, without AOSP."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tarfile
import urllib.request

ROOT = Path(__file__).resolve().parents[1]


def sha(data):
    return hashlib.sha256(data).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cache', type=Path, default=ROOT / 'target/android-toolchain')
    parser.add_argument('--output', type=Path, default=ROOT / 'target/vulkan-android')
    parser.add_argument('--offline', action='store_true')
    parser.add_argument('--cc', default='clang')
    args = parser.parse_args()
    cache, output = args.cache.resolve(), args.output.resolve()
    prepare = [sys.executable, str(ROOT / 'scripts/prepare-android.py'), '--cache', str(cache)]
    if args.offline:
        prepare.append('--offline')
    subprocess.run(prepare, check=True)
    manifest = json.loads((ROOT / 'scripts/android-upstream.json').read_text())
    pin = manifest['vndk_headers']
    archive = cache / pin['archive']
    if not archive.exists() or sha(archive.read_bytes()) != pin['sha256']:
        if args.offline:
            raise RuntimeError('missing or changed VNDK header snapshot')
        with urllib.request.urlopen(pin['url'], timeout=60) as response:
            data = response.read()
        if sha(data) != pin['sha256']:
            raise RuntimeError('VNDK header snapshot digest mismatch')
        archive.write_bytes(data)
    headers = cache / 'vndk-hwc-headers'
    with tarfile.open(archive) as source:
        for entry in source:
            name = Path(entry.name)
            if name.is_absolute() or '..' in name.parts:
                raise RuntimeError('invalid header path')
            if not entry.isfile() or not str(name).endswith('.h'):
                continue
            if not str(name).startswith(('hardware/libhardware/include/', 'system/core/libcutils/include/', 'system/core/libsystem/include/')):
                continue
            target = headers / name
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(source.extractfile(entry).read())
    output.mkdir(parents=True, exist_ok=True)
    rustroot = Path(subprocess.check_output(['rustc', '--print', 'sysroot'], text=True).strip())
    host = subprocess.check_output(['rustc', '-vV'], text=True).split('host: ')[1].splitlines()[0]
    # Clang's ELF linker driver selects its flavor by the final path component.
    linkdir = output / 'hwc-link-inputs'
    linkdir.mkdir(exist_ok=True)
    elf_linker = linkdir / 'ld.lld'
    if elf_linker.is_symlink():
        elf_linker.unlink()
    elf_linker.symlink_to(rustroot / 'lib/rustlib' / host / 'bin/rust-lld')
    sysroot = cache / 'sysroot'
    lib = sysroot / f"usr/lib/aarch64-linux-android/{manifest['api']}"
    source = ROOT / 'accelerator/android/hwcomposer.c'
    command = [args.cc, f"--target=aarch64-linux-android{manifest['api']}",
               f'--sysroot={sysroot}', f'--ld-path={elf_linker}', '-std=c11',
               '-O2', '-mno-outline-atomics', '-Wall', '-Wextra', '-Werror',
               '-fPIC', '-shared', '-nostdlib', str(lib / 'crtbegin_so.o')]
    for include in ['hardware/libhardware/include', 'system/core/libcutils/include', 'system/core/libsystem/include']:
        command.extend(['-isystem', str(headers / include)])
    command.extend([str(source), '-L', str(lib), '-Wl,--no-undefined',
                    '-Wl,--build-id=sha1', '-Wl,-z,noexecstack', '-lc', '-ldl',
                    str(lib / 'crtend_so.o'), '-o', str(output / 'hwcomposer.trames.so')])
    subprocess.run(command, check=True)
    record = dict(api=manifest['api'], abi='arm64-v8a', headers=pin, command=command,
                  sources={p.name: sha(p.read_bytes()) for p in [source, source.with_name('hwc-events.h'), source.with_name('hwc-crop.h'), source.with_name('buffer.h')]},
                  output_sha256=sha((output / 'hwcomposer.trames.so').read_bytes()))
    (output / 'hwcomposer-manifest.json').write_text(json.dumps(record, indent=2) + '\n')
    print(json.dumps(record))


if __name__ == '__main__':
    main()
