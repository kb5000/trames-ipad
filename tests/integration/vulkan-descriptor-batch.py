#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Run actual Linux ICD serialization against a recording host-call boundary."""
import os
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
headers = root / 'target/vulkan-upstream/headers/Vulkan-Headers-cfebfc96b2b0bce93da7d12f2c14cc01793ae25c/include'
with tempfile.TemporaryDirectory(prefix='trames-descriptor-batch-') as directory:
    work = Path(directory)
    # Linux ICD logic is host-testable; macOS has no sys/auxv.h. The fixture
    # supplies getauxval and NativeCall, never opens a GPU or changes system auxv.
    (work / 'sys').mkdir()
    (work / 'sys/auxv.h').write_text('unsigned long getauxval(unsigned long);\n')
    subprocess.run([
        os.environ.get('CC', 'cc'), '-O2', '-Wall', '-Wextra', '-Werror',
        '-Wno-misleading-indentation', '-I', str(work), '-I', str(headers),
        str(root / 'tests/fixtures/vulkan-descriptor-batch.c'),
        '-pthread', '-o', str(work / 'test'),
    ], check=True)
    subprocess.run([str(work / 'test')], check=True)
