#!/usr/bin/env python3
"""Reassemble the repository's verified Firefox HPKG without external downloads."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import tempfile


def sha256(path):
    result = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            result.update(block)
    return result.hexdigest()


def assemble(output=None):
    here = Path(__file__).resolve().parent
    receipt = json.loads((here / "verified-build.json").read_text())
    bundle = json.loads((here / "packages/bundle.json").read_text())
    package = receipt["package"]
    for field in ("filename", "size_bytes", "sha256"):
        if bundle[field] != package[field]:
            raise ValueError(f"Bundle and verified build disagree: {field}")
    if Path(package["filename"]).name != package["filename"]:
        raise ValueError("Package filename must be a basename")
    target = output or here.parents[1] / "generated.firefox/packages" / package["filename"]
    if target.exists():
        if target.stat().st_size == package["size_bytes"] and sha256(target) == package["sha256"]:
            return target
        raise ValueError(f"Refusing to replace a different package: {target}")
    target.parent.mkdir(parents=True, exist_ok=True)
    temporary = None
    try:
        with tempfile.NamedTemporaryFile(dir=target.parent, prefix=".firefox-", delete=False) as stream:
            temporary = Path(stream.name)
            for part in bundle["parts"]:
                name = part["filename"]
                if Path(name).name != name:
                    raise ValueError("Part filename must be a basename")
                source = here / "packages" / name
                if source.stat().st_size != part["size_bytes"] or sha256(source) != part["sha256"]:
                    raise ValueError(f"Corrupt or incomplete package part: {source}")
                with source.open("rb") as content:
                    for block in iter(lambda: content.read(1024 * 1024), b""):
                        stream.write(block)
            stream.flush()
            os.fsync(stream.fileno())
        if temporary.stat().st_size != package["size_bytes"] or sha256(temporary) != package["sha256"]:
            raise ValueError("Reassembled HPKG failed verification")
        temporary.chmod(0o644)
        # Publish without replacing a file created by another build process.
        try:
            os.link(temporary, target)
        except FileExistsError:
            if sha256(target) != package["sha256"]:
                raise ValueError(f"Another process created a different package: {target}")
        return target
    finally:
        if temporary is not None:
            temporary.unlink(missing_ok=True)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    print(f"Verified bundled Firefox package: {assemble(args.output)}")
