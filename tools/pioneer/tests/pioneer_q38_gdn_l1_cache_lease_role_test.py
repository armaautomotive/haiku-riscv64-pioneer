#!/usr/bin/env python3
"""The L1 cache lease must remain offline metadata only."""
import os
import pathlib
import subprocess

assert not any(k.startswith(("PIONEER_", "TT_PIONEER_")) for k in os.environ)
header = pathlib.Path(__file__).resolve().parents[1] / "PioneerQ38GdnL1CacheLeaseInspection.h"
compiler = os.environ.get("CXX", "c++")

def check(defines, accepted):
    source = "".join(f"#define {name} {value}\n" for name, value in defines.items())
    source += f'#include "{header}"\nint main() {{ return 0; }}\n'
    result = subprocess.run([compiler, "-std=c++17", "-Wall", "-Wextra", "-Werror",
                             "-fsyntax-only", "-x", "c++", "-"],
                            input=source, text=True, capture_output=True)
    assert (result.returncode == 0) == accepted, (defines, result.stderr)

candidate = {"PIONEER_Q38_GDN_L1_CACHE_LEASE_INSPECTION": "1"}
check(candidate, True)
for role in ("PIONEER_Q38_WORK_INFERENCE", "PIONEER_Q38_CPU_VALIDATION",
             "PIONEER_Q38_FULL_MODEL_CPU_REFERENCE", "PIONEER_Q38_FULL_OFFLOAD_NATIVE",
             "PIONEER_Q38_HYBRID_FULL_NATIVE", "PIONEER_Q38_GDN_RECURRENT_NATIVE",
             "PIONEER_Q38_GDN_STATE_INIT_NATIVE", "PIONEER_Q38_GDN_INPUT_OWNER",
             "PIONEER_Q38_RESIDENT_GDN_OPERATOR_IMAGE", "_KERNEL_MODE"):
    check({**candidate, role: "1"}, False)
check({}, False)
print("PASS cache-lease role gate: inspection accepted; Work/reference/native/kernel roles refused")
