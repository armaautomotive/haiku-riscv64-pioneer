#!/usr/bin/env python3
"""Compile the exact Haiku adapter into an intentionally non-linkable object."""
from pathlib import Path
import hashlib
import json
import os
import re
import subprocess
import sys

PAYLOAD_SHA = "8b29db0d577e2e1929faefa3a1ce5b23c09c241d7a413560abe0722b4f7de32f"
MANIFEST_SHA = "b2722e2c31c78cb96f481a17864918405462585870e481f8ed538f15619dcc3e"


def kernel_flags(sdk, root):
    tool = str(sdk / "cross-tools-riscv64/bin/riscv64-unknown-haiku-")
    release = sdk / "objects/haiku/riscv64/release"
    headers = sdk / "objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop/headers"
    include = []
    for sub in ("kernel", "kernel/arch/riscv64", "kernel/arch/generic", "", "drivers",
                "system", "system/arch/riscv64", "shared"):
        include.extend(("-isystem", str(root / "headers/private" / sub)))
    include.extend(("-isystem", str(root / "build/config_headers"), "-I" + str(release / "system/kernel"),
                    "-I" + str(headers), "-idirafter", str(headers / "posix")))
    for path in (headers / "os", *sorted((headers / "os").iterdir())):
        if path.is_dir():
            include.extend(("-isystem", str(path)))
    roles = ('BANKS', 'PANEL', 'AFFINE', 'BLOCK', 'STREAM', 'ROWS', 'COHORT',
             'COHORT_SEALED', 'COHORT_PAYLOAD', 'DISPATCH', 'DISPATCH_VECTOR')
    flags = [tool + "g++", "-std=gnu++17", "-O2", "-Wall", "-Wextra", "-Werror", "-fno-exceptions",
             "-fno-rtti", "-fstack-usage", "-ffreestanding", "-finline", "-fno-semantic-interposition",
             "-fno-tree-vectorize", "-fno-use-cxa-atexit", "-fno-strict-aliasing",
             "-fno-delete-null-pointer-checks", "-mcmodel=medany", "-fpic", "-D_KERNEL_MODE",
             "-DTT_PIONEER_DRIVER_RELOAD", "-DPIONEER_DRIVER_BUILD_TAG=0x2026111700010222ULL",
             "-DPIONEER_Q38_TWIN_Q8_NATIVE_APERTURE=1", *("-DTT_PIONEER_EXPERIMENTAL_Q38_" + r for r in roles),
             *include]
    return tool, flags


def payload_flags(payload, root):
    assert payload.is_absolute()
    record = (payload / "manifest.json").read_bytes()
    assert hashlib.sha256(record).hexdigest() == MANIFEST_SHA
    manifest = json.loads(record)
    assert hashlib.sha256((payload / "PioneerQ38TwinQ8Payload.h").read_bytes()).hexdigest() == manifest["payload_sha256"] == PAYLOAD_SHA
    return manifest, ["-fdata-sections", "-I" + str(payload), "-I" + str(root / "src/add-ons/kernel/drivers/misc")]


def main():
    if len(sys.argv) not in (3, 4) or not __debug__:
        raise SystemExit("use ABSOLUTE_HAIKU_SDK_BUILD ABSOLUTE_NEW_OUTPUT [ABSOLUTE_GENERATED_PAYLOAD_DIRECTORY]")
    sdk, out = map(Path, sys.argv[1:3])
    payload = Path(sys.argv[3]) if len(sys.argv) == 4 else None
    assert sdk.is_absolute() and sdk.is_dir() and out.is_absolute()
    assert not any(k.startswith(("PIONEER_", "TT_PIONEER_")) for k in os.environ)
    here = Path(__file__).resolve().parent
    root = here.parents[2]
    tool, flags = kernel_flags(sdk, root)
    if payload:
        manifest, extra = payload_flags(payload, root)
        flags.extend(["-DPIONEER_Q38_TWIN_Q8_COMPILE_PAYLOAD=1", *extra])
    out.mkdir(mode=0o700)
    source = here / "pioneer_q38_twin_q8_aperture_kernel_compile.cpp"
    obj = out / "aperture-NODRV.o"
    subprocess.run([*flags, "-c", source, "-o", obj], cwd=out, check=True)
    defined = subprocess.check_output([tool + "nm", "--defined-only", obj], text=True)
    names = {line.split()[-1] for line in defined.splitlines() if line.split()}
    assert not names & {"api_version", "init_hardware", "init_driver", "uninit_driver", "publish_devices", "find_device"}
    assert "q8_aperture_compile_only" in names
    undefined = subprocess.check_output([tool + "nm", "-uC", obj], text=True)
    for symbol in ("Q8CompileOnlyOwner::Scope()", "Q8CompileOnlyOwner::WorkerStopProved()",
                   "map_physical_memory", "delete_area", *(() if payload else ("Q8CompileOnlyRom::Section(",))):
        assert symbol in undefined, symbol
    if payload:
        table = subprocess.check_output([tool + "objdump", "-t", obj], text=True)
        arrays = {}
        for line in table.splitlines():
            row = line.split()
            if len(row) >= 6 and row[2] == "O" and "PioneerQ38TwinQ8Payload" in row[-1]:
                match = re.search(r"(Section[01]_[0-9]|Weights[01])E$", row[-1])
                if match:
                    value = match.group(1)
                    assert value not in arrays and int(row[0], 16) == 0
                    out_data = out / (value + ".raw")
                    subprocess.run([tool+"objcopy", "-O", "binary", "-j", row[-3], obj, out_data], cwd=out, check=True)
                    data = out_data.read_bytes()
                    assert data and len(data) == int(row[-2], 16)
                    arrays[value] = hashlib.sha256(data).hexdigest()
        ordered = ("globalworker", "precise", "dense", "ffn", "service", "fetch", "affine", "text", "gdn", "boot")
        assert len(arrays) == 22
        for side in range(2):
            assert arrays[f"Weights{side}"] == manifest["weight_sha256"][side]
            for index, label in enumerate(ordered):
                assert arrays[f"Section{side}_{index}"] == manifest["section_sha256"][side][label]
        print("All 20 linked sections and two padded weight arrays match byte-for-byte")
    frames = obj.with_suffix(".su").read_text().splitlines()
    own = [line.split("	") for line in frames if "TwinQ8" in line or "q8_aperture_compile_only(" in line]
    actual = [int(row[1]) for row in own if len(row) == 3 and row[2] == "static"]
    assert own and len(actual) == len(own) and max(actual) <= 1024, own
    for extra in (["-UPIONEER_Q38_TWIN_Q8_NATIVE_APERTURE"],
                  ["-UPIONEER_Q38_TWIN_Q8_NATIVE_APERTURE", "-DPIONEER_Q38_TWIN_Q8_NATIVE_APERTURE=0"],
                  ["-DTT_PIONEER_Q38_WORK_MODE"],
                  ["-UPIONEER_DRIVER_BUILD_TAG", "-DPIONEER_DRIVER_BUILD_TAG=0x2026111700010217ULL"]):
        trial = subprocess.run([*flags, *extra, "-E", source], cwd=out, stdout=subprocess.DEVNULL,
                               stderr=subprocess.PIPE, text=True)
        assert trial.returncode and ("Twin Q8 aperture requires" in trial.stderr
                                     or "Twin Q8 L1 lease is private" in trial.stderr), extra
    print(hashlib.sha256(obj.read_bytes()).hexdigest(), obj)
    print(f"PASS genuine Haiku aperture types and static frame max {max(actual)} B; INERT object, no driver or device")


if __name__ == "__main__":
    main()
