#!/usr/bin/env python3
"""Run the readback gate against the real saved images and disposable mutations."""

import shutil
import subprocess
import sys
from pathlib import Path
from tempfile import TemporaryDirectory


AUDITOR = Path(__file__).resolve().parents[1] / "audit_pioneer_pcie_sd_readback.py"
SECTOR = 512


def run(reference, capture, problem=None):
    result = subprocess.run([sys.executable, "-I", AUDITOR, reference, capture],
                            capture_output=True, text=True, timeout=60)
    if problem is None:
        assert result.returncode == 0 and "PASS: exact boot regions" in result.stdout, result
    else:
        assert result.returncode == 1 and problem in result.stderr, result


def main(reference, capture):
    run(reference, reference)
    run(reference, capture)
    with TemporaryDirectory(prefix="pioneer-sd-audit-test-") as directory:
        copy = Path(directory) / "mutated.img"
        shutil.copyfile(capture, copy)
        for offset, error in (
            (446, "MBR, pre-partition space or FAT16 boot area changed"),
            (2055 * SECTOR, "MBR, pre-partition space or FAT16 boot area changed"),
            (2312 * SECTOR, "FAT copies disagree"),
            (2568 * SECTOR + 14, "existing FAT16 root slot changed"),
            (2608 * SECTOR, "boot file changed: ::/fip.bin"),
            (268435456 - 512, "unexpected changed data LBA"),
        ):
            with copy.open("r+b") as image:
                image.seek(offset)
                original = image.read(1)
                image.seek(offset)
                image.write(bytes([original[0] ^ 1]))
            run(reference, copy, error)
            with copy.open("r+b") as image:
                image.seek(offset)
                image.write(original)
        alias = Path(directory) / "capture-symlink.img"
        alias.symlink_to(capture.resolve())
        run(reference, alias, "not a regular file")
        subprocess.run(["mmd", "-i", f"{copy}@@1048576", "::/unapproved"],
                       check=True, timeout=30)
        run(reference, copy, "unexpected new FAT path")
    print("PASS: reference and captured metadata accepted; eight readback refusals exercised")


if __name__ == "__main__":
    if len(sys.argv) != 3:
        sys.exit("usage: pioneer_pcie_sd_readback_test.py REVIEWED_IMAGE RAW_CAPTURE")
    main(*map(Path, sys.argv[1:]))
