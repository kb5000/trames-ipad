#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Reject stale iOS drivers and incomplete/mismatched private Mesa ABI pairs."""
import hashlib
import importlib.util
import json
from pathlib import Path
import tempfile

root = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location('ios_artifact', root / 'scripts/check-ios-guest.py')
artifact = importlib.util.module_from_spec(spec)
spec.loader.exec_module(artifact)
digest = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()
mesa = artifact.module('build-zink-gallium')
with tempfile.TemporaryDirectory(prefix='trames-ios-graphics-') as temporary:
    output = Path(temporary)
    (output / 'libtrames_vulkan_icd.so').write_bytes(b'fixture ICD')
    driver = {'wire_revision': 20, 'icd_sha256': digest(output / 'libtrames_vulkan_icd.so'),
              'build_recipe_sha256': digest(root / 'scripts/build-vulkan-guest.sh'),
              'source_sha256': {str(p.relative_to(root)): digest(p)
                                for p in (root / 'accelerator/vulkan').iterdir()
                                if p.suffix in ('.c', '.inc', '.S')}}
    (output / 'manifest.json').write_text(json.dumps(driver))
    names = [('libEGL.so.1', 'egl_sha256'), ('libgallium-' + mesa.VERSION + '.so', 'gallium_sha256')]
    for name, _ in names:
        (output / name).write_bytes(name.encode())
    manifest = {'mesa': mesa.VERSION, 'source_sha256': mesa.SOURCE_SHA,
                'patches': {name: digest(root / 'patches/mesa' / name) for name in mesa.PATCHES},
                **{key: digest(output / name) for name, key in names}}
    path = output / 'egl-manifest.json'
    path.write_text(json.dumps(manifest))
    artifact.check(output)
    for change in ('missing-gallium', 'different-egl', 'different-gallium', 'unpatched-mesa'):
        for name, _ in names:
            (output / name).write_bytes(name.encode())
        current = dict(manifest)
        if change == 'missing-gallium':
            (output / names[1][0]).unlink()
        elif change == 'unpatched-mesa':
            current['patches'] = {}
        else:
            (output / names[0 if change == 'different-egl' else 1][0]).write_bytes(b'mismatched library')
        path.write_text(json.dumps(current))
        try:
            artifact.check(output)
        except (OSError, ValueError):
            continue
        raise AssertionError('invalid iOS graphics payload admitted: ' + change)
    native = artifact.module('build-moltenvk')
    directory = output / 'aarch64-apple-ios'
    directory.mkdir()
    library = directory / 'libtrames_ffi.a'
    library.write_bytes(b'fixture native Rust archive')
    inputs = {name.lower() + '_sha256': digest(value)
              for name, value in vars(native).items()
              if name == 'PATCH' or name.endswith('_PATCH')}
    inputs['transform_feedback_patch_sha256'] = inputs.pop('tf_patch_sha256')
    inputs['cross_dispatch_patch_sha256'] = inputs.pop('cross_patch_sha256')
    inputs.update(recipe_sha256=digest(root / 'scripts/build-moltenvk.py'),
                  manifest_sha256=digest(native.MANIFEST), arch='arm64', platform='ios',
                  transform_feedback_sources={p.name: digest(p)
                                              for p in native.NATIVE_TF.iterdir() if p.is_file()})
    record = {'inputs': inputs, 'rust_sha256': digest(library), 'rust_source_sha256': artifact.rust_sources()}
    path = directory / 'native-vulkan.json'
    path.write_text(json.dumps(record))
    artifact.check_native(directory)
    for change in ('old-recipe', 'missing-host-image-fix', 'changed-rust-source', 'unmatched-rust-archive'):
        current = dict(record, inputs=dict(inputs))
        if change == 'old-recipe':
            current['inputs']['recipe_sha256'] = 'old'
        elif change == 'missing-host-image-fix':
            current['inputs'].pop('host_image_patch_sha256')
        elif change == 'changed-rust-source':
            current['rust_source_sha256'] = {}
        else:
            current['rust_sha256'] = 'different archive'
        path.write_text(json.dumps(current))
        try:
            artifact.check_native(directory)
        except ValueError:
            continue
        raise AssertionError('stale iOS native library admitted: ' + change)
    video = output / 'video'
    video.mkdir()
    (video / 'trames_drv_video.so').write_bytes(b'fixture video driver')
    driver = {'wire_revision': 2, 'needed': 'libc.musl-aarch64.so.1',
              'sha256': digest(video / 'trames_drv_video.so'),
              'build_recipe_sha256': digest(root / 'runtime/scripts/build-video-guest.sh'),
              'source_sha256': {str(p.relative_to(root / 'runtime')): digest(p)
                                for p in (root / 'runtime/crates/trames-video/guest').iterdir()
                                if p.suffix in ('.c', '.h', '.S')}}
    path = video / 'manifest.json'
    path.write_text(json.dumps(driver))
    names = ['libavcodec.so.61', 'libavutil.so.59', 'libswresample.so.5', 'libswscale.so.8']
    for name in names:
        (video / name).write_bytes(name.encode())
    ffmpeg = {'bitstream_protocol': 1, 'libavcodec_major': 61,
              'patch_sha256': digest(root / 'runtime/patches/ffmpeg/0001-vaapi-original-bitstream-v1.patch'),
              'libraries': {name: digest(video / name) for name in names}}
    (video / 'ffmpeg-manifest.json').write_text(json.dumps(ffmpeg))
    artifact.check_video(video)
    for change in ('old-video-wire', 'wrong-libc', 'changed-video-driver', 'changed-codec'):
        current = dict(driver)
        if change == 'old-video-wire':
            current['wire_revision'] = 1
        elif change == 'wrong-libc':
            current['needed'] = 'libc.so.6'
        elif change == 'changed-video-driver':
            current['sha256'] = 'old'
        else:
            (video / names[0]).write_bytes(b'wrong FFmpeg')
        path.write_text(json.dumps(current))
        try:
            artifact.check_video(video)
        except ValueError:
            continue
        raise AssertionError('stale iOS video library admitted: ' + change)
print('iOS guest/native graphics and paired VA-API/FFmpeg artifact checks passed')
