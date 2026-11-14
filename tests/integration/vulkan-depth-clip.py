#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Check independent depth clip/clamp semantics through the complete guest ICD."""
import importlib.util
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location('m6_repository', ROOT / 'tests/integration/m6.py')
m6 = importlib.util.module_from_spec(spec)
spec.loader.exec_module(m6)
fixtures = Path(os.getenv('TRAMES_DEPTH_CLIP_FIXTURES', ROOT / 'tests/fixtures/bin'))
driver = Path(os.getenv('TRAMES_VULKAN_GUEST_DIR', ROOT / 'target/vulkan-guest'))
runner = os.getenv('TRAMES_RUNNER', str(ROOT / 'target/release/trames-run'))
shaders = ['vulkan-depth-clip.vert.spv', 'vulkan-depth-clip.frag.spv',
           'vulkan-depth-clip-explicit.frag.spv', 'vulkan-depth-clip-early.frag.spv']
reference = subprocess.run([str(fixtures / 'vulkan-depth-clip-host'),
                            *[str(fixtures / s) for s in shaders]], capture_output=True, timeout=120)
assert reference.returncode == 0 and b'vulkan-depth-clip-ok: 432 renders;' in reference.stdout, reference
assert b'MVK-BUILD-ERROR' not in reference.stdout + reference.stderr, reference
with tempfile.TemporaryDirectory(prefix='trames-depth-clip-') as directory:
    tmp = Path(directory)
    m6.repository(tmp / 'repository', ('vulkan-loader',))
    packages = sorted((tmp / 'repository').rglob('*.apk'))
    base = [runner, '--storage', str(tmp / 'store'), '--cpus', '4', '--vulkan',
            '--max-instructions', '100000000000']
    imports = []
    for package in packages:
        imports += ['--file', '/tmp/' + package.name + '=' + str(package)]
    install = subprocess.run([*base, '--rootfs', str(m6.CACHE / 'alpine-minirootfs-3.22.6-aarch64.tar.gz'),
                              *imports, '--guest', '/sbin/apk', '--repositories-file', '/dev/null',
                              '--no-progress', 'add', *['/tmp/' + p.name for p in packages]],
                             capture_output=True, timeout=180)
    assert install.returncode == 0 and b'ERROR:' not in install.stdout + install.stderr, install
    imports = ['--file', '/usr/lib/libtrames_vulkan_icd.so=' + str(driver / 'libtrames_vulkan_icd.so'),
               '--file', '/usr/share/vulkan/icd.d/trames.json=' + str(driver / 'trames.json'),
               '--exec-file', '/tmp/vulkan-depth-clip=' + str(fixtures / 'vulkan-depth-clip')]
    for shader in shaders:
        imports += ['--file', '/tmp/' + shader + '=' + str(fixtures / shader)]
    result = subprocess.run([*base, '--env', 'VK_DRIVER_FILES=/usr/share/vulkan/icd.d/trames.json',
                             *imports, '--guest', '/tmp/vulkan-depth-clip',
                             *['/tmp/' + s for s in shaders]], capture_output=True, timeout=600)
    assert result.returncode == 0 and b'vulkan-depth-clip-ok: 432 renders;' in result.stdout, result
    assert b'MVK-BUILD-ERROR' not in result.stdout + result.stderr, result
    print(result.stdout.decode(), end='')
print('Native and 4-CPU guest: all 432 color/depth oracles and cache reload passed')
