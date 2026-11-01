#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Real ALSA -> native PCM acceptance; capture retains aggregate levels only.

Build with scripts/build-audio-guest.sh on ARM64 Linux. Requires an existing
isolated Linux store with alsa-lib, and an authorized Mac Dev bundle.
"""
import argparse
import hashlib
import json
from pathlib import Path
import shlex
import subprocess
import uuid


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('runner', 'storage', 'artifacts', 'output'):
        parser.add_argument('--' + name, type=Path, required=True)
    parser.add_argument('--mode', choices=['output', 'capture', 'duplex', 'clock'], default='output')
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    guest = '/opt/trames-audio-check-' + uuid.uuid4().hex
    files = {name: args.artifacts / name for name in ('audio-native', 'libasound_module_pcm_trames.so')}
    # The fixture's config is intentionally isolated from any user ALSA config.
    config = args.output / 'asound.conf'
    config.write_text('pcm.trames { type trames }\npcm.!default { type plug slave.pcm trames }\n')
    files['asound.conf'] = config
    command = [str(args.runner.resolve()), '--storage', str(args.storage.resolve()),
               '--audio', '--no-pty', '--network', 'off', '--cpus', '4', '--memory-mib', 'auto', '--timeout', '25']
    if args.mode in ('capture', 'duplex'):
        command.append('--audio-input')
    for name, file in files.items():
        command.extend(['--exec-file', guest + '/' + name + '=' + str(file.resolve())])
    environment = {'ALSA_PLUGIN_DIR': guest, 'ALSA_CONFIG_PATH': guest + '/asound.conf'}
    script = 'export ' + ' '.join(k + '=' + shlex.quote(v) for k, v in environment.items())
    program = [guest + '/audio-native'] + ([] if args.mode == 'output' else ['--' + args.mode])
    result = subprocess.run(command + ['/bin/sh', '-ec', script + '; exec ' + shlex.join(program)],
                            capture_output=True, timeout=40)
    (args.output / 'stdout.log').write_bytes(result.stdout)
    (args.output / 'stderr.log').write_bytes(result.stderr)
    samples = [json.loads(line) for line in result.stdout.decode().splitlines() if line.startswith('{')]
    report = {'passed': result.returncode == 0, 'returncode': result.returncode, 'mode': args.mode,
              'samples': samples, 'sha256': {n: hashlib.sha256(p.read_bytes()).hexdigest() for n, p in files.items()}}
    (args.output / 'result.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report))
    raise SystemExit(0 if report['passed'] else 1)


if __name__ == '__main__':
    main()
