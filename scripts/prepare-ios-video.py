#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Stage verified musl VA-API and ABI-compatible FFmpeg decoder resources."""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import shutil

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('driver', type=Path)
parser.add_argument('ffmpeg', type=Path)
parser.add_argument('--output', type=Path, default=ROOT / 'target/vulkan-guest/Video')
args = parser.parse_args()
digest = lambda path: hashlib.sha256(path.read_bytes()).hexdigest()
record = json.loads((args.ffmpeg / 'manifest.json').read_text())
out = args.output
out.mkdir(parents=True, exist_ok=True)
for name in ['trames_drv_video.so', 'manifest.json', 'GPL-3.0.txt']:
    shutil.copyfile(args.driver / name, out / name)
libraries = {}
for soname in ['libavcodec.so.61', 'libavutil.so.59', 'libswresample.so.5', 'libswscale.so.8']:
    source = (args.ffmpeg / 'lib' / soname).resolve()
    if record['libraries'].get(source.name) != digest(source):
        parser.error('FFmpeg decoder binary does not match its manifest: ' + soname)
    shutil.copyfile(source, out / soname)
    libraries[soname] = digest(source)
record['libraries'] = libraries
(out / 'ffmpeg-manifest.json').write_text(json.dumps(record, indent=2) + '\n')
notices = out / 'Notices'
notices.mkdir(exist_ok=True)
shutil.copyfile(args.ffmpeg / 'COPYING.LGPLv2.1', notices / 'FFmpeg-LGPL-2.1.txt')
shutil.copyfile(ROOT / 'runtime/patches/ffmpeg/0001-vaapi-original-bitstream-v1.patch',
                notices / '0001-vaapi-original-bitstream-v1.patch')
spec = importlib.util.spec_from_file_location('payload', ROOT / 'scripts/check-ios-guest.py')
payload = importlib.util.module_from_spec(spec)
spec.loader.exec_module(payload)
payload.check_video(out)
print('Staged paired iOS video payload:', out)
