#!/usr/bin/env python3
"""Exercise the bounded head NoC batch in a fake fabric; no device."""
import os
from pathlib import Path
import subprocess
from tempfile import TemporaryDirectory


here = Path(__file__).resolve().parent
compiler = os.environ.get("CXX", "c++")
header = here.parent / "PioneerQ38HeadCsrBatch.h"
command = [compiler, "-std=c++17", "-fsyntax-only", "-x", "c++", "-"]
roles = ("_KERNEL_MODE", "PIONEER_DRIVER_BUILD_TAG", "PIONEER_Q38_FULL_OFFLOAD_NATIVE",
         "PIONEER_Q38_WORK_INFERENCE", "TT_PIONEER_Q38_WORK_MODE", "PIONEER_Q38_CPU_VALIDATION",
         "PIONEER_Q38_FULL_MODEL_CPU_REFERENCE", "PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE")
include = f'#include "{header}"\n'
enabled = "#define PIONEER_Q38_HEAD_CSR_BATCH_INSPECTION 1\n"
for preamble in ("", "#define PIONEER_Q38_HEAD_CSR_BATCH_INSPECTION 0\n",
                 *(enabled + f"#define {role} 1\n" for role in roles)):
    result = subprocess.run(command, input=preamble + include, capture_output=True, text=True)
    assert result.returncode and "isolated inspection component" in result.stderr, result
late_native = enabled + include + "#define PIONEER_Q38_FULL_OFFLOAD_NATIVE 1\n" + include
result = subprocess.run(command, input=late_native, capture_output=True, text=True)
assert result.returncode and "isolated inspection component" in result.stderr, result

with TemporaryDirectory(prefix="pioneer-head-csr-", dir="/private/tmp") as temporary:
    binary = Path(temporary) / "test"
    subprocess.run([compiler, "-std=c++17", "-O1", "-g", "-Wall", "-Wextra", "-Werror",
                    "-fsanitize=address,undefined", str(here / "pioneer_q38_head_csr_batch_test.cpp"),
                    "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True, timeout=20, env=dict(os.environ, UBSAN_OPTIONS="halt_on_error=1"))
print("PASS head CSR isolation: 11 invalid, mixed or late build roles refused")
