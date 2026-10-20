#!/usr/bin/env python3
"""Clone a Haiku BFS image, include verified Firefox, and preserve its EFI disk prefix."""
import argparse
import json
from pathlib import Path
import shutil
import struct
import subprocess

from bundled_package import assemble, sha256


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--bfs-shell", type=Path, required=True,
                        help="Host bfs_shell with the repository's clonefs command")
    parser.add_argument("--base-bfs", type=Path, required=True)
    parser.add_argument("--base-sha256", required=True)
    parser.add_argument("--efi-image", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True, help="New output directory")
    parser.add_argument("--size-mib", type=int, default=768)
    args = parser.parse_args()
    for path in (args.base_bfs, args.efi_image, args.bfs_shell):
        if not path.is_file() or path.is_symlink():
            parser.error(f"Expected a regular input file: {path}")
    if args.size_mib < 600:
        parser.error("At least 600 MiB is required for the base and Firefox")
    if args.output.exists():
        parser.error("Refusing to replace an existing output directory")
    if sha256(args.base_bfs) != args.base_sha256:
        parser.error("Base BFS checksum mismatch")
    # Paths go through bfs_shell's own command parser; reject control/quote characters.
    for path in (args.base_bfs, args.output):
        if any(c in str(path.resolve()) for c in '\n\r"'):
            parser.error("Image paths cannot contain quotes or line breaks")
    package = assemble().resolve()
    if any(c in str(package) for c in '\n\r"'):
        parser.error("Package path cannot contain quotes or line breaks")
    efi_hash = sha256(args.efi_image)
    with args.efi_image.open("rb") as stream:
        mbr = bytearray(stream.read(512))
    if mbr[510:512] != b"\x55\xaa" or mbr[450] != 0xef or mbr[466] != 0xeb:
        parser.error("Expected EFI/BFS MBR layout in the EFI image")
    efi_start, efi_sectors = struct.unpack_from("<II", mbr, 454)
    bfs_start, bfs_sectors = struct.unpack_from("<II", mbr, 470)
    if not (0 < efi_start < efi_start + efi_sectors <= bfs_start
            and (bfs_start + bfs_sectors) * 512 <= args.efi_image.stat().st_size
            and mbr[482] == mbr[498] == 0):
        parser.error("Unexpected or overlapping input partitions")
    args.output.mkdir(parents=True)
    output = args.output.resolve()
    bfs = output / "haiku-pioneer-firefox.bfs"
    with bfs.open("xb") as stream:
        stream.truncate(args.size_mib * 1024 * 1024)
    tool = str(args.bfs_shell.resolve())
    with (output / "initialize.log").open("w") as log:
        subprocess.run([tool, "--initialize", str(bfs), "Haiku", "block_size 2048"],
                       stdout=log, stderr=subprocess.STDOUT, check=True)
    commands = [
        f'clonefs "{args.base_bfs.resolve()}"',
        # A redistributable image must generate fresh SSH identity/access settings.
        "rm -rf /myfs/home/config/settings/ssh",
        "rm -f /myfs/system/settings/launch/pioneer-ssh",
        f'cp :"{package}" /myfs/system/packages/{package.name}',
        "sync", "checkfs -c",
        f'cp /myfs/system/packages/{package.name} :"{output / "firefox-readback.hpkg"}"',
        f'cp /myfs/system/packages/haiku-r1~beta6_hrev99999-1-riscv64.hpkg :"{output / "haiku-readback.hpkg"}"',
        "info", "sync",
    ]
    command_file = output / "commands"
    command_file.write_text("\n".join(commands) + "\n")
    replies = output / "replies"
    with (output / "populate.log").open("w") as log:
        subprocess.run(["/bin/sh", "-c", 'exec "$1" -n "$2" 3<"$3" 4>"$4"',
                        "bfs", tool, str(bfs), str(command_file), str(replies)],
                       stdout=log, stderr=subprocess.STDOUT, check=True)
    if replies.read_text().splitlines() != ["0"] * len(commands):
        raise RuntimeError(f"Image population failed; discard output and inspect {output / 'populate.log'}")
    if sha256(output / "firefox-readback.hpkg") != sha256(package):
        raise RuntimeError("Image Firefox readback mismatch")
    if sha256(args.base_bfs) != args.base_sha256 or sha256(args.efi_image) != efi_hash:
        raise RuntimeError("An input image changed during construction")
    image = output / "haiku-pioneer-firefox.image"
    prefix_bytes = bfs_start * 512
    with args.efi_image.open("rb") as source, image.open("xb") as target:
        remaining = prefix_bytes
        while remaining:
            block = source.read(min(remaining, 1024 * 1024))
            if not block:
                raise RuntimeError("Truncated EFI prefix")
            target.write(block)
            remaining -= len(block)
        with bfs.open("rb") as content:
            shutil.copyfileobj(content, target, length=1024 * 1024)
        struct.pack_into("<I", mbr, 474, bfs.stat().st_size // 512)
        target.seek(0)
        target.write(mbr)
    receipt = {
        "status": "populated_and_readback_verified; boot test pending",
        "base_bfs": str(args.base_bfs.resolve()), "base_sha256": args.base_sha256,
        "efi_source": str(args.efi_image.resolve()), "efi_source_sha256": efi_hash,
        "image": str(image), "image_sha256": sha256(image), "size_bytes": image.stat().st_size,
        "bfs_image": str(bfs), "bfs_sha256": sha256(bfs),
        "bfs_partition_offset": prefix_bytes, "bfs_partition_size": bfs.stat().st_size,
        "firefox_sha256": sha256(package),
        "haiku_package_sha256": sha256(output / "haiku-readback.hpkg"),
        "clone_attributes_metadata_indices": "verified by clonefs",
        "filesystem_check": "pass", "firefox_package_readback": "pass",
        "embedded_ssh_identity_and_access_settings": "removed from output",
    }
    (output / "image-receipt.json").write_text(json.dumps(receipt, indent=2) + "\n")
    print(json.dumps(receipt, indent=2))


if __name__ == "__main__":
    main()
