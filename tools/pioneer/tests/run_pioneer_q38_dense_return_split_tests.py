#!/usr/bin/env python3
"""Build and simulate the split dense-return ROOT profile; never install it."""
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

assert __debug__ and len(sys.argv) == 2 and not os.environ.get("PYTHONOPTIMIZE")
assert not any(k.startswith(("PIONEER_", "TT_PIONEER_")) for k in os.environ)
build = Path(sys.argv[1]).resolve()
assert build.is_absolute() and build.is_dir()
here = Path(__file__).resolve().parent
root = here.parent
builder = root / "build_qwen38_token_root.py"
flags = ["--inspect-candidate", "--cancel-boundary", "--f32-hidden",
         "--unroll-dense-seals", "--boundary-seals", "--unroll-quiet",
         "--worker-local-source-guards", "--precise-activation",
         "--single-closed-audit", "--worker-sealed-admission", "--bf16-fpu",
         "--hybrid-bfp"]
env = os.environ.copy()
headers = build / "objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop/headers"
env["CPATH"] = os.pathsep.join((str(headers / "posix"), str(headers)))
tool = build / "cross-tools-riscv64/bin/riscv64-unknown-haiku-"

def run(*args, quiet=False):
    subprocess.run([str(x) for x in args], check=True, env=env,
                   stdout=subprocess.DEVNULL if quiet else None)

with tempfile.TemporaryDirectory(prefix="pioneer-qgc1-split-", dir="/private/tmp") as tmp:
    tmp = Path(tmp)
    # The selector must remain isolated to the exact unprofiled inspection role.
    for profile in (flags[:-1], [*flags, "--hybrid-matrix-attention"]):
        result = subprocess.run([sys.executable, str(builder), str(build),
                                 str(tmp / "refused"), *profile,
                                 "--dense-return-split"],
                                env=env, capture_output=True)
        assert result.returncode != 0 and not (tmp / "refused").exists()

    image = tmp / "image"
    run(sys.executable, builder, build, image, *flags,
        "--dense-return-split", quiet=True)
    symbols = subprocess.check_output([str(tool) + "nm", "-nC",
                                       str(image / "program.elf")],
                                      text=True, env=env)
    names = ("q38_joined_dense_frame", "LiveBlock::Compute(",
             "Joined::Read(", "Joined::BindProjection(", "Block::BindFfn(")
    for name in names:
        rows = [line for line in symbols.splitlines() if name in line]
        assert rows, ("missing relocated dense-return routine", name)
        for row in rows:
            address = int(row.split()[0], 16)
            assert 0x20000 <= address < 0x30000, (name, row)
    assert (image / "text.bin").stat().st_size <= 0x20000
    assert (image / "bf16.bin").stat().st_size <= 0x10000

    (image / "program.demangled").write_bytes(subprocess.check_output(
        [str(tool) + "nm", "-nC", str(image / "program.elf")], env=env))
    test = tmp / "actual-root"
    host_env = env.copy()
    host_env.pop("CPATH", None)
    subprocess.run([os.environ.get("CXX", "c++"), "-std=c++17", "-O1", "-g",
                    "-Wall", "-Wextra", "-Werror", "-ffp-contract=off",
                    "-fsanitize=address,undefined",
                    str(here / "pioneer_q38_hybrid_actual_root_test.cpp"),
                    "-o", str(test)], check=True, env=host_env)
    subprocess.run([str(test), str(image)], check=True, env=host_env)

print("PASS QGC1 split selector isolation, no ROOT/STOP overlap, relocated live "
      "dense-return routines, linked ROOT simulation; offline only, no full "
      "model/device/TPS qualification")
