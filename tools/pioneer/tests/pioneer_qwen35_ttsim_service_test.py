#!/usr/bin/env python3
"""Run the compiled Q35 service and roles on 120 simulated Blackhole tiles.

Pass a locally built Tenstorrent libttsim and the generated standalone
service and shared-role image directories. This uses the simulator's PCI API;
it neither accesses hardware nor measures inference or P100A performance.
"""

import argparse
import ctypes
import hashlib
import math
from pathlib import Path
import struct


def projection_reference(fixture, quant, input_offset, tile, rows=1920, weight_offset=0):
    """Independently decode byte-major Q4_K, Q5_K, Q6_K or Q8_0 on the host.

    This is an offline reference for the simulator fixture, not an inference
    path. The fixed synthetic fixture has exactly representable outputs;
    real model weights require a floating-point tolerance.
    """
    inputs = struct.unpack_from('<2560f', fixture, input_offset)
    block_bytes = {'q4': 144, 'q5': 176, 'q6': 210, 'q8': 34}[quant]
    blocks = 80 if quant == 'q8' else 10
    values, norms = [], []
    for row in range(rows):
        total = 0.0
        norm = 0.0
        for block in range(blocks):
            base = weight_offset + (row // 16 * blocks + block) * tile + row % 16
            data = fixture[base:base + block_bytes * 16:16]
            if quant == 'q8':
                d = struct.unpack_from('<e', data)[0]
                for j in range(32):
                    q = struct.unpack_from('<b', data, 2 + j)[0]
                    term = inputs[block * 32 + j] * d * q
                    total += term
                    norm += abs(term)
                continue
            if quant == 'q6':
                d = struct.unpack_from('<e', data, 208)[0]
                for pos in range(256):
                    half, rem = divmod(pos, 128)
                    group, lane = divmod(rem, 32)
                    lowbyte = data[half * 64 + (group % 2) * 32 + lane]
                    low = (lowbyte & 15) if group < 2 else (lowbyte >> 4)
                    high = (data[128 + half * 32 + lane] >> (group * 2)) & 3
                    q = (low | (high << 4)) - 32
                    scale = struct.unpack_from('<b', data, 192 + pos // 16)[0]
                    term = inputs[block * 256 + pos] * d * scale * q
                    total += term
                    norm += abs(term)
                continue
            d, dmin = struct.unpack_from('<ee', data)
            scales = data[4:16]
            for group in range(8):
                if group < 4:
                    scale = scales[group] & 63
                    minimum = scales[group + 4] & 63
                else:
                    scale = (scales[group + 4] & 15) | ((scales[group - 4] >> 6) << 4)
                    minimum = (scales[group + 4] >> 4) | ((scales[group] >> 6) << 4)
                packed = (48 if quant == 'q5' else 16) + (group // 2) * 32
                activation = block * 256 + group * 32
                for j in range(32):
                    q = data[packed + j]
                    q = (q & 15) if group % 2 == 0 else (q >> 4)
                    if quant == 'q5' and data[16 + j] & (1 << group):
                        q |= 16
                    term = inputs[activation + j] * (d * scale * q - dmin * minimum)
                    total += term
                    norm += abs(term)
        if not math.isfinite(total) or not math.isfinite(norm):
            raise ValueError(f'non-finite {quant} reference at row {row}')
        values.append(total)
        norms.append(norm)
    return values, norms


def quantized_activation_reference(values, quant):
    """Independent Q8_K/Q8_0 activation values for an offline dot reference."""
    def f32(value):
        return struct.unpack('<f', struct.pack('<f', value))[0]

    result = []
    width = 32 if quant == 'q8' else 256
    for first in range(0, len(values), width):
        block = values[first:first + width]
        selected = max(block, key=abs)
        if not selected:
            result.extend([0.0] * width)
            continue
        if quant == 'q8':
            scale = f32(abs(selected) / 127.0)
            inverse = f32(1.0 / scale)
            stored_scale = struct.unpack('<e', struct.pack('<e', scale))[0]
            for value in block:
                scaled = f32(value * inverse)
                rounded = int(math.copysign(math.floor(abs(scaled) + 0.5), scaled))
                result.append(f32(rounded * stored_scale))
        else:
            inverse = f32(127.0 / abs(selected))
            stored_scale = f32(1.0 / inverse)
            if selected > 0:
                inverse, stored_scale = -inverse, -stored_scale
            for value in block:
                rounded = round(f32(value * inverse))
                result.append(f32(rounded * stored_scale))
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('library', type=Path)
    parser.add_argument('image', type=Path)
    parser.add_argument('roles', type=Path)
    parser.add_argument('--model-projection-type', choices=('q4', 'q5', 'q6'), default='q4',
                        help='quantization selected when preparing a model projection fixture')
    group = parser.add_mutually_exclusive_group()
    group.add_argument('--projection-fixture', type=Path,
                       help='also execute the separately generated fixed projection')
    group.add_argument('--model-projection-fixture', type=Path,
                       help='also execute a fixture prepared from the pinned 4B model')
    group.add_argument('--fused-model-fixture', type=Path,
                       help='execute full QKV, Alpha and Beta model projections on 120 workers')
    group.add_argument('--resident-templates', type=Path,
                       help='start the resident graph image using synthetic per-core templates')
    group.add_argument('--resident-sequence-fixture', type=Path,
                       help='run embedding, residual, RMS and fused projection steps from model weights')
    group.add_argument('--delta-shards-fixture', type=Path,
                       help='run the experimental two-stage recurrent update on 120 workers')
    group.add_argument('--silu-fixture', type=Path,
                       help='also execute the separately generated 120-core SiLU operation')
    group.add_argument('--rms-fixture', type=Path,
                       help='also execute the separately generated 120-core RMS normalization')
    parser.add_argument('--resident-corrupt-core', type=int,
                        help='corrupt one resident template to exercise its startup fault')
    parser.add_argument('--resident-fault-command', type=Path,
                        help='submit a resident command against deliberately invalid stream records')
    args = parser.parse_args()
    if args.resident_corrupt_core is not None and (not args.resident_templates
            or not 0 <= args.resident_corrupt_core < 120):
        parser.error('--resident-corrupt-core requires resident templates and a core in 0..119')
    if args.resident_fault_command and (not args.resident_templates or args.resident_corrupt_core is not None):
        parser.error('--resident-fault-command requires resident templates and no corrupt core')
    template_directory = args.resident_sequence_fixture or args.delta_shards_fixture
    template_path = template_directory / 'templates.bin' if template_directory else args.resident_templates
    resident_templates = template_path.read_bytes() if template_path else None
    if resident_templates is not None and len(resident_templates) != 120 * 512:
        raise ValueError('unexpected resident-template fixture size')
    image = args.image
    if not (image / 'activation-sharing.marker').is_file():
        raise ValueError('expected sharing-enabled standalone service image')
    if bool(args.delta_shards_fixture) != (image / 'delta-shard.marker').is_file():
        raise ValueError('delta shard fixture and service image do not match')
    boot = (image / 'boot.bin').read_bytes()
    text = (image / 'text.bin').read_bytes()
    dispatch = (image / 'dispatch.bin').read_bytes()
    dispatch_limit = 0x7000 if resident_templates is not None else 0x5c00
    if boot != bytes.fromhex('6fc00600') or not 0 < len(text) <= 0xe000 or not 0 < len(dispatch) <= dispatch_limit:
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
        projection_fixture = args.projection_fixture or args.model_projection_fixture or args.fused_model_fixture
        if args.fused_model_fixture:
            fixture = (projection_fixture / 'fused-fixture.bin').read_bytes()
            invocations = (projection_fixture / 'fused-invocations.bin').read_bytes()
            if len(fixture) != 14659584 or len(invocations) != 120 * 1128:
                raise ValueError('unexpected fused model fixture size')
            tensors = ((0, 14417920, 'e45da19dfd2000f2c4b40f456a5663dc6677f7990a1626c897e7219387d9ca91'),
                       (14417920, 92160, 'c66aaea468bd19d584e6259ab39ef7770ed29a93d59ebe0dcdb9dd04d1f1d16d'),
                       (14512128, 92160, 'd04988bc4158cb6936a5ae77eb6b21299d2c81bf8e5536ee0e2d41e635996ffc'))
            for offset, size, digest in tensors:
                if hashlib.sha256(fixture[offset:offset + size]).hexdigest() != digest:
                    raise ValueError('fused fixture weights do not match the pinned 4B tensors')
            input_offset = 14606336
            outputs = (('QKV', 8192, 14618624), ('Alpha', 32, 14651392), ('Beta', 32, 14655488))
            fused_expected = (projection_reference(fixture, 'q5', input_offset, 2816, 8192, 0),
                              projection_reference(fixture, 'q8', input_offset, 576, 32, 14417920),
                              projection_reference(fixture, 'q8', input_offset, 576, 32, 14512128))
            print('Prepared independent fused references', flush=True)
            invocation_size = 1128
        elif projection_fixture:
            quant = args.model_projection_type if args.model_projection_fixture else 'q4'
            tile = {'q4': 2304, 'q5': 2816, 'q6': 3392}[quant]
            weight_bytes = 120 * 10 * tile
            input_offset = (weight_bytes + 4095) & ~4095
            output_offset = input_offset + 12288
            fixture = (projection_fixture / 'fixture.bin').read_bytes()
            invocations = (projection_fixture / 'invocations.bin').read_bytes()
            expected = (projection_fixture / 'expected.bin').read_bytes()
            if len(fixture) != output_offset + 8192 or len(invocations) != 120 * 1128 or len(expected) != 1920 * 4:
                raise ValueError('unexpected fixed-projection fixture size')
            digests = {'q4': '550cc5316cf0f1bf1427a0f2d269cc1b74736e6fe9f04751629deb7e1b2ab26d',
                       'q5': 'decc4d81af4674d2458311a2a5dd7bdbf55cf6ea6ed57a40f6d5b7b8b96babd0',
                       'q6': 'c59d4ad12f9bfaab96d3b3a94a855d63dfceb4de060b3843f5cef4d09562abf5'}
            if args.model_projection_fixture and hashlib.sha256(fixture[:weight_bytes]).hexdigest() != digests[quant]:
                raise ValueError('model fixture weights do not match the pinned 4B tensor slice')
            values, norms = projection_reference(fixture, quant, input_offset, tile)
            reference = struct.pack('<1920f', *values)
            if args.projection_fixture and reference != expected:
                raise ValueError('fixed Q4_K fixture does not match the independent reference')
            expected = reference
            invocation_size = 1128
        if projection_fixture:
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
        role_stop = 3 if projection_fixture else 1
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
            if resident_templates is not None:
                template = bytearray(resident_templates[core * 512:(core + 1) * 512])
                if core == args.resident_corrupt_core:
                    template[0] ^= 1
                write(bar + 0xab100, bytes(template))
                write(bar + 0xab550, bytes([0xa5]) * 16)
                write(bar + 0xab580, bytes([0xa5]) * 4)
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
            if core == args.resident_corrupt_core:
                if words[5:8] != (6, 4, 6):
                    raise RuntimeError(f'core {core}: corrupt template did not fault: {words}')
                continue
            if words[5:8] != (1, 0, 0) or words[15] != core:
                raise RuntimeError(f'core {core}: service failed startup: {words}')
            if resident_templates is not None and (read(bar + 0xab550, 16) != bytes(4) + bytes([0xa5]) * 12
                                                    or read(bar + 0xab580, 4) != bytes(4)):
                raise RuntimeError(f'core {core}: resident initialization did not run')
            if invocations is not None:
                write(bar + 0xaa000, invocations[core * invocation_size:(core + 1) * invocation_size])

        if args.delta_shards_fixture:
            directory = args.delta_shards_fixture
            command = (directory / 'command.bin').read_bytes()
            records = (directory / 'records.bin').read_bytes()
            initial = (directory / 'initial.bin').read_bytes()
            expected = (directory / 'expected.bin').read_bytes()
            layouts = ((0x10000000, 8192 * 4), (0x11000000, 4096 * 4),
                       (0x12000000, 32 * 4), (0x13000000, 32 * 4),
                       (0x14000000, 32 * 4), (0x15000000, 32 * 4),
                       (0x16000000, 128 * 4), (0x20000000, 32 * 128 * 128 * 4),
                       (0x30000000, 32 * 128 * 128 * 4))
            if (len(command) != 128 or len(records) != 120 * 3 * 2048
                    or len(initial) != sum(size for _, size in layouts)
                    or len(expected) != (8192 + 32 * 128 * 128) * 4):
                raise ValueError('unexpected delta shard fixture size')
            cursor = 0
            for start, size in layouts:
                offset = 0
                while offset < size:
                    address = start + offset
                    select(2, address, 18, 12)
                    count = min(0x10000, size - offset, 0x200000 - (address & 0x1fffff))
                    write(bar + 0x400000 + (address & 0x1fffff), initial[cursor + offset:cursor + offset + count])
                    offset += count
                cursor += size
            for core in range(120):
                address = 0x04000000 + core * 0x100000
                select(2, address, 18, 12)
                write(bar + 0x400000 + (address & 0x1fffff), records[core * 6144:(core + 1) * 6144])
            select_worker(0)
            write(bar + 0xab080, command[:64])
            write(bar + 0xab0c0, command[64:])
            for attempt in range(1000):
                lib.libttsim_clock(100000)
                snapshots = []
                for core in range(120):
                    select_worker(core)
                    snapshots.append(struct.unpack('<16I', read(mailbox_address, 64)))
                if all(words[5] == 4 for words in snapshots):
                    break
                if any(words[5] not in (1, 2, 4) for words in snapshots):
                    raise RuntimeError('unexpected worker state during sharded recurrent update')
            else:
                raise RuntimeError('sharded recurrent sequence did not reach its intentional terminal fault')
            for core, words in enumerate(snapshots):
                if (words[2:8] != (1, 0, 1, 4, 4, 5)
                        or words[9:15] != (0x31464735, 1, 2, 120, 0x80000004, 0)
                        or words[15] != core):
                    raise RuntimeError(f'core {core}: sharded sequence failed before both barriers: {words}')
            for name, address, rows, expected_offset in (('QKV', 0x10000000, 8192, 0),
                    ('state', 0x30000000, 32 * 128 * 128, 8192 * 4)):
                actual = bytearray()
                offset = 0
                while offset < rows * 4:
                    current = address + offset
                    select(2, current, 18, 12)
                    count = min(0x10000, rows * 4 - offset, 0x200000 - (current & 0x1fffff))
                    actual.extend(read(bar + 0x400000 + (current & 0x1fffff), count))
                    offset += count
                maximum = 0.0
                for row, (got,), (want,) in zip(range(rows), struct.iter_unpack('<f', actual),
                        struct.iter_unpack('<f', expected[expected_offset:expected_offset + rows * 4])):
                    error = abs(got - want)
                    if not math.isfinite(got) or error > 2e-5 * max(1.0, abs(want)):
                        raise RuntimeError(f'{name} row {row}: {got} differs from full-head reference {want}')
                    maximum = max(maximum, error)
                print(f'{name}: {rows} values match full-head reference; maximum absolute error {maximum:.9g}', flush=True)
            select_worker(0)
            if struct.unpack('<8I', read(bar + 0xab310, 32))[7] != 0:
                raise RuntimeError('intentionally faulted shard sequence published a candidate')
            print('PASS 120 simulated workers completed sharded update and normalization; '
                  'third record intentionally faults; NO HARDWARE')
            return

        if args.resident_sequence_fixture:
            directory = args.resident_sequence_fixture
            command = (directory / 'command.bin').read_bytes()
            records = (directory / 'records.bin').read_bytes()
            weights = (directory / 'weights.bin').read_bytes()
            norm = (directory / 'norm.bin').read_bytes()
            projection = (directory / 'projection.bin').read_bytes()
            gate = (directory / 'gate.bin').read_bytes()
            if (len(command) != 128 or len(records) != 120 * 6 * 2048 or len(weights) != 33920
                    or hashlib.sha256(weights).hexdigest()
                    != 'd784c0b7fe8df8aab8d4797a8ea5a2c25b5db49a84c60a3ad93819086355871d'
                    or len(norm) != 10240 or hashlib.sha256(norm).hexdigest()
                    != 'fd26aeb9265ba4588eca258e4c7fb27d2b82198eef72bbca4d8d46136b90c104'
                    or len(projection) != 14604288 or len(gate) != 5898240
                    or hashlib.sha256(gate).hexdigest()
                    != '9e3596cdad68531922f9a5e8775e576d4254ba45f66281c203c3baa64d0cadbb'):
                raise ValueError('unexpected authenticated resident sequence fixture')
            tensors = ((0, 14417920, 'e45da19dfd2000f2c4b40f456a5663dc6677f7990a1626c897e7219387d9ca91'),
                       (14417920, 92160, 'c66aaea468bd19d584e6259ab39ef7770ed29a93d59ebe0dcdb9dd04d1f1d16d'),
                       (14512128, 92160, 'd04988bc4158cb6936a5ae77eb6b21299d2c81bf8e5536ee0e2d41e635996ffc'))
            for offset, size, digest in tensors:
                if hashlib.sha256(projection[offset:offset + size]).hexdigest() != digest:
                    raise ValueError('sequence projection weights do not match the pinned 4B tensors')
            # Independently decode token 42 (lane 10) from the byte-major
            # Q6_K blocks. This is an offline simulator reference only.
            expected = []
            for block in range(10):
                tile = weights[block * 3392:(block + 1) * 3392]
                data = tile[10:210 * 16:16]
                scale = struct.unpack_from('<e', data, 208)[0]
                for pos in range(256):
                    half, rem = divmod(pos, 128)
                    group, lane = divmod(rem, 32)
                    lowbyte = data[half * 64 + (group % 2) * 32 + lane]
                    low = (lowbyte & 15) if group < 2 else (lowbyte >> 4)
                    high = (data[128 + half * 32 + lane] >> (group * 2)) & 3
                    q = (low | (high << 4)) - 32
                    s = struct.unpack_from('<b', data, 192 + pos // 16)[0]
                    expected.append(scale * s * q)
            address = 0x10010900
            select(2, address, 18, 12)
            write(bar + 0x400000 + (address & 0x1fffff), weights)
            address = 0x20030000
            select(2, address, 18, 12)
            write(bar + 0x400000 + (address & 0x1fffff), norm)
            offset = 0
            while offset < len(projection):
                address = 0x28000000 + offset
                select(2, address, 18, 12)
                count = min(0x10000, len(projection) - offset, 0x200000 - (address & 0x1fffff))
                write(bar + 0x400000 + (address & 0x1fffff), projection[offset:offset + count])
                offset += count
            offset = 0
            while offset < len(gate):
                address = 0x29000000 + offset
                select(2, address, 18, 12)
                count = min(0x10000, len(gate) - offset, 0x200000 - (address & 0x1fffff))
                write(bar + 0x400000 + (address & 0x1fffff), gate[offset:offset + count])
                offset += count
            for address in (0x20000000, 0x20010000, 0x20020000):
                select(2, address, 18, 12)
                write(bar + 0x400000 + (address & 0x1fffff),
                      struct.pack('<2560I', *([0x7fc00000] * 2560)))
            for address, rows in ((0x21000000, 8192), (0x21010000, 32), (0x21020000, 32), (0x21030000, 4096)):
                select(2, address, 18, 12)
                write(bar + 0x400000 + (address & 0x1fffff),
                      struct.pack(f'<{rows}I', *([0x7fc00000] * rows)))
            for core in range(120):
                address = 0x04000000 + core * 0x100000
                select(2, address, 18, 12)
                write(bar + 0x400000 + (address & 0x1fffff), records[core * 12288:(core + 1) * 12288])
            select_worker(0)
            write(bar + 0xab080, command[:64])
            write(bar + 0xab0c0, command[64:])
            for attempt in range(200):
                lib.libttsim_clock(100000)
                snapshots = []
                for core in range(120):
                    select_worker(core)
                    snapshots.append(struct.unpack('<16I', read(mailbox_address, 64)))
                if all(words[5] == 4 for words in snapshots):
                    break
                if any(words[5] not in (1, 2, 4) for words in snapshots):
                    raise RuntimeError('unexpected resident state during sequence')
            else:
                raise RuntimeError('resident sequence did not reach its intentional terminal fault')
            for core, words in enumerate(snapshots):
                if (words[2:8] != (1, 0, 1, 4, 4, 5)
                        or words[9:15] != (0x31464735, 1, 5, 120, 0x80000004, 0)
                        or words[15] != core):
                    raise RuntimeError(f'core {core}: sequence failed before all five barriers: {words}')
            for name, address in (('embedding', 0x20000000), ('residual save', 0x20010000)):
                select(2, address, 18, 12)
                actual = struct.unpack('<2560f', read(bar + 0x400000 + (address & 0x1fffff), 2560 * 4))
                maximum = 0.0
                for row, (got, want) in enumerate(zip(actual, expected)):
                    if not math.isfinite(got) or abs(got - want) > 1e-6 * max(1.0, abs(want)):
                        raise RuntimeError(f'{name} row {row}: {got} differs from {want}')
                    maximum = max(maximum, abs(got - want))
                print(f'{name}: 2,560 values match independent Q6_K reference; '
                      f'maximum absolute error {maximum:.9g}', flush=True)
            norm_weights = struct.unpack('<2560f', norm)
            rms_scale = 1.0 / math.sqrt(sum(value * value for value in expected) / 2560 + 1e-6)
            address = 0x20020000
            select(2, address, 18, 12)
            actual = struct.unpack('<2560f', read(bar + 0x400000 + (address & 0x1fffff), 2560 * 4))
            maximum = 0.0
            for row, (got, value, weight) in enumerate(zip(actual, expected, norm_weights)):
                want = value * rms_scale * weight
                if not math.isfinite(got) or abs(got - want) > 2e-5 * max(1.0, abs(want)):
                    raise RuntimeError(f'RMS row {row}: {got} differs from {want}')
                maximum = max(maximum, abs(got - want))
            print(f'RMS: 2,560 values match independent reference; maximum absolute error {maximum:.9g}', flush=True)
            # The RMS output has already been independently checked above.
            # Projections quantize it to Q8_K or Q8_0 before the dot product.
            actual_rms = actual
            reference_fixtures = {quant: projection + struct.pack('<2560f',
                *quantized_activation_reference(actual, quant)) for quant in ('q5', 'q8')}
            specs = (('QKV', 8192, 0x21000000, 'q5', 2816, 0),
                     ('Alpha', 32, 0x21010000, 'q8', 576, 14417920),
                     ('Beta', 32, 0x21020000, 'q8', 576, 14512128))
            for name, rows, address, quant, tile, weight_offset in specs:
                values, norms = projection_reference(reference_fixtures[quant], quant, len(projection),
                                                     tile, rows, weight_offset)
                select(2, address, 18, 12)
                actual = struct.unpack(f'<{rows}f', read(bar + 0x400000 + (address & 0x1fffff), rows * 4))
                maximum = worst = 0.0
                for row, (got, want, norm_value) in enumerate(zip(actual, values, norms)):
                    error = abs(got - want)
                    ratio = error / max(1.0, norm_value)
                    maximum = max(maximum, error)
                    worst = max(worst, ratio)
                    if not math.isfinite(got) or ratio > 2e-6:
                        raise RuntimeError(f'{name} row {row}: {got} differs from {want}; normalized error {ratio}')
                print(f'{name}: {rows} values match independent reference; maximum absolute error '
                      f'{maximum:.9g}; maximum normalized error {worst:.9g}', flush=True)
            gate_input = struct.pack('<2560f', *quantized_activation_reference(actual_rms, 'q4'))
            values, norms = projection_reference(gate + gate_input, 'q4', len(gate), 2304, 4096)
            address = 0x21030000
            select(2, address, 18, 12)
            gate_actual = struct.unpack('<4096f', read(bar + 0x400000 + (address & 0x1fffff), 4096 * 4))
            maximum = worst = 0.0
            for row, (got, want, norm_value) in enumerate(zip(gate_actual, values, norms)):
                error = abs(got - want)
                ratio = error / max(1.0, norm_value)
                maximum, worst = max(maximum, error), max(worst, ratio)
                if not math.isfinite(got) or ratio > 2e-6:
                    raise RuntimeError(f'gate row {row}: {got} differs from {want}; normalized error {ratio}')
            print(f'gate: 4096 values match independent reference; maximum absolute error '
                  f'{maximum:.9g}; maximum normalized error {worst:.9g}', flush=True)
            select_worker(0)
            if struct.unpack('<8I', read(bar + 0xab310, 32))[7] != 0:
                raise RuntimeError('intentionally faulted sequence published a candidate')
            print('PASS resident graph completed embedding, residual, RMS, fused and gate projection barriers '
                  'on 120 simulated workers; sixth record intentionally faults; NO HARDWARE')
            return

        if args.resident_fault_command:
            command = args.resident_fault_command.read_bytes()
            if len(command) != 128:
                raise ValueError('unexpected resident fault command size')
            # The fixture assigns one stream per worker in bank 0. Explicitly
            # zero the first record rather than relying on simulator RAM state.
            for core in range(120):
                address = 0x04000000 + core * 0x100000
                select(2, address, 18, 12)
                write(bar + 0x400000 + (address & 0x1fffff), bytes(2048))
            select_worker(0)
            write(bar + 0xab080, command[:64])
            write(bar + 0xab0c0, command[64:])
            for attempt in range(100):
                lib.libttsim_clock(100000)
                snapshots = []
                for core in range(120):
                    select_worker(core)
                    snapshots.append(struct.unpack('<16I', read(mailbox_address, 64)))
                if all(words[5] == 4 for words in snapshots):
                    break
                if any(words[5] not in (1, 2, 4) for words in snapshots):
                    raise RuntimeError('unexpected resident state while testing invalid records')
            else:
                raise RuntimeError('resident fault did not reach all 120 simulated workers')
            for core, words in enumerate(snapshots):
                if (words[2:8] != (1, 0, 1, 4, 4, 5)
                        or words[9:15] != (0x31464735, 1, 0, 120, 0x80000004, 0)
                        or words[15] != core):
                    raise RuntimeError(f'core {core}: unexpected graph fault: {words}')
            lib.libttsim_clock(50000)
            for core, before in enumerate(snapshots):
                select_worker(core)
                if struct.unpack('<16I', read(mailbox_address, 64)) != before:
                    raise RuntimeError(f'core {core}: graph fault was not retained')
            select_worker(0)
            if struct.unpack('<8I', read(bar + 0xab310, 32))[7] != 0:
                raise RuntimeError('faulted resident graph published a candidate')
            print('PASS resident command reached 120 simulated workers; invalid stream records '
                  'produced retained fetch faults without a completion candidate; NO HARDWARE')
            return

        sequence = 0
        if invocations is not None:
            for run in range(2):
                if run and projection_fixture:
                    bits = struct.unpack_from('<2560I', fixture, input_offset)
                    data = struct.pack('<2560I', *(value ^ 0x80000000 for value in bits))
                    if args.fused_model_fixture:
                        sentinels = [(0x4001000 + offset, fixture[offset:offset + rows * 4])
                                     for _, rows, offset in outputs]
                    else:
                        bits = struct.unpack('<1920I', expected)
                        expected = struct.pack('<1920I', *(value ^ 0x80000000 for value in bits))
                        sentinels = [(0x4001000 + output_offset, fixture[output_offset:output_offset + len(expected)])]
                    for address, payload in [(0x4001000 + input_offset, data), *sentinels]:
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
                for attempt in range(300 if args.fused_model_fixture else 100):
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
                if args.fused_model_fixture:
                    for (name, rows, offset), (reference, norms) in zip(outputs, fused_expected):
                        address = 0x4001000 + offset
                        select(2, address, 18, 12)
                        actual = struct.unpack(f'<{rows}f', read(bar + 0x400000 + (address & 0x1fffff), rows * 4))
                        maximum = worst = 0.0
                        for row, (got, want, norm) in enumerate(zip(actual, reference, norms)):
                            want = -want if run else want
                            error = abs(got - want)
                            ratio = error / max(1.0, norm)
                            maximum = max(maximum, error)
                            worst = max(worst, ratio)
                            if not math.isfinite(got) or ratio > 2e-6:
                                raise RuntimeError(f'{name} run {run + 1}, row {row}: '
                                                   f'{got} differs from {want}; normalized error {ratio}')
                        print(f'{name} run {run + 1}: maximum absolute error {maximum:.9g}; '
                              f'maximum error / sum of absolute products {worst:.9g}', flush=True)
                elif projection_fixture:
                    address = 0x4001000 + output_offset
                    select(2, address, 18, 12)
                    data = read(bar + 0x400000 + (address & 0x1fffff), len(expected))
                    if args.projection_fixture:
                        if data != expected:
                            raise RuntimeError('simulated fixed projection output did not match expected values')
                    else:
                        actual = struct.unpack('<1920f', data)
                        reference = struct.unpack('<1920f', expected)
                        maximum = worst = 0.0
                        for row, (got, want, norm) in enumerate(zip(actual, reference, norms)):
                            error = abs(got - want)
                            ratio = error / max(1.0, norm)
                            maximum = max(maximum, error)
                            worst = max(worst, ratio)
                            if not math.isfinite(got) or ratio > 2e-6:
                                raise RuntimeError(f'model projection run {run + 1}, row {row}: '
                                                   f'{got} differs from {want}; normalized error {ratio}')
                        print(f'model projection run {run + 1}: maximum absolute error {maximum:.9g}; '
                              f'maximum error / sum of absolute products {worst:.9g}', flush=True)
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
            if core == args.resident_corrupt_core:
                write32(role_address + 32, role_stop)
                continue
            if words[2:8] != (sequence, sequence, sequence, 1, 0, 0):
                raise RuntimeError(f'core {core}: service did not return to ready: {words}')
            write32(role_address + 32, role_stop)
            write32(mailbox_address + 32, 0x53544f50)

        lib.libttsim_clock(30000)
        for core in range(120):
            select_worker(core)
            words = struct.unpack('<16I', read(mailbox_address, 64))
            if core == args.resident_corrupt_core:
                if words[5:8] != (6, 4, 6):
                    raise RuntimeError(f'core {core}: startup fault was not retained: {words}')
                roles = struct.unpack('<16I', read(role_address, 64))
                if roles[8:11] != (role_stop, role_stop, role_stop):
                    raise RuntimeError(f'core {core}: reader/writer did not stop: {roles}')
                continue
            if words[2:8] != (sequence, sequence, sequence, 5, 0, 0) or words[15] != core:
                raise RuntimeError(f'core {core}: service did not stop cleanly: {words}')
            roles = struct.unpack('<16I', read(role_address, 64))
            if roles[8:11] != (role_stop, role_stop, role_stop):
                raise RuntimeError(f'core {core}: reader/writer did not stop: {roles}')
        if resident_templates is not None:
            if args.resident_corrupt_core is None:
                print('PASS compiled resident graph service starts and stops on 120 simulator cores; NO HARDWARE')
            else:
                print(f'PASS corrupt resident template faults core {args.resident_corrupt_core}; '
                      'other cores stop; SIMULATED ONLY')
        else:
            print('PASS compiled Q35 service and reader/writer roles start and stop on 120 Blackhole simulator cores; NO HARDWARE')
        if invocations is not None:
            if args.fused_model_fixture:
                print('PASS two resident 116+2+2 QKV/Alpha/Beta projections match '
                      '8,256 independently checked values each; SIMULATED ONLY')
            elif args.projection_fixture:
                print('PASS two resident 120-core Q4_K projections match 1,920 independently checked values each; SIMULATED ONLY')
            elif args.model_projection_fixture:
                print(f'PASS two resident 120-core model-weight {quant.upper()}_K projections match '
                      '1,920 independently checked values each; SIMULATED ONLY')
            elif args.silu_fixture:
                print('PASS two resident 120-core SiLU runs match 9,216 values each; SIMULATED ONLY')
            else:
                print('PASS two resident 120-core RMS runs match 2,560 values each; SIMULATED ONLY')
    finally:
        lib.libttsim_exit()


if __name__ == '__main__':
    main()
