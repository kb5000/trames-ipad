#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
import json
from pathlib import Path
import runpy
import subprocess
root = Path(__file__).resolve().parents[1]
pin = json.loads((root / 'runtime-version.json').read_text())
actual = subprocess.check_output(['git', '-C', str(root / 'runtime'), 'rev-parse', 'HEAD'], text=True).strip()
assert actual == pin['revision'], ('runtime revision mismatch', actual, pin['revision'])
assert not subprocess.check_output(['git', '-C', str(root / 'runtime'), 'status', '--porcelain'], text=True).strip(), 'runtime has uncommitted changes'
assert subprocess.check_output(['git', 'config', '-f', str(root / '.gitmodules'), 'submodule.runtime.url'], text=True).strip() == pin['repository']
runpy.run_path(str(root / 'runtime/scripts/check-execution-boundaries.py'), run_name='__main__')

# Product feature unification must never pull executable-code generation into iPad.
tree = subprocess.check_output(['cargo', 'tree', '--locked', '--all-features', '-p', 'trames-cpu', '--edges', 'normal', '--prefix', 'none'], text=True)
assert 'trames-interpreter ' in tree
assert 'trames-jit ' not in tree, 'iPad must not compile/link JIT code'
print('iPad CPU dependency graph verified: shared interpreter, no JIT')
