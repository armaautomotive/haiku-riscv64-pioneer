#!/usr/bin/env python3
"""Extract verified sources into fresh directories and apply the port patches."""
import argparse
import json
from pathlib import Path
import subprocess
import sys
import tarfile
import tempfile

from fetch_sources import sha256


COMPONENTS = {
    "firefox": ("firefox-158.0.source.tar.xz", "firefox-158.0", "firefox-158.0", [
        "firefox-158.0.patchset", "firefox_launcher-158.0.patchset",
        "local:firefox-cross-riscv64.patch"]),
    "nspr": ("nspr-4.39.tar.gz", "nspr-4.39", "nspr-4.39/nspr", [
        "nspr-4.39.patchset", "local:nspr-riscv64.patch"]),
    "nss": ("nss-3.130.tar.gz", "nss-3.130", "nss-3.130/nss", ["nss-3.130.patchset"]),
    "libffi": ("libffi-3.4.8.tar.gz", "libffi-3.4.8", "libffi-3.4.8", [
        "local:libffi-haiku-closures.patch"]),
    "libevent": ("libevent-2.1.13-stable.tar.gz", "libevent-2.1.13-stable",
        "libevent-2.1.13-stable", ["libevent-2.1.13.patchset"]),
    "fontconfig": ("fontconfig-2.18.2.tar.xz", "fontconfig-2.18.2", "fontconfig-2.18.2", [
        "fontconfig-2.18.2.patchset"]),
    "freetype": ("freetype-2.14.3.tar.xz", "freetype-2.14.3", "freetype-2.14.3", []),
    "expat": ("expat-2.7.3.tar.xz", "expat-2.7.3", "expat-2.7.3", []),
    "pkgconf": ("pkgconf-2.3.0.tar.xz", "pkgconf-2.3.0", "pkgconf-2.3.0", []),
    "m4": ("m4-1.4.19.tar.xz", "m4-1.4.19", "m4-1.4.19", []),
    "autoconf": ("autoconf-2.72.tar.xz", "autoconf-2.72", "autoconf-2.72", []),
}


def refresh_vendor_checksums(source):
    """Record patched vendor files without disabling Cargo's file checks."""
    count = 0
    for manifest in (source / "third_party/rust").glob("*/.cargo-checksum.json"):
        checksum = json.loads(manifest.read_text())
        files = {}
        for path in manifest.parent.rglob("*"):
            if path.is_file() and path != manifest:
                files[path.relative_to(manifest.parent).as_posix()] = sha256(path)
        if files != checksum.get("files"):
            checksum["files"] = files
            manifest.write_text(json.dumps(checksum, sort_keys=True) + "\n")
            count += 1
    print(f"Refreshed file checksums for {count} patched vendor crates", flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--work", type=Path, required=True)
    parser.add_argument("--source-root", type=Path)
    parser.add_argument("components", nargs="+", choices=COMPONENTS)
    args = parser.parse_args()
    if sys.version_info < (3, 12):
        parser.error("Python 3.12+ is required for filtered archive extraction")
    here = Path(__file__).resolve().parent
    lock = json.loads((here / "sources.lock.json").read_text())
    locked = {entry["name"]: entry for entry in lock["assets"]}
    downloads = args.work.resolve() / "downloads"
    source_root = (args.source_root or args.work / "source").resolve()
    source_root.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix=".firefox-case-check-", dir=source_root) as temporary:
        directory = Path(temporary)
        (directory / "case-probe").touch()
        if (directory / "CASE-PROBE").exists():
            parser.error("Source extraction requires a case-sensitive filesystem")
    for component in args.components:
        archive, top, directory, patch_names = COMPONENTS[component]
        if (source_root / top).exists():
            raise SystemExit(f"Refusing to re-extract or re-patch existing directory: {source_root / top}")
        inputs = [archive] + [name for name in patch_names if not name.startswith("local:")]
        for name in inputs:
            path = downloads / name
            if not path.is_file() or sha256(path) != locked[name]["sha256"]:
                raise SystemExit(f"Missing or mismatched source input: {path}")
        def extraction_filter(member, destination):
            name = member.name
            while name.startswith("./"):
                name = name[2:]
            if name in ("", ".") and member.isdir():
                return None
            if not name.startswith(top + "/") and name.rstrip("/") != top:
                raise ValueError(f"Unexpected archive root: {member.name}")
            member.name = name
            return tarfile.data_filter(member, destination)

        with tarfile.open(downloads / archive, mode="r|*") as source:
            source.extractall(source_root, filter=extraction_filter)
        target = source_root / directory
        for name in patch_names:
            path = (here / "patches" / name.removeprefix("local:")) if name.startswith("local:") else downloads / name
            subprocess.run(["patch", "--batch", "--forward", "-p1", "-i", str(path)], cwd=target, check=True)
        if component == "firefox":
            (target / "mach").chmod(0o755)
            (target / "build/cargo-linker").chmod(0o755)
            refresh_vendor_checksums(target)
        (source_root / top / ".haiku-riscv-port.json").write_text(json.dumps({
            "haikuports_revision": lock["haikuports_revision"],
            "archive_sha256": locked[archive]["sha256"],
            "patches": {name: sha256((here / "patches" / name.removeprefix("local:"))
                if name.startswith("local:") else downloads / name) for name in patch_names},
        }, indent=2) + "\n")
        print(f"Prepared {component}: {target}", flush=True)


if __name__ == "__main__":
    main()
