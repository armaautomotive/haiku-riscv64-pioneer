#!/usr/bin/env python3
"""Build a new local image file; never open an SD device or flash."""

import argparse
import hashlib
import shutil
import stat
import struct
import subprocess
from pathlib import Path
from tempfile import TemporaryDirectory

STOCK_SHA = "20b0e2db84a1a52aa37ad3cc155ee55206384c2f81da1a1e0cd88d8c1389adaf"
BOOT_OFFSET = 1048576
INITRD = "::/riscv64/initrd.img"
UNTOUCHED = (
    "::/zsbl.bin",
    "::/fip.bin",
    "::/riscv64/riscv64_Image",
    "::/riscv64/fw_dynamic.bin",
    "::/riscv64/sg2042-milkv-pioneer.dtb",
    "::/riscv64/sg2042-evb-v1.dtb",
    "::/riscv64/sg2042-evb-v2.dtb",
)


def sha256(path):
    digest = hashlib.sha256()
    with path.open("rb") as src:
        for chunk in iter(lambda: src.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def archive(executable):
    """Linux newc: a console, a sysfs mount point, and a single executable."""
    items = [
        ("dev", stat.S_IFDIR | 0o755, b"", 0, 0),
        ("dev/console", stat.S_IFCHR | 0o600, b"", 5, 1),
        ("sys", stat.S_IFDIR | 0o755, b"", 0, 0),
        ("init", stat.S_IFREG | 0o755, executable, 0, 0),
        ("TRAILER!!!", 0, b"", 0, 0),
    ]
    out = bytearray()
    for ino, (name, mode, data, rmajor, rminor) in enumerate(items, 1):
        path = name.encode("ascii") + b"\0"
        fields = (ino, mode, 0, 0, 2 if stat.S_ISDIR(mode) else 1, 0,
                  len(data), 0, 0, rmajor, rminor, len(path), 0)
        out += b"070701" + "".join(f"{n:08x}" for n in fields).encode()
        out += path
        out += b"\0" * (-len(out) % 4)
        out += data
        out += b"\0" * (-len(out) % 4)
    return out


def read_regular(path):
    if path.is_symlink() or not path.is_file():
        raise ValueError(f"regular file required: {path}")
    return path


def executable_bytes(executable):
    read_regular(executable)
    blob = executable.read_bytes()
    if not 64 <= len(blob) <= 1 << 20 or blob[:8] != b"\x7fELF\x02\x01\x01\0":
        raise ValueError("expected small ELF64 little-endian System V executable")
    elf_type, machine = struct.unpack_from("<HH", blob, 16)
    entry, phoff = struct.unpack_from("<QQ", blob, 24)
    phentsize, phnum = struct.unpack_from("<HH", blob, 54)
    if ((elf_type, machine) != (2, 243) or phentsize != 56 or not phnum
            or phoff + phentsize * phnum > len(blob)):
        raise ValueError("expected executable for RV64 without a dynamic loader")
    entry_ok = False
    for i in range(phnum):
        kind, flags, offset, vaddr, _, filesize, memsize, _ = struct.unpack_from(
            "<IIQQQQQQ", blob, phoff + i * phentsize)
        if kind in (2, 3):  # PT_DYNAMIC, PT_INTERP
            raise ValueError("dynamic executable refused")
        if kind == 1:
            if filesize > memsize or offset + filesize > len(blob):
                raise ValueError("invalid load segment")
            entry_ok |= bool(flags & 1) and vaddr <= entry < vaddr + filesize
    if not entry_ok:
        raise ValueError("entry is not in a file-backed executable segment")
    return blob


def extract(image, name, path):
    subprocess.run(["mcopy", "-n", "-i", f"{image}@@{BOOT_OFFSET}", name, path],
                   check=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("stock", type=Path, help="exact RevyOS Pioneer 20251226 firmware SD image")
    parser.add_argument("probe", type=Path, help="freestanding Linux RV64 probe executable")
    parser.add_argument("output", type=Path, help="new local .img file")
    args = parser.parse_args()
    if args.output.suffix != ".img":
        parser.error("output must be a new .img file")
    if args.output.exists() or args.output.is_symlink():
        parser.error("output must not already exist")
    try:
        stock = read_regular(args.stock).absolute()
        probe = executable_bytes(args.probe)
    except ValueError as error:
        parser.error(str(error))
    if stock.stat().st_size != 268435456 or sha256(stock) != STOCK_SHA:
        parser.error("size or SHA-256 does not match the published stock SD image")
    with TemporaryDirectory(prefix="pioneer-pcie-sd-") as directory:
        tmp = Path(directory)
        work = tmp / "unverified.img"
        initrd = tmp / "initrd.img"
        initrd.write_bytes(archive(probe))
        # Only a generated regular temporary file is passed to mtools.
        shutil.copyfile(stock, work)
        if sha256(work) != STOCK_SHA:
            raise ValueError("stock image changed during copy")
        subprocess.run(["mcopy", "-o", "-i", f"{work}@@{BOOT_OFFSET}", initrd, INITRD],
                       check=True)
        extract(work, INITRD, tmp / "readback")
        if (tmp / "readback").read_bytes() != initrd.read_bytes():
            raise ValueError("initrd mismatch after write")
        before, after = tmp / "before", tmp / "after"
        before.mkdir()
        after.mkdir()
        for name in UNTOUCHED:
            extract(stock, name, before)
            extract(work, name, after)
            leaf = name.rsplit("/", 1)[-1]
            if sha256(before / leaf) != sha256(after / leaf):
                raise ValueError(f"unexpected change: {name}")
        with stock.open("rb") as old, work.open("rb") as new:
            if old.read(BOOT_OFFSET) != new.read(BOOT_OFFSET):
                raise ValueError("partition table or pre-partition bytes changed")
        expected = sha256(work)
        with args.output.open("xb") as dst, work.open("rb") as src:
            shutil.copyfileobj(src, dst)
        if sha256(args.output) != expected:
            raise ValueError("final file does not match the audited image")
        print(f"{expected}  {args.output}")
        print(f"{sha256(initrd)}  initrd.img (readback verified)")


if __name__ == "__main__":
    main()
