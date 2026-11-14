#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Build/stage the iOS Rust archive with a source-checked native Vulkan record."""
import argparse
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('target', choices=['aarch64-apple-ios', 'aarch64-apple-ios-sim'])
parser.add_argument('moltenvk', type=Path)
args = parser.parse_args()
spec = importlib.util.spec_from_file_location('payload', ROOT / 'scripts/check-ios-guest.py')
payload = importlib.util.module_from_spec(spec)
spec.loader.exec_module(payload)
digest = lambda path: hashlib.sha256(path.read_bytes()).hexdigest()
native = args.moltenvk.resolve()
record = json.loads((native / 'build.json').read_text())
payload.check_native_inputs(record['inputs'], args.target)
if digest(native / 'libMoltenVK.a') != record['library_sha256']:
    parser.error('native MoltenVK binary does not match its build record')
environment = dict(os.environ, TRAMES_MOLTENVK_STATIC_DIR=str(native), IPHONEOS_DEPLOYMENT_TARGET='17.0')
sdk = 'iphonesimulator' if args.target.endswith('-sim') else 'iphoneos'
environment['SDKROOT'] = subprocess.check_output(['xcrun', '--sdk', sdk, '--show-sdk-path'],
                                               env=environment, text=True).strip()
subprocess.run(['cargo', 'build', '-p', 'trames-ffi', '--release', '--target', args.target, '--locked'],
               cwd=ROOT, env=environment, check=True)
output = ROOT / 'target/ios-link' / args.target
output.mkdir(parents=True, exist_ok=True)
library = output / 'libtrames_ffi.a'
shutil.copyfile(ROOT / 'target' / args.target / 'release/libtrames_ffi.a', library)
record['rust_sha256'] = digest(library)
record['rust_source_sha256'] = payload.rust_sources()
(output / 'native-vulkan.json').write_text(json.dumps(record, indent=2) + '\n')
payload.check_native(output)
print('Staged source-checked iOS Rust archive:', output)
