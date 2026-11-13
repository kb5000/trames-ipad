#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Boot the original Android desktop in an isolated, already prepared store.

This is a framework/graphics integration test, not a replacement Android init or
an installer. Never point it at a registered environment: optional fixtures and
APK installation modify the supplied store. Mouse/keyboard acceptance is manual.
"""
import argparse
import base64
import importlib.util
import json
import os
from pathlib import Path
import subprocess
import tempfile
import time

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location("android_foundation", Path(__file__).with_name("android-foundation.py"))
foundation = importlib.util.module_from_spec(spec)
spec.loader.exec_module(foundation)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--runner", type=Path, required=True)
    parser.add_argument("--store", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--cpus", type=int, default=12)
    parser.add_argument("--boot-timeout", type=int, default=180)
    parser.add_argument("--settle", type=int, default=15)
    parser.add_argument("--no-windows", action="store_true")
    parser.add_argument("--fixtures", type=Path, help="build-vulkan-android.py output directory")
    parser.add_argument("--install-calendar", action="store_true", help="reinstall the image's original Calendar APK into /data/app")
    parser.add_argument("--calendar-package", default="org.lineageos.etar")
    parser.add_argument("--screenshot", action="store_true")
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    report = dict(cpus=args.cpus, passed=False)
    prefix = [str(args.runner.resolve()), "debug", "run", "--storage", str(args.store.resolve())]
    env = dict(os.environ, TRAMES_MAC_BUNDLED="1")
    fixtures = {}
    if args.fixtures:
        for name in ["android-memory", "android-netlink", "android-acl", "android-sync", "android-gralloc", "android-egl", "vulkan-hal-probe"]:
            guest = "/system/bin/trames-desktop-" + name
            subprocess.run(prefix + ["--fs", "put", "--replace", str((args.fixtures / name).resolve()), guest], env=env, check=True, capture_output=True)
            fixtures[name] = guest
    with tempfile.TemporaryDirectory(prefix="trames-desktop-acceptance-") as temporary, (args.output / "boot.log").open("w") as log:
        control = Path(temporary) / "control"
        command = prefix + ["--android-init", "--exec-socket", str(control), "--cpus", str(args.cpus), "--no-pty", "--network", "off", "--vulkan", "--metrics"]
        if args.no_windows:
            command.append("--no-windows")
        process = subprocess.Popen(command + ["/system/bin/init", "second_stage"], env=env, stdout=log, stderr=subprocess.STDOUT)

        def call(name, argv, timeout=30, required=True):
            reply = foundation.rpc(control.with_suffix(".exec"), argv, timeout=timeout)
            report[name] = reply
            if required and reply["status"]:
                raise RuntimeError(name + ": " + reply["output"])
            return reply["output"]

        try:
            started = time.monotonic()
            deadline = started + args.boot_timeout
            while time.monotonic() < deadline:
                if process.poll() is not None:
                    raise RuntimeError("Android init exited: " + log.name)
                if control.with_suffix(".exec").exists():
                    reply = foundation.rpc(control.with_suffix(".exec"), ["/system/bin/getprop", "sys.boot_completed"])
                    if reply["status"] == 0 and reply["output"].strip() == "1":
                        break
                time.sleep(1)
            else:
                raise RuntimeError("original framework did not complete boot")
            report["boot_seconds"] = round(time.monotonic() - started, 3)
            print("Original framework completed boot", report["boot_seconds"], flush=True)
            core = ["/system/bin/toybox", "pidof", "surfaceflinger", "system_server", "android.hardware.graphics.composer@2.1-service"]
            before = call("core_pids_before", core).strip()
            call("home", ["/system/bin/am", "start", "-W", "-a", "android.intent.action.MAIN", "-c", "android.intent.category.HOME"], timeout=60)
            # sys.boot_completed precedes private-user unlocking and the switch
            # from FallbackHome to Launcher. Require the actual resumed app.
            while time.monotonic() < deadline:
                home = call("home_activity", ["/system/bin/dumpsys", "activity", "activities"])
                if any("Resumed" in line and "com.android.launcher3" in line for line in home.splitlines()):
                    break
                time.sleep(1)
            else:
                raise RuntimeError("original Launcher did not resume")
            report["desktop_seconds"] = round(time.monotonic() - started, 3)
            if args.install_calendar:
                source = call("calendar_source", ["/system/bin/pm", "path", args.calendar_package]).strip().splitlines()[0].removeprefix("package:")
                call("calendar_copy", ["/system/bin/toybox", "cp", source, "/data/local/tmp/trames-calendar.apk"])
                installed = call("calendar_install", ["/system/bin/pm", "install", "-r", "/data/local/tmp/trames-calendar.apk"], timeout=90)
                if "Success" not in installed:
                    raise RuntimeError("original APK was not installed")
            call("calendar", ["/system/bin/am", "start", "-W", "-n", args.calendar_package + "/com.android.calendar.AllInOneActivity"], timeout=60)
            for name, guest in fixtures.items():
                argv = [guest] + (["/data/local/tmp"] if name == "android-acl" else [])
                call(name, argv)
            time.sleep(args.settle)
            if call("core_pids_after", core).strip() != before:
                raise RuntimeError("composer/framework restarted during acceptance")
            call("properties", ["/system/bin/getprop"])
            call("activity", ["/system/bin/dumpsys", "activity", "activities"])
            call("input", ["/system/bin/dumpsys", "input"])
            logs = call("logcat", ["/system/bin/logcat", "-d", "-b", "all"])
            report["anrs"] = [line for line in logs.splitlines() if "ANR in " in line]
            if "Fatal signal" in logs or "FATAL EXCEPTION" in logs:
                raise RuntimeError("Android process crashed; inspect logcat")
            if args.screenshot:
                image = call("screenshot", ["/system/bin/sh", "-c", "screencap -p /data/local/tmp/trames-desktop.png && base64 /data/local/tmp/trames-desktop.png"], timeout=45)
                (args.output / "desktop.png").write_bytes(base64.b64decode(image))
                report["screenshot"] = "desktop.png"
            call("return_home", ["/system/bin/am", "start", "-a", "android.intent.action.MAIN", "-c", "android.intent.category.HOME"])
            report["passed"] = True
            print("PASS original Launcher, Calendar and retained native graphics", flush=True)
        except Exception as error:
            report["error"] = str(error)
            raise
        finally:
            try:
                if process.poll() is None and control.with_suffix(".exec").exists():
                    call("diagnostic_logcat", ["/system/bin/logcat", "-d", "-b", "all"], required=False)
            finally:
                process.terminate()
                try:
                    process.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    process.kill()
                    process.wait()
                (args.output / "summary.json").write_text(json.dumps(report, indent=2) + "\n")


if __name__ == "__main__":
    main()
