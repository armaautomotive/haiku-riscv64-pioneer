#!/usr/bin/env python3
"""Create a private synthetic card-format fixture from pinned CPU data only."""
import hashlib
import os
from pathlib import Path
import struct
import sys
from compare_pioneer_q38_observed_trace import require, stable_bytes
from verify_pioneer_q38_qgw8_cpu_hidden_snapshot import SNAPSHOT_BYTES, WIDTH

PIN = 'ce1ed4778227d6deb4bc0fd85440facf2a680c7d60b2e7d92c60e74abd73c98a'

def main():
    require(len(sys.argv) == 3, 'usage: script ABS_CPU_SNAPSHOT NEW_ABS_SYNTHETIC_FIXTURE')
    source, target = map(Path, sys.argv[1:])
    require(source.is_absolute() and target.is_absolute(), 'absolute paths required')
    original = stable_bytes(source, SNAPSHOT_BYTES)
    require(hashlib.sha256(original).hexdigest() == PIN, 'CPU snapshot hash')
    # Converted FFN final hidden comes from the pinned CPU snapshot; this is
    # deliberately NOT a card observation, despite using the same wire shape.
    hidden = original[(16 + WIDTH)*4:(16 + 2*WIDTH)*4]
    header = [0x31444851, 1, 0, 248319, 64, WIDTH, 1, 1, 0x00010129, 0x20261107, 5, 0, 2, 0, 0, 0]
    data = struct.pack('<16I', *header) + hidden + struct.pack('<I', 0x454e4448)
    fd = os.open(target, os.O_WRONLY | os.O_CREAT | os.O_EXCL | os.O_NOFOLLOW, 0o600)
    with os.fdopen(fd, 'wb') as out:
        out.write(data)
        out.flush()
        os.fsync(out.fileno())
    print('SYNTHETIC_CPU_FIXTURE_NOT_CARD sha256=' + hashlib.sha256(data).hexdigest())

if __name__ == '__main__':
    try:
        main()
    except (ValueError, OSError) as error:
        print('ERROR: ' + str(error), file=sys.stderr)
        sys.exit(1)
