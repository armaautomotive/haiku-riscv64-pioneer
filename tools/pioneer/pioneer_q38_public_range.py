#!/usr/bin/env python3
"""One public-model range to one Pioneer;no directory/file-listing service."""
import hashlib
import http.server
import os
import socket
import stat
import sys
import time
from pioneer_q38_range_hashes import TOTAL, PUBLISHED, RANGES


def identity(fd):
    s = os.fstat(fd)
    return (s.st_dev, s.st_ino, s.st_size, s.st_mtime_ns, s.st_ctime_ns)


def main():
    if len(sys.argv) != 3:
        raise SystemExit("Usage:pioneer_q38_public_range.py VERIFIED_MODEL RANGE_INDEX|--repair-tails")
    if sys.argv[2] == "--repair-tails":
        # Only the three64MiB leaves containing known interrupted-resume
        # boundaries;whole receiver SHA must still prove the entire model.
        specs = [(1, 1610612736), (4, 1342177280), (5, 1409286144)]
        requests = []
        for index, offset in specs:
            first = sum(n for _, n in RANGES[:index]) + offset
            requests.append((RANGES[index][0], first, first + 67108864 - 1, 67108864))
    elif sys.argv[2].isdigit() and int(sys.argv[2]) < len(RANGES):
        index = int(sys.argv[2])
        first = sum(n for _, n in RANGES[:index])
        name, length = RANGES[index]
        requests = [(name, first, first + length - 1, length)]
    else:
        raise SystemExit("STOP:fixed public range selector required")
    allowed = {f"bytes={first}-{last}": (name, first, last, length)
               for name, first, last, length in requests}
    fd = os.open(sys.argv[1], os.O_RDONLY | os.O_NOFOLLOW)
    try:
        before = identity(fd)
        if not stat.S_ISREG(os.fstat(fd).st_mode) or before[2] != TOTAL:
            raise SystemExit("STOP:exact regular public model required")
        whole = hashlib.sha256()
        while True:
            data = os.read(fd, 16 * 1024 * 1024)
            if not data:
                break
            whole.update(data)
        if whole.hexdigest() != PUBLISHED or identity(fd) != before:
            raise SystemExit("STOP:source SHA/identity failure")

        class Server(http.server.HTTPServer):
            success = False
            attempts = 0
            timeout = 1
            completed = set()

            def get_request(self):
                connection, peer = super().get_request()
                connection.settimeout(15)
                return connection, peer

        class Handler(http.server.BaseHTTPRequestHandler):
            protocol_version = "HTTP/1.0"

            def log_message(self, *args):
                pass  # never log arbitrary client input/headers

            def allowed(self):
                return (self.client_address[0] == "10.0.0.247"
                        and self.path == "/qwen38-c600de0300ae8a0e.gguf"
                        and identity(fd) == before)

            def do_HEAD(self):
                self.server.attempts += 1
                if not self.allowed():
                    self.send_error(404)
                    return
                self.send_response(200)
                self.send_header("Content-Length", str(TOTAL))
                self.end_headers()

            def do_GET(self):
                self.server.attempts += 1
                ranges = self.headers.get_all("Range", [])
                if not self.allowed() or len(ranges) != 1 or ranges[0] not in allowed:
                    self.send_error(416)
                    return
                name, first, last, length = allowed[ranges[0]]
                self.send_response(206)
                self.send_header("Content-Length", str(length))
                self.send_header("Content-Range", f"bytes {first}-{last}/{TOTAL}")
                self.send_header("Cache-Control", "no-store")
                self.end_headers()
                sent = 0
                digest = hashlib.sha256()
                try:
                    while sent < length:
                        if identity(fd) != before:
                            raise OSError("source changed")
                        chunk = os.pread(fd, min(1024 * 1024, length - sent), first + sent)
                        if not chunk:
                            raise OSError("short source")
                        self.wfile.write(chunk)
                        digest.update(chunk)
                        sent += len(chunk)
                    self.wfile.flush()
                    self.server.completed.add(ranges[0])
                    self.server.success = len(self.server.completed) == len(allowed)
                    print(f"PUBLIC_RANGE_SENT name={name} first={first} bytes={sent} sha256={digest.hexdigest()};RECEIVER_SHA_STILL_REQUIRED", flush=True)
                except (OSError, socket.timeout):
                    print(f"PUBLIC_RANGE_INTERRUPTED sent={sent};NO_SUCCESS_CLAIM", flush=True)

        with Server(("10.0.0.211", 0), Handler) as server:
            print(f"PUBLIC_RANGE_READY port={server.server_port} ranges={requests};deadline=900s;all_fixed_ranges_then_close", flush=True)
            deadline = time.monotonic() + 900
            while not server.success and server.attempts < 12 and time.monotonic() < deadline:
                server.handle_request()
            if not server.success:
                raise SystemExit("STOP:sender deadline/attempt bound;receiver remains unverified")
        print("PUBLIC_RANGE_LISTENER_CLOSED", flush=True)
    finally:
        os.close(fd)


if __name__ == "__main__":
    main()
