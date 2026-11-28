#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Exercise the actual ICD's pre-HLT recording with owned, delayed inputs."""
import argparse
import os
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--sanitizers', choices=('address,undefined', 'undefined'),
                    default='address,undefined')
args = parser.parse_args()
headers = root / 'target/vulkan-upstream/headers/Vulkan-Headers-cfebfc96b2b0bce93da7d12f2c14cc01793ae25c/include'
with tempfile.TemporaryDirectory(prefix='trames-command-batch-') as directory:
    work = Path(directory)
    (work / 'sys').mkdir()
    (work / 'sys/auxv.h').write_text('unsigned long getauxval(unsigned long);\n')
    subprocess.run([
        os.environ.get('CC', 'cc'), '-O2', '-Wall', '-Wextra', '-Werror',
        '-Wno-misleading-indentation', '-fsanitize=' + args.sanitizers,
        '-I', str(work), '-I', str(headers),
        str(root / 'tests/fixtures/vulkan-command-batch.c'),
        '-pthread', '-o', str(work / 'test'),
    ], check=True)
    subprocess.run([str(work / 'test')], check=True)
