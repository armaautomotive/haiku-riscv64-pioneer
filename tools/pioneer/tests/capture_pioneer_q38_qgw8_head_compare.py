#!/usr/bin/env python3
"""Capture one full-vocabulary QGW8 CPU observation to a new private log."""
import os
from pathlib import Path
import subprocess
import sys

if len(sys.argv) not in (4, 5, 7) or any(not Path(x).is_absolute() for x in sys.argv[1:4]):
    raise SystemExit('usage: capture_pioneer_q38_qgw8_head_compare.py ABS_BINARY ABS_MODEL NEW_ABS_LOG [OBSERVED_TOKEN [--save-hidden NEW_ABS_PATH]]')
binary, model, log = sys.argv[1:4]
observed = sys.argv[4:5]
if observed and (not observed[0].isascii() or not observed[0].isdigit()
                 or len(observed[0]) > 6 or int(observed[0]) >= 248320):
    raise SystemExit('observed token must be a decimal vocabulary ID')
hidden = sys.argv[5:]
if hidden and (hidden[0] != '--save-hidden' or not Path(hidden[1]).is_absolute()
               or os.path.abspath(hidden[1]) == os.path.abspath(log) or os.path.lexists(hidden[1])):
    raise SystemExit('hidden snapshot requires a distinct new absolute path')
flags = os.O_WRONLY | os.O_CREAT | os.O_EXCL | getattr(os, 'O_NOFOLLOW', 0) | getattr(os, 'O_CLOEXEC', 0)
fd = os.open(log, flags, 0o600)
with os.fdopen(fd, 'w', encoding='utf-8') as record:
    def publish(line):
        record.write(line)
        record.flush()
        sys.stdout.write(line)
        sys.stdout.flush()
    publish('CAPTURE_START mode=fixed-one-qgw8-head token=248319'
            + (' observed=' + observed[0] if observed else '')
            + (' hidden_snapshot=requested' if hidden else '') + ' CPU_ONLY\n')
    try:
        with subprocess.Popen([binary, '--fixed-one-qgw8-head', model, *observed, *hidden], stdout=subprocess.PIPE,
                              stderr=subprocess.STDOUT, text=True, encoding='utf-8', errors='replace', bufsize=1) as process:
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
