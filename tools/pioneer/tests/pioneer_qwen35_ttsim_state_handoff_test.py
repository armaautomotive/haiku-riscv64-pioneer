#!/usr/bin/env python3
"""Validate the simulator handoff framing and position-only command patch."""
import hashlib
import struct

from pioneer_qwen35_ttsim_service_test import resident_handoff_payload, resident_position_one_command


def command(position):
    words = [0] * 32
    words[:10] = [0x31444335, 1, 1, 22, 1, 521, 1000000, 0, 42, position]
    value = 2166136261
    for word in words[:10]:
        value = ((value ^ word) * 16777619) & 0xffffffff
    words[10] = value
    words[16:] = list(range(16))
    return struct.pack('<32I', *words)


def rejected(function, *args):
    try:
        function(*args)
    except ValueError:
        return
    raise AssertionError('invalid simulator handoff accepted')


def main():
    zero, one = command(0), command(1)
    assert resident_position_one_command(zero, one) == one
    rejected(resident_position_one_command, one, one)
    rejected(resident_position_one_command, zero, zero)
    rejected(resident_position_one_command, zero, one[:-1])
    changed = bytearray(one); changed[32] ^= 1
    rejected(resident_position_one_command, zero, changed)
    changed = bytearray(one); changed[40] ^= 1
    rejected(resident_position_one_command, zero, changed)
    payload = bytes(range(256)) * ((24576 * 4 + 524288 * 4) // 256)
    raw = b'Q35SIMW1' + hashlib.sha256(payload).digest() + payload
    history, state = resident_handoff_payload(raw)
    assert history == payload[:24576 * 4] and state == payload[24576 * 4:]
    rejected(resident_handoff_payload, raw[:-1])
    changed = bytearray(raw); changed[-1] ^= 1
    rejected(resident_handoff_payload, changed)
    rejected(resident_handoff_payload, b'BADMAGIC' + raw[8:])
    print('PASS simulator state framing, corruption rejection and position-only command; NO HARDWARE')


if __name__ == '__main__':
    main()
