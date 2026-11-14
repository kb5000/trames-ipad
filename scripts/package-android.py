#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Package only Trames Android adapters and a pinned upstream download recipe.

Waydroid images, APKs and user data are never included. Matching source exports
are separate artifacts, served alongside the adapter archive when publishing.
"""
import argparse
import gzip
import hashlib
import io
import json
from pathlib import Path
import subprocess
import tarfile

ROOT = Path(__file__).resolve().parents[1]
COMPONENTS = {
    "vendor/lib64/hw/vulkan.trames.so": "vulkan.trames.so",
    "vendor/lib64/hw/gralloc.trames.so": "gralloc.trames.so",
    "vendor/bin/hw/trames-hal-service": "trames-hal-service",
    "system/bin/trames-media-service": "trames-media-service",
    "vendor/lib64/hw/hwcomposer.trames.so": "hwcomposer.trames.so",
}


def add(archive, name, data, mode=0o644):
    info = tarfile.TarInfo(name)
    info.mode = mode
    info.size = len(data)
    archive.addfile(info, io.BytesIO(data))


def package(components, output, source_output):
    if output == source_output or output.exists() or source_output.exists():
        raise ValueError("output exists or aliases source output; choose new filenames")
    pins = json.loads((ROOT / "runtime/scripts/android-images.json").read_text())
    data = {guest: (components / name).read_bytes() for guest, name in COMPONENTS.items()}
    for name, value in data.items():
        if value[:5] != b"\x7fELF\x02" or value[18:20] != b"\xb7\0":
            raise ValueError("component is not ARM64 ELF: " + name)
    revision = lambda repo: subprocess.check_output(["git", "-C", str(repo), "rev-parse", "HEAD"], text=True).strip()
    provenance = {"trames-ipad": revision(ROOT), "trames-runtime": revision(ROOT / "runtime")}
    sources = dict(archive=source_output.name, repositories={name: dict(revision=rev, url=f"https://github.com/kb5000/{name}/archive/{rev}.tar.gz") for name, rev in provenance.items()})
    upstream = components / "android-media-sources.tar.gz"
    sources["android_media"] = dict(archive=upstream.name,
                                    sha256=hashlib.sha256(upstream.read_bytes()).hexdigest(),
                                    inputs=json.loads((ROOT / "scripts/android-upstream.json").read_text())["media_service_inputs"])
    metadata = dict(schema=1, package_kind="adapters", target="android", abi="arm64-v8a", android_api=33,
                    graphics_contract=1, images=pins,
                    components={"/"+name: dict(size=len(value), sha256=hashlib.sha256(value).hexdigest()) for name, value in data.items()},
                    source_revision=provenance["trames-ipad"], runtime_revision=provenance["trames-runtime"],
                    limitations=["experimental", "ARM64 only", "SELinux disabled", "no emulated external storage/FUSE", "no Android pressure-based app killing", "host audio not accepted", "iPad Android acceptance pending"])
    for path in [output, source_output]:
        path.parent.mkdir(parents=True, exist_ok=True)
    partial = output.with_name(output.name + ".partial")
    source_partial = source_output.with_name(source_output.name + ".partial")
    try:
        with tarfile.open(source_partial, "x:gz") as archive:
            for repo, name in [(ROOT, "trames-ipad"), (ROOT / "runtime", "trames-runtime")]:
                add(archive, name+".tar.gz", subprocess.check_output(["git", "-C", str(repo), "archive", "--format=tar.gz", "HEAD"]))
            add(archive, "GPL-3.0.txt", (ROOT / "LICENSE").read_bytes())
            add(archive, upstream.name, upstream.read_bytes())
        with partial.open("xb") as raw, gzip.GzipFile(filename="", fileobj=raw, mode="wb", mtime=0, compresslevel=6) as compressed, tarfile.open(fileobj=compressed, mode="w|") as archive:
            add(archive, ".trames-android.json", (json.dumps(metadata, indent=2)+"\n").encode())
            for name, value in data.items():
                add(archive, name, value, 0o755 if name.startswith(("vendor/bin/", "system/bin/")) else 0o644)
            add(archive, "vendor/waydroid.prop", b"ro.hardware.vulkan=trames\nro.hardware.egl=angle\nro.hardware.gralloc=trames\nro.hardware.hwcomposer=trames\n")
            add(archive, ".trames-android/GPL-3.0.txt", (ROOT / "LICENSE").read_bytes())
            add(archive, ".trames-android/SOURCES.json", (json.dumps(sources, indent=2)+"\n").encode())
        source_partial.rename(source_output)
        partial.rename(output)
    except BaseException:
        partial.unlink(missing_ok=True)
        source_partial.unlink(missing_ok=True)
        raise
    for path in [output, source_output]:
        sha = hashlib.sha256(path.read_bytes()).hexdigest()
        path.with_name(path.name+".sha256").write_text(sha+"  "+path.name+"\n")
    record = dict(filename=output.name, size=output.stat().st_size, sha256=hashlib.sha256(output.read_bytes()).hexdigest(), metadata=metadata, sources=sources)
    output.with_name(output.name+".json").write_text(json.dumps(record, indent=2)+"\n")
    return record


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--components", type=Path, default=ROOT / "target/vulkan-android")
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--source-output", type=Path)
    args = parser.parse_args()
    record = package(args.components, args.output, args.source_output or args.output.with_name(args.output.name.removesuffix(".tar.gz")+"-sources.tar.gz"))
    print(json.dumps({key: record[key] for key in ["filename", "size", "sha256", "sources"]}, indent=2))


if __name__ == "__main__":
    main()
