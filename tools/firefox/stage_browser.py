#!/usr/bin/env python3
"""Archive a staged Firefox runtime and private libraries for native acceptance.

Run the Mozilla installer's stage-package target first. This creates a test
bundle, not a distro-approved package, and never connects to another machine.
"""
import argparse
import gzip
import hashlib
import json
from pathlib import Path
import shutil
import struct
import tarfile


def check_elf(path):
    with path.open("rb") as file:
        header = file.read(64)
    if (header[:6] != b"\x7fELF\x02\x01"
            or struct.unpack_from("<H", header, 18)[0] != 243
            or struct.unpack_from("<I", header, 48)[0] & 6 != 4):
        raise SystemExit(f"Expected Haiku RISC-V ELF64 with double-float ABI: {path}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--work", type=Path, required=True)
    args = parser.parse_args()
    work = args.work.resolve()
    here = Path(__file__).resolve().parent
    app = work / "obj/dist/firefox"
    prefix = work / "prefix"
    stage = work / "native-browser-stage"
    if stage.exists():
        parser.error(f"Refusing to overwrite the existing native stage: {stage}")
    for name in ("firefox", "libxul.so"):
        check_elf(app / name)
    for name in ("omni.ja", "browser/omni.ja", "application.ini"):
        if not (app / name).is_file():
            parser.error(f"Missing Mozilla runtime file: {app / name}")
    shutil.copytree(app, stage / "app")
    (stage / "lib").mkdir()
    for library in sorted((prefix / "lib").glob("*.so*")):
        if "testlib" in library.name:
            continue
        if not library.is_symlink():
            check_elf(library)
        shutil.copy2(library, stage / "lib" / library.name, follow_symlinks=False)
    shutil.copytree(prefix / "etc/fonts", stage / "etc/fonts", symlinks=True)
    shutil.copytree(prefix / "share/fontconfig", stage / "share/fontconfig", symlinks=True)
    config = stage / "etc/fonts/fonts.conf"
    config.write_text(config.read_text().replace(
        "<cachedir>/boot/home/develop/firefox-port-20261019/font-cache</cachedir>",
        '<cachedir prefix="xdg">fontconfig</cachedir>'))
    (stage / "tools").mkdir()
    for source, name in ((prefix / "bin/shlibsign", "shlibsign"),
                         (work / "probe-package/browser-probe-server", "browser-probe-server"),
                         (here / "probes/acceptance.html", "acceptance.html"),
                         (here / "probes/user.js", "user.js")):
        shutil.copy2(source, stage / "tools" / name)
    licenses = {
        "firefox-notice.txt": work / "source/firefox-158.0/LICENSE",
        "firefox-licenses.html": work / "obj/toolkit/content/license.html",
        "nspr.txt": work / "source/nspr-4.39/nspr/LICENSE",
        "nss.txt": work / "source/nss-3.130/nss/COPYING",
        "nss-zlib.txt": work / "source/nss-3.130/nss/lib/zlib/LICENSE",
        "libffi.txt": work / "source/libffi-3.4.8/LICENSE",
        "libevent.txt": work / "source/libevent-2.1.13-stable/LICENSE",
        "fontconfig.txt": work / "source/fontconfig-2.18.2/COPYING",
        "freetype.txt": work / "source/freetype-2.14.3/LICENSE.TXT",
        "freetype-FTL.txt": work / "source/freetype-2.14.3/docs/FTL.TXT",
        "expat.txt": work / "source/expat-2.7.3/COPYING",
    }
    (stage / "licenses").mkdir()
    for name, source in licenses.items():
        shutil.copy2(source, stage / "licenses" / name)
    shutil.copytree(here, stage / "source-material",
                    ignore=shutil.ignore_patterns("__pycache__", "*.pyc"))
    (stage / "NOTICE.txt").write_text(
        "Experimental unbranded Firefox 158.0 on Haiku/RISC-V.\n"
        "Native browser acceptance is pending; this is not a distro release.\n"
        "Copyright and license notices are preserved in licenses/.\n"
        "The FreeType Project provides font rendering; see licenses/freetype-FTL.txt.\n"
        "Pinned corresponding sources, local changes, and build instructions are\n"
        "recorded in source-material/sources.lock.json, patches/, and README.md.\n")
    launcher = stage / "run-firefox.sh"
    launcher.write_text('''#!/bin/sh
set -eu
test "$(uname -s)" = Haiku
test "$(uname -m)" = riscv64
cd "$(dirname "$0")"
bundle=$(pwd)
export LIBRARY_PATH="$bundle/app:$bundle/app/lib:$bundle/lib:/boot/system/lib"
export FONTCONFIG_FILE="$bundle/etc/fonts/fonts.conf"
export FONTCONFIG_PATH="$bundle/etc/fonts"
export XDG_CACHE_HOME="${XDG_CACHE_HOME:-/boot/home/config/cache}"
mkdir -p "$XDG_CACHE_HOME/fontconfig"
for module in libsoftokn3 libfreebl3 libnssdbm3; do
    if ! test -f "$bundle/lib/$module.chk"; then
        timeout 30 "$bundle/tools/shlibsign" -i "$bundle/lib/$module.so"
    fi
done
exec "$bundle/app/firefox" "$@"
''')
    launcher.chmod(0o755)
    hashes = {}
    for path in sorted(stage.rglob("*")):
        if path.is_file():
            name = path.relative_to(stage).as_posix()
            if "\n" in name or "\\" in name:
                raise SystemExit(f"Unsupported checksum filename: {name!r}")
            hashes[name] = hashlib.sha256(path.read_bytes()).hexdigest()
    (stage / "manifest.sha256").write_text("".join(
        f"{digest}  {name}\n" for name, digest in hashes.items()))
    (stage / "manifest.json").write_text(json.dumps({
        "status": "native_acceptance_pending", "files": hashes,
        "application_ini": (app / "application.ini").read_text(),
    }, indent=2) + "\n")

    def normalize(info):
        info.mtime = info.uid = info.gid = 0
        info.uname = info.gname = ""
        info.pax_headers = {}
        return info

    archive = work / "firefox-158.0-haiku-riscv64-native-test.tar.gz"
    with archive.open("wb") as raw:
        with gzip.GzipFile(filename="", mode="wb", fileobj=raw, mtime=0) as compressed:
            with tarfile.open(fileobj=compressed, mode="w|") as output:
                output.add(stage, arcname="firefox-native-test", filter=normalize)
    print(f"Native test archive: {archive}")
    print(f"SHA-256: {hashlib.sha256(archive.read_bytes()).hexdigest()}")
    print("Browser acceptance remains pending. No distro image was changed.")


if __name__ == "__main__":
    main()
