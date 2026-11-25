#!/usr/bin/env python3
"""Run the compiled Q35 service and roles on 120 simulated Blackhole tiles.

Pass a locally built Tenstorrent libttsim and the generated standalone
service and shared-role image directories. This uses the simulator's PCI API;
it neither accesses hardware nor measures inference or P100A performance.
"""

import argparse
import ctypes
from pathlib import Path
import struct


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('library', type=Path)
    parser.add_argument('image', type=Path)
    parser.add_argument('roles', type=Path)
    parser.add_argument('--projection-fixture', type=Path,
                        help='also execute the separately generated fixed projection')
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
        if args.projection_fixture:
            fixture = (args.projection_fixture / 'fixture.bin').read_bytes()
            invocations = (args.projection_fixture / 'invocations.bin').read_bytes()
            expected = (args.projection_fixture / 'expected.bin').read_bytes()
            if len(fixture) != 2785280 or len(invocations) != 120 * 1128 or len(expected) != 1920 * 4:
                raise ValueError('unexpected fixed-projection fixture size')
            offset = 0
            while offset < len(fixture):
                address = 0x4001000 + offset
                select(2, address, 18, 12)
                count = min(0x10000, len(fixture) - offset, 0x200000 - (address & 0x1fffff))
                write(bar + 0x400000 + (address & 0x1fffff), fixture[offset:offset + count])
                offset += count
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
                write(bar + 0xaa000, invocations[core * 1128:(core + 1) * 1128])

        sequence = 0
        if invocations is not None:
            for core in range(120):
                select_worker(core)
                write32(mailbox_address + 8, 1)
            for attempt in range(100):
                lib.libttsim_clock(100000)
                completed = 0
                for core in range(120):
                    select_worker(core)
                    words = struct.unpack('<16I', read(mailbox_address, 64))
                    if words[5] in (4, 6):
                        roles = struct.unpack('<16I', read(role_address, 64))
                        raise RuntimeError(f'core {core}: projection failed: {words}; roles={roles}')
                    completed += words[5] == 3
                if completed == 120:
                    break
            else:
                raise RuntimeError('fixed projection did not complete on all 120 simulated cores')
            address = 0x4001000 + 2777088
            select(2, address, 18, 12)
            if read(bar + 0x400000 + (address & 0x1fffff), len(expected)) != expected:
                raise RuntimeError('simulated fixed projection output did not match expected values')
            for core in range(120):
                select_worker(core)
                write32(mailbox_address + 12, 1)
            lib.libttsim_clock(30000)
            sequence = 1

        for core in range(120):
            select_worker(core)
            words = struct.unpack('<16I', read(mailbox_address, 64))
            if words[2:8] != (sequence, sequence, sequence, 1, 0, 0):
                raise RuntimeError(f'core {core}: service did not return to ready: {words}')
            write32(role_address + 32, sequence + 1)
            write32(mailbox_address + 32, 0x53544f50)

        lib.libttsim_clock(30000)
        for core in range(120):
            select_worker(core)
            words = struct.unpack('<16I', read(mailbox_address, 64))
            if words[2:8] != (sequence, sequence, sequence, 5, 0, 0) or words[15] != core:
                raise RuntimeError(f'core {core}: service did not stop cleanly: {words}')
            roles = struct.unpack('<16I', read(role_address, 64))
            if roles[8:11] != (sequence + 1, sequence + 1, sequence + 1):
                raise RuntimeError(f'core {core}: reader/writer did not stop: {roles}')
        print('PASS compiled Q35 service and reader/writer roles start and stop on 120 Blackhole simulator cores; NO HARDWARE')
        if invocations is not None:
            print('PASS fixed Q4_K matrix projection matches all 1,920 expected values; SIMULATED ONLY')
    finally:
        lib.libttsim_exit()


if __name__ == '__main__':
    main()
