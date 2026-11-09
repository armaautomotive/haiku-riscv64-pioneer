#!/usr/bin/env python3
"""Synthetic controls for CPU-only layer reference verification."""
import array
import hashlib
from unittest.mock import patch
import verify_pioneer_q38_qgw8_cpu_layer_reference as verifier

raw = array.array('f', [1.0]) * (64*verifier.WORDS)
layer = raw.tobytes()
snapshot = bytes(64) + bytes(verifier.WORDS*4) + layer[-verifier.WORDS*4:] + bytes(4)
pin = hashlib.sha256(snapshot).hexdigest()
with patch.object(verifier, 'CPU_SNAPSHOT_PIN', pin), patch.object(verifier, 'inspect') as inspect:
    assert verifier.verify(layer, b'original', snapshot) == hashlib.sha256(layer).hexdigest()
    inspect.assert_called_once()
    for bad_raw, bad_snapshot in ((layer[:-1], snapshot), (layer, snapshot[:-1]),
                                  (layer[:-1]+b'\0', snapshot),
                                  (b'\0\0\x80\x7f'+layer[4:], snapshot)):
        try:
            verifier.verify(bad_raw, b'original', bad_snapshot)
        except ValueError:
            pass
        else:
            raise AssertionError('invalid layer reference accepted')
print('PASS CPU reference last-layer/length/finite/prior-pin synthetic checks; NO DEVICE')
