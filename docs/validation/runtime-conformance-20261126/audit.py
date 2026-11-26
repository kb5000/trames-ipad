#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Verify source-bound conformance evidence without builds or guest execution."""
from pathlib import Path
import gzip
import hashlib
import importlib.util
import json
import re
import tarfile
import tempfile

HERE = Path(__file__).resolve().parent

def load(path):
    return json.loads(path.read_text())

def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def cases(path):
    text = gzip.open(path, 'rt').read() if path.suffix == '.gz' else path.read_text()
    return parser.parse_results(text)[0]


def outside_functions(text, names):
    # These four retained fixture bodies have balanced braces in their strings.
    spans = []
    for name in names:
        start = text.index('fn ' + name + '(')
        end = text.index('{', start) + 1
        depth = 1
        while depth:
            depth += (text[end] == '{') - (text[end] == '}')
            end += 1
        spans.append((start, end))
    for start, end in sorted(spans, reverse=True):
        text = text[:start] + text[end:]
    return text

index = load(HERE / 'evidence-index.json')
for name, expected in index['files'].items():
    assert digest(HERE / name) == expected, name
spec = importlib.util.spec_from_file_location('frozen_conformance_parser', HERE / 'log-parser.py')
parser = importlib.util.module_from_spec(spec)
spec.loader.exec_module(parser)
scope = load(HERE / 'source-scope.json')
assert len(scope['runtime_test_delta']) == 16
assert set(scope['runtime_test_delta']) == set(scope['source_hashes']['final'])
assert len(set(scope['affected_cases'])) == 4
assert set(scope['followup_delta']) == {'crates/trames-session/src/' + n for n in scope['changed_functions']}
with tempfile.TemporaryDirectory(prefix='trames-conformance-audit-') as temp:
    root = Path(temp)
    with tarfile.open(HERE / 'raw-evidence.tar.gz') as archive:
        members = archive.getmembers()
        assert len(members) < 1000 and sum(m.size for m in members) < 32 * 1024 * 1024
        for member in members:
            p = Path(member.name)
            assert not p.is_absolute() and '..' not in p.parts
            assert member.isfile() or member.isdir()
        archive.extractall(root)
    assert {str(p.relative_to(root)) for p in root.rglob('*') if p.is_file()} == set(index['raw_files'])
    for name, expected in index['raw_files'].items():
        assert digest(root / name) == expected, name
    for variant in ('parent', 'full', 'final'):
        for path, expected in scope['source_hashes'][variant].items():
            assert digest(root / 'sources' / variant / Path(path).name) == expected
    for entry in scope['production_prefixes']:
        before = (root / 'sources/parent' / entry['path']).read_bytes()
        after = (root / 'sources/final' / entry['path']).read_bytes()
        marker = b'\n#[cfg(all(test, trames_interpreter_conformance))]\nmod tests'
        assert marker in before and before.split(marker)[0] == after.split(marker)[0]
        assert hashlib.sha256(before.split(marker)[0]).hexdigest() == entry['prefix_sha256']
    process = (root / 'sources/final/process.rs').read_text()
    prefix_paths = {p['path'] for p in scope['production_prefixes']}
    for path in scope['source_hashes']['final']:
        name = Path(path).name
        if name not in prefix_paths:
            assert '#[cfg(all(test, trames_interpreter_conformance))]\n#[path = "' + name + '"]' in process
    for name, functions in scope['changed_functions'].items():
        before = (root / 'sources/full' / name).read_text()
        after = (root / 'sources/final' / name).read_text()
        assert outside_functions(before, functions) == outside_functions(after, functions)

    cloud = load(HERE / 'cloud-checks.json')
    baseline = cloud['baseline']; exact = cloud['exact']
    assert baseline['head_sha'] == scope['ipad_full'] and baseline['conclusion'] == 'failure'
    assert exact['head_sha'] == scope['ipad_exact'] and exact['conclusion'] == 'success'
    assert len(exact['jobs']) == 2 and all(j['conclusion'] == 'success' for j in exact['jobs'])
    assert next(j for j in baseline['jobs'] if 'macos-15' in j['name'])['conclusion'] == 'success'
    selected = set(scope['affected_cases'])
    counts = {}
    old_failures = load(root / 'historical/040b-failure.json')['failed_cases']
    retirement_failure = load(root / 'historical/66cb-failure.json')['failed_cases']
    assert len(set(old_failures)) == 18 and len(retirement_failure) == 1
    for os_name in ('ubuntu-24.04', 'macos-15'):
        base = root / 'cloud-baseline' / os_name
        fix = root / 'cloud-exact' / os_name
        for folder, ipad, runtime in [(base, scope['ipad_full'], scope['runtime_full']), (fix, scope['ipad_exact'], scope['runtime'])]:
            for filename, expected in [('ipad.commit', ipad), ('runtime.commit', runtime), ('cli.commit', scope['cli'])]:
                assert (folder / filename).read_text().strip() == expected
        for lane in (['native', 'qemu'] if os_name == 'ubuntu-24.04' else ['native']):
            old = cases(base / (lane + '.log.gz'))
            new = cases(fix / (lane + '.log.gz'))
            assert len(old) == 545 and list(old.values()).count('ignored') == 3
            assert set(new) == selected and set(new.values()) == {'ok'}
            launcher = load(fix / (lane + '.json'))
            assert launcher['passed'] and launcher['cargo_exit_code'] == 0
            assert set(launcher['requested']) == selected and launcher['cases'] == new
            assert all(status == 'ok' for name, status in old.items() if name not in selected and status != 'ignored')
            if lane == 'qemu':
                assert {name for name, status in old.items() if status == 'FAILED'} == {scope['affected_cases'][0]}
                assert all(old[n] == 'ok' for n in old_failures + retirement_failure)
            else:
                assert list(old.values()).count('ok') == 542
            assert sum(name not in selected and status == 'ok' for name, status in old.items()) == 538
            old.update(new)
            assert len(old) == 545 and list(old.values()).count('ok') == 542
            assert list(old.values()).count('ignored') == 3
            counts[os_name + '/' + lane] = dict(reused_passed=538, rechecked_passed=4, ignored=3)
    local = load(root / 'followup-local/linux/ipad-ci-v4-source.json')
    for path, expected in scope['source_hashes']['final'].items():
        assert local['files']['runtime/' + path] == expected
    # The full local QEMU pass and earlier incomplete diagnostic remain distinct.
    assert load(root / 'linux/ipad-ci-v3-full-qemu.json')['code'] == 0
    assert load(root / 'linux/ipad-ci-qualified-qemu.json')['code'] == -15
    for path in [root / 'followup-local/mac/mac-v4-exact.json', root / 'followup-local/linux/ipad-ci-v4-native-exact.json', root / 'followup-local/linux/ipad-ci-v4-qemu-exact.json']:
        data = load(path)
        assert data['passed'] and set(data['cases']) == selected and set(data['cases'].values()) == {'ok'}
    negative = load(root / 'followup-local/mac/zero-match.json')
    assert negative['passed'] is False and negative['cargo_exit_code'] == 0 and not negative['cases']
    assert 'cases must be exact Rust test names' in (root / 'followup-local/mac/rejected-filter.stderr').read_text()
    assert not (root / 'followup-local/mac/v4-format.log').read_bytes()
    assert 'Finished `dev` profile' in (root / 'followup-local/mac/mac-v4-clippy.log').read_text()
    tooling = load(root / 'tooling/parser-check.json')
    assert tooling['source_sha256'] == digest(HERE / 'log-parser.py')
    assert len(tooling['real_logs']) == 6 and tooling['ambiguous_or_missing_results_rejected'] == 2
    assert tooling['launcher_zero_match_exit_code'] == 2
    result = dict(passed=True, original_qemu_failures_closed=18, native_retirement_failure_closed=1,
                  final_unique_passed_per_lane=542, original_ignored_per_lane=3, cloud_lanes=counts,
                  source_test_only=True, exact_selection_rejects_empty_matches=True,
                  production_runtime_changed_by_this_batch=False, full_product_or_device_release_qualified=False,
                  runtime=scope['runtime'], ipad_ci=scope['ipad_exact'])
    (HERE / 'audit-result.json').write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result, indent=2))
