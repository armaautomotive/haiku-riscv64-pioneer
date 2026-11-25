#!/usr/bin/env python3
"""Run the compiled Q35 service and roles on 120 simulated Blackhole tiles.

Pass a locally built Tenstorrent libttsim and the generated standalone
service and shared-role image directories. This uses the simulator's PCI API;
it neither accesses hardware nor measures inference or P100A performance.
"""

import argparse
import ctypes
import math
from pathlib import Path
import struct


def check_fixed_projection_reference(fixture, expected):
    """Independently decode the synthetic byte-major Q4_K blocks on the host.

    This is an offline reference for the simulator fixture, not an inference
    path. The fixture is constructed so every resulting integer is exactly
    representable in binary32.
    """
    inputs = struct.unpack_from('<2560f', fixture, 2764800)
    for row in range(1920):
        total = 0.0
        for block in range(10):
            base = (row // 16 * 10 + block) * 2304 + row % 16
            data = fixture[base:base + 2304:16]
            d, dmin = struct.unpack_from('<ee', data)
            scales = data[4:16]
            for group in range(8):
                if group < 4:
                    scale = scales[group] & 63
                    minimum = scales[group + 4] & 63
                else:
                    scale = (scales[group + 4] & 15) | ((scales[group - 4] >> 6) << 4)
                    minimum = (scales[group + 4] >> 4) | ((scales[group] >> 6) << 4)
                packed = 16 + (group // 2) * 32
                activation = block * 256 + group * 32
                for j in range(32):
                    q = data[packed + j]
                    q = (q & 15) if group % 2 == 0 else (q >> 4)
                    total += inputs[activation + j] * (d * scale * q - dmin * minimum)
        if not math.isfinite(total) or struct.pack('<f', total) != expected[row * 4:row * 4 + 4]:
            raise ValueError(f'Q4_K fixture reference mismatch at row {row}')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('library', type=Path)
    parser.add_argument('image', type=Path)
    parser.add_argument('roles', type=Path)
    group = parser.add_mutually_exclusive_group()
    group.add_argument('--projection-fixture', type=Path,
                       help='also execute the separately generated fixed projection')
    group.add_argument('--silu-fixture', type=Path,
                       help='also execute the separately generated 120-core SiLU operation')
    group.add_argument('--rms-fixture', type=Path,
                       help='also execute the separately generated 120-core RMS normalization')
    args = parser.parse_args()
    image = args.image
    if not (image / 'activation-sharing.marker').is_file():
        raise ValueError('expected sharing-enabled standalone service image')
    boot = (image / 'boot.bin').read_bytes()
    text = (image / 'text.bin').read_bytes()
    dispatch = (image / 'dispatch.bin').read_bytes()
    if boot != bytes.fromhex('6fc00600') or not 0 < len(text) <= 0xe000 or not 0 < len(dispatch) <= 0x5c00:
        raise ValueError('invalid service image layout')
    if len(text) % 4 or len(dispatch) % 4:
        raise ValueError('unaligned service image')
    role_images = []
    for name, address, limit in (('reader-shim.bin', 0x70000, 0x70100),
                                 ('writer-shim.bin', 0x70100, 0x70200),
                                 ('reader-code.bin', 0xb0000, 0xb5e00),
                                 ('writer-code.bin', 0xb6000, 0xb8e00)):
        data = (args.roles / name).read_bytes()
        if not data or len(data) % 4 or len(data) > limit - address:
            raise ValueError('invalid role image: ' + name)
        role_images.append((address, data))

    lib = ctypes.CDLL(str(args.library.resolve()))
    lib.libttsim_pci_config_rd32.argtypes = [ctypes.c_uint32, ctypes.c_uint32]
    lib.libttsim_pci_config_rd32.restype = ctypes.c_uint32
    lib.libttsim_pci_mem_wr_bytes.argtypes = [ctypes.c_uint64, ctypes.c_void_p, ctypes.c_uint32]
    lib.libttsim_pci_mem_rd_bytes.argtypes = [ctypes.c_uint64, ctypes.c_void_p, ctypes.c_uint32]
    lib.libttsim_clock.argtypes = [ctypes.c_uint32]
    lib.libttsim_init()
    try:
        if lib.libttsim_pci_config_rd32(0, 0) != 0xb1401e52:
            raise RuntimeError('simulator is not Blackhole')
        bar = ((lib.libttsim_pci_config_rd32(0, 0x14) << 32)
               | (lib.libttsim_pci_config_rd32(0, 0x10) & ~15))

        def write(address, data):
            buf = ctypes.create_string_buffer(data, len(data))
            lib.libttsim_pci_mem_wr_bytes(address, buf, len(data))

        def read(address, size):
            buf = ctypes.create_string_buffer(size)
            lib.libttsim_pci_mem_rd_bytes(address, buf, size)
            return buf.raw

        def write32(address, value):
            write(address, struct.pack('<I', value))

        def select(index, address, x, y):
            page = address >> 21
            cfg = (page & 0xffffffff, (page >> 32) | ((x | (y << 6)) << 11), 0)
            for word, value in enumerate(cfg):
                write32(bar + 0x1fc00000 + index * 12 + word * 4, value)

        def select_worker(core):
            column = core // 10
            x = column + (1 if column < 7 else 3)
            y = 2 + core % 10
            select(0, 0, x, y)
            select(1, 0xffb121b0, x, y)

        def jump(source, target):
            delta = target - source
            if delta & 1 or not -(1 << 20) <= delta < (1 << 20):
                raise ValueError('jump outside JAL range')
            return (((delta >> 20) & 1) << 31 | ((delta >> 1) & 0x3ff) << 21
                    | ((delta >> 11) & 1) << 20 | ((delta >> 12) & 0xff) << 12 | 0x6f)

        mailbox_address = bar + 0xab000
        role_address = bar + 0xab500
        handoff_address = bar + 0xab540
        reset_address = bar + 0x200000 + (0xffb121b0 & 0x1fffff)
        invocations = None
        invocation_size = 0
        if args.projection_fixture:
            fixture = (args.projection_fixture / 'fixture.bin').read_bytes()
            invocations = (args.projection_fixture / 'invocations.bin').read_bytes()
            expected = (args.projection_fixture / 'expected.bin').read_bytes()
            if len(fixture) != 2785280 or len(invocations) != 120 * 1128 or len(expected) != 1920 * 4:
                raise ValueError('unexpected fixed-projection fixture size')
            check_fixed_projection_reference(fixture, expected)
            invocation_size = 1128
            offset = 0
            while offset < len(fixture):
                address = 0x4001000 + offset
                select(2, address, 18, 12)
                count = min(0x10000, len(fixture) - offset, 0x200000 - (address & 0x1fffff))
                write(bar + 0x400000 + (address & 0x1fffff), fixture[offset:offset + count])
                offset += count
        elif args.silu_fixture:
            invocations = (args.silu_fixture / 'silu-invocations.bin').read_bytes()
            gate = (args.silu_fixture / 'silu-gate.bin').read_bytes()
            up = (args.silu_fixture / 'silu-up.bin').read_bytes()
            if len(invocations) != 120 * 432 or len(gate) != 9216 * 4 or len(up) != len(gate):
                raise ValueError('unexpected SiLU fixture size')
            invocation_size = 432
            for address, data in ((0x4001000, gate), (0x4011000, up)):
                select(2, address, 18, 12)
                write(bar + 0x400000 + (address & 0x1fffff), data)
        elif args.rms_fixture:
            invocations = (args.rms_fixture / 'rms-invocations.bin').read_bytes()
            rms_input = (args.rms_fixture / 'rms-input.bin').read_bytes()
            rms_weight = (args.rms_fixture / 'rms-weight.bin').read_bytes()
            if len(invocations) != 120 * 516 or len(rms_input) != 2560 * 4 or len(rms_weight) != len(rms_input):
                raise ValueError('unexpected RMS fixture size')
            invocation_size = 516
            for address, data in ((0x4001000, rms_input), (0x4011000, rms_weight), (0x4021000, bytes(9216 * 4))):
                select(2, address, 18, 12)
                write(bar + 0x400000 + (address & 0x1fffff), data)
        role_stop = 3 if args.projection_fixture else 1
        for core in range(120):
            select_worker(core)
            if struct.unpack('<I', read(reset_address, 4))[0] != 0x47800:
                raise RuntimeError(f'core {core}: unexpected initial reset')
            for address, data in ((0x6000, boot), (0x72000, text), (0xa0400, dispatch), *role_images):
                write(bar + address, data)
                if read(bar + address, len(data)) != data:
                    raise RuntimeError(f'core {core}: service image mismatch at {address:#x}')
            words = [0x314d4435, 3, 0, 0, 0, 1, 0, 0, 0] + [0] * 6 + [core]
            write(mailbox_address, struct.pack('<16I', *words))
            write(role_address, struct.pack('<16I', 0x31524d35, 2, *([0] * 14)))
            write(handoff_address, bytes(12))
            write32(bar, jump(0, 0x70000))
            write32(bar + 0x12000, jump(0x12000, 0x70100))
            write32(reset_address, 0x7000)  # Release BRISC and NCRISC only.

        lib.libttsim_clock(50000)
        for core in range(120):
            select_worker(core)
            handoff = struct.unpack('<3I', read(handoff_address, 12))
            if handoff != (0x31, 0x32, 0):
                raise RuntimeError(f'core {core}: role snapshots not ready: {handoff}')
            if read(bar + 0xb9000, 0x2000) != bytes(0x2000) or read(bar + 0xbb000, 0x2000) != bytes(0x2000):
                raise RuntimeError(f'core {core}: simulated private-RAM copy mismatch')
            write32(handoff_address + 8, 0x53)

        lib.libttsim_clock(20000)
        for core in range(120):
            select_worker(core)
            words = struct.unpack('<16I', read(role_address, 64))
            if words[11:13] != (0x52333551, 0x57333551):
                raise RuntimeError(f'core {core}: reader/writer not started: {words}')
            write32(reset_address, 0x6000)  # Release TRISC0; keep TRISC1/2 held.

        lib.libttsim_clock(30000)
        for core in range(120):
            select_worker(core)
            words = struct.unpack('<16I', read(mailbox_address, 64))
            if words[5:8] != (1, 0, 0) or words[15] != core:
                raise RuntimeError(f'core {core}: service failed startup: {words}')
            if invocations is not None:
                write(bar + 0xaa000, invocations[core * invocation_size:(core + 1) * invocation_size])

        sequence = 0
        if invocations is not None:
            for run in range(2):
                if run and args.projection_fixture:
                    input_offset = 2764800
                    bits = struct.unpack_from('<2560I', fixture, input_offset)
                    data = struct.pack('<2560I', *(value ^ 0x80000000 for value in bits))
                    bits = struct.unpack('<1920I', expected)
                    expected = struct.pack('<1920I', *(value ^ 0x80000000 for value in bits))
                    output_offset = 2777088
                    sentinel = fixture[output_offset:output_offset + len(expected)]
                    for address, payload in ((0x4001000 + input_offset, data),
                                             (0x4001000 + output_offset, sentinel)):
                        select(2, address, 18, 12)
                        write(bar + 0x400000 + (address & 0x1fffff), payload)
                elif run and args.silu_fixture:
                    gates = struct.unpack('<9216f', gate)
                    ups = struct.unpack('<9216f', up)
                    gate = struct.pack('<9216f', *(-value for value in gates))
                    up = struct.pack('<9216f', *(-value for value in ups))
                    for address, data in ((0x4001000, gate), (0x4011000, up)):
                        select(2, address, 18, 12)
                        write(bar + 0x400000 + (address & 0x1fffff), data)
                elif run and args.rms_fixture:
                    inputs = struct.unpack('<2560f', rms_input)
                    rms_input = struct.pack('<2560f', *(-value for value in inputs))
                    for address, data in ((0x4001000, rms_input), (0x4021000, bytes(9216 * 4))):
                        select(2, address, 18, 12)
                        write(bar + 0x400000 + (address & 0x1fffff), data)
                sequence += 1
                for core in range(120):
                    select_worker(core)
                    write32(mailbox_address + 8, sequence)
                for attempt in range(100):
                    lib.libttsim_clock(100000)
                    completed = 0
                    for core in range(120):
                        select_worker(core)
                        words = struct.unpack('<16I', read(mailbox_address, 64))
                        if words[5] in (4, 6):
                            roles = struct.unpack('<16I', read(role_address, 64))
                            raise RuntimeError(f'core {core}: operation failed: {words}; roles={roles}')
                        completed += words[5] == 3 and words[4] == sequence
                    if completed == 120:
                        break
                else:
                    raise RuntimeError('operation did not complete on all 120 simulated cores')
                if args.projection_fixture:
                    address = 0x4001000 + 2777088
                    select(2, address, 18, 12)
                    if read(bar + 0x400000 + (address & 0x1fffff), len(expected)) != expected:
                        raise RuntimeError('simulated fixed projection output did not match expected values')
                elif args.silu_fixture:
                    address = 0x4001000
                    select(2, address, 18, 12)
                    values = struct.unpack('<9216f', read(bar + 0x400000 + (address & 0x1fffff), len(gate)))
                    gates = struct.unpack('<9216f', gate)
                    ups = struct.unpack('<9216f', up)
                    for row, (actual, a, b) in enumerate(zip(values, gates, ups)):
                        reference = a / (1.0 + math.exp(-a)) * b
                        if not math.isfinite(actual) or abs(actual - reference) > 2e-5 * max(1.0, abs(reference)):
                            raise RuntimeError(f'SiLU run {run + 1}, row {row}: {actual} differs from reference {reference}')
                else:
                    address = 0x4021000
                    select(2, address, 18, 12)
                    values = struct.unpack('<2560f', read(bar + 0x400000 + (address & 0x1fffff), len(rms_input)))
                    inputs = struct.unpack('<2560f', rms_input)
                    weights = struct.unpack('<2560f', rms_weight)
                    scale = 1.0 / math.sqrt(sum(value * value for value in inputs) / len(inputs) + 1e-6)
                    for row, (actual, value, weight) in enumerate(zip(values, inputs, weights)):
                        reference = value * scale * weight
                        if not math.isfinite(actual) or abs(actual - reference) > 2e-5 * max(1.0, abs(reference)):
                            raise RuntimeError(f'RMS run {run + 1}, row {row}: {actual} differs from reference {reference}')
                for core in range(120):
                    select_worker(core)
                    write32(mailbox_address + 12, sequence)
                lib.libttsim_clock(30000)
                for core in range(120):
                    select_worker(core)
                    words = struct.unpack('<16I', read(mailbox_address, 64))
                    if words[2:8] != (sequence, sequence, sequence, 1, 0, 0):
                        raise RuntimeError(f'core {core}: service did not return to ready: {words}')

        for core in range(120):
            select_worker(core)
            words = struct.unpack('<16I', read(mailbox_address, 64))
            if words[2:8] != (sequence, sequence, sequence, 1, 0, 0):
                raise RuntimeError(f'core {core}: service did not return to ready: {words}')
            write32(role_address + 32, role_stop)
            write32(mailbox_address + 32, 0x53544f50)

        lib.libttsim_clock(30000)
        for core in range(120):
            select_worker(core)
            words = struct.unpack('<16I', read(mailbox_address, 64))
            if words[2:8] != (sequence, sequence, sequence, 5, 0, 0) or words[15] != core:
                raise RuntimeError(f'core {core}: service did not stop cleanly: {words}')
            roles = struct.unpack('<16I', read(role_address, 64))
            if roles[8:11] != (role_stop, role_stop, role_stop):
                raise RuntimeError(f'core {core}: reader/writer did not stop: {roles}')
        print('PASS compiled Q35 service and reader/writer roles start and stop on 120 Blackhole simulator cores; NO HARDWARE')
        if invocations is not None:
            if args.projection_fixture:
                print('PASS two resident 120-core Q4_K projections match 1,920 independently checked values each; SIMULATED ONLY')
            elif args.silu_fixture:
                print('PASS two resident 120-core SiLU runs match 9,216 values each; SIMULATED ONLY')
            else:
                print('PASS two resident 120-core RMS runs match 2,560 values each; SIMULATED ONLY')
    finally:
        lib.libttsim_exit()


if __name__ == '__main__':
    main()
