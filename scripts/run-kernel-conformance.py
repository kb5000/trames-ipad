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


def parse_results(text):
    """Recover libtest records even when worker stderr interrupts a result line."""
    prefixes = list(re.finditer(r'\btest ([A-Za-z_]\w*(?:::[A-Za-z_]\w*)*) \.\.\. ', text))
    completed = {}
    for index, match in enumerate(prefixes):
        block = text[match.end():prefixes[index + 1].start() if index + 1 < len(prefixes) else len(text)]
        block = re.split(r'\n(?:\n)?(?:failures:|test result:)', block, maxsplit=1)[0]
        statuses = re.findall(r'(?<![A-Za-z_])(ok|FAILED|ignored)(?=[\s,]|trames pid|$)', block)
        if len(statuses) != 1 or match[1] in completed:
            raise ValueError('ambiguous or incomplete libtest result: ' + match[1])
        completed[match[1]] = statuses[0]
    summaries = re.findall(r'^test result:.*$', text, re.M)
    counts = re.findall(r'^test result: (?:ok|FAILED)\. (\d+) passed; (\d+) failed; (\d+) ignored;', text, re.M)
    if len(counts) == 1:
        actual = tuple(list(completed.values()).count(status) for status in ('ok', 'FAILED', 'ignored'))
        if actual != tuple(map(int, counts[0])):
            raise ValueError('named results do not match the libtest summary')
    elif completed:
        raise ValueError('missing or multiple libtest summaries')
    return completed, summaries


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
    with args.output.open('wb') as log:
        process = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        for line in process.stdout:
            log.write(line)
            sys.stdout.buffer.write(line)
            sys.stdout.buffer.flush()
        code = process.wait()
    parse_error = None
    try:
        completed, summaries = parse_results(args.output.read_text(errors='replace'))
    except ValueError as error:
        completed, summaries = {}, []
        parse_error = str(error)
    passed = (code == 0 and 'ok' in completed.values() and 'FAILED' not in completed.values()
              and (not cases or (set(completed) == set(cases) and set(completed.values()) == {'ok'})))
    result = dict(passed=passed, command=command, requested=cases, cargo_exit_code=code,
                  cases=completed, summary=summaries, parse_error=parse_error)
    args.output.with_suffix('.json').write_text(json.dumps(result, indent=2) + '\n')
    if not passed:
        print('Conformance failed or exact selection did not execute every requested case.', file=sys.stderr)
    return 0 if passed else (code or 2)


if __name__ == '__main__':
    raise SystemExit(main())
