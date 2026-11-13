#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Install an original APK through PackageManager in an existing test session.

Streams through the existing bounded exec protocol, including concurrent output
and early failure. No offline store modification, guest HTTP server or ADB daemon.
"""
import argparse
import importlib.util
import json
from pathlib import Path
import select
import socket
import struct
import time

spec = importlib.util.spec_from_file_location("android_foundation", Path(__file__).with_name("android-foundation.py"))
foundation = importlib.util.module_from_spec(spec)
spec.loader.exec_module(foundation)


def install(control, apk, timeout=180):
    request = json.dumps(dict(argv=["/system/bin/pm", "install", "-r", "-S", str(apk.stat().st_size)], environment=[], cwd="/", tty=False, size=None)).encode()
    outgoing = bytearray(b"TRMEXEC1" + struct.pack(">I", len(request)) + request)
    incoming = bytearray()
    output = []
    eof = False
    sent = 0
    started = time.monotonic()
    with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as stream, apk.open("rb") as source:
        stream.connect(str(control))
        stream.setblocking(False)
        while time.monotonic() - started < timeout:
            if len(outgoing) < 65536 and not eof:
                data = source.read(4096)
                outgoing.extend(bytes([0 if data else 1]) + struct.pack(">I", len(data)) + data)
                eof = not data
            ready, writable, _ = select.select([stream], [stream] if outgoing else [], [], 1)
            if writable:
                try:
                    size = stream.send(outgoing)
                except BlockingIOError:
                    size = 0
                del outgoing[:size]
                sent += size
            if ready:
                data = stream.recv(65536)
                if not data:
                    raise RuntimeError("APK install connection closed before status")
                incoming.extend(data)
                while len(incoming) >= 5:
                    kind = incoming[0]
                    size = struct.unpack(">I", incoming[1:5])[0]
                    if size > 65536:
                        raise RuntimeError("invalid exec response size")
                    if len(incoming) < size + 5:
                        break
                    body = bytes(incoming[5:size+5])
                    del incoming[:size+5]
                    if kind == 3:
                        return dict(status=struct.unpack(">i", body)[0], output="".join(output), seconds=round(time.monotonic()-started, 3), wire_bytes=sent)
                    output.append(body.decode(errors="replace"))
        raise TimeoutError("PackageManager install exceeded its test deadline")


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--control", type=Path, required=True)
    p.add_argument("--apk", type=Path, required=True)
    p.add_argument("--output", type=Path, required=True)
    p.add_argument("--package", required=True)
    p.add_argument("--activity")
    args = p.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    reply = install(args.control, args.apk)
    (args.output / "install.json").write_text(json.dumps(reply, indent=2)+"\n")
    print(reply, flush=True)
    if reply["status"] or "Success" not in reply["output"]:
        raise RuntimeError("original APK installation failed")
    for name, argv in [("package", ["/system/bin/dumpsys", "package", args.package]), ("launcher", ["/system/bin/cmd", "package", "resolve-activity", "--brief", args.package])]:
        reply = foundation.rpc(args.control, argv, timeout=30)
        (args.output / (name+".json")).write_text(json.dumps(reply, indent=2)+"\n")
        print(name, reply["status"], reply["output"][:500], flush=True)
    activity = args.activity or reply["output"].strip().splitlines()[-1]
    if "/" not in activity:
        raise RuntimeError("APK has no resolved launcher activity")
    reply = foundation.rpc(args.control, ["/system/bin/am", "start", "-W", "-n", activity], timeout=60)
    (args.output / "launch.json").write_text(json.dumps(reply, indent=2)+"\n")
    print("launch", reply, flush=True)


if __name__ == "__main__":
    main()
