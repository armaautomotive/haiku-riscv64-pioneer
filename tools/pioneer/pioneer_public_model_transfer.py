#!/usr/bin/env python3
"""One-file temporary LAN transfer for the PUBLIC pinned Qwen3.8 model only.

No directory listing,arbitrary path,uploads,model execution or GPU access.
Only Pioneer10.0.0.247 may request the hash-named endpoint. SHA256 is checked
before serving;the receiver MUST independently verify the published SHA256.
Stops after all declared ranges or a bounded deadline. HTTP is public data,not secrets.
"""
import argparse
import hashlib
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import ipaddress
import os
from pathlib import Path
import re
import stat
import threading

from tenstorrent_qwen38_plan import FULL_BYTES, SHA256

ENDPOINT = f"/qwen38-{SHA256[:16]}.gguf"


def transfer_ranges(start, parts, size):
    if not 0 <= start < size or not 1 <= parts <= 16 or size-start < parts:
        raise ValueError("invalid bounded transfer partition")
    chunk = (size-start+parts-1)//parts
    return [(first, min(first+chunk, size)-1) for first in range(start, size, chunk)]


def byte_range(value, size):
    if value is None:
        return 0, size - 1, False
    if len(value) > 64:
        raise ValueError("oversized Range")
    match = re.fullmatch(r"bytes=(\d+)-(\d*)", value)
    if not match:
        raise ValueError("one explicit byte range required")
    start, end = int(match[1]), int(match[2]) if match[2] else size - 1
    if not 0 <= start <= end < size:
        raise ValueError("Range outside pinned file")
    return start, end, True


def parent_range(first, last, declared):
    """Permit only a suffix of ONE declared range; never cross its end."""
    parent = next((span for span in declared
                   if span[0] <= first <= span[1] and last == span[1]), None)
    if parent is None:
        raise ValueError("GET must match one declared range")
    return parent


class Handler(BaseHTTPRequestHandler):
    def log_message(self, *_):
        pass

    def do_HEAD(self):
        self.transfer(False)

    def do_GET(self):
        self.transfer(True)

    def transfer(self, body):
        if self.client_address[0] != "10.0.0.247" or self.path != ENDPOINT:
            self.send_error(403)
            return
        info = os.fstat(self.server.model_fd)
        if (info.st_size, info.st_mtime_ns, info.st_ino) != self.server.identity:
            self.send_error(409)
            return
        try:
            first, last, partial = byte_range(self.headers.get("Range"), FULL_BYTES)
            parent = parent_range(first, last, self.server.expected_ranges) if body else None
        except ValueError:
            self.send_error(416)
            return
        self.send_response(206 if partial else 200)
        self.send_header("Content-Type", "application/octet-stream")
        self.send_header("Content-Length", str(last-first+1))
        self.send_header("Accept-Ranges", "bytes")
        self.send_header("ETag", f'"{SHA256}"')
        if partial:
            self.send_header("Content-Range", f"bytes {first}-{last}/{FULL_BYTES}")
        self.end_headers()
        if not body:
            return
        position = first
        try:
            while position <= last:
                data = os.pread(self.server.model_fd, min(1024**2, last-position+1), position)
                if not data:
                    raise OSError("unexpected pinned source truncation")
                self.wfile.write(data)
                position += len(data)
            self.wfile.flush()
        except OSError as error:
            print(f"TRANSFER_INTERRUPTED at byte{position}: {type(error).__name__};resume permitted", flush=True)
            return
        print(f"PUBLIC_MODEL_TRANSFER_SENT bytes{last-first+1} range{first}-{last};RECEIVER_SHA256_STILL_REQUIRED", flush=True)
        with self.server.completion_lock:
            self.server.completed_ranges.add(parent)
            complete = self.server.completed_ranges == self.server.expected_ranges
        if complete:
            self.server.shutdown()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("model", type=Path)
    parser.add_argument("--bind", required=True, type=ipaddress.IPv4Address)
    parser.add_argument("--start", type=int, default=0, help="already received pinned prefix bytes")
    parser.add_argument("--parts", type=int, default=1, help="bounded1..16 contiguous remaining ranges")
    parser.add_argument("--port", type=int, default=0, help="0 chooses a fresh ephemeral port")
    parser.add_argument("--deadline", type=int, default=1800, help="bounded60..7200 seconds")
    args = parser.parse_args()
    if not args.bind.is_private or args.model.name != "Qwen3.8-27B-Q4_K_M.gguf":
        parser.error("private LAN address and exact public model filename required")
    if not 0 <= args.port <= 65535 or not 60 <= args.deadline <= 7200:
        parser.error("invalid port or bounded transfer deadline")
    ranges = transfer_ranges(args.start, args.parts, FULL_BYTES)
    descriptor = os.open(args.model, os.O_RDONLY | os.O_NOFOLLOW)
    try:
        info = os.fstat(descriptor)
        if not stat.S_ISREG(info.st_mode) or info.st_size != FULL_BYTES:
            raise ValueError("not the complete pinned public model")
        sha, position = hashlib.sha256(), 0
        while position < FULL_BYTES:
            data = os.pread(descriptor, min(4*1024**2, FULL_BYTES-position), position)
            if not data:
                raise ValueError("truncated source")
            sha.update(data)
            position += len(data)
        if sha.hexdigest() != SHA256:
            raise ValueError("public model SHA256 mismatch")
        identity = (info.st_size, info.st_mtime_ns, info.st_ino)
        current = os.fstat(descriptor)
        if identity != (current.st_size, current.st_mtime_ns, current.st_ino):
            raise ValueError("source changed during verification")
        with ThreadingHTTPServer((str(args.bind), args.port), Handler) as server:
            server.model_fd, server.identity = descriptor, identity
            server.expected_ranges, server.completed_ranges = set(ranges), set()
            server.completion_lock = threading.Lock()
            timer = threading.Timer(args.deadline, server.shutdown)
            timer.daemon = True
            timer.start()
            print(f"VERIFIED_PUBLIC_MODEL_URL=http://{server.server_address[0]}:{server.server_address[1]}{ENDPOINT} allowed_peer=10.0.0.247 deadline={args.deadline}s", flush=True)
            print("DECLARED_RANGES="+",".join(f"{first}-{last}" for first,last in ranges), flush=True)
            try:
                server.serve_forever(poll_interval=0.2)
            finally:
                timer.cancel()
            print("PUBLIC_MODEL_LISTENER_CLOSED", flush=True)
    finally:
        os.close(descriptor)


if __name__ == "__main__":
    main()
