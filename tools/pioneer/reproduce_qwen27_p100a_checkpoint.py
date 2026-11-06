#!/usr/bin/env python3
"""Reproduce the last completed BF16 trial's artifacts. BUILD ONLY; no SSH.

Exports a fixed git commit, never the current worktree. The local llama git
object database supplies frozen headers only; no dirty submodule files are
copied. All output, including the exported source, stays in a new directory.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys


SOURCE_COMMIT = "17d8162774564f8b57746880b72e273a6040f1b0"
LLAMA_COMMIT = "c060ca974c773c7c3d17fd1b66dc9d312bc292c0"
EXPORT_PATHS = (
    "headers", "build/config_headers", "src/add-ons/kernel",
    "src/apps/assistant", "tools/pioneer",
)
ARTIFACTS = {
    "native/tenstorrent_pci_probe-bf16": (
        2884184,
        "d8b1c561c9895841c6389576b53027d6e6998b3761dac12f47dbec961fb8bbb9",
    ),
    "client/HaikuAssistantQwen27Bf16FullOffload": (
        77584,
        "0190291350ca5eb56a235fcf6c2d6e3c000a7146f9f4b86fd7cd28a13051feec",
    ),
    "client/pioneer-q38-bf16-full-offload-smoke": (
        224952,
        "aa9aac510e66d7bfba98b61019181ddc6662639badbb7ebc4edcdb2f91d822ef",
    ),
}


def verify_artifact(path, expected):
    size, digest = expected
    if not path.is_file() or path.is_symlink():
        raise RuntimeError(f"missing or symlink artifact: {path}")
    actual = (path.stat().st_size, hashlib.sha256(path.read_bytes()).hexdigest())
    if actual != expected:
        raise RuntimeError(f"artifact mismatch: {path.name}: {actual}")
    return {"bytes": size, "sha256": digest}


def validate_paths(sdk, libraries, out):
    for label, path in (("SDK", sdk), ("libraries", libraries)):
        if not path.is_absolute() or not path.is_dir():
            raise RuntimeError(f"{label} must be an existing absolute directory")
    if not (sdk / "cross-tools-riscv64/bin/riscv64-unknown-haiku-g++").is_file():
        raise RuntimeError("missing RISC-V cross compiler")
    if not out.is_absolute() or out.exists() or out.is_symlink():
        raise RuntimeError("output must be a NEW absolute directory")
    if not out.parent.is_dir() or out.parent.resolve() != out.parent:
        raise RuntimeError("output parent must exist and have no symlink components")


def reproduce(sdk, libraries, out):
    if not __debug__ or os.environ.get("PYTHONOPTIMIZE"):
        raise RuntimeError("optimized Python is not supported by the frozen builders")
    if any(k.startswith(("PIONEER_", "TT_PIONEER_")) for k in os.environ):
        raise RuntimeError("unset injected Pioneer environment overrides")
    # Git selectors must not redirect the clean export or nested header archive.
    # Strip them for children; ordinary app variables such as GIT_PAGER are benign.
    environment = {k: v for k, v in os.environ.items() if not k.startswith("GIT_")}
    validate_paths(sdk, libraries, out)
    repo = Path(__file__).resolve().parents[2]
    out.mkdir(mode=0o700)
    source = out / "source"
    source.mkdir(mode=0o700)
    commands, exits = [], {}

    def run(name, command):
        command = list(map(str, command))
        commands.append({"name": name, "argv": command})
        (out / "commands.json").write_text(json.dumps(commands, indent=2) + "\n")
        with (out / (name + ".stdout")).open("xb") as stdout, \
                (out / (name + ".stderr")).open("xb") as stderr:
            result = subprocess.run(command, stdout=stdout, stderr=stderr, cwd=out,
                                    env=environment)
        exits[name] = result.returncode
        (out / "actual-exits.json").write_text(json.dumps(exits, indent=2) + "\n")
        if result.returncode or (out / (name + ".stderr")).stat().st_size:
            raise RuntimeError(f"{name} did not close cleanly; inspect {out}")
        print(f"CLOSED {name} exit=0 stderr=0", flush=True)

    archive = out / "checkpoint-source.tar"
    run("export", ["git", "-C", repo, "archive", "--format=tar",
                   "--output=" + str(archive), SOURCE_COMMIT, *EXPORT_PATHS])
    run("unpack", ["tar", "-xf", archive, "-C", source])
    (source / "src/libs").mkdir(parents=True)
    run("llama-objects", ["git", "clone", "--quiet", "--shared", "--no-checkout",
                          "--", repo / "src/libs/llama.cpp", source / "src/libs/llama.cpp"])
    run("llama-commit", ["git", "-C", source / "src/libs/llama.cpp",
                         "cat-file", "-t", LLAMA_COMMIT])
    if (out / "llama-commit.stdout").read_text() != "commit\n":
        raise RuntimeError("frozen llama header commit is missing")
    pioneer = source / "tools/pioneer"
    run("native-build", [sys.executable, pioneer / "build_q38_bf16_full_native.py",
                         sdk, out / "native"])
    run("client-build", [sys.executable, pioneer / "build_assistant_qwen27_bf16_full_offload.py",
                         sdk, libraries, out / "client"])
    artifacts = {name: verify_artifact(out / name, expected)
                 for name, expected in ARTIFACTS.items()}
    result = {
        "source_commit": SOURCE_COMMIT,
        "llama_headers_commit": LLAMA_COMMIT,
        "artifacts": artifacts,
        "actual_exits": exits,
        "scope": "BUILD_ONLY: byte-identical to completed 105-a trial; no hardware action",
        "not_proven": ["new execution", "broad model quality", "warm tokens/sec", "speedup"],
        "warning": "105-a is spent; do not execute its old smoke/parent on hardware",
    }
    (out / "checkpoint.json").write_text(json.dumps(result, indent=2) + "\n")
    print("PASS CLEAN_CHECKPOINT all3 artifacts match the completed trial; "
          "NO hardware action or speed claim", flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sdk", type=Path, required=True)
    parser.add_argument("--libraries", type=Path, required=True)
    parser.add_argument("--out", type=Path, required=True)
    args = parser.parse_args()
    try:
        reproduce(args.sdk, args.libraries, args.out)
    except (OSError, RuntimeError) as error:
        print(f"STOP: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
