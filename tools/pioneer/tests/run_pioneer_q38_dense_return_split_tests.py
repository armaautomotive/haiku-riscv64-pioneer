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
    for profile in (flags[:-1], [*flags, "--qgw8-coarse-profile"]):
        result = subprocess.run([sys.executable, str(builder), str(build),
                                 str(tmp / "refused"), *profile,
                                 "--dense-return-split"],
                                env=env, capture_output=True)
        assert result.returncode != 0 and not (tmp / "refused").exists()

    image = tmp / "image"
    run(sys.executable, builder, build, image, *flags,
        "--hybrid-matrix-attention", "--dense-return-split", quiet=True)
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
                    "-DPIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION=1",
                    "-DPIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT=1",
                    str(here / "pioneer_q38_hybrid_actual_root_test.cpp"),
                    "-o", str(test)], check=True, env=host_env)
    subprocess.run([str(test), str(image)], check=True, env=host_env)

    old = tmp / "old-qgw5"
    leaf = tmp / "bfp4-leaf"
    run(sys.executable, builder, build, old, *flags, "--dense-return-split",
        quiet=True)
    (old / "program.demangled").write_bytes(subprocess.check_output(
        [str(tool) + "nm", "-nC", str(old / "program.elf")], env=env))
    run(sys.executable, root / "build_bfp4_worker_leaf.py", build, leaf)
    for name, option in (("qgw7-control", None),
                         ("qgw7-three-product-pv", "--three-product-pv-inspection")):
        worker = tmp / name
        args = [sys.executable, root / "build_q38_hybrid_matrix_attention_worker.py",
                build, worker]
        if option:
            args.append(option)
        run(*args)
        handoff = tmp / (name + "-root-worker-handoff")
        compile_args = [os.environ.get("CXX", "c++"), "-std=c++17", "-O1", "-g",
                        "-Wall", "-Wextra", "-Werror", "-ffp-contract=off",
                        "-fsanitize=address,undefined",
                        "-DPIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION=1",
                        "-DPIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT=1"]
        if option:
            compile_args.append("-DPIONEER_Q38_TEST_THREE_PRODUCT_PV=1")
        compile_args += [str(here / "pioneer_q38_hybrid_matrix_root_worker_test.cpp"),
                         "-o", str(handoff)]
        subprocess.run(compile_args, check=True, env=host_env)
        subprocess.run([str(handoff), str(image), str(old), str(worker), str(leaf)],
                       check=True, env={**host_env,
                           "ASAN_OPTIONS": "quarantine_size_mb=32",
                           "UBSAN_OPTIONS": "halt_on_error=1"})

print("PASS QGC1 split selector isolation, no ROOT/STOP overlap, relocated live "
      "dense-return routines, linked QGW7 root-to-attention-worker handoff and "
      "next projection with both four- and three-product PV worker variants; "
      "QGW5-identity control root rebuilt with the same bounds "
      "fix; offline only, no full model/device/TPS qualification")
