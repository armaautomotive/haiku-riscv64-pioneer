#!/usr/bin/env python3
"""Prepare a native HPKG build from a browser that passed runtime acceptance.

The native package tool and NSS signer run later on Haiku. No image, installed
package, browser profile, or remote machine is changed by this host-side step.
"""
import argparse
import configparser
import gzip
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import tarfile


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--work", type=Path, required=True)
    parser.add_argument("--acceptance", type=Path, required=True)
    parser.add_argument("--native-capture", type=Path, required=True)
    args = parser.parse_args()
    work = args.work.resolve()
    here = Path(__file__).resolve().parent
    tested = work / "native-browser-stage"
    evidence = json.loads(args.acceptance.read_text())
    capabilities = evidence.get("capabilities", {})
    if (evidence.get("status") != "PASS" or capabilities.get("platformName") != "haiku"
            or capabilities.get("moz:headless") is not False
            or capabilities.get("acceptInsecureCerts") is not False
            or capabilities.get("browserVersion") != "158.0"
            or any(name not in evidence.get("checks", {}) for name in
                   ("page_fixture", "https_certificate_validation", "multiple_tabs"))):
        parser.error("A passing, non-headless Haiku browser acceptance report is required")
    application = configparser.ConfigParser()
    application.read(tested / "app/application.ini")
    if application.get("App", "BuildID") != capabilities.get("moz:buildID"):
        parser.error("The accepted build ID does not match the staged browser")
    if not args.native_capture.read_bytes().startswith(b"\x89PNG\r\n\x1a\n"):
        parser.error("A visually inspected native window PNG is required")
    manifest = json.loads((tested / "manifest.json").read_text())["files"]
    for name, expected in manifest.items():
        if name.startswith(("app/", "lib/", "etc/", "share/")):
            if digest(tested / name) != expected:
                parser.error(f"Accepted runtime has changed: {name}")
    package_work = work / "package-work"
    if package_work.exists():
        parser.error(f"Refusing to overwrite {package_work}")
    root = package_work / "package-root"
    bundle = root / "apps/FirefoxExperimental"
    bundle.mkdir(parents=True)
    for directory in ("app", "lib", "etc", "share", "licenses"):
        shutil.copytree(tested / directory, bundle / directory, symlinks=True)
    shutil.copytree(here, bundle / "source-material",
                    ignore=shutil.ignore_patterns("__pycache__", "*.pyc"))
    shutil.copy2(args.acceptance, bundle / "source-material/browser-acceptance.json")
    shutil.copy2(args.native_capture, bundle / "source-material/native-window.png")
    shutil.copy2(work / "source/firefox-158.0/tools/haiku-launcher/launcher.cpp",
                 package_work / "launcher.cpp")
    subprocess.run(["patch", "--batch", "--forward", "-p1", "-i",
                    here / "patches/native-launcher.patch"], cwd=package_work, check=True)
    subprocess.run([work / "wrappers/cxx", "-O2", package_work / "launcher.cpp", "-lbe",
                    "-o", bundle / "Firefox Experimental"], check=True)
    shutil.copy2(package_work / "launcher.cpp", bundle / "source-material/native-launcher.cpp")
    preferences = bundle / "app/defaults/pref/haiku-riscv64.js"
    preferences.parent.mkdir(parents=True, exist_ok=True)
    preferences.write_text(
        "// Defaults for the experimental Null-GL Haiku/RISC-V build.\n"
        'pref("gfx.webrender.software", true);\n'
        'pref("webgl.disabled", true);\n'
        'pref("browser.shell.checkDefaultBrowser", false);\n')
    (root / "bin").mkdir()
    (root / "bin/firefox-experimental").symlink_to(
        "../apps/FirefoxExperimental/Firefox Experimental")
    menu = root / "data/deskbar/menu/Applications"
    menu.mkdir(parents=True)
    (menu / "Firefox Experimental").symlink_to(
        "../../../../apps/FirefoxExperimental/Firefox Experimental")
    # Unique license filenames avoid masking another package's license text.
    license_directory = root / "data/licenses"
    license_directory.mkdir()
    texts = []
    for name in ("nspr.txt", "nss.txt", "nss-zlib.txt", "libffi.txt", "libevent.txt",
                 "fontconfig.txt", "freetype.txt", "freetype-FTL.txt", "expat.txt"):
        texts.append(f"\n===== {name} =====\n" + (bundle / "licenses" / name).read_text())
    (license_directory / "FirefoxExperimental").write_text("".join(texts))
    (root / ".PackageInfo").write_text('''name firefox_experimental
version 158.0-1
architecture riscv64
summary "Experimental native browser for Haiku RISC-V"
description "Unbranded Firefox with native Haiku windows and software rendering.
JIT, WebRTC, and WebGL are disabled in this initial port. Includes private
runtime libraries, license notices, pinned source inputs, and port patches."
packager "Haiku RISC-V port"
vendor "Haiku RISC-V port"
copyrights {
    "1995-2026 Mozilla Developers and Contributors"
    "Runtime library contributors; see bundled license notices"
}
licenses { "FirefoxExperimental" }
provides {
    firefox_experimental = 158.0
    app:FirefoxExperimental = 158.0
    cmd:firefox_experimental = 158.0
}
requires {
    haiku
    lib:libstdc++
    lib:libgcc_s
}
''')
    shutil.copy2(tested / "tools/shlibsign", package_work / "shlibsign")
    native = package_work / "build-package.sh"
    native.write_text('''#!/bin/sh
set -eu
test "$(uname -s)" = Haiku
test "$(uname -m)" = riscv64
cd "$(dirname "$0")"
package_work=$(pwd)
bundle="$package_work/package-root/apps/FirefoxExperimental"
export LIBRARY_PATH="$bundle/app:$bundle/app/lib:$bundle/lib:/boot/system/lib"
sha256sum -c input-manifest.sha256 > input-verification.log
for module in libsoftokn3 libfreebl3 libnssdbm3; do
    timeout 30 ./shlibsign -i "$bundle/lib/$module.so"
done
test ! -e firefox_experimental-158.0-1-riscv64.hpkg
package create -1 -C package-root firefox_experimental-158.0-1-riscv64.hpkg
package info firefox_experimental-158.0-1-riscv64.hpkg
sha256sum firefox_experimental-158.0-1-riscv64.hpkg
''')
    native.chmod(0o755)
    hashes = {}
    for path in sorted(package_work.rglob("*")):
        if path.is_file():
            name = path.relative_to(package_work).as_posix()
            if "\n" in name or "\\" in name:
                parser.error(f"Unsupported checksum filename: {name!r}")
            hashes[name] = digest(path)
    (package_work / "input-manifest.sha256").write_text("".join(
        f"{checksum}  {name}\n" for name, checksum in hashes.items()))

    def normalize(info):
        info.mtime = info.uid = info.gid = 0
        info.uname = info.gname = ""
        info.pax_headers = {}
        return info

    archive = work / "firefox-158.0-riscv64-package-input.tar.gz"
    with archive.open("wb") as raw:
        with gzip.GzipFile(filename="", mode="wb", fileobj=raw, mtime=0) as compressed:
            with tarfile.open(fileobj=compressed, mode="w|") as output:
                output.add(package_work, arcname="firefox-package-work", filter=normalize)
    print(f"Native package input: {archive}")
    print(f"SHA-256: {digest(archive)}")
    print("Copy to Haiku and run build-package.sh. Retest the extracted HPKG before distro inclusion.")


if __name__ == "__main__":
    main()
