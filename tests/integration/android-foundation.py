#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Original Android init/APEX/property/log/Binder acceptance on a disposable store.

The supplied ARM64 Waydroid rootfs must preserve original owners and symlinks.
Use only a disposable, unregistered store: this script replaces its test files
and boot-property overlay.
The C probe is runtime/tests/fixtures/android-init-probe.c, compiled statically
for Linux ARM64. Optional Vulkan inputs use the original Android loader/HAL.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import socket
import struct
import subprocess
import tempfile
import time

ROOT = Path(__file__).resolve().parents[2]


def rpc(path, argv, timeout=15):
    def read(stream, size):
        data = bytearray()
        while len(data) < size:
            chunk = stream.recv(size - len(data))
            if not chunk:
                raise RuntimeError("exec connection closed before status")
            data.extend(chunk)
        return bytes(data)

    request = json.dumps(dict(argv=argv, environment=[], cwd="/", tty=False, size=None)).encode()
    output = []
    with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as stream:
        stream.settimeout(timeout)
        stream.connect(str(path))
        stream.sendall(b"TRMEXEC1" + struct.pack(">I", len(request)) + request + b"\x01\0\0\0\0")
        while True:
            head = read(stream, 5)
            size = struct.unpack(">I", head[1:])[0]
            if size > 65536:
                raise RuntimeError("invalid exec response size")
            body = read(stream, size)
            if head[0] == 3:
                return dict(status=struct.unpack(">i", body)[0], output="".join(output))
            output.append(body.decode(errors="replace"))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--runner", required=True, type=Path)
    parser.add_argument("--store", required=True, type=Path)
    parser.add_argument("--rootfs", type=Path)
    parser.add_argument("--probe", required=True, type=Path)
    parser.add_argument("--cpus", default="1,4,12")
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--vulkan-driver", type=Path)
    parser.add_argument("--vulkan-probe", type=Path)
    parser.add_argument("--hal-probe", type=Path)
    parser.add_argument("--shader", type=Path)
    args = parser.parse_args()
    gpu = [args.vulkan_driver, args.vulkan_probe, args.shader]
    if any(gpu) and not all(gpu):
        parser.error("supply all three Vulkan inputs")
    cpus = [int(n) for n in args.cpus.split(",")]
    env = dict(os.environ, TRAMES_MAC_BUNDLED="1")
    prefix = [str(args.runner.resolve()), "debug", "run", "--storage", str(args.store.resolve())]
    args.output.mkdir(parents=True, exist_ok=True)

    def offline(*command):
        result = subprocess.run(prefix + list(command), env=env, text=True, capture_output=True)
        if result.returncode:
            raise RuntimeError(result.stdout + result.stderr)
        return result.stdout

    if args.rootfs:
        offline("--rootfs", str(args.rootfs.resolve()), "--fs", "ls", "/")
    files = {
        "/trames-init-foundation.rc": ROOT / "runtime/tests/fixtures/android-init-foundation.rc",
        "/trames-init-probe": args.probe,
    }
    properties = "ro.boot.init_rc=/trames-init-foundation.rc\nro.cold_boot_done=true\nro.zygote=zygote64\nro.logd.auditd=false\nro.logd.kernel=false\nsys.use_memfd=true\n"
    if all(gpu):
        properties += "ro.hardware.vulkan=trames\n"
        files.update({"/vendor/lib64/hw/vulkan.trames.so": args.vulkan_driver,
                      "/system/bin/trames-vulkan-probe": args.vulkan_probe,
                      "/system/bin/trames-vulkan-probe.spv": args.shader})
        if args.hal_probe:
            files["/system/bin/trames-vulkan-hal-probe"] = args.hal_probe
    overlay = args.output / "waydroid.prop"
    overlay.write_text(properties)
    files["/vendor/waydroid.prop"] = overlay
    for guest, host in files.items():
        offline("--fs", "put", "--replace", str(host.resolve()), guest)
    results = dict(inputs={guest: hashlib.sha256(host.read_bytes()).hexdigest()
                           for guest, host in files.items()}, cases=[])
    summary = args.output / "summary.json"
    for count in cpus:
        case = dict(cpus=count)
        results["cases"].append(case)
        with tempfile.TemporaryDirectory(prefix="trames-android-") as temporary:
            control = Path(temporary) / "control"
            log = args.output / f"android-cpu{count}.log"
            with log.open("w") as output:
                command = prefix + ["--android-init", "--exec-socket", str(control), "--cpus", str(count), "--no-pty", "--no-windows", "--network", "off", "--timeout", "90", "--metrics"]
                if all(gpu):
                    command.append("--vulkan")
                process = subprocess.Popen(command + ["/system/bin/init", "second_stage"], env=env, stdout=output, stderr=subprocess.STDOUT)
                try:
                    deadline = time.monotonic() + 30
                    last = None
                    while time.monotonic() < deadline:
                        if process.poll() is not None:
                            raise RuntimeError("Android session exited: " + log.read_text()[-4000:])
                        if control.with_suffix(".exec").exists():
                            ready = rpc(control.with_suffix(".exec"), ["/system/bin/getprop", "trames.foundation.roundtrip"])
                            if ready["status"] == 0 and "original-property-service" in ready["output"]:
                                last = rpc(control.with_suffix(".exec"), ["/system/bin/toybox", "cat", "/data/trames-foundation/ok"])
                                if last["status"] == 0 and "PASS original init" in last["output"]:
                                    break
                        time.sleep(.2)
                    else:
                        raise RuntimeError("Android foundation marker missing: " + str(last))
                    case["foundation"] = last
                    case["properties"] = {}
                    for key in ["trames.foundation.roundtrip", "logd.ready", "ro.hardware", "ro.hardware.vulkan", "ro.product.cpu.abilist", "init.svc.servicemanager"]:
                        reply = rpc(control.with_suffix(".exec"), ["/system/bin/getprop", key])
                        if reply["status"]:
                            raise RuntimeError(str(reply))
                        case["properties"][key] = reply["output"].strip()
                    if all(gpu):
                        if args.hal_probe:
                            reply = rpc(control.with_suffix(".exec"), ["/system/bin/toybox", "chmod", "755", "/system/bin/trames-vulkan-hal-probe"])
                            if reply["status"]:
                                raise RuntimeError(str(reply))
                            case["hal"] = rpc(control.with_suffix(".exec"), ["/system/bin/trames-vulkan-hal-probe"])
                            if case["hal"]["status"]:
                                raise RuntimeError("Android HAL discovery failed: " + str(case["hal"]))
                        reply = rpc(control.with_suffix(".exec"), ["/system/bin/toybox", "chmod", "755", "/system/bin/trames-vulkan-probe"])
                        if reply["status"]:
                            raise RuntimeError(str(reply))
                        reply = rpc(control.with_suffix(".exec"), ["/system/bin/trames-vulkan-probe", "/system/bin/trames-vulkan-probe.spv"], timeout=45)
                        case["vulkan"] = reply
                        if reply["status"] or "vulkan-compute-ok" not in reply["output"]:
                            case["vulkan_log"] = rpc(control.with_suffix(".exec"), ["/system/bin/logcat", "-d", "-b", "all", "-s", "vulkan:V", "*:S"])
                            raise RuntimeError("Android Vulkan compute failed: " + str(reply))
                    case["passed"] = True
                    print(json.dumps(case), flush=True)
                except Exception as error:
                    case["error"] = str(error)
                    raise
                finally:
                    process.terminate()
                    try:
                        process.wait(timeout=5)
                    except subprocess.TimeoutExpired:
                        process.kill()
                        process.wait()
                    summary.write_text(json.dumps(results, indent=2) + "\n")
    print("PASS: original Android init/APEX/property/log/Binder" + (" and Vulkan compute" if all(gpu) else ""))


if __name__ == "__main__":
    main()
