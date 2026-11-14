#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""An old internally consistent ICD must not pass current app packaging."""
import hashlib
import importlib.util
import json
from pathlib import Path
import tempfile

root = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location('artifact', root / 'scripts/check-guest-vulkan.py')
artifact = importlib.util.module_from_spec(spec)
spec.loader.exec_module(artifact)
digest = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()
with tempfile.TemporaryDirectory(prefix='trames-guest-artifact-') as temporary:
    source = Path(temporary)
    inputs = source / 'accelerator/vulkan'
    inputs.mkdir(parents=True)
    (inputs / 'icd.c').write_text('old interface\n')
    recipe = source / 'scripts/build-vulkan-guest.sh'
    recipe.parent.mkdir()
    recipe.write_text('current recipe\n')
    output = source / 'output'
    output.mkdir()
    binary = output / 'libtrames_vulkan_icd.so'
    binary.write_bytes(b'fixture artifact')
    manifest = {'wire_revision': 21, 'source_sha256': {
        'accelerator/vulkan/icd.c': digest(inputs / 'icd.c')},
        'build_recipe_sha256': digest(recipe), 'icd_sha256': digest(binary)}
    path = output / 'manifest.json'
    path.write_text(json.dumps(manifest))
    artifact.check(source, output)
    for change in ('new-source', 'old-wire', 'changed-recipe', 'changed-binary'):
        (inputs / 'icd.c').write_text('old interface\n')
        recipe.write_text('current recipe\n')
        binary.write_bytes(b'fixture artifact')
        test = dict(manifest)
        if change == 'new-source':
            (inputs / 'icd.c').write_text('new image import interface\n')
        elif change == 'old-wire':
            test['wire_revision'] = 14
        elif change == 'changed-recipe':
            recipe.write_text('different linkage flags\n')
        else:
            binary.write_bytes(b'unmatched binary')
        path.write_text(json.dumps(test))
        try:
            artifact.check(source, output)
        except ValueError:
            continue
        raise AssertionError('stale artifact admitted: ' + change)
print('guest Vulkan source, wire, recipe and binary mismatch rejection passed')
