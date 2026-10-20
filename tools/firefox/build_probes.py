#!/usr/bin/env python3
"""Build and archive native dependency probes; does not connect to Haiku."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import tarfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--work", type=Path, required=True)
    parser.add_argument("--rust", type=Path, required=True)
    args = parser.parse_args()
    work, rust = args.work.resolve(), args.rust.resolve()
    probes = Path(__file__).resolve().parent / "probes"
    stage = work / "probe-package"
    if stage.exists():
        parser.error(f"Refusing to overwrite an existing probe package: {stage}")
    stage.mkdir()
    prefix = work / "prefix"
    env = dict(os.environ, RUST_TARGET_PATH=str(work))
    # Compile directly so these probes always use the current sysroot, even
    # immediately after a bootstrap that invalidated Cargo's cached metadata.
    for name, flags in (("rust-smoke", []), ("rust-smoke-abort", ["-C", "panic=abort"])):
        subprocess.run([rust / "bin/rustc", "--edition=2021", "--target", "riscv64gc-unknown-haiku",
                        *flags, probes / "rust-smoke/src/main.rs", "-o", stage / name],
                       env=env, check=True)
    definitions = {
        "libffi": ([prefix / "include"], ["ffi"]),
        "libevent": ([prefix / "include"], ["event_pthreads", "event"]),
        "msgpack": ([work / "source/firefox-158.0/third_party/msgpack/include"], []),
        "nspr": ([prefix / "include/nspr"], ["nspr4"]),
        "nss": ([prefix / "include/nss", prefix / "include/nspr"], ["nss3", "ssl3", "nspr4"]),
        "fontconfig": ([prefix / "include"], ["fontconfig"]),
        "va-copy": ([], []),
    }
    for name, (includes, libraries) in definitions.items():
        subprocess.run([work / "wrappers/cc", "-Wall", "-Wextra", "-Werror",
                        *["-I" + str(path) for path in includes], probes / (name + "-smoke.c"),
                        "-L" + str(prefix / "lib"), "-Wl,-rpath-link," + str(prefix / "lib"),
                        *["-l" + library for library in libraries], "-o", stage / (name + "-smoke")], check=True)
    subprocess.run([work / "wrappers/cc", "-Wall", "-Wextra", "-Werror",
                    probes / "browser-probe-server.c", "-lnetwork", "-o",
                    stage / "browser-probe-server"], check=True)
    shutil.copy2(probes / "acceptance.html", stage / "acceptance.html")
    shutil.copy2(probes / "user.js", stage / "user.js")
    shutil.copytree(prefix / "lib", stage / "lib", symlinks=True,
                    ignore=shutil.ignore_patterns("*.a", "*.la", "pkgconfig"))
    shutil.copytree(prefix / "etc/fonts", stage / "etc/fonts", symlinks=True)
    shutil.copytree(prefix / "share/fontconfig", stage / "share/fontconfig", symlinks=True)
    shutil.copy2(prefix / "bin/shlibsign", stage / "shlibsign")
    script = stage / "run-probes.sh"
    script.write_text('''#!/bin/sh
set -eu
cd "$(dirname "$0")"
probe_directory=$(pwd)
export LIBRARY_PATH="$probe_directory/lib:/boot/system/lib"
export FONTCONFIG_FILE="$probe_directory/etc/fonts/fonts.conf"
export FONTCONFIG_PATH="$probe_directory/etc/fonts"
test "$(uname -m)" = riscv64
sha256sum manifest.json
for probe in rust-smoke rust-smoke-abort va-copy-smoke libffi-smoke libevent-smoke msgpack-smoke nspr-smoke; do
    timeout 30 "./$probe"
done
# NSS module integrity files must be produced on the target architecture.
for library in libsoftokn3.so libfreebl3.so libnssdbm3.so; do
    timeout 30 ./shlibsign -i "$probe_directory/lib/$library"
done
timeout 30 ./nss-smoke
timeout 60 ./fontconfig-smoke
printf '%s\n' 'FIREFOX_DEPENDENCY_PROBES_PASS (not a browser acceptance test)'
''')
    script.chmod(0o755)
    hashes = {}
    for path in stage.rglob("*"):
        if path.is_file():
            hashes[path.relative_to(stage).as_posix()] = hashlib.sha256(path.read_bytes()).hexdigest()
    (stage / "manifest.json").write_text(json.dumps(hashes, indent=2, sort_keys=True) + "\n")

    def normalize(info):
        info.mtime = 0
        info.pax_headers = {}
        return info

    archive = work / "dependency-probes.tar.gz"
    with tarfile.open(archive, "w:gz") as output:
        output.add(stage, arcname="firefox-dependency-probes", filter=normalize)
    print(f"Native probe archive: {archive}")
    print("Copy to Haiku, extract, and run sh firefox-dependency-probes/run-probes.sh.")


if __name__ == "__main__":
    main()
