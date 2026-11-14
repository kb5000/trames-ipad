#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Build only an ARM64 HIDL HAL launcher using the selected image's libraries."""
import argparse
import base64
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tarfile
import urllib.request

ROOT = Path(__file__).resolve().parents[1]


def sha(data):
    return hashlib.sha256(data).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rootfs", type=Path, required=True)
    parser.add_argument("--cache", type=Path, default=ROOT / "target/android-toolchain")
    parser.add_argument("--output", type=Path, default=ROOT / "target/vulkan-android")
    parser.add_argument("--offline", action="store_true")
    parser.add_argument("--cc", default="clang")
    parser.add_argument("--aidl", type=Path, help="Android 13 host AIDL compiler (automatic on macOS)")
    args = parser.parse_args()
    cache, output = args.cache.resolve(), args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    prepare = [sys.executable, str(ROOT / "scripts/prepare-android.py"), "--cache", str(cache)]
    if args.offline:
        prepare.append("--offline")
    subprocess.run(prepare, check=True)
    manifest = json.loads((ROOT / "scripts/android-upstream.json").read_text())
    pin = manifest["platform_cxx"]
    for name, url, expected, encoded in [
        (pin["archive"], pin["url"], pin["sha256"], False),
        (pin["license_archive"], pin["license_url"], pin["license_sha256"], True),
    ]:
        path = cache / name
        if not path.exists() or sha(path.read_bytes()) != expected:
            if args.offline:
                raise RuntimeError("missing or changed platform libc++ input: " + name)
            with urllib.request.urlopen(url, timeout=30) as response:
                data = response.read()
            if encoded:
                data = base64.b64decode(data, validate=True)
            if sha(data) != expected:
                raise RuntimeError("platform libc++ input digest mismatch: " + name)
            path.write_bytes(data)
    headers = cache / "platform-libcxx"
    with tarfile.open(cache / pin["archive"]) as archive:
        for item in archive:
            name = Path(item.name)
            if name.is_absolute() or ".." in name.parts:
                raise RuntimeError("unsafe libc++ header path")
            if item.isfile():
                path = headers / name
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_bytes(archive.extractfile(item).read())
    inputs = output / "hal-link-inputs"
    inputs.mkdir(exist_ok=True)
    libraries = {}
    with tarfile.open(args.rootfs) as archive:
        members = {item.name.removeprefix("./"): item for item in archive.getmembers()}
        for name in ["libhidlbase.so", "libc++.so"]:
            member = members["system/lib64/" + name]
            data = archive.extractfile(member).read()
            if data[:5] != b"\x7fELF\x02" or data[18:20] != b"\xb7\x00":
                raise RuntimeError("selected image library is not ARM64: " + name)
            (inputs / name).write_bytes(data)
            libraries[name] = sha(data)
    sysroot = cache / "sysroot"
    lib = sysroot / f"usr/lib/aarch64-linux-android/{manifest['api']}"
    rust = subprocess.check_output(["rustc", "-vV"], text=True)
    host = next(line.split(": ", 1)[1] for line in rust.splitlines() if line.startswith("host: "))
    rustroot = Path(subprocess.check_output(["rustc", "--print", "sysroot"], text=True).strip())
    linker = rustroot / "lib/rustlib" / host / "bin/rust-lld"
    # Keep the ELF linker name so Clang selects the original lld driver mode.
    linkname = inputs / "ld.lld"
    if linkname.is_symlink():
        linkname.unlink()
    if linkname.exists():
        raise RuntimeError("refusing to replace a non-symlink linker")
    linkname.symlink_to(linker)
    source = ROOT / "accelerator/android/hal-service.cpp"
    command = [args.cc, f"--target=aarch64-linux-android{manifest['api']}",
               f"--sysroot={sysroot}", f"--ld-path={linkname}", "-O2", "-mno-outline-atomics",
               "-Wall", "-Wextra", "-Werror", "-std=c++17", "-fno-exceptions", "-fno-rtti",
               "-nostdinc++", "-nostdlib++", "-isystem", str(headers), "-fPIE", "-pie", "-nostdlib",
               str(lib / "crtbegin_dynamic.o"), str(source), str(inputs / "libhidlbase.so"),
               str(inputs / "libc++.so"), "-L", str(lib), "-Wl,--no-undefined",
               "-Wl,--allow-shlib-undefined", "-Wl,--build-id=sha1", "-Wl,-z,noexecstack",
               "-Wl,--dynamic-linker,/system/bin/linker64", "-lc", "-lm", "-ldl",
               str(lib / "crtend_android.o"), "-o", str(output / "trames-hal-service")]
    subprocess.run(command, check=True)
    record = dict(api=manifest["api"], abi="arm64-v8a", platform_cxx=pin,
                  source_sha256=sha(source.read_bytes()), libraries_sha256=libraries,
                  command=command, output_sha256=sha((output / "trames-hal-service").read_bytes()))
    (output / "hal-service-manifest.json").write_text(json.dumps(record, indent=2) + "\n")
    shutil.copyfile(cache / pin["license_archive"], output / "libcxx-LICENSE.TXT")
    print(json.dumps(dict(output=str(output / "trames-hal-service"), sha256=record["output_sha256"])))
    build_media(args, manifest, cache, output, inputs, lib, linkname)


def build_media(args, manifest, cache, output, inputs, lib, linker):
    """Only register the image's original OMX objects; keep Codec2 intact."""
    pin = manifest["vndk_headers"]
    archive = cache / pin["archive"]
    if not archive.exists() or sha(archive.read_bytes()) != pin["sha256"]:
        if args.offline:
            raise RuntimeError("missing or changed VNDK header snapshot")
        with urllib.request.urlopen(pin["url"], timeout=60) as response:
            data = response.read()
        if sha(data) != pin["sha256"]:
            raise RuntimeError("VNDK header snapshot digest mismatch")
        archive.write_bytes(data)
    headers = cache / "vndk-v33-headers"
    with tarfile.open(archive) as source:
        for item in source:
            name = Path(item.name)
            if name.is_absolute() or ".." in name.parts:
                raise RuntimeError("unsafe VNDK header path")
            if item.isfile():
                path = headers / name
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_bytes(source.extractfile(item).read())
    resource_dirs, resource_sources, resource_record = prepare_media_resources(args, manifest, cache, headers)
    libraries = {}
    names = ["libhidlbase.so", "libc++.so", "libstagefright_omx.so",
             "android.hardware.media.omx@1.0.so", "libutils.so", "libstagefright_xmlparser.so",
             "libmedia.so", "libmediautils.so", "libbinder.so", "libbinder_ndk.so", "liblog.so", "libstagefright.so"]
    with tarfile.open(args.rootfs) as source:
        members = {item.name.removeprefix("./"): item for item in source.getmembers()}
        for name in names:
            data = source.extractfile(members["system/lib64/" + name]).read()
            if data[:5] != b"\x7fELF\x02" or data[18:20] != b"\xb7\x00":
                raise RuntimeError("selected media library is not ARM64: " + name)
            (inputs / name).write_bytes(data)
            libraries[name] = sha(data)
    source = ROOT / "accelerator/android/media-service.cpp"
    command = [args.cc, f"--target=aarch64-linux-android{manifest['api']}",
               f"--sysroot={cache / 'sysroot'}", f"--ld-path={linker}",
               "-O2", "-mno-outline-atomics", "-Wall", "-Wextra", "-Werror",
               "-Wno-inconsistent-missing-override", "-Wno-deprecated-declarations", "-Wno-sign-compare",
               "-std=c++17", "-fno-exceptions", "-fno-rtti", "-nostdinc++", "-nostdlib++",
               # API 33 Bionic already provides these locale inlines. The VNDK
               # libc++ snapshot also supports older Bionic; omit its fallback.
               "-D_LIBCPP_SUPPORT_XLOCALE_POSIX_L_FALLBACK_H"]
    for name in pin["include_dirs"]:
        command.extend(["-isystem", str(headers / name)])
    for path in resource_dirs:
        command.extend(["-isystem", str(path)])
    command.extend(["-fPIE", "-pie", "-nostdlib", str(lib / "crtbegin_dynamic.o"), str(source)])
    command.extend(str(path) for path in resource_sources)
    command.extend(str(inputs / name) for name in names)
    command.extend(["-L", str(lib), "-Wl,--no-undefined", "-Wl,--allow-shlib-undefined",
                    "-Wl,--build-id=sha1", "-Wl,-z,noexecstack",
                    "-Wl,--dynamic-linker,/system/bin/linker64", "-lc", "-lm", "-ldl",
                    str(lib / "crtend_android.o"), "-o", str(output / "trames-media-service")])
    subprocess.run(command, check=True)
    record = dict(api=manifest["api"], abi="arm64-v8a", vndk_headers=pin, resources=resource_record,
                  source_sha256=sha(source.read_bytes()), libraries_sha256=libraries,
                  command=command, output_sha256=sha((output / "trames-media-service").read_bytes()))
    (output / "media-service-manifest.json").write_text(json.dumps(record, indent=2) + "\n")
    # Keep upstream source/header material beside, never inside, the tiny
    # executable adapter payload. The package's separate source export uses it.
    with tarfile.open(output / "android-media-sources.tar.gz", "w:gz") as archive:
        archive.add(cache / pin["archive"], arcname=pin["archive"])
        for name, item in manifest["media_service_inputs"].items():
            if not name.startswith("aidl_"):
                archive.add(cache / item["archive"], arcname=item["archive"])
    print(json.dumps(dict(output=str(output / "trames-media-service"), sha256=record["output_sha256"])))


def prepare_media_resources(args, manifest, cache, headers):
    """Build the unmodified upstream resource manager, not a successful stub."""
    pins = manifest["media_service_inputs"]
    paths = {}
    for name, pin in pins.items():
        if name == "aidl_darwin" and args.aidl:
            continue
        if name == "aidl_darwin" and sys.platform != "darwin":
            raise RuntimeError("pass --aidl with an Android 13 host AIDL compiler")
        path = cache / pin["archive"]
        if not path.exists() or sha(path.read_bytes()) != pin["sha256"]:
            if args.offline:
                raise RuntimeError("missing or changed media service input: " + name)
            with urllib.request.urlopen(pin["url"], timeout=60) as response:
                data = response.read()
            if pin.get("base64"):
                data = base64.b64decode(data, validate=True)
            if sha(data) != pin["sha256"]:
                raise RuntimeError("media service input digest mismatch: " + name)
            path.write_bytes(data)
        paths[name] = path
    work = cache / "media-service"
    for name in ["resource_source", "mediautils_headers"]:
        with tarfile.open(paths[name]) as archive:
            for item in archive:
                path = Path(item.name)
                if path.is_absolute() or ".." in path.parts:
                    raise RuntimeError("unsafe media service source path")
                if item.isfile():
                    out = work / name / path
                    out.parent.mkdir(parents=True, exist_ok=True)
                    out.write_bytes(archive.extractfile(item).read())
    battery = work / "mediautils_headers/batterystats/IBatteryStats.h"
    battery.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(paths["battery_header"], battery)
    aidl = args.aidl.resolve() if args.aidl else paths["aidl_darwin"]
    if not args.aidl:
        aidl.chmod(0o755)
    source = work / "resource_source"
    generated = work / "generated"
    include = generated / "include"
    include.mkdir(parents=True, exist_ok=True)
    for path in sorted((source / "aidl").rglob("*.aidl")):
        subprocess.run([str(aidl), "--lang=ndk", "--structured", "-I" + str(source / "aidl"),
                        "-h", str(include), "-o", str(generated), str(path)], check=True)
    dirs = [source, include, work / "mediautils_headers"]
    dirs.extend(headers / name for name in [
        "frameworks/av/media/libmedia/include", "frameworks/av/media/libstagefright/include",
        "frameworks/native/libs/binder/ndk/include_cpp", "frameworks/native/libs/binder/ndk/include_ndk",
        "frameworks/native/libs/binder/ndk/include_platform", "system/core/libcutils/include_outside_system"])
    sources = [source / name for name in ["ResourceManagerService.cpp", "ResourceObserverService.cpp",
                                         "ServiceLog.cpp", "IMediaResourceMonitor.cpp"]]
    sources.extend(generated / "android/media" / name for name in [
        "IResourceObserver.cpp", "IResourceObserverService.cpp", "MediaObservableFilter.cpp", "MediaObservableParcel.cpp"])
    return dirs, sources, dict(inputs=pins, aidl_sha256=sha(aidl.read_bytes()))


if __name__ == "__main__":
    main()
