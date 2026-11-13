#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Exercise the normal Mac Android target in a disposable environment registry.

Requires a Trames Android package and an independently obtained official APK.
Does not accept app licenses, upload benchmark results or modify stable stores.
"""
import argparse
import json
import os
from pathlib import Path
import subprocess
import tempfile
import time


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--runner", type=Path, required=True)
    p.add_argument("--rootfs", type=Path, required=True)
    p.add_argument("--apk", type=Path, required=True)
    p.add_argument("--package", required=True)
    p.add_argument("--output", type=Path, required=True)
    p.add_argument("--registry", type=Path, help="reuse an explicitly supplied disposable registry")
    args = p.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    records = []
    with tempfile.TemporaryDirectory(prefix="trames-android-target-") as temporary:
        registry = args.registry.resolve() if args.registry else Path(temporary) / "registry"
        env = dict(os.environ, TRAMES_HOME=str(registry))

        def run(*command, ok=True, timeout=180, boot_probe=False):
            started = time.monotonic()
            r = subprocess.run([str(args.runner.resolve()), *command], env=env,
                               stdin=subprocess.DEVNULL, text=True, capture_output=True,
                               timeout=timeout)
            record = dict(argv=command, code=r.returncode, stdout=r.stdout,
                          stderr=r.stderr, seconds=round(time.monotonic()-started, 3))
            records.append(record)
            (args.output / "commands.json").write_text(json.dumps(records, indent=2)+"\n")
            if boot_probe:
                assert r.returncode in (0, 127), record
            else:
                assert (r.returncode == 0) == ok, record
            return r.stdout

        def ready():
            deadline = time.monotonic()+180
            while time.monotonic() < deadline:
                if run("-e", "android", "exec", "--", "/system/bin/getprop", "sys.boot_completed", boot_probe=True).strip() == "1":
                    return
                time.sleep(1)
            raise RuntimeError("original Android framework did not finish boot")

        created = False
        try:
            if not (registry / "environments/android/config.json").exists():
                run("env", "create", "android", "--target", "android", "--rootfs",
                    str(args.rootfs.resolve()))
            created = True
            config = json.loads(run("env", "config", "android"))
            assert config["target"] == "android" and config["distribution"] == "android"
            assert config["vulkan"] and config["command"] == ["/system/bin/init", "second_stage"]
            run("env", "create", "missing-package", "--target", "android", ok=False)
            run("-e", "android", "config", "set", "init", "systemd", ok=False)
            run("env", "start", "android")
            ready()
            assert run("-e", "android", "exec", "--", "/system/bin/getprop", "dalvik.vm.dex2oat64.enabled").strip() == "true"
            run("-e", "android", "exec", "--", "/system/bin/sh", "-c", "exit 7", ok=False)
            assert "Success" in run("-e", "android", "apk", "install", str(args.apk.resolve()))
            assert args.package in run("-e", "android", "apk", "list")
            launched = run("-e", "android", "apk", "launch", args.package)
            if "Status: ok" not in launched or "Error:" in launched:
                run("-e", "android", "exec", "--", "/system/bin/dumpsys", "user")
                run("-e", "android", "exec", "--", "/system/bin/cmd", "package", "resolve-activity", "--brief", args.package)
                raise RuntimeError("Android launcher intent failed: " + launched)
            run("-e", "android", "exec", "--", "/system/bin/sh", "-c", "echo persistent > /data/local/tmp/target-test")
            invalid = Path(temporary) / "invalid.apk"
            invalid.write_bytes(b"not an APK")
            run("-e", "android", "apk", "install", str(invalid), ok=False, timeout=30)
            assert args.package in run("-e", "android", "apk", "list")
            run("env", "stop", "android")
            run("env", "start", "android")
            ready()
            assert args.package in run("-e", "android", "apk", "list")
            assert "persistent" in run("-e", "android", "exec", "--", "/system/bin/cat", "/data/local/tmp/target-test")
            print("PASS Android target, original APK install/launch, early-error stream and persistent restart", flush=True)
        finally:
            if created:
                run("env", "stop", "android")


if __name__ == "__main__":
    main()
