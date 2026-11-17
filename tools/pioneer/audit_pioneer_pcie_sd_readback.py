#!/usr/bin/env python3
"""Audit image-sized regular-file captures of the reviewed Pioneer probe SD."""

import argparse
import hashlib
import mmap
import os
from pathlib import Path
import re
import stat
import struct
import subprocess


IMAGE_SHA = "6cbcf4c526f8c5b5670d5737d26c118ca5ebdb63bf81d7ab9d466f8ae3a102de"
SECTOR = 512
IMAGE_SIZE = 268435456
BOOT_LBA = 2048
FAT_A, FAT_B, ROOT, DATA = 2056, 2312, 2568, 2600
SECTORS_PER_CLUSTER = 8
METADATA = {".Spotlight-V100", ".fseventsd", ".Trashes", ".DS_Store"}


def require(ok, message):
    if not ok:
        raise ValueError(message)


def read_image(path):
    require(stat.S_ISREG(path.lstat().st_mode), f"not a regular file: {path}")
    with os.fdopen(os.open(path, os.O_RDONLY | os.O_NOFOLLOW | os.O_NONBLOCK), "rb") as src:
        info = os.fstat(src.fileno())
        require(stat.S_ISREG(info.st_mode) and info.st_size == IMAGE_SIZE,
                f"wrong image type or size: {path}")
        return mmap.mmap(src.fileno(), 0, access=mmap.ACCESS_READ)


def mtools(image, command, *args):
    return subprocess.check_output([command, "-i", f"{image}@@{BOOT_LBA * SECTOR}",
                                    *args], timeout=30)


def entries(image):
    lines = mtools(image, "mdir", "-a", "-b", "-/", "::/").decode("ascii").splitlines()
    names = set(lines)
    require(len(names) == len(lines) and 0 < len(names) <= 1024,
            "unexpected directory listing")
    for name in names:
        require(re.fullmatch(r"::/[a-zA-Z0-9._/ -]+", name) is not None
                and all(part not in ("", ".", "..") for part in name[3:].rstrip("/").split("/")),
                "unexpected FAT path")
    return names


def metadata_clusters(image, added):
    clusters = set()
    max_cluster = 1 + (IMAGE_SIZE // SECTOR - DATA) // SECTORS_PER_CLUSTER
    for entry in sorted(added):
        name = entry.rstrip("/")
        line = mtools(image, "mshowfat", name).decode("ascii").strip()
        require(line.startswith(name + " "), f"unexpected cluster listing: {entry}")
        chain = line[len(name) + 1:]
        if chain == "Root directory or empty file":
            require(not entry.endswith("/") and mtools(image, "mcopy", name, "-") == b"",
                    f"unresolved directory or nonempty file: {entry}")
            continue
        require(re.fullmatch(r"(?:\s*<[0-9]+(?:-[0-9]+)?>)+", chain) is not None,
                f"malformed cluster chain: {entry}")
        for lo, hi in re.findall(r"<([0-9]+)(?:-([0-9]+))?>", chain):
            first, last = int(lo), int(hi or lo)
            require(2 <= first <= last <= max_cluster, "cluster outside FAT16 data area")
            span = set(range(first, last + 1))
            require(not clusters.intersection(span), "cross-linked metadata clusters")
            clusters.update(span)
    return clusters


def audit(expected_path, actual_path):
    with read_image(expected_path) as expected, read_image(actual_path) as actual:
        require(hashlib.sha256(expected).hexdigest() == IMAGE_SHA, "wrong reference image SHA-256")
        require(expected[:FAT_A * SECTOR] == actual[:FAT_A * SECTOR],
                "MBR, pre-partition space or FAT16 boot area changed")
        original, observed = entries(expected_path), entries(actual_path)
        require(original <= observed, "original directory entry missing")
        added = observed - original
        require(all(name[3:].split("/")[0] in METADATA for name in added),
                "unexpected new FAT path")
        for name in sorted(name for name in original if not name.endswith("/")):
            require(mtools(expected_path, "mcopy", name, "-") == mtools(actual_path, "mcopy", name, "-"),
                    f"boot file changed: {name}")

        old_fat, new_fat = (image[FAT_A * SECTOR:FAT_B * SECTOR] for image in (expected, actual))
        require(expected[FAT_B * SECTOR:ROOT * SECTOR] == old_fat
                and actual[FAT_B * SECTOR:ROOT * SECTOR] == new_fat, "FAT copies disagree")
        allocated = metadata_clusters(actual_path, added)
        fat_changes = set()
        for idx, ((old,), (new,)) in enumerate(zip(struct.iter_unpack("<H", old_fat),
                                                    struct.iter_unpack("<H", new_fat))):
            if old != new:
                require(old == 0 and new != 0, "existing FAT allocation changed")
                fat_changes.add(idx)
        require(fat_changes == allocated, "FAT changes not limited to newly allocated metadata")

        root_end = False
        new_entries = 0
        label_timestamp_changed = False
        for offset in range(ROOT * SECTOR, DATA * SECTOR, 32):
            before, after = expected[offset:offset + 32], actual[offset:offset + 32]
            if offset == ROOT * SECTOR and before[:12] == b"BOOTFIRM   \x08":
                # macOS updates only the root volume label's modification
                # date/time when adding its folders (not any boot entry).
                label_timestamp_changed = before[22:26] != after[22:26]
                before = before[:22] + after[22:26] + before[26:]
            root_end |= after[0] == 0
            if root_end or before[0] not in (0, 0xe5) or after[0] == 0xe5:
                require(before == after, "existing FAT16 root slot changed")
            elif before != after and after[11] != 0x0f:  # Count non-LFN entries.
                require(after[11] & 8 == 0, "new root volume label")
                new_entries += 1
        require(new_entries == sum("/" not in name[3:].rstrip("/") for name in added),
                "new root entries do not match the metadata listing")
        changed_data = 0
        for first in range(DATA * SECTOR, IMAGE_SIZE, 1 << 20):
            end = min(IMAGE_SIZE, first + (1 << 20))
            if expected[first:end] == actual[first:end]:
                continue
            for offset in range(first, end, SECTOR):
                if expected[offset:offset + SECTOR] != actual[offset:offset + SECTOR]:
                    cluster = (offset // SECTOR - DATA) // SECTORS_PER_CLUSTER + 2
                    require(cluster in allocated, f"unexpected changed data LBA {offset // SECTOR}")
                    changed_data += 1
        print(f"Capture SHA-256: {hashlib.sha256(actual).hexdigest()}")
        print(f"PASS: exact boot regions and files; {len(added)} metadata paths, "
              f"{len(allocated)} new clusters, {changed_data} changed data sectors, "
              f"volume-label timestamp changed={label_timestamp_changed}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("expected", type=Path)
    parser.add_argument("capture", type=Path)
    args = parser.parse_args()
    try:
        audit(args.expected.absolute(), args.capture.absolute())
    except (OSError, ValueError, subprocess.SubprocessError) as error:
        parser.exit(1, f"REFUSED: {error}\n")


if __name__ == "__main__":
    main()
