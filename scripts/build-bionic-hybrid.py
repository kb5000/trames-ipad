#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Build static original/NativeCall Bionic replacement libraries, without AOSP.

Compilation needs only the pinned layout, Clang and the native Rust ELF linker.
The small resource supplies precompiled code and a cold-install recipe; --libc
also verifies/exports complete libraries from the pinned image (not an NDK
stub). Nothing patches a loaded process. Original dynamic symbols, versions,
relocations, TLS and initializers survive.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
ORIGINAL_SHA256 = "6aa289f3b6042793ccacd873ce3724dcaae240943112eec126f7be654f01e6cc"
FUNCTIONS = ("memcpy memmove memset memcmp strlen memchr memrchr strnlen strcmp strncmp "
             "strchr strrchr strchrnul strcpy stpcpy strncpy stpncpy strcat strncat mempcpy strlcpy strlcat").split()
FUNCTIONS += "memmem strstr strcasestr strspn strcspn strpbrk strcasecmp strncasecmp strsep strtok_r strtol strtoll strtoul strtoull strtoimax strtoumax atoi atol atoll strtod strtof atof".split()
IFUNCS = "memchr stpcpy strchr strchrnul strcmp strcpy strlen strncmp strrchr".split()


def align(n, boundary=4096):
    return (n + boundary - 1) & -boundary


class Elf:
    def __init__(self, data):
        self.data = data
        kind, machine = struct.unpack_from("<HH", data, 16)
        if data[:6] != b"\x7fELF\x02\x01" or kind not in (2, 3) or machine != 183:
            raise ValueError("expected little-endian ARM64 ELF")
        self.phoff, self.shoff = struct.unpack_from("<QQ", data, 32)
        phsize, phnum, shsize, shnum, names = struct.unpack_from("<HHHHH", data, 54)
        if phsize != 56 or shsize != 64:
            raise ValueError("unexpected ELF table entry size")
        self.ph = [struct.unpack_from("<IIQQQQQQ", data, self.phoff + i * 56) for i in range(phnum)]
        self.sh = [struct.unpack_from("<IIQQQQIIQQ", data, self.shoff + i * 64) for i in range(shnum)]
        strings = self.section_bytes(self.sh[names])
        self.sections = {strings[s[0]:].split(b"\0", 1)[0].decode(): s for s in self.sh}

    def section_bytes(self, section):
        return self.data[section[4]:section[4] + section[5]]

    def symbols(self, section=".symtab"):
        symbols = self.sections[section]
        strings = self.section_bytes(self.sh[symbols[6]])
        result = {}
        for offset in range(symbols[4], symbols[4] + symbols[5], 24):
            name, info, other, index, address, size = struct.unpack_from("<IBBHQQ", self.data, offset)
            name = strings[name:].split(b"\0", 1)[0].decode()
            if name and index:
                result[name] = dict(address=address, size=size, kind=info & 15)
        return result

    def offset(self, address, size=4):
        for kind, flags, offset, va, pa, filesz, memsz, alignment in self.ph:
            if kind == 1 and va <= address and address + size <= va + filesz:
                return offset + address - va
        raise ValueError(f"unbacked ELF address: {address:#x}")


def branch(source, target):
    distance = target - source
    if distance % 4 or not -(1 << 27) <= distance < (1 << 27):
        raise ValueError("static branch out of range")
    return 0x14000000 | ((distance >> 2) & 0x3ffffff)


def displaced(word, source, destination):
    """Relocate precisely one entry instruction, never an arbitrary code block."""
    if word & 0x1f000000 == 0x10000000:
        immediate = ((word >> 29) & 3) | (((word >> 5) & 0x7ffff) << 2)
        if immediate & 0x100000: immediate -= 0x200000
        target = ((source & -4096) + (immediate << 12)) if word & 0x80000000 else source + immediate
        delta = ((target & -4096) - (destination & -4096)) >> 12
        if not -(1 << 20) <= delta < (1 << 20): raise ValueError('ADRP relocation out of range')
        register = word & 31
        adrp = 0x90000000 | ((delta & 3) << 29) | (((delta >> 2) & 0x7ffff) << 5) | register
        if word & 0x80000000: return [adrp, branch(destination + 4, source + 4)]
        add = 0x91000000 | ((target & 4095) << 10) | (register << 5) | register
        return [adrp, add, branch(destination + 8, source + 4)]
    # CBZ/CBNZ or B.cond: inverted short branch skips a long-range B.
    if word & 0x7e000000 == 0x34000000 or word & 0xff000010 == 0x54000000:
        immediate = (word >> 5) & 0x7ffff
        if immediate & 0x40000:
            immediate -= 0x80000
        inverse = word ^ (0x1000000 if word & 0x7e000000 == 0x34000000 else 1)
        inverse = (inverse & ~(0x7ffff << 5)) | (2 << 5)
        return [inverse, branch(destination + 4, source + immediate * 4), branch(destination + 8, source + 4)]
    # The pinned regular entries use BTI, integer/NEON arithmetic, or STP.
    # Refuse PC-relative instructions instead of silently corrupting them.
    if (word & 0x1f000000 == 0x10000000 or word & 0x7c000000 == 0x14000000
            or word & 0x3b000000 == 0x18000000 or word & 0x7e000000 == 0x36000000):
        raise ValueError(f"unsupported PC-relative entry instruction: {word:#x}")
    return [word, branch(destination + 4, source + 4)]


def build(original, output, cc, jemalloc=False):
    layout_path = ROOT / "accelerator/android/bionic-layout.json"
    layout = json.loads(layout_path.read_text())
    if layout["original_sha256"] != ORIGINAL_SHA256 or set(layout["functions"]) != set(FUNCTIONS):
        raise ValueError("unsupported Bionic layout")
    source = original.read_bytes() if original else None
    if source is not None and hashlib.sha256(source).hexdigest() != ORIGINAL_SHA256:
        raise ValueError("Bionic input does not match the pinned Waydroid libc; no files replaced")
    output.mkdir(parents=True, exist_ok=True)
    file_offset = align(layout["original_size"])
    address = align(max(p[3] + p[6] for p in layout["program_headers"] if p[0] == 1))
    phnum = len(layout["program_headers"]) + 1
    text_address = address + align(phnum * 56, 16)
    # Relocated original entry prefixes are private fallback functions. They
    # resume the untouched body; no clone of allocator, TLS or Bionic C state.
    targets = {name: entry.get("bulk", entry["body"])["address"] for name, entry in layout["functions"].items()}
    if jemalloc:
        targets.update({"jemalloc_" + name: entry["address"] for name, entry in layout['jemalloc'].items()})
    original_entries = sorted(set(targets.values()))
    trampoline_size = 16
    trampolines = {entry: text_address + i * trampoline_size for i, entry in enumerate(original_entries)}
    trampoline_words = []
    for entry in original_entries:
        body = next((v.get("bulk", v["body"]) for v in layout["functions"].values()
                    if v.get("bulk", v["body"])["address"] == entry), None)
        if body is None: body = next(v for v in layout['jemalloc'].values() if v['address'] == entry)
        word = int.from_bytes(bytes.fromhex(body["original"]), "little")
        prefix = body.get("prefix", [])
        words = prefix + displaced(word, entry, trampolines[entry] + 4 * len(prefix))
        if len(words) > trampoline_size // 4:
            raise ValueError("original Bionic continuation exceeds its static slot")
        trampoline_words.extend(words + [0xd503201f] * (trampoline_size // 4 - len(words)))
    wrapper_address = text_address + len(trampoline_words) * 4
    rust = Path(subprocess.check_output(["rustc", "--print", "sysroot"], text=True).strip())
    host = next(s.split(": ", 1)[1] for s in subprocess.check_output(["rustc", "-vV"], text=True).splitlines() if s.startswith("host: "))
    linker = rust / "lib/rustlib" / host / "bin/rust-lld"
    compiler = [cc, "--target=aarch64-linux-android33", "-O2", "-ffreestanding", "-fno-builtin",
                "-fno-stack-protector", "-fno-unwind-tables", "-fno-asynchronous-unwind-tables",
                "-fPIC", "-fvisibility=hidden", "-mno-outline-atomics", "-Wall", "-Wextra", "-Werror"]
    with tempfile.TemporaryDirectory(prefix="build-", dir=output) as temporary:
        tmp = Path(temporary)
        subprocess.run(compiler + ["-c", str(ROOT / "accelerator/android/bionic.c"), "-o", str(tmp / "wrappers.o")], check=True)
        subprocess.run(compiler + ["-c", str(ROOT / "accelerator/guest/calls.S"), "-o", str(tmp / "calls.o")], check=True)
        subprocess.run(compiler + ["-c", str(ROOT / "accelerator/android/bionic-entry.S"), "-o", str(tmp / "entries.o")], check=True)
        subprocess.run(compiler + ["-c", str(ROOT / "accelerator/android/bionic-calls.S"), "-o", str(tmp / "bionic-calls.o")], check=True)
        subprocess.run(compiler + ["-c", str(ROOT / "accelerator/musl/integer-calls.S"), "-o", str(tmp / "integers.o")], check=True)
        if jemalloc:
            subprocess.run(compiler + ["-c", str(ROOT / "accelerator/android/bionic-jemalloc.S"), "-o", str(tmp / "jemalloc.o")], check=True)
        script = tmp / "native.ld"
        script.write_text(f"SECTIONS {{ . = {wrapper_address}; .text : {{ *(.text*) *(.rodata*) }} .data : {{ *(.data*) *(.bss*) }} /DISCARD/ : {{ *(.comment) *(.note*) *(.eh_frame*) }} }}\nASSERT(SIZEOF(.data) == 0, \"Bionic wrappers must have no writable host state\")\n")
        command = [str(linker), "-flavor", "gnu", "-m", "aarch64elf", "--no-undefined", "--build-id=none", "--entry=trames_bionic_large_memcpy", "-T", str(script)]
        command += [f"--defsym=trames_orig_{name}={trampolines[entry]}" for name, entry in targets.items()]
        command += [f"--defsym=trames_original_small_copy={layout['functions']['memcpy']['body']['address']}"]
        command += [f"--defsym={name}={address}" for name, address in layout['references'].items()]
        if jemalloc:
            command += [f"--defsym=trames_jemalloc_{name}_original={trampolines[entry['address']]}" for name, entry in layout['jemalloc'].items()]
            command += [f"--defsym=trames_jemalloc_{name}_resume={entry['resume']}" for name, entry in layout['jemalloc'].items()]
        command += [str(tmp / name) for name in ['wrappers.o', 'calls.o', 'entries.o', 'bionic-calls.o', 'integers.o'] + (['jemalloc.o'] if jemalloc else [])]
        command += ["-o", str(tmp / "payload.so")]
        subprocess.run(command, check=True)
        payload = Elf((tmp / "payload.so").read_bytes())
        text = payload.sections[".text"]
        if text[3] != wrapper_address or payload.sections.get(".rela.dyn", (0,) * 6)[5]:
            raise ValueError("Bionic payload has dynamic relocations or unexpected placement")
        wrappers = payload.symbols()
        code = struct.pack("<" + "I" * len(trampoline_words), *trampoline_words) + payload.section_bytes(text)
    patches = []
    def patch(name, entry, replacement):
        before = bytes.fromhex(entry["original"])
        if len(before) != len(replacement):
            raise ValueError("static Bionic patch length mismatch")
        patches.append(dict(name=name, offset=entry["offset"], original=before.hex(), nativecall=replacement.hex()))
    seen = set()
    for name in FUNCTIONS:
        function = layout["functions"][name]
        target = wrappers[("trames_bionic_large_" if "bulk" in function else "trames_bionic_") + name]["address"]
        entries = [function.get("bulk", function["body"])]
        if name in IFUNCS:
            # Keep STT_GNU_IFUNC and every symbol version. The resolver now
            # returns a compile-time-selected address, with no HWCAP dispatch.
            resolver = function["resolver"]["address"]
            delta = (target >> 12) - (resolver >> 12)
            if not -(1 << 20) <= delta < (1 << 20):
                raise ValueError("IFUNC static address out of ADRP range")
            adrp = 0x90000000 | ((delta & 3) << 29) | (((delta >> 2) & 0x7ffff) << 5)
            words = [adrp, 0x91000000 | ((target & 4095) << 10), 0xd65f03c0]
            patch(name + "_resolver", function["resolver"], struct.pack("<3I", *words))
            mte = function.get("mte")
            if mte:
                entries.append(mte)
        for entry in entries:
            if entry["address"] in seen:
                continue
            seen.add(entry["address"])
            patch(name, entry, struct.pack("<I", branch(entry["address"], target)))
    if jemalloc:
        for name, entry in layout['jemalloc'].items():
            patch('jemalloc_' + name, entry, struct.pack('<I', branch(entry['address'], wrappers['trames_jemalloc_' + name]['address'])))
    ph = list(layout["program_headers"])
    segment_size = text_address - address + len(code)
    ph[0] = (6, 4, file_offset, address, address, phnum * 56, phnum * 56, 8)
    ph.append((1, 5, file_offset, address, address, segment_size, segment_size, 4096))
    tail = (bytes(file_offset - layout["original_size"])
            + b"".join(struct.pack("<IIQQQQQQ", *p) for p in ph)
            + bytes(text_address - address - phnum * 56) + code)
    patch("program_header_offset", dict(offset=32, original=struct.pack("<Q", layout["program_header_offset"]).hex()), struct.pack("<Q", file_offset))
    patch("program_header_count", dict(offset=56, original=struct.pack("<H", phnum - 1).hex()), struct.pack("<H", phnum))
    # Keep the upstream build ID for build-ID-based Android debug symbols. The
    # manifest's SHA-256 identifies the derivative, not that original build ID.
    if source is not None:
        patched = bytearray(source)
        for p in patches:
            old, new, offset = bytes.fromhex(p["original"]), bytes.fromhex(p["nativecall"]), p["offset"]
            if patched[offset:offset + len(old)] != old:
                raise ValueError("Bionic layout differs from the original image")
            patched[offset:offset + len(new)] = new
        patched.extend(tail)
        if Elf(patched).symbols(".dynsym") != Elf(source).symbols(".dynsym"):
            raise ValueError("Bionic dynamic ABI changed")
        (output / "libc-original.so").write_bytes(source)
        (output / "libc-nativecall.so").write_bytes(patched)
    (output / "bionic-nativecall.bin").write_bytes(tail)
    inputs = [Path(__file__), layout_path, ROOT / "accelerator/android/bionic.c", ROOT / "accelerator/android/bionic-entry.S", ROOT / "accelerator/android/bionic-calls.S", ROOT / "accelerator/android/bionic-jemalloc.S", ROOT / "accelerator/guest/calls.S", ROOT / "accelerator/musl/integer-calls.S"]
    manifest = dict(schema=1, kind="bionic-static", android_api=33, abi="arm64-v8a",
                    original_sha256=ORIGINAL_SHA256, original_size=layout["original_size"], native_profile=65537, functions=FUNCTIONS,
                    bionic_profile=65545, jemalloc=jemalloc,
                    tail_sha256=hashlib.sha256(tail).hexdigest(), patches=patches,
                    added_code_bytes=len(code), added_file_bytes=len(tail),
                    original_paths=["/system/apex/com.android.runtime/lib64/bionic/libc.so", "/system/lib64/bootstrap/libc.so"],
                    compiler=subprocess.check_output([cc, "--version"], text=True).splitlines()[0],
                    sources_sha256={str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest() for p in inputs})
    (output / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    shutil.copyfile(ROOT / "LICENSE", output / "GPL-3.0.txt")
    return manifest


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--libc", type=Path, help="also verify and export complete libraries from this pinned original")
    parser.add_argument("--output", type=Path, default=ROOT / "target/bionic-hybrid")
    parser.add_argument("--cc", default=os.environ.get("CC", "clang"))
    parser.add_argument("--jemalloc", action="store_true", help="experimental native guest-tcache blocks (default: original allocator)")
    args = parser.parse_args()
    manifest = build(args.libc, args.output.resolve(), args.cc, jemalloc=args.jemalloc)
    print(json.dumps({k: manifest[k] for k in ["functions", "tail_sha256", "added_code_bytes", "added_file_bytes"]}, indent=2))


if __name__ == "__main__":
    main()
