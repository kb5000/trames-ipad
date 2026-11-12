#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Reject an internally consistent but stale guest ICD before app packaging."""
import argparse
import hashlib
import json
from pathlib import Path


def check(root, directory):
    manifest = json.loads((directory / 'manifest.json').read_text())
    digest = lambda path: hashlib.sha256(path.read_bytes()).hexdigest()
    sources = {str(path.relative_to(root)): digest(path)
               for path in sorted((root / 'accelerator/vulkan').iterdir())
               if path.suffix in ('.c', '.inc', '.S')}
    if (manifest.get('wire_revision') != 20 or manifest.get('source_sha256') != sources
            or manifest.get('build_recipe_sha256') != digest(root / 'scripts/build-vulkan-guest.sh')):
        raise ValueError('Guest Vulkan inputs are stale; run scripts/build-vulkan-guest.sh')
    if manifest.get('icd_sha256') != digest(directory / 'libtrames_vulkan_icd.so'):
        raise ValueError('Guest Vulkan binary does not match its manifest')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('directory', type=Path, nargs='?', default=Path('target/vulkan-guest'))
    args = parser.parse_args()
    try:
        check(Path(__file__).resolve().parents[1], args.directory.resolve())
    except (OSError, ValueError) as error:
        parser.exit(1, str(error) + '\n')
