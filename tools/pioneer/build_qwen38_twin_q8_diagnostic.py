#!/usr/bin/env python3
"""Link the exact private Q8 diagnostic. Never stage, install or open a device."""
from hashlib import sha256
import json
import os
from pathlib import Path
import re
import subprocess
import sys

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
sys.path.insert(0, str(HERE / "tests"))
from build_qwen38_twin_q8_aperture_compile_only import kernel_flags, payload_flags

API = {"api_version", "init_hardware", "init_driver", "uninit_driver",
       "publish_devices", "find_device"}
LABELS = ("globalworker", "precise", "dense", "ffn", "service", "fetch",
          "affine", "text", "gdn", "boot")


def verify_image(tool, binary, out, manifest, capture, run):
    symbols = capture([tool + "objdump", "-t", binary])
    sections = {}
    for line in capture([tool + "objdump", "-h", binary]).splitlines():
        entry = line.split()
        if len(entry) >= 7 and entry[0].isdigit():
            sections[entry[1]] = int(entry[3], 16)
    arrays, dumps = {}, {}
    for line in symbols.splitlines():
        row = line.split()
        if len(row) < 6 or row[2] != "O" or "PioneerQ38TwinQ8Payload" not in row[-1]:
            continue
        match = re.search(r"(Section[01]_[0-9]|Weights[01])E$", row[-1])
        if not match:
            continue
        name, section = match.group(1), row[-3]
        assert name not in arrays and section.startswith(".rodata") and section in sections, row
        if section not in dumps:
            target = out / (section.lstrip(".") + ".bin")
            run([tool + "objcopy", "-O", "binary", "-j", section, binary, target])
            dumps[section] = target.read_bytes()
        offset, size = int(row[0], 16) - sections[section], int(row[-2], 16)
        assert size > 0 and 0 <= offset <= len(dumps[section]) - size, row
        arrays[name] = sha256(dumps[section][offset:offset + size]).hexdigest()
    assert len(arrays) == 22, arrays
    for arm in range(2):
        assert arrays[f"Weights{arm}"] == manifest["weight_sha256"][arm]
        for index, label in enumerate(LABELS):
            assert arrays[f"Section{arm}_{index}"] == manifest["section_sha256"][arm][label]
    return arrays


def main():
    if len(sys.argv) != 4 or not __debug__:
        raise SystemExit("use ABSOLUTE_HAIKU_SDK ABSOLUTE_PINNED_PAYLOAD ABSOLUTE_NEW_OUTPUT")
    sdk, payload, out = map(Path, sys.argv[1:])
    assert sdk.is_absolute() and sdk.is_dir() and out.is_absolute() and not out.is_symlink()
    assert not any(k.startswith(("PIONEER_", "TT_PIONEER_")) for k in os.environ)
    tool, flags = kernel_flags(sdk, ROOT)
    cxx = flags[0]
    source_dir = ROOT / "src/add-ons/kernel/drivers/misc"
    source = source_dir / "pioneer_native_q38_twin_q8_driver.cpp"
    manifest, extra = payload_flags(payload, ROOT)
    includes = flags[flags.index("-isystem"):]
    out.mkdir(mode=0o700)

    def run(args, **options):
        return subprocess.run(args, cwd=out, check=True, **options)

    def capture(args):
        return run(args, capture_output=True, text=True).stdout

    def toolfile(name):
        return capture([cxx, "-print-file-name=" + name]).strip()

    refusals = [
        ["-UPIONEER_Q38_TWIN_Q8_NATIVE_APERTURE"],
        ["-UPIONEER_Q38_TWIN_Q8_NATIVE_APERTURE", "-DPIONEER_Q38_TWIN_Q8_NATIVE_APERTURE=0"],
        ["-UTT_PIONEER_DRIVER_RELOAD"],
        ["-UTT_PIONEER_EXPERIMENTAL_Q38_DISPATCH"],
        ["-UPIONEER_DRIVER_BUILD_TAG", "-DPIONEER_DRIVER_BUILD_TAG=0x2026111700010220ULL"],
        ["-ffast-math"],
        *(["-D" + name + "=1"] for name in ("TT_PIONEER_Q38_WORK_MODE", "PIONEER_Q38_WORK_INFERENCE",
          "PIONEER_Q38_CPU_VALIDATION", "PIONEER_Q38_FULL_OFFLOAD_NATIVE", "PIONEER_NATIVE_BFP_DIAGNOSTIC")),
    ]
    for alternate in refusals:
        result = subprocess.run([*flags, *extra, *alternate, "-E", source], cwd=out,
                                stdout=subprocess.DEVNULL, stderr=subprocess.PIPE, text=True)
        assert result.returncode and ("Exact tag221 comparison driver role required" in result.stderr
                                      or "Twin Q8 diagnostic cannot share" in result.stderr), alternate

    obj = out / "native.o"
    profile = out / "profile.o"
    run([*flags, *extra, "-c", source, "-o", obj])
    run([*flags, "-c", source_dir / "PioneerPciProfile.cpp", "-o", profile])
    release = sdk / "objects/haiku/riscv64/release"
    glue = release / "system/glue"
    binary = out / "tenstorrent_pci_probe-twin-q8-221"
    run([cxx, "-shared", "-nostdlib", "-Wl,--no-undefined,-soname=tenstorrent_pci_probe",
         toolfile("crtbeginS.o"), glue / "haiku_version_glue.o", obj, profile,
         release / "system/kernel/kernel.so", toolfile("libgcc.a"), toolfile("crtendS.o"),
         "-o", binary])
    exports = capture([tool + "nm", "-D", "--defined-only", binary])
    assert API <= {row.split()[-1] for row in exports.splitlines() if row.split()}
    legacy = "PioneerTwinQ8InertLegacyFind"
    disassembly = capture([tool + "objdump", "-d", f"--disassemble=_Z{len(legacy)}{legacy}PKc", binary])
    instructions = [m.groups() for m in re.finditer(
        r"^\s*[0-9a-f]+:\s+[0-9a-f]+\s+([a-z.]+)\s*([^\n]*)$", disassembly, re.M)]
    assert [(op, args.strip()) for op, args in instructions] == [
        ("add", "sp,sp,-16"), ("sd", "s0,8(sp)"), ("add", "s0,sp,16"), ("ld", "s0,8(sp)"),
        ("li", "a0,0"), ("add", "sp,sp,16"), ("ret", "")]
    (out / "legacy-entry.disassembly").write_text(disassembly)
    arrays = verify_image(tool, binary, out, manifest, capture, run)
    frames = [line.split("\t") for line in obj.with_suffix(".su").read_text().splitlines()
              if "TwinQ8" in line and "InertLegacyFind" not in line]
    assert frames and all(len(row) == 3 and row[2] == "static" for row in frames), frames
    frame_max = max(int(row[1]) for row in frames)
    assert frame_max <= 2048, frame_max

    client = out / "pioneer-q38-twin-q8-compare"
    sha = out / "sha2.o"
    warnings = ["-O2", "-Wall", "-Wextra", "-Werror"]
    # Preserve the bundled libsolv source; only its pre-existing C style warnings are exempt.
    run([tool + "gcc", "-std=gnu11", *warnings, "-Wno-old-style-declaration",
         "-Wno-array-parameter", *includes, "-c",
         ROOT / "src/libs/libsolv/solv/sha2.c", "-o", sha])
    gcc_libs = ["-L" + str(p) for p in sorted((sdk / "build_packages").glob("gcc_syslibs-*/lib"))]
    assert gcc_libs
    run([cxx, "-std=gnu++17", *warnings, *includes, HERE / "pioneer_q38_twin_q8_compare.cpp",
         sha, "-B" + str(glue) + "/", "-L" + str(release / "system/libroot"), *gcc_libs, "-o", client])
    imports = capture([tool + "nm", "-u", client])
    assert "llama_" not in imports and "ggml_" not in imports
    output = {"build_only": True, "tag": "2026111700010221", "arrays": arrays,
              "static_new_frame_max": frame_max, "role_refusals": len(refusals),
              **{p.name: sha256(p.read_bytes()).hexdigest() for p in (binary, client)}}
    (out / "manifest.json").write_text(json.dumps(output, sort_keys=True, indent=2) + "\n")
    for p in (binary, client, out / "manifest.json"):
        print(sha256(p.read_bytes()).hexdigest(), p)
    print(f"PASS tag221 link: 22 verified arrays, {len(refusals)} role refusals; "
          f"new static frame max {frame_max} B; BUILD ONLY")


if __name__ == "__main__":
    main()
