#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Original Android loader, 44 libc functions and cold static-library selection.

Uses small disposable stores; never boots Android or touches a registered
environment/device. The original libc is an input from the pinned image.
"""
import argparse
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import shutil
import statistics
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def module(path):
    spec = importlib.util.spec_from_file_location("bionic_builder", path)
    result = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(result)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rootfs", type=Path, required=True, help="extracted pinned Waydroid image (read only)")
    parser.add_argument("--runner", type=Path, default=ROOT / "target/release/trames-run")
    parser.add_argument("--output", type=Path, default=ROOT / "target/bionic-hybrid/acceptance")
    parser.add_argument("--cache", type=Path, default=ROOT / "target/android-toolchain")
    parser.add_argument("--bench-rounds", type=int, default=3)
    parser.add_argument("--baseline-bundle", type=Path, help="also compare a previously validated static Bionic bundle")
    parser.add_argument("--baseline-runner", type=Path, help="paired previous runner for a product before/after comparison")
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    rootfs, output, cache = args.rootfs.resolve(), args.output.resolve(), args.cache.resolve()
    builder = module(ROOT / "scripts/build-bionic-hybrid.py")
    bionic = "/system/apex/com.android.runtime/lib64/bionic"
    original = rootfs / (bionic[1:] + "/libc.so")
    bundle = output / "bundle"
    manifest = builder.build(original, bundle, "clang")
    lib = cache / "sysroot/usr/lib/aarch64-linux-android/33"
    probe = output / "android-bionic"
    subprocess.run(["clang", "--target=aarch64-linux-android33", "--sysroot=" + str(cache / "sysroot"),
                    "--ld-path=" + str(cache / "bin/ld.lld"), "-O2", "-fno-builtin", "-mno-outline-atomics",
                    "-Wall", "-Wextra", "-Werror", "-fPIE", "-pie", "-nostdlib", str(lib / "crtbegin_dynamic.o"),
                    str(ROOT / "tests/fixtures/android-bionic.c"), "-L" + str(lib), "-Wl,--no-undefined",
                    "-Wl,--dynamic-linker,/system/bin/linker64", "-lc", "-ldl", "-lm", str(lib / "crtend_android.o"),
                    "-o", str(probe)], check=True)
    if args.bench_rounds < 1:
        parser.error("--bench-rounds must be positive")
    results = dict(original_sha256=builder.ORIGINAL_SHA256, manifest=manifest, cases={}, benchmark={},
                   jemalloc_experiment={},
                   inputs={str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest()
                           for p in [Path(__file__), ROOT / "tests/fixtures/android-bionic.c"]},
                   runner_sha256=hashlib.sha256(args.runner.read_bytes()).hexdigest())
    if args.baseline_runner:
        results["baseline_runner_sha256"] = hashlib.sha256(args.baseline_runner.read_bytes()).hexdigest()
    if args.baseline_bundle:
        results["baseline_manifest"] = json.loads((args.baseline_bundle / "manifest.json").read_text())
    with tempfile.TemporaryDirectory(prefix="stores-", dir=output) as temporary:
        store = Path(temporary) / "store"
        prefix = [str(args.runner.resolve()), "--storage", str(store), "--cpus", "4", "--metrics",
                  "--max-instructions", "4000000000", "--memory-budget", "512MiB", "--process-memory-budget", "512MiB",
                  "--env", "LD_LIBRARY_PATH=" + bionic]
        files = {bionic + "/libc.so": original, "/system/lib64/bootstrap/libc.so": original,
                 bionic + "/libdl.so": rootfs / (bionic[1:] + "/libdl.so"),
                 bionic + "/libm.so": rootfs / (bionic[1:] + "/libm.so"),
                 "/system/bin/linker64": rootfs / "system/apex/com.android.runtime/bin/linker64",
                 "/bionic-probe": probe}
        imports = [part for guest, host in files.items() for part in ["--exec-file", guest + "=" + str(host)]]

        def run(name, accelerator, extra=(), argument=None, expected=0, runner=None):
            command = prefix + ["--accelerator", accelerator] + list(extra)
            if runner: command[0] = str(runner.resolve())
            command += ["--guest", "/system/bin/linker64", "/bionic-probe"] + ([argument] if argument else [])
            done = subprocess.run(command, capture_output=True, text=True, timeout=120)
            text = done.stdout + done.stderr
            (output / (name + ".log")).write_text(text)
            if done.returncode != expected:
                raise RuntimeError(f"{name}: exit {done.returncode}, expected {expected}: " + text[-2500:])
            metrics = {}
            for line in text.splitlines():
                if line.startswith("trames native metrics: "):
                    metrics = json.loads(line.split(": ", 1)[1])
            results["cases"][name] = dict(exit=done.returncode, native=metrics)
            return text, metrics

        text, metrics = run("original", "off", imports)
        assert "PASS Bionic 44 functions" in text and metrics["calls"] == 0
        numeric_reference = [line for line in text.splitlines() if line.startswith(('NUMERIC ', 'FLOAT '))]
        run('allocator-original-info', 'off', argument='allocator-info')
        run('allocator-original-cache-info', 'off', argument='allocator-cache-info')
        select = ["--bionic-bundle", str(bundle)]
        export = Path(temporary) / "selected-libc.so"
        text, metrics = run("nativecall", "pure", select + ["--export", bionic + "/libc.so=" + str(export)])
        assert "PASS Bionic 44 functions" in text and min(metrics["function_calls"][:19]) > 0
        actual = [line for line in text.splitlines() if line.startswith(('NUMERIC ', 'FLOAT '))]
        if actual != numeric_reference:
            differences = [dict(original=x, native=y) for x, y in zip(numeric_reference, actual) if x != y]
            (output / 'numeric-differences.json').write_text(json.dumps(differences, indent=2))
            raise RuntimeError('Bionic numeric differential mismatch: ' + str(differences[:3]))
        assert metrics["calls"] == metrics["worker_calls"] and not metrics["rejected"] and not metrics["faults"]
        assert export.read_bytes() == (bundle / "libc-nativecall.so").read_bytes()
        run('allocator-native-info', 'pure', select, argument='allocator-info')
        run('allocator-native-cache-info', 'pure', select, argument='allocator-cache-info')
        run('allocator-original-cache-stress', 'off', select, argument='allocator-cache-stress')
        experimental = output / 'jemalloc-bundle'
        results['jemalloc_manifest'] = builder.build(original, experimental, 'clang', jemalloc=True)
        allocator_select = ['--bionic-bundle', str(experimental)]
        text, cache_metrics = run('allocator-native-cache-stress', 'pure', allocator_select, argument='allocator-cache-stress')
        assert min(cache_metrics['bionic_operations'][10:12]) > 0 and not cache_metrics['faults']
        allocator_variants = [('original', 'off', select), ('nativecall', 'pure', select),
                              ('jemalloc', 'pure', allocator_select)]
        for round in range(args.bench_rounds):
            for variant, accelerator, selection in allocator_variants[::1 if round % 2 == 0 else -1]:
                text, _ = run(f'allocator-{variant}-cache-bench-{round + 1}', accelerator, selection, argument='allocator-cache-bench')
                line = next(line for line in text.splitlines() if line.startswith('BENCH malloc-free-64 '))
                results['jemalloc_experiment'].setdefault(variant, []).append(int(line.split()[-1]))
        run("nativecall-noop", "pure", select)
        # Same executable code with harmless trailing, unmapped bytes gives a
        # distinct installed variant. Exercise real provenance-based upgrade
        # and downgrade, including loading/executing both complete libraries.
        upgrade = Path(temporary) / "upgraded-bundle"
        upgrade.mkdir()
        upgraded_tail = (bundle / "bionic-nativecall.bin").read_bytes() + bytes(4)
        upgraded_manifest = dict(manifest, added_file_bytes=len(upgraded_tail),
                                 tail_sha256=hashlib.sha256(upgraded_tail).hexdigest())
        (upgrade / "manifest.json").write_text(json.dumps(upgraded_manifest))
        (upgrade / "bionic-nativecall.bin").write_bytes(upgraded_tail)
        text, _ = run("variant-upgrade", "pure", ["--bionic-bundle", str(upgrade)])
        assert "PASS Bionic 44 functions" in text
        text, _ = run("variant-downgrade", "pure", select)
        assert "PASS Bionic 44 functions" in text
        restored = Path(temporary) / "restored-libc.so"
        text, metrics = run("restored", "off", select + ["--export", bionic + "/libc.so=" + str(restored)])
        assert "PASS Bionic 44 functions" in text and metrics["calls"] == 0
        assert restored.read_bytes() == original.read_bytes()
        # Tail corruption is rejected before either libc is replaced.
        bad = Path(temporary) / "bad-bundle"
        bad.mkdir()
        shutil.copyfile(bundle / "manifest.json", bad / "manifest.json")
        damaged = bytearray((bundle / "bionic-nativecall.bin").read_bytes()); damaged[-1] ^= 1
        (bad / "bionic-nativecall.bin").write_bytes(damaged)
        text, _ = run("corrupt-resource", "pure", ["--bionic-bundle", str(bad)], expected=125)
        assert "digest mismatch" in text
        # Fail on the second inode after the first was replaced. Disabling
        # thunking must still recover the first inode without a completed record.
        run("forget-record", "off", argument="forget-record")
        (output / "conflict").write_text("retained staging conflict")
        text, _ = run("partial-install", "pure", select + ["--file", "/system/lib64/bootstrap/libc.so.trames-new=" + str(output / "conflict")], expected=125)
        run("recover-original", "off", select, argument="cleanup")
        run("after-recovery", "off", select)
        # An unsupported second original must prevent replacement of the first.
        unknown = Path(temporary) / "unknown.so"
        changed = bytearray(original.read_bytes()); changed[-1] ^= 1
        unknown.write_bytes(changed)
        unknown_imports = [part for guest, host in files.items() for part in
                           ["--exec-file", guest + "=" + str(unknown if guest == "/system/lib64/bootstrap/libc.so" else host)]]
        prefix[2] = str(Path(temporary) / "unsupported-store")
        run("unsupported-image", "pure", unknown_imports + select, expected=125)
        preserved = Path(temporary) / "preserved.so"
        run("unsupported-preserved", "off", ["--export", bionic + "/libc.so=" + str(preserved)], argument="bench")
        assert preserved.read_bytes() == original.read_bytes()
        prefix[2] = str(store)
        # Compare exactly the same Bionic binary/fixture under the interpreter.
        variants = [("original", "off", select), ("nativecall", "pure", select)]
        if args.baseline_bundle:
            variants.append(('previous', 'pure', ['--bionic-bundle', str(args.baseline_bundle.resolve())]))
        for round in range(args.bench_rounds):
            for variant, accelerator, selection in variants[::1 if round % 2 == 0 else -1]:
                text, _ = run(f"bench-{variant}-{round + 1}", accelerator, selection, argument="bench",
                              runner=args.baseline_runner if variant == "previous" else None)
                for line in text.splitlines():
                    if line.startswith("BENCH "):
                        _, name, size, count, ns = line.split()
                        sample = results["benchmark"].setdefault(name, {}).setdefault(variant,
                            dict(bytes=int(size), count=int(count), samples_ns=[]))
                        sample["samples_ns"].append(int(ns))
    for variants in results["benchmark"].values():
        for sample in variants.values():
            sample["nanoseconds"] = statistics.median(sample["samples_ns"])
        variants["speedup"] = variants["original"]["nanoseconds"] / variants["nativecall"]["nanoseconds"]
        if 'previous' in variants:
            variants['previous_speedup'] = variants['previous']['nanoseconds'] / variants['nativecall']['nanoseconds']
    (output / "summary.json").write_text(json.dumps(results, indent=2) + "\n")
    print(json.dumps(dict(cases={k: v["exit"] for k, v in results["cases"].items()}, benchmark=results["benchmark"]), indent=2))


if __name__ == "__main__":
    main()
