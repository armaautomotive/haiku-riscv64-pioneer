#!/usr/bin/env python3
"""Exercise the Assistant batch selector before cross-build dependencies."""
import os
import pathlib
import subprocess
import tempfile


ROOT = pathlib.Path(__file__).resolve().parents[3]
BUILDER = ROOT / "tools/pioneer/build_qwen38_inference.sh"


def invoke(root: pathlib.Path, overrides: dict[str, str]) -> subprocess.CompletedProcess[str]:
    build = root / "build"
    libraries = root / "libraries"
    build.mkdir(exist_ok=True)
    libraries.mkdir(exist_ok=True)
    env = {"PATH": os.environ.get("PATH", "/usr/bin:/bin"), **overrides}
    return subprocess.run(
        ["sh", str(BUILDER), str(build), str(libraries), str(root / "worker")],
        env=env,
        text=True,
        capture_output=True,
        check=False,
    )


with tempfile.TemporaryDirectory(prefix="pioneer-q38-batch-build-gate-") as temp:
    root = pathlib.Path(temp)
    valid_without_cache = invoke(
        root,
        {
            "PIONEER_Q38_DISPATCH_INFERENCE": "1",
            "PIONEER_Q38_VECTOR_INFERENCE": "1",
            "PIONEER_Q38_EXECUTION_MODE": "work",
            "PIONEER_Q38_SPAN_PRELOAD": "1",
            "PIONEER_Q38_SPAN_FAST": "1",
            "PIONEER_Q38_ASSISTANT_WORKER": "1",
            "PIONEER_Q38_WORK_BATCH_CLIENT": "1",
        },
    )
    # The gate should admit this role and continue to pinned-library checks;
    # status 1 is the expected missing-library refusal. Status 2 means a role
    # gate refused it before the real cross-build prerequisites.
    assert valid_without_cache.returncode == 1, (
        valid_without_cache.returncode,
        valid_without_cache.stdout,
        valid_without_cache.stderr,
    )

    invalid_validation = invoke(
        root,
        {
            "PIONEER_Q38_DISPATCH_INFERENCE": "1",
            "PIONEER_Q38_VECTOR_INFERENCE": "1",
            "PIONEER_Q38_EXECUTION_MODE": "cpu-validated",
            "PIONEER_Q38_SPAN_PRELOAD": "1",
            "PIONEER_Q38_SPAN_FAST": "1",
            "PIONEER_Q38_ASSISTANT_WORKER": "1",
            "PIONEER_Q38_WORK_BATCH_CLIENT": "1",
        },
    )
    assert invalid_validation.returncode == 2
    assert "libllama.so.0.2.0" not in invalid_validation.stderr

    invalid_non_assistant = invoke(
        root,
        {
            "PIONEER_Q38_DISPATCH_INFERENCE": "1",
            "PIONEER_Q38_VECTOR_INFERENCE": "1",
            "PIONEER_Q38_EXECUTION_MODE": "work",
            "PIONEER_Q38_SPAN_PRELOAD": "1",
            "PIONEER_Q38_SPAN_FAST": "1",
            "PIONEER_Q38_ASSISTANT_WORKER": "0",
            "PIONEER_Q38_WORK_BATCH_CLIENT": "1",
        },
    )
    assert invalid_non_assistant.returncode == 2
    assert "libllama.so.0.2.0" not in invalid_non_assistant.stderr

print("PASS Work batch build gate: ordinary authenticated model source admitted; validation/non-Assistant roles refused")
