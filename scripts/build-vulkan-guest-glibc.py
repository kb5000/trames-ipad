#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Cross-compile the Trames Vulkan ICD against Debian 13 glibc (aarch64-linux-gnu)."""
import argparse
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def build(output: Path, glibc_dev: Path, libc: Path, headers: Path) -> Path:
    rust = Path(subprocess.check_output(['rustc', '--print', 'sysroot'], text=True).strip())
    host = next(
        line.split(': ', 1)[1]
        for line in subprocess.check_output(['rustc', '-vV'], text=True).splitlines()
        if line.startswith('host: ')
    )
    linker = rust / 'lib/rustlib' / host / 'bin/rust-lld'
    clang_inc = Path(subprocess.check_output(['clang', '-print-resource-dir'], text=True).strip()) / 'include'
    output.mkdir(parents=True, exist_ok=True)
    target = output / 'libtrames_vulkan_icd_glibc.so'

    with tempfile.TemporaryDirectory() as tmp:
        tmp = Path(tmp)
        uapi = tmp / 'uapi'
        (uapi / 'linux').mkdir(parents=True)
        (uapi / 'linux/errno.h').write_text(
            '#define EPERM 1\n#define ENOENT 2\n#define EINTR 4\n#define EIO 5\n'
            '#define EAGAIN 11\n#define ENOMEM 12\n#define EACCES 13\n#define EFAULT 14\n'
            '#define EBUSY 16\n#define EEXIST 17\n#define EINVAL 22\n#define ENOSYS 38\n'
            '#define ETIMEDOUT 110\n'
        )
        (uapi / 'linux/types.h').write_text(
            '#include <stdint.h>\n'
            'typedef uint8_t __u8;\ntypedef int8_t __s8;\n'
            'typedef uint16_t __u16;\ntypedef int16_t __s16;\n'
            'typedef uint32_t __u32;\ntypedef int32_t __s32;\n'
            'typedef uint64_t __u64;\ntypedef int64_t __s64;\n'
        )
        cflags = [
            'clang', '--target=aarch64-linux-gnu', '-O2', '-fPIC', '-fvisibility=hidden',
            '-Wall', '-Wextra', '-Werror', '-Wno-misleading-indentation',
            '-nostdinc',
            '-isystem', str(clang_inc),
            '-isystem', str(glibc_dev / 'usr/include/aarch64-linux-gnu'),
            '-isystem', str(glibc_dev / 'usr/include'),
            '-isystem', str(uapi),
            '-I', str(headers),
            '-I', str(ROOT / 'accelerator/vulkan'),
        ]
        subprocess.run(cflags + ['-c', str(ROOT / 'accelerator/vulkan/icd.c'), '-o', str(tmp / 'icd.o')], check=True)
        subprocess.run(cflags + ['-c', str(ROOT / 'accelerator/vulkan/calls.S'), '-o', str(tmp / 'calls.o')], check=True)
        subprocess.run([
            str(linker), '-flavor', 'gnu', '-m', 'aarch64linux',
            '-shared', '--build-id=sha1', '-z', 'noexecstack', '--no-undefined',
            str(tmp / 'icd.o'), str(tmp / 'calls.o'), str(libc),
            '-o', str(target),
        ], check=True)
    return target


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=ROOT / 'target/vulkan-guest-glibc')
    parser.add_argument('--glibc-dev', type=Path, default=ROOT / 'target/glibc-hybrid/dev')
    parser.add_argument('--libc', type=Path, default=ROOT / 'target/glibc-hybrid/binary/usr/lib/aarch64-linux-gnu/libc.so.6')
    parser.add_argument(
        '--headers',
        type=Path,
        default=ROOT / 'target/vulkan-upstream/headers/Vulkan-Headers-cfebfc96b2b0bce93da7d12f2c14cc01793ae25c/include',
    )
    args = parser.parse_args()
    built = build(args.output, args.glibc_dev, args.libc, args.headers)
    print(f'Built {built} ({built.stat().st_size} bytes)')


if __name__ == '__main__':
    main()
