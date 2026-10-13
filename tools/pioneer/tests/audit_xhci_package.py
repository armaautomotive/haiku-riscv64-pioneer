#!/usr/bin/env python3
"""Compare two package-extracted trees; allow only xHCI's payload to change.

Supply host `package list -a` outputs for archive timestamps and metadata:
extraction can change directory/symlink timestamps on the host.
Haiku's typed attributes are preserved as user.haiku.* extended attributes by
the Darwin package tool. Unrelated host attributes are not package content.
"""

import argparse
import ctypes
import hashlib
import os
import re
import stat
import sys
from pathlib import Path


def digest(path):
    result = hashlib.sha256()
    with path.open("rb") as stream:
        for data in iter(lambda: stream.read(1024 * 1024), b""):
            result.update(data)
    return result.hexdigest()


def attributes(path):
    if hasattr(os, "listxattr"):
        return {
            name: os.getxattr(path, name, follow_symlinks=False)
            for name in os.listxattr(path, follow_symlinks=False)
            if name.startswith("user.haiku.")
        }
    if sys.platform != "darwin":
        raise RuntimeError("No supported typed-attribute reader on this host")
    # Darwin's Python does not expose os.listxattr. Read the same no-follow
    # native API used by Haiku's fs_attr_bsdxattr.h compatibility layer.
    libc = ctypes.CDLL(None, use_errno=True)
    libc.listxattr.argtypes = [ctypes.c_char_p, ctypes.c_void_p,
                              ctypes.c_size_t, ctypes.c_int]
    libc.listxattr.restype = ctypes.c_ssize_t
    libc.getxattr.argtypes = [ctypes.c_char_p, ctypes.c_char_p, ctypes.c_void_p,
                             ctypes.c_size_t, ctypes.c_uint32, ctypes.c_int]
    libc.getxattr.restype = ctypes.c_ssize_t
    name = os.fsencode(path)
    size = libc.listxattr(name, None, 0, 1)  # XATTR_NOFOLLOW
    if size < 0:
        raise OSError(ctypes.get_errno(), "listxattr failed", str(path))
    buffer = ctypes.create_string_buffer(size)
    if libc.listxattr(name, buffer, size, 1) != size:
        raise RuntimeError("Attribute listing changed: " + str(path))
    result = {}
    for attribute in buffer.raw.split(b"\0"):
        if not attribute.startswith(b"user.haiku."):
            continue
        length = libc.getxattr(name, attribute, None, 0, 0, 1)
        if length < 0:
            raise OSError(ctypes.get_errno(), "getxattr failed", str(path))
        value = ctypes.create_string_buffer(length)
        if libc.getxattr(name, attribute, value, length, 0, 1) != length:
            raise RuntimeError("Attribute value changed: " + str(path))
        result[os.fsdecode(attribute)] = value.raw
    return result


def archive_listing(path):
    result = {}
    parents = []
    current = None
    package_attributes = []
    in_package_attributes = False
    pattern = re.compile(r"^( *)(.*?) {2,}(\d+ {2,}\d{4}-.*)$")
    for line in path.read_text().splitlines():
        if line == "package-attributes:":
            in_package_attributes = True
        if in_package_attributes and pattern.fullmatch(line) is not None:
            in_package_attributes = False
        if in_package_attributes:
            package_attributes.append(line)
            continue
        if not line.strip():
            continue
        if line.lstrip().startswith("<"):
            if current is None:
                raise RuntimeError("Attribute without an archive entry")
            result[current][1].append(line.strip())
            continue
        match = pattern.fullmatch(line)
        if match is None or len(match[1]) % 2:
            raise RuntimeError("Unrecognized archive listing line: " + line)
        depth = len(match[1]) // 2
        if depth > len(parents):
            raise RuntimeError("Missing archive parent: " + line)
        parents[depth:] = [match[2]]
        current = "/".join(parents)
        if current in result:
            raise RuntimeError("Duplicate archive entry: " + current)
        result[current] = (match[3], [])
    for metadata, values in result.values():
        values.sort()
    return result, package_attributes


def entries(root):
    result = {}
    for directory, names, files in os.walk(root, followlinks=False):
        for name in names + files:
            path = Path(directory) / name
            relative = path.relative_to(root).as_posix()
            info = path.lstat()
            record = {
                "mode": info.st_mode,
                "uid": info.st_uid,
                "gid": info.st_gid,
                "attributes": attributes(path),
            }
            if stat.S_ISREG(info.st_mode):
                record.update(size=info.st_size, sha256=digest(path),
                              mtime_ns=info.st_mtime_ns)
            elif stat.S_ISLNK(info.st_mode):
                record["target"] = os.readlink(path)
            elif not stat.S_ISDIR(info.st_mode):
                raise RuntimeError("Unsupported entry type: " + relative)
            result[relative] = record
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("original", type=Path)
    parser.add_argument("candidate", type=Path)
    parser.add_argument("--expected-sha256", required=True)
    parser.add_argument("--original-list", type=Path, required=True)
    parser.add_argument("--candidate-list", type=Path, required=True)
    arguments = parser.parse_args()
    if archive_listing(arguments.original_list) != archive_listing(
            arguments.candidate_list):
        raise RuntimeError("Archive metadata or package attributes changed")
    original = entries(arguments.original)
    candidate = entries(arguments.candidate)
    if original.keys() != candidate.keys():
        raise RuntimeError("Package entry paths changed")
    entry = "add-ons/kernel/busses/usb/xhci"
    if entry not in original:
        raise RuntimeError("xHCI entry is missing")
    if candidate[entry].get("sha256") != arguments.expected_sha256:
        raise RuntimeError("Candidate xHCI hash does not match the built driver")
    if original[entry]["sha256"] == candidate[entry]["sha256"]:
        raise RuntimeError("Candidate did not change xHCI")
    for path, before in original.items():
        after = candidate[path].copy()
        before = before.copy()
        if path == entry:
            before.pop("sha256")
            after.pop("sha256")
        if before != after:
            raise RuntimeError("Unexpected package change: " + path)
    files = sum(stat.S_ISREG(value["mode"]) for value in original.values())
    links = sum(stat.S_ISLNK(value["mode"]) for value in original.values())
    print(f"PASS: {len(original)} paths, {files} files, {links} symlinks; "
          "only xHCI payload changed; modes and Haiku attributes unchanged")


if __name__ == "__main__":
    main()
