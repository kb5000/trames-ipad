#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Validate the actual iOS resource payload, including paired Mesa libraries."""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def module(name):
    spec = importlib.util.spec_from_file_location(name, ROOT / 'scripts' / (name + '.py'))
    result = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(result)
    return result


def check(directory):
    module('check-guest-vulkan').check(ROOT, directory)
    manifest = json.loads((directory / 'egl-manifest.json').read_text())
    digest = lambda path: hashlib.sha256(path.read_bytes()).hexdigest()
    mesa = module('build-zink-gallium')
    patches = {name: digest(ROOT / 'patches/mesa' / name) for name in mesa.PATCHES}
    if (manifest.get('mesa') != mesa.VERSION or manifest.get('source_sha256') != mesa.SOURCE_SHA
            or manifest.get('patches') != patches):
        raise ValueError('Stale/unpaired iOS Mesa payload; run scripts/build-zink-egl.py')
    for name, key in [('libEGL.so.1', 'egl_sha256'),
                      ('libgallium-' + mesa.VERSION + '.so', 'gallium_sha256')]:
        if manifest.get(key) != digest(directory / name):
            raise ValueError('iOS Mesa binary does not match its paired manifest: ' + name)


def check_native_inputs(inputs, target):
    native = module('build-moltenvk')
    digest = lambda path: hashlib.sha256(path.read_bytes()).hexdigest()
    aliases = {'TF_PATCH': 'transform_feedback_patch_sha256',
               'CROSS_PATCH': 'cross_dispatch_patch_sha256'}
    expected = {aliases.get(name, name.lower() + '_sha256'): digest(value)
                for name, value in vars(native).items()
                if name == 'PATCH' or name.endswith('_PATCH')}
    expected.update(recipe_sha256=digest(ROOT / 'scripts/build-moltenvk.py'),
                    manifest_sha256=digest(native.MANIFEST), metal_private_api=False,
                    transform_feedback_sources={p.name: digest(p)
                                                for p in sorted(native.NATIVE_TF.iterdir()) if p.is_file()},
                    platform='ios-simulator' if target.endswith('-sim') else 'ios', arch='arm64')
    if any(inputs.get(key) != value for key, value in expected.items()):
        raise ValueError('Stale native iOS MoltenVK; rebuild with scripts/build-moltenvk.py')


def check_native(directory):
    record = json.loads((directory / 'native-vulkan.json').read_text())
    check_native_inputs(record['inputs'], directory.name)
    if record['rust_sha256'] != hashlib.sha256((directory / 'libtrames_ffi.a').read_bytes()).hexdigest():
        raise ValueError('iOS Rust archive does not match its native Vulkan build record')
    if record.get('rust_source_sha256') != rust_sources():
        raise ValueError('Stale iOS Rust archive; run scripts/stage-ios-rust.py')


def rust_sources():
    paths = {ROOT / name for name in ['Cargo.toml', 'Cargo.lock', 'rust-toolchain.toml',
                                     'runtime/Cargo.toml', 'runtime/Cargo.lock']}
    for folder in ['crates', 'runtime/crates', 'runtime/vendor', '.cargo']:
        paths.update(p for p in (ROOT / folder).rglob('*')
                     if p.suffix in ('.rs', '.toml', '.lock', '.json', '.c', '.h', '.m', '.mm', '.S'))
    return {str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest()
            for p in sorted(paths) if p.is_file()}


def check_video(directory):
    digest = lambda path: hashlib.sha256(path.read_bytes()).hexdigest()
    runtime = ROOT / 'runtime'
    driver = json.loads((directory / 'manifest.json').read_text())
    sources = {str(p.relative_to(runtime)): digest(p)
               for p in (runtime / 'crates/trames-video/guest').iterdir()
               if p.suffix in ('.c', '.h', '.S')}
    if (driver.get('wire_revision') != 2 or driver.get('source_sha256') != sources
            or driver.get('build_recipe_sha256') != digest(runtime / 'scripts/build-video-guest.sh')
            or 'libc.musl-aarch64.so.1' not in driver.get('needed', '')
            or driver.get('sha256') != digest(directory / 'trames_drv_video.so')):
        raise ValueError('Stale iOS VA-API driver; rebuild runtime/scripts/build-video-guest.sh')
    ffmpeg = json.loads((directory / 'ffmpeg-manifest.json').read_text())
    if (ffmpeg.get('bitstream_protocol') != 1 or ffmpeg.get('libavcodec_major') != 61
            or ffmpeg.get('patch_sha256') != digest(runtime / 'patches/ffmpeg/0001-vaapi-original-bitstream-v1.patch')):
        raise ValueError('Stale/unpaired iOS FFmpeg original-bitstream adapter')
    names = {'libavcodec.so.61', 'libavutil.so.59', 'libswresample.so.5', 'libswscale.so.8'}
    if set(ffmpeg.get('libraries', {})) != names:
        raise ValueError('Incomplete iOS FFmpeg decoder payload')
    for name in names:
        if ffmpeg['libraries'][name] != digest(directory / name):
            raise ValueError('iOS FFmpeg binary does not match its manifest: ' + name)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('directory', type=Path, nargs='?',
                        default=ROOT / 'apps/ios/Trames/GuestVulkan')
    parser.add_argument('--rust-dir', type=Path, help='also verify the staged native Rust archive')
    parser.add_argument('--video-dir', type=Path, help='also verify paired VA-API/FFmpeg resources')
    args = parser.parse_args()
    try:
        check(args.directory.resolve())
        if args.rust_dir:
            check_native(args.rust_dir.resolve())
        if args.video_dir:
            check_video(args.video_dir.resolve())
    except (OSError, ValueError, KeyError) as error:
        parser.exit(1, str(error) + '\n')
