#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Original ART GC and lmkd Reaper acceptance on a disposable Android store.

Needs the pinned image, compiled AndroidMemoryStress.dex and the harness linked
with unchanged AOSP Reaper.cpp. The small init script runs original init/APEX,
linkerconfig, logd and lmkd. With --automatic, original lmkd must select and kill
the cached process through PSI. Never use a registered environment's store.
"""
import argparse
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import re
import subprocess
import tempfile
import time
import zipfile

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location("foundation", Path(__file__).with_name("android-foundation.py"))
foundation = importlib.util.module_from_spec(spec)
spec.loader.exec_module(foundation)
INIT = """import /init.environ.rc
import /system/etc/init/logd.rc
import /system/etc/init/servicemanager.rc
import /system/etc/init/apexd.rc
import /system/etc/init/lmkd.rc
on early-init
    exec_start apexd-bootstrap
    update_linker_config
    setprop sys.use_memfd true
on init
    start logd
    start servicemanager
    start apexd
    start lmkd
    mkdir /data/local 0755 root root
    mkdir /data/local/tmp 0771 shell shell
    mkdir /data/dalvik-cache 0771 root root
    setprop trames.memory.init ready
"""


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--runner", type=Path, required=True)
    parser.add_argument("--store", type=Path, required=True)
    parser.add_argument("--dex", type=Path, required=True)
    parser.add_argument("--reaper", type=Path)
    parser.add_argument("--automatic", type=Path, help="original lmkd protocol/pressure fixture; uses a bounded 384 MiB session")
    parser.add_argument("--zygote-client", type=Path, help="exercise original Zygote post-fork ART and mm retirement")
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--samples", type=int, default=3)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    env = dict(os.environ, TRAMES_MAC_BUNDLED="1")
    prefix = [str(args.runner.resolve()), "--storage", str(args.store.resolve())]
    report = {"passed": False, "runner_sha256": hashlib.sha256(args.runner.read_bytes()).hexdigest(),
              "dex_sha256": hashlib.sha256(args.dex.read_bytes()).hexdigest(), "gc": []}
    def save():
        (args.output / "summary.json").write_text(json.dumps(report, indent=2) + "\n")
    def offline(*argv):
        result = subprocess.run(prefix + ["--fs", "--", *argv], env=env,
                                capture_output=True, text=True, timeout=30, check=True)
        return result.stdout
    rc = args.output / "memory-init.rc"
    rc.write_text(INIT)
    if args.zygote_client:
        with rc.open("a") as stream:
            stream.write("""
service memory-zygote /system/bin/sh /data/local/tmp/memory-zygote.sh
    disabled
    user root
    group root system readproc
    socket zygote stream 660 root system
    socket usap_pool_primary stream 660 root system
    stdio_to_kmsg
""")
        jar = args.output / "android-memory-stress.jar"
        with zipfile.ZipFile(jar, "w") as archive:
            archive.write(args.dex, "classes.dex")
        launcher = args.output / "memory-zygote.sh"
        launcher.write_text('''#!/system/bin/sh
while read verb key value; do
    if [ "$verb" = export ]; then export "$key=$value"; fi
done < /data/system/environ/classpath
exec /system/bin/app_process64 -Xzygote "-Xbootclasspath:$BOOTCLASSPATH:/data/local/tmp/android-memory-stress.jar" /system/bin --zygote --abi-list=arm64-v8a --socket-name=zygote --enable-lazy-preload
''')
        for host, guest in [(jar, "/data/local/tmp/android-memory-stress.jar"), (launcher, "/data/local/tmp/memory-zygote.sh"), (args.zygote_client, "/data/local/tmp/android-memory-zygote")]:
            offline("put", "--replace", str(host.resolve()), guest)
    prop = args.output / "waydroid.prop"
    prop.write_text("ro.boot.init_rc=/trames-memory-init.rc\nro.logd.auditd=false\nro.logd.kernel=false\nro.lmk.debug=true\n")
    for host, guest in [(rc, "/trames-memory-init.rc"), (prop, "/vendor/waydroid.prop"),
                        (args.dex, "/data/local/tmp/android-memory-stress.dex")]:
        offline("put", "--replace", str(host.resolve()), guest)
    if args.reaper:
        offline("put", "--replace", str(args.reaper.resolve()), "/data/local/tmp/android-memory-reaper")
        report["reaper_sha256"] = hashlib.sha256(args.reaper.read_bytes()).hexdigest()
    if args.automatic:
        offline("put","--replace",str(args.automatic.resolve()),"/data/local/tmp/android-memory-automatic")
    bootenv = []
    for line in offline("cat", "/init.environ.rc").splitlines():
        fields = line.strip().split(" ", 2)
        if len(fields) == 3 and fields[0] == "export":
            bootenv.append(fields[1] + "=" + fields[2])
    with tempfile.TemporaryDirectory(prefix="trames-memory-") as temporary, (args.output / "boot.log").open("w") as log:
        control = Path(temporary) / "control"
        socket = control.with_suffix(".exec")
        bounded=["--memory-mib","384"] if args.automatic else []
        process = subprocess.Popen(prefix + bounded + ["--android-init", "--exec-socket", str(control), "--cpus", "4",
            "--no-pty", "--no-windows", "--network", "off", "--timeout", "160", "--metrics", "--",
            "/system/bin/init", "second_stage"], env=env, stdout=log, stderr=subprocess.STDOUT)
        report["host_pid"] = process.pid
        save()
        def call(argv, timeout=20, environment=()):
            return foundation.rpc(socket, argv, timeout=timeout, environment=environment)
        try:
            deadline = time.monotonic() + 35
            while time.monotonic() < deadline:
                if process.poll() is not None:
                    raise RuntimeError("original init exited")
                if socket.exists():
                    ready = call(["/system/bin/getprop", "trames.memory.init"])
                    if ready["status"] == 0 and ready["output"].strip() == "ready":
                        break
                time.sleep(.2)
            else:
                raise RuntimeError("original init timeout")
            report["properties"] = call(["/system/bin/getprop"])
            derived = call(["/system/bin/sh", "-c", "mkdir -p /data/system/environ; /apex/com.android.sdkext/bin/derive_classpath && cat /data/system/environ/classpath"])
            if derived["status"]:
                raise RuntimeError("original derive_classpath failed")
            for line in derived["output"].splitlines():
                fields = line.split(" ", 2)
                if len(fields) == 3 and fields[0] == "export":
                    bootenv.append(fields[1] + "=" + fields[2])
            report["environment"] = bootenv
            for sample in range(args.samples):
                reply = call(["/apex/com.android.art/bin/dalvikvm64", "-Xms8m", "-Xmx128m", "-verbose:gc",
                    "-XX:DumpGCPerformanceOnShutdown", "-cp", "/data/local/tmp/android-memory-stress.dex", "AndroidMemoryStress"],
                    timeout=60, environment=bootenv)
                (args.output / f"art-{sample}.txt").write_text(reply["output"])
                match = re.search(r"MEMORY_STRESS (\{.*\})", reply["output"])
                if reply["status"] or not match:
                    raise RuntimeError("original ART stress failed: " + str(reply))
                values = json.loads(match[1])
                if values["allocated_bytes"] != 536870912 or values["checksum"] != 3145728:
                    raise RuntimeError("incomplete ART workload")
                report["gc"].append(values)
                save()
            if args.reaper:
                report["reaper"] = call(["/system/bin/sh", "-c", "chmod 755 /data/local/tmp/android-memory-reaper && /data/local/tmp/android-memory-reaper"], timeout=20)
                if report["reaper"]["status"] or "PASS original AOSP13 Reaper.cpp" not in report["reaper"]["output"]:
                    raise RuntimeError("original reaper component failed")
            if args.zygote_client:
                call(["/system/bin/sh", "-c", "chmod 755 /data/local/tmp/android-memory-zygote; setprop ctl.start memory-zygote"])
                time.sleep(2)
                spawned = call(["/data/local/tmp/android-memory-zygote"], timeout=30)
                report["zygote"] = {"spawn": spawned}
                match = re.search(r"ZYGOTE_CHILD (\d+)", spawned["output"])
                if spawned["status"] or not match:
                    raise RuntimeError("original Zygote spawn failed")
                pid = match[1]
                before = call(["/system/bin/toybox", "cat", "/proc/" + pid + "/status"])
                report["zygote"]["resident"] = before
                if not re.search(r"VmRSS:\s+[1-9]\d* kB", before["output"]):
                    raise RuntimeError("missing live original Zygote child residency")
                deadline = time.monotonic() + 20
                while time.monotonic() < deadline:
                    after = call(["/system/bin/toybox", "cat", "/proc/" + pid + "/status"])
                    if after["status"] or not re.search(r"VmRSS:\s+[1-9]\d* kB", after["output"]):
                        break
                    time.sleep(.25)
                else:
                    raise RuntimeError("original Zygote child did not release its mm")
                report["zygote"]["retired"] = after
                call(["/system/bin/setprop", "ctl.stop", "memory-zygote"])
            if args.automatic:
                report["phase"] = "automatic"
                save()
                report["automatic"] = call(["/system/bin/sh","-c","chmod 755 /data/local/tmp/android-memory-automatic && /data/local/tmp/android-memory-automatic"],timeout=40)
                if report["automatic"]["status"] or "PASS original lmkd automatic PSI reclaim" not in report["automatic"]["output"]:
                    raise RuntimeError("original LMKD automatic reclaim failed: "+str(report["automatic"]))
            report["pressure"] = call(["/system/bin/toybox", "cat", "/proc/pressure/memory", "/proc/vmstat"])
            report["logcat"] = call(["/system/bin/logcat", "-d", "-b", "all"])
            (args.output / "logcat.txt").write_text(report["logcat"]["output"])
            if args.samples and "concurrent copying paused:" not in report["logcat"]["output"]:
                raise RuntimeError("missing original collector/pause evidence")
            if (args.reaper or args.automatic) and re.search(r"process_mrelease \d+ failed: (?!No such process)", report["logcat"]["output"]):
                raise RuntimeError("unexpected original reaper error")
            if args.automatic:
                logs = report["logcat"]["output"]
                victim = re.search(r"victim=(\d+)", report["automatic"]["output"])[1]
                reaped = re.search(r"Process " + victim + r" was reaped in", logs)
                exited_first = re.search(r"process_mrelease " + victim + r" failed: No such process", logs)
                report["automatic"]["reaper_result"] = "reaped" if reaped else "exit_mmap won" if exited_first else "missing"
                if "Using psi monitors for memory pressure detection" not in logs or not re.search(r"Kill '.*' \(" + victim + r"\), .*oom_score_adj 900 to free", logs) or not (reaped or exited_first):
                    raise RuntimeError("missing original PSI selection/reaper log evidence")
            if args.zygote_client:
                logs = report["logcat"]["output"]
                if "MEMORY_STRESS {" not in logs or "invalid LFH signature" in logs:
                    raise RuntimeError("missing clean original post-fork ART workload evidence")
            report["passed"] = True
            parts = (["ART GC"] if args.samples else []) + (["Reaper.cpp"] if args.reaper else []) + (["Zygote post-fork"] if args.zygote_client else []) + (["lmkd automatic PSI reclaim"] if args.automatic else [])
            print("PASS original Android memory lifecycle: " + ", ".join(parts), flush=True)
        except Exception as error:
            report["error"] = str(error)
            if socket.exists():
                try:
                    report["error_logcat"] = call(["/system/bin/logcat", "-d", "-b", "all"], timeout=10)
                except Exception as log_error:
                    report["logcat_error"] = str(log_error)
            raise
        finally:
            process.terminate()
            try:
                process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait()
            save()


if __name__ == "__main__":
    main()
