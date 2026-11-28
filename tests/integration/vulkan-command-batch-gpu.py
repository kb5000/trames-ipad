#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Compare pre-HLT packets with an older immediate ICD on the same real GPU.

Inputs are the original Alpine rootfs/loader, current compute/render fixtures
and an immediate ICD saved before rebuilding. No mock/native-driver substitution.
"""
import argparse
import json
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--runner', type=Path, default=root / 'target/release/trames-run')
parser.add_argument('--baseline', type=Path, required=True)
parser.add_argument('--inputs', type=Path, required=True)
parser.add_argument('--loader', type=Path, required=True)
parser.add_argument('--rootfs', type=Path, default=root / 'target/vulkan-guest/acceptance/alpine-minirootfs.tar.gz')
parser.add_argument('--output', type=Path, required=True)
args = parser.parse_args()
args.output.mkdir(parents=True, exist_ok=True)
results = []
cases = [('compute', ''), ('compute', 'coherent'), ('compute', 'batch-template'),
         ('render', 'compatible-dynamic'), ('render', 'clear-attachments')]
with tempfile.TemporaryDirectory(prefix='trames-command-gpu-') as directory:
    work = Path(directory)
    for cpus in (1, 4):
        base = [str(args.runner), '--cpus', str(cpus), '--vulkan', '--metrics',
                '--max-instructions', '10000000000']
        for kind, mode in cases:
            pair = []
            for label, icd in [('immediate', args.baseline),
                               ('packet', args.inputs / 'libtrames_vulkan_icd.so')]:
                name = f'{cpus}-{kind}-{mode or "default"}-{label}'
                command = [*base, '--storage', str(work / name), '--rootfs', str(args.rootfs),
                           '--env', 'VK_DRIVER_FILES=/usr/share/vulkan/icd.d/trames.json']
                for guest, host in [
                    ('/usr/lib/libvulkan.so.1', args.loader),
                    ('/usr/lib/libtrames_vulkan_icd.so', icd),
                    ('/usr/share/vulkan/icd.d/trames.json', args.inputs / 'trames.json'),
                    ('/tmp/compute.spv', args.inputs / 'compute.spv'),
                    ('/tmp/render.vert.spv', args.inputs / 'render.vert.spv'),
                    ('/tmp/render.frag.spv', args.inputs / 'render.frag.spv'),
                ]:
                    command += ['--file', f'{guest}={host}']
                command += ['--exec-file', f'/tmp/probe={args.inputs / ("vulkan-" + kind)}']
                image = args.output / f'{name}.ppm'
                if kind == 'render':
                    command += ['--export', f'/tmp/frame.ppm={image}']
                    guest_args = ['/tmp/render.vert.spv', '/tmp/render.frag.spv', '/tmp/frame.ppm']
                else:
                    guest_args = ['/tmp/compute.spv']
                if mode:
                    guest_args.append(mode)
                process = subprocess.run([*command, '--guest', '/tmp/probe', *guest_args],
                                         capture_output=True, timeout=120)
                (args.output / f'{name}.stdout').write_bytes(process.stdout)
                (args.output / f'{name}.stderr').write_bytes(process.stderr)
                assert process.returncode == 0, (name, process.returncode, process.stderr[-4000:])
                assert f'vulkan-{kind}-ok'.encode() in process.stdout, name
                metrics = json.loads(next(line.removeprefix('trames metrics: ')
                                          for line in process.stderr.decode().splitlines()
                                          if line.startswith('trames metrics: ')))
                pair.append((process.stdout, image.read_bytes() if kind == 'render' else None,
                             metrics['worker_gpu_calls']))
            assert pair[0][:2] == pair[1][:2], (cpus, kind, mode, 'GPU output changed')
            assert pair[1][2] < pair[0][2], (cpus, kind, mode, pair)
            record = dict(cpus=cpus, kind=kind, mode=mode or 'default',
                          immediate_calls=pair[0][2], packet_calls=pair[1][2],
                          identical_output=True)
            results.append(record)
            print(json.dumps(record), flush=True)
(args.output / 'results.json').write_text(json.dumps(results, indent=2) + '\n')
