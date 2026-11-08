#!/usr/bin/env python3
"""Synthetic fixture creation only; no real model or accelerator access."""
import hashlib
import os
from pathlib import Path
import sys
import tempfile

import prepare_pioneer_q38_qgw8_cpu_head_fixture as fixture
from compare_pioneer_q38_qgw8_hidden import decode_card

with tempfile.TemporaryDirectory(prefix='qgw8-cpu-fixture-', dir='/private/tmp') as name:
    directory = Path(name)
    source, target = directory / 'cpu.bin', directory / 'cpu-fixture-not-device.bin'
    data = bytes(fixture.SNAPSHOT_BYTES)
    source.write_bytes(data)
    fixture.PIN = hashlib.sha256(data).hexdigest()
    sys.argv = ['fixture', str(source), str(target)]
    fixture.main()
    output = target.read_bytes()
    vector, generation = decode_card(output, hashlib.sha256(output).hexdigest())
    assert generation == 5 and len(vector) == fixture.WIDTH and not any(vector)
    try:
        fixture.main()
    except FileExistsError:
        pass
    else:
        raise AssertionError('existing fixture was overwritten')
    link = directory / 'link'
    link.symlink_to(target)
    sys.argv[-1] = str(link)
    try:
        fixture.main()
    except FileExistsError:
        pass
    else:
        raise AssertionError('symlink target was followed')
    assert target.read_bytes() == output
    os.unlink(link)
    fixture.PIN = '0' * 64
    sys.argv[-1] = str(link)
    try:
        fixture.main()
    except ValueError:
        pass
    else:
        raise AssertionError('bad source hash was accepted')
    assert not link.exists()
print('PASS CPU-only fixture pin and no-overwrite/symlink guards; synthetic only')
