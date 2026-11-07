#!/usr/bin/env python3
"""Capture the one fixed-token BFP8 48..58 CPU comparison to a private log."""
import os
from pathlib import Path
import subprocess
import sys

if len(sys.argv) != 4 or any(not Path(x).is_absolute() for x in sys.argv[1:]):
    raise SystemExit('usage: capture_pioneer_q38_mask11_compare.py ABS_BINARY ABS_MODEL NEW_ABS_LOG')
binary, model, log = sys.argv[1:]
flags = os.O_WRONLY | os.O_CREAT | os.O_EXCL | getattr(os, 'O_NOFOLLOW', 0) | getattr(os, 'O_CLOEXEC', 0)
fd = os.open(log, flags, 0o600)
with os.fdopen(fd, 'w', encoding='utf-8') as record:
    def publish(line):
        record.write(line)
        record.flush()
        sys.stdout.write(line)
        sys.stdout.flush()
    publish('CAPTURE_START fixed_mask11=07ff000000000000 token=248319 CPU_ONLY\n')
    try:
        with subprocess.Popen([binary, '--fixed-mask11', model, '07ff000000000000'],
                              stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                              text=True, encoding='utf-8', errors='replace', bufsize=1) as process:
            assert process.stdout is not None
            for line in process.stdout:
                publish(line)
            code = process.wait()
    except OSError as error:
        publish('CAPTURE_LAUNCH_ERROR ' + str(error) + '\n')
        os.fsync(record.fileno())
        raise SystemExit(1)
    publish(f'CAPTURE_EXIT {code}\n')
    os.fsync(record.fileno())
    if code:
        raise SystemExit(1)
