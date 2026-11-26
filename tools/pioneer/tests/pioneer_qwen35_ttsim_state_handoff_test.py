#!/usr/bin/env python3
"""Validate the simulator handoff framing and position-only command patch."""
import hashlib
import struct

from pioneer_qwen35_ttsim_service_test import (audit_resident_candidate, resident_handoff_payload,
    resident_next_decode_command, resident_position_one_command, validate_resident_prompt_commands)


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


def prompt_command(trigger, epoch, first, position):
    words = list(struct.unpack('<32I', command(position)))
    words[2:5] = [trigger, epoch, first]
    checksum = 2166136261
    for word in words[:10]:
        checksum = ((checksum ^ word) * 16777619) & 0xffffffff
    words[10] = checksum
    marker = 0x40000000 | trigger
    words[16:] = [epoch, (~epoch) & 0xffffffff, marker, (~marker) & 0xffffffff] * 4
    return struct.pack('<32I', *words)


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
    first, second = prompt_command(1, 22, 1, 0), prompt_command(2, 23, 522, 1)
    validate_resident_prompt_commands(first, second)
    rejected(validate_resident_prompt_commands, first, prompt_command(2, 23, 521, 1))
    changed = bytearray(second); changed[-1] ^= 1
    rejected(validate_resident_prompt_commands, first, changed)
    payload = bytes(range(256)) * ((24576 * 4 + 524288 * 4) // 256)
    raw = b'Q35SIMW1' + hashlib.sha256(payload).digest() + payload
    history, state = resident_handoff_payload(raw)
    assert history == payload[:24576 * 4] and state == payload[24576 * 4:]
    rejected(resident_handoff_payload, raw[:-1])
    changed = bytearray(raw); changed[-1] ^= 1
    rejected(resident_handoff_payload, changed)
    rejected(resident_handoff_payload, b'BADMAGIC' + raw[8:])
    receipt = [0x31435235, 2, 1, 22, 1, 521]
    checksum = 2166136261
    for word in receipt:
        checksum = ((checksum ^ word) * 16777619) & 0xffffffff
    receipt += [checksum, 1]
    snapshots = []
    senders = []
    for core in range(120):
        mailbox = [0] * 16
        mailbox[2:8] = [1, 1, 1, 1, 0, 0]
        mailbox[15] = core
        snapshots.append(tuple(mailbox))
        checksum = 0x31535335 ^ 1 ^ (22 << 7) ^ ((core * 0x9e3779b9) & 0xffffffff)
        senders.append((1, 22, core, checksum))
    selected = [0]
    def select(core):
        selected[0] = core
    def read(address, size):
        if address == 0xab310 and size == 32:
            return struct.pack('<8I', *receipt)
        if address == 0xab000 and size == 64:
            return struct.pack('<16I', *snapshots[selected[0]])
        if address == 0xab550 and size == 16:
            return struct.pack('<4I', *senders[selected[0]])
        raise AssertionError('unexpected simulated read')
    audit_resident_candidate(tuple(receipt), first, snapshots, select, read, 0)
    senders[47] = (1, 22, 47, 0)
    try:
        audit_resident_candidate(tuple(receipt), first, snapshots, select, read, 0)
        raise AssertionError('bad sender receipt accepted')
    except RuntimeError:
        pass
    senders[47] = (1, 22, 47, 0x31535335 ^ 1 ^ (22 << 7) ^ ((47 * 0x9e3779b9) & 0xffffffff))
    decode_words = list(struct.unpack('<32I', first)); decode_words[5] = 524
    checksum = 2166136261
    for word in decode_words[:10]:
        checksum = ((checksum ^ word) * 16777619) & 0xffffffff
    decode_words[10] = checksum
    decode = struct.pack('<32I', *decode_words)
    next_decode = resident_next_decode_command(decode, 261)
    next_words = struct.unpack('<32I', next_decode)
    assert next_words[2:10] == (2, 23, 525, 524, 1000000, 0, 261, 1)
    hybrid_words = decode_words.copy();hybrid_words[5] = 532
    checksum = 2166136261
    for word in hybrid_words[:10]:
        checksum = ((checksum ^ word) * 16777619) & 0xffffffff
    hybrid_words[10] = checksum
    hybrid_next = resident_next_decode_command(struct.pack('<32I', *hybrid_words), 261)
    assert struct.unpack('<32I', hybrid_next)[2:10] == (2, 23, 533, 532, 1000000, 0, 261, 1)
    paired_words = decode_words.copy();paired_words[5] = 500
    checksum = 2166136261
    for word in paired_words[:10]:
        checksum = ((checksum ^ word) * 16777619) & 0xffffffff
    paired_words[10] = checksum
    paired_next = resident_next_decode_command(struct.pack('<32I', *paired_words), 261)
    assert struct.unpack('<32I', paired_next)[2:10] == (2, 23, 501, 500, 1000000, 0, 261, 1)
    rejected(resident_next_decode_command, decode, -1)
    rejected(resident_next_decode_command, decode, 248320)
    rejected(resident_next_decode_command, decode[:-1], 261)
    for offset in (0, 8, 20, 36, 40, 44, 64, 124):
        changed = bytearray(decode); changed[offset] ^= 1
        rejected(resident_next_decode_command, changed, 261)
    for index, value in ((2, 0x0fffffff), (3, 0xffffffff), (4, 0x0fffffff - 524)):
        exhausted = decode_words.copy(); exhausted[index] = value
        checksum = 2166136261
        for word in exhausted[:10]:
            checksum = ((checksum ^ word) * 16777619) & 0xffffffff
        exhausted[10] = checksum
        marker = 0x40000000 | exhausted[2]
        exhausted[16:] = [exhausted[3], (~exhausted[3]) & 0xffffffff,
                          marker, (~marker) & 0xffffffff] * 4
        rejected(resident_next_decode_command, struct.pack('<32I', *exhausted), 261)
    receipt[5] = 524
    checksum = 2166136261
    for word in receipt[:6]:
        checksum = ((checksum ^ word) * 16777619) & 0xffffffff
    receipt[6] = checksum
    token, bits, sequence = 123, 0x3f800000, 524
    checksum = (0x31524135 ^ token ^ ((bits << 7) & 0xffffffff)
                ^ (bits >> 25) ^ ((sequence * 0x9e3779b9) & 0xffffffff))
    root = list(snapshots[0]); root[9:15] = [token, bits, sequence, 0x31524135, (~token) & 0xffffffff, checksum]
    snapshots[0] = tuple(root)
    assert audit_resident_candidate(tuple(receipt), decode, snapshots, select, read, 0, decode=True) == token
    for index in range(9, 15):
        forged = list(root); forged[index] ^= 1; snapshots[0] = tuple(forged)
        try:
            audit_resident_candidate(tuple(receipt), decode, snapshots, select, read, 0, decode=True)
            raise AssertionError(f'bad argmax word {index} accepted')
        except RuntimeError:
            pass
    snapshots[0] = tuple(root)
    try:
        audit_resident_candidate(tuple(receipt), decode, snapshots, select, read, 0)
        raise AssertionError('decode receipt accepted as prompt')
    except RuntimeError:
        pass
    print('PASS simulator handoff, command freshness and 120-worker receipt audit; NO HARDWARE')


if __name__ == '__main__':
    main()
