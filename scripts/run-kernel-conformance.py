#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Run the full conformance target or exact named cases; reject empty matches."""
import argparse
import json
import os
from pathlib import Path
import re
import subprocess
import sys


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cases', default=os.environ.get('CONFORMANCE_TEST_FILTER', ''))
    parser.add_argument('--target')
    parser.add_argument('--manifest-path')
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    cases = list(dict.fromkeys(re.split(r'[,\s]+', args.cases.strip()))) if args.cases.strip() else []
    if any(not re.fullmatch(r'[A-Za-z_]\w*(?:::[A-Za-z_]\w*)*', name, re.ASCII) for name in cases):
        parser.error('cases must be exact Rust test names separated by commas or whitespace')
    command = ['cargo', 'test', '-p', 'trames-core', '--test', 'kernel-conformance', '--locked']
    if args.target:
        command += ['--target', args.target]
    if args.manifest_path:
        command += ['--manifest-path', args.manifest_path]
    if cases:
        command += ['--', '--exact', *cases]
    args.output.parent.mkdir(parents=True, exist_ok=True)
    completed = {}
    summaries = []
    with args.output.open('wb') as log:
        process = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        for line in process.stdout:
            log.write(line)
            sys.stdout.buffer.write(line)
            sys.stdout.buffer.flush()
            text = line.decode(errors='replace').strip()
            case = re.fullmatch(r'test (.+?) \.\.\. (ok|FAILED|ignored).*', text)
            if case:
                completed[case[1]] = case[2]
            if text.startswith('test result:'):
                summaries.append(text)
        code = process.wait()
    passed = (code == 0 and 'ok' in completed.values() and 'FAILED' not in completed.values()
              and (not cases or (set(completed) == set(cases) and set(completed.values()) == {'ok'})))
    result = dict(passed=passed, command=command, requested=cases, cargo_exit_code=code,
                  cases=completed, summary=summaries)
    args.output.with_suffix('.json').write_text(json.dumps(result, indent=2) + '\n')
    if not passed:
        print('Conformance failed or exact selection did not execute every requested case.', file=sys.stderr)
    return 0 if passed else (code or 2)


if __name__ == '__main__':
    raise SystemExit(main())
