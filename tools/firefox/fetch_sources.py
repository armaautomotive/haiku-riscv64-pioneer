#!/usr/bin/env python3
"""Download and verify the pinned Firefox bring-up inputs; no builds or installs."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess


def sha256(path):
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(8 * 1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--work", type=Path, required=True)
    parser.add_argument("--offline", action="store_true",
                        help="Verify cached inputs without making network requests")
    args = parser.parse_args()
    lock = json.loads(Path(__file__).with_name("sources.lock.json").read_text())
    downloads = args.work.resolve() / "downloads"
    downloads.mkdir(parents=True, exist_ok=True)
    for asset in lock["assets"]:
        name = asset["name"]
        if Path(name).name != name or not asset["url"].startswith("https://"):
            raise SystemExit(f"Invalid source-lock entry: {name}")
        path = downloads / name
        if path.is_file() and sha256(path) == asset["sha256"]:
            print(f"Verified {name}", flush=True)
            continue
        if args.offline:
            raise SystemExit(f"Missing or mismatched cached input: {path}")
        temporary = path.with_name(name + ".part")
        subprocess.run(["curl", "--fail", "--location", "--proto", "=https",
                        "--retry", "2", "--connect-timeout", "20", "--max-time", "600",
                        "--output", str(temporary), asset["url"]], check=True)
        if sha256(temporary) != asset["sha256"]:
            raise SystemExit(f"Checksum mismatch: {temporary}; not used")
        temporary.replace(path)
        print(f"Downloaded and verified {name}", flush=True)


if __name__ == "__main__":
    main()
