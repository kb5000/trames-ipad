#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Verify adapter-only distribution without copying upstream images or stores."""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import tarfile
import tempfile

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location("android_package", ROOT / "scripts/package-android.py")
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--components", type=Path, default=ROOT / "target/vulkan-android")
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix="trames-adapter-package-") as work:
        work = Path(work)
        output, sources = work / "adapters.tar.gz", work / "sources.tar.gz"
        result = module.package(args.components, output, sources)
        assert output.stat().st_size < 1024*1024
        before = output.read_bytes()
        with tarfile.open(output) as archive:
            entries = archive.getmembers()
            assert entries[0].name == ".trames-android.json" and len(entries) == 7
            manifest = json.load(archive.extractfile(entries[0]))
            assert manifest["package_kind"] == "adapters"
            assert manifest["images"] == json.loads((ROOT / "runtime/scripts/android-images.json").read_text())
            for item in entries:
                assert item.isfile() and not item.name.endswith((".apk", ".img", ".zip", ".tar.gz"))
            for name, meta in manifest["components"].items():
                payload = archive.extractfile(name.lstrip("/")).read()
                assert meta == dict(size=len(payload), sha256=hashlib.sha256(payload).hexdigest())
            source_manifest = json.load(archive.extractfile(".trames-android/SOURCES.json"))
            assert source_manifest["archive"] == sources.name
        with tarfile.open(sources) as archive:
            assert set(archive.getnames()) == {"trames-ipad.tar.gz", "trames-runtime.tar.gz", "GPL-3.0.txt"}
        assert result["sha256"] == hashlib.sha256(before).hexdigest()
        try:
            module.package(args.components, output, sources)
        except ValueError:
            pass
        else:
            raise AssertionError("existing artifacts were overwritten")
        assert output.read_bytes() == before
    print("PASS adapter-only payload, pinned sources, hashes, separate source exports and no overwrite")


if __name__ == "__main__":
    main()
