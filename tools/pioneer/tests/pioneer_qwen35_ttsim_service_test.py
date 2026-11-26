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


def resident_handoff_payload(raw):
    """Decode a simulator-only state snapshot; its hash detects corruption."""
    if (len(raw) != 40 + 24576 * 4 + 524288 * 4 or raw[:8] != b'Q35SIMW1'
            or raw[8:40] != hashlib.sha256(raw[40:]).digest()):
        raise ValueError('corrupt simulator state handoff')
    return raw[40:40 + 24576 * 4], raw[40 + 24576 * 4:]


def resident_position_one_command(initial, candidate):
    """Check that the second command changes only position and checksum."""
    if (len(initial) != 128 or len(candidate) != 128 or initial[:36] != candidate[:36]
            or initial[44:] != candidate[44:]):
        raise ValueError('unexpected position-one command')
    old = struct.unpack('<32I', initial)
    new = struct.unpack('<32I', candidate)
    if old[9] != 0 or new[9] != 1:
        raise ValueError('unexpected resident command position')
    for words in (old, new):
        checksum = 2166136261
        for word in words[:10]:
            checksum = ((checksum ^ word) * 16777619) & 0xffffffff
        if checksum != words[10]:
            raise ValueError('invalid resident command checksum')
    return candidate


def validate_resident_prompt_commands(first, second):
    """Validate the two full-count synthetic prompt requests and commits."""
    if len(first) != 128 or len(second) != 128:
        raise ValueError('invalid full-prompt command size')
    commands = [struct.unpack('<32I', raw) for raw in (first, second)]
    for index, words in enumerate(commands):
        if (words[0:2] != (0x31444335, 1) or words[5] != 521
                or words[8] != 42 or words[9] != index or any(words[11:16])):
            raise ValueError('invalid full-prompt command payload')
        checksum = 2166136261
        for word in words[:10]:
            checksum = ((checksum ^ word) * 16777619) & 0xffffffff
        marker = 0x40000000 | words[2]
        if (checksum != words[10] or words[16:] !=
                (words[3], (~words[3]) & 0xffffffff, marker, (~marker) & 0xffffffff) * 4):
            raise ValueError('invalid full-prompt command commit')
    one, two = commands
    if (two[2] != one[2] + 1 or two[3] != one[3] + 1
            or two[4] != one[4] + one[5] or two[5:9] != one[5:9]):
        raise ValueError('full-prompt commands do not have fresh disjoint tickets')


def resident_next_decode_command(initial, token):
    """Create one fresh position-one decode ticket after an audited result."""
    if len(initial) != 128 or not 0 <= token < 248320:
        raise ValueError('invalid decode handoff input')
    words = list(struct.unpack('<32I', initial))
    trigger, epoch, first, count = words[2:6]
    marker = 0x40000000 | trigger
    checksum = 2166136261
    for word in words[:10]:
        checksum = ((checksum ^ word) * 16777619) & 0xffffffff
    if (words[:2] != [0x31444335, 1] or not trigger or not epoch or not first
            or count not in (524, 532) or words[9] != 0 or any(words[11:16])
            or words[10] != checksum or not words[6] or words[8] >= 248320
            or words[16:] != [epoch, (~epoch) & 0xffffffff, marker, (~marker) & 0xffffffff] * 4
            or trigger >= 0x0fffffff or epoch == 0xffffffff
            or first + 2 * count - 1 > 0x0fffffff):
        raise ValueError('invalid or exhausted initial decode ticket')
    words[2:5] = [trigger + 1, epoch + 1, first + count]
    words[8:10] = [token, 1]
    checksum = 2166136261
    for word in words[:10]:
        checksum = ((checksum ^ word) * 16777619) & 0xffffffff
    words[10] = checksum
    marker = 0x40000000 | words[2]
    words[16:] = [words[3], (~words[3]) & 0xffffffff, marker, (~marker) & 0xffffffff] * 4
    return struct.pack('<32I', *words)


def audit_resident_candidate(receipt, command, snapshots, select_worker, read, bar, decode=False):
    """Audit the simulated root candidate and each local sender receipt."""
    words = struct.unpack('<32I', command)
    trigger, epoch, first, count = words[2:6]
    expected = [0x31435235, 2, trigger, epoch, first, count]
    checksum = 2166136261
    for word in expected:
        checksum = ((checksum ^ word) * 16777619) & 0xffffffff
    expected += [checksum, trigger]
    select_worker(0)
    if tuple(expected) != receipt or struct.unpack('<8I', read(bar + 0xab310, 32)) != receipt:
        raise RuntimeError('resident candidate is invalid or unstable')
    if len(snapshots) != 120:
        raise RuntimeError('incomplete simulated fleet snapshot')
    for core, snapshot in enumerate(snapshots):
        select_worker(core)
        again = struct.unpack('<16I', read(bar + 0xab000, 64))
        if (snapshot != again or snapshot[2:8] != (trigger, trigger, trigger, 1, 0, 0)
                or snapshot[15] != core):
            raise RuntimeError(f'core {core}: resident mailbox did not settle')
        if decode and core == 0:
            token, bits, sequence, magic, complement, observed = snapshot[9:15]
            expected_result = (0x31524135 ^ token ^ ((bits << 7) & 0xffffffff)
                               ^ (bits >> 25) ^ ((sequence * 0x9e3779b9) & 0xffffffff))
            if (token >= 248320 or sequence != first + count - 1 or magic != 0x31524135
                    or complement != ((~token) & 0xffffffff) or observed != expected_result
                    or bits & 0x7f800000 == 0x7f800000):
                raise RuntimeError('resident argmax result is invalid')
        elif any(snapshot[9:15]):
            raise RuntimeError(f'core {core}: unexpected mailbox result')
        sender = struct.unpack('<4I', read(bar + 0xab550, 16))
        sender_again = struct.unpack('<4I', read(bar + 0xab550, 16))
        sender_checksum = (0x31535335 ^ trigger ^ ((epoch << 7) & 0xffffffff)
                           ^ (epoch >> 25) ^ ((core * 0x9e3779b9) & 0xffffffff))
        if sender != sender_again or sender != (trigger, epoch, core, sender_checksum):
            raise RuntimeError(f'core {core}: resident sender receipt did not settle')
    return snapshots[0][9] if decode else None


def projection_reference(fixture, quant, input_offset, tile, rows=1920, weight_offset=0, input_rows=2560):
    """Independently decode byte-major Q4_K, Q5_K, Q6_K or Q8_0 on the host.

    This is an offline reference for the simulator fixture, not an inference
    path. The fixed synthetic fixture has exactly representable outputs;
    real model weights require a floating-point tolerance.
    """
    if input_rows <= 0 or input_rows % (32 if quant == 'q8' else 256):
        raise ValueError('invalid reference projection input width')
    inputs = struct.unpack_from(f'<{input_rows}f', fixture, input_offset)
    block_bytes = {'q4': 144, 'q5': 176, 'q6': 210, 'q8': 34}[quant]
    blocks = input_rows // (32 if quant == 'q8' else 256)
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
    group.add_argument('--dense-attention-fixture', type=Path,
                       help='run synthetic dense attention through the resident workers')
    group.add_argument('--recorded-layer-fixture', type=Path,
                       help='run the first recorded model layer using the production bank layout')
    group.add_argument('--silu-fixture', type=Path,
                       help='also execute the separately generated 120-core SiLU operation')
    group.add_argument('--rms-fixture', type=Path,
                       help='also execute the separately generated 120-core RMS normalization')
    parser.add_argument('--resident-corrupt-core', type=int,
                        help='corrupt one resident template to exercise its startup fault')
    parser.add_argument('--resident-fault-command', type=Path,
                        help='submit a resident command against deliberately invalid stream records')
    parser.add_argument('--resident-delta-sharded', action='store_true',
                        help='use sharded-delta stream templates for a resident fault test')
    parser.add_argument('--resident-export-state', type=Path,
                        help='save verified position-zero simulator state to a new file')
    parser.add_argument('--resident-import-state', type=Path,
                        help='use exported simulator state with position-zero templates')
    parser.add_argument('--resident-position-one-command', type=Path,
                        help='position-one command for a second simulator run with the same templates')
    parser.add_argument('--resident-export-layer-output', type=Path,
                        help='save the independently checked position-zero layer output to a new file')
    parser.add_argument('--recorded-layer-reference', type=Path,
                        help='compare the recorded model layer with a previously verified output')
    parser.add_argument('--recorded-token-reference', type=Path,
                        help='compare a full recorded decode token with an offline reference')
    parser.add_argument('--recorded-warm-layer-reference', type=Path,
                        help='compare a second resident decode layer output with an offline reference')
    parser.add_argument('--recorded-warm-token-reference', type=Path,
                        help='compare the second resident decode token with an offline reference')
    parser.add_argument('--ttsim-unprofiled', action='store_true',
                        help='explicitly allow the non-packageable simulator image without cycle profiling')
    parser.add_argument('--flash-hybrid', action='store_true',
                        help='explicitly allow the simulator-only hybrid attention image and fixture')
    args = parser.parse_args()
    if ((args.resident_export_state or args.resident_import_state or args.resident_position_one_command)
            and not args.resident_sequence_fixture):
        parser.error('resident state options require --resident-sequence-fixture')
    if bool(args.recorded_layer_fixture) != bool(args.recorded_layer_reference):
        parser.error('recorded layer requires both fixture and reference')
    if args.recorded_token_reference and not args.recorded_layer_fixture:
        parser.error('recorded token reference requires a recorded layer fixture')
    if (bool(args.recorded_warm_layer_reference) != bool(args.recorded_warm_token_reference)
            or (args.recorded_warm_layer_reference and not args.recorded_token_reference)):
        parser.error('warm recorded decode requires both warm references and a full decode fixture')
    if args.resident_export_layer_output and (not args.resident_sequence_fixture
            or args.resident_export_layer_output.exists() or args.resident_export_layer_output.is_symlink()):
        parser.error('layer output export requires a resident sequence and a new path')
    if bool(args.resident_import_state) != bool(args.resident_position_one_command):
        parser.error('resident state import requires a position-one command, and vice versa')
    if args.resident_export_state and args.resident_import_state:
        parser.error('export and import must run in separate simulator processes')
    if args.resident_export_state and (args.resident_export_state.exists()
                                       or args.resident_export_state.is_symlink()):
        parser.error('resident state output already exists')
    if args.resident_corrupt_core is not None and (not args.resident_templates
            or not 0 <= args.resident_corrupt_core < 120):
        parser.error('--resident-corrupt-core requires resident templates and a core in 0..119')
    if args.resident_fault_command and (not args.resident_templates or args.resident_corrupt_core is not None):
        parser.error('--resident-fault-command requires resident templates and no corrupt core')
    if args.resident_delta_sharded and not args.resident_fault_command:
        parser.error('--resident-delta-sharded requires a resident fault command')
    template_directory = (args.resident_sequence_fixture or args.delta_shards_fixture
                          or args.dense_attention_fixture or args.recorded_layer_fixture)
    template_path = template_directory / 'templates.bin' if template_directory else args.resident_templates
    resident_templates = template_path.read_bytes() if template_path else None
    if resident_templates is not None and len(resident_templates) != 120 * 512:
        raise ValueError('unexpected resident-template fixture size')
    image = args.image
    if args.ttsim_unprofiled != (image / 'ttsim-unprofiled.marker').is_file():
        raise ValueError('unprofiled simulator flag and image marker do not match')
    if args.flash_hybrid != (image / 'flash-hybrid.marker').is_file():
        raise ValueError('flash hybrid simulator flag and image marker do not match')
    if not (image / 'activation-sharing.marker').is_file():
        raise ValueError('expected sharing-enabled standalone service image')
    resident_shards = (args.resident_sequence_fixture is not None
                       and (args.resident_sequence_fixture / 'delta-params.bin').is_file())
    if bool(args.delta_shards_fixture or args.dense_attention_fixture or args.recorded_layer_fixture or resident_shards or args.resident_delta_sharded) != (image / 'delta-shard.marker').is_file():
        raise ValueError('delta shard fixture and service image do not match')
    dense_sharded = False
    dense_hybrid = False
    if args.dense_attention_fixture:
        metadata = (args.dense_attention_fixture / 'metadata.bin').read_bytes()
        if len(metadata) != 12:
            raise ValueError('unexpected dense attention metadata size')
        _, _, shard_flag = struct.unpack('<3I', metadata)
        if shard_flag not in (0, 1, 2):
            raise ValueError('invalid dense attention shard flag')
        dense_sharded = shard_flag == 1
        dense_hybrid = shard_flag == 2
    if dense_sharded != (image / 'dense-shard.marker').is_file():
        raise ValueError('dense shard fixture and service image do not match')
    recorded_hybrid = bool(args.recorded_layer_fixture
                           and (args.recorded_layer_fixture / 'flash-hybrid.marker').is_file())
    if (dense_hybrid or recorded_hybrid) != args.flash_hybrid:
        raise ValueError('hybrid dense fixture and service image do not match')
    boot = (image / 'boot.bin').read_bytes()
    text = (image / 'text.bin').read_bytes()
    dispatch = (image / 'dispatch.bin').read_bytes()
    flash = (image / 'flash.bin').read_bytes() if args.flash_hybrid else b''
    dispatch_limit = 0x7000 if resident_templates is not None else 0x5c00
    if boot != bytes.fromhex('6fc00600') or not 0 < len(text) <= 0xe000 or not 0 < len(dispatch) <= dispatch_limit:
        raise ValueError('invalid service image layout')
    if len(text) % 4 or len(dispatch) % 4:
        raise ValueError('unaligned service image')
    if args.flash_hybrid and (not flash or len(flash) > 0x8000 or len(flash) % 4):
        raise ValueError('invalid simulator-only flash code region')
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
            extra = ((0xd0000, flash),) if args.flash_hybrid else ()
            for address, data in ((0x6000, boot), (0x72000, text), (0xa0400, dispatch), *extra, *role_images):
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

        if args.recorded_layer_fixture:
            directory = args.recorded_layer_fixture
            command = (directory / 'command.bin').read_bytes()
            records = (directory / 'records.bin').read_bytes()
            entries = [line.split() for line in (directory / 'layout.txt').read_text().splitlines()]
            configs = [tuple(map(int, row[1:])) for row in entries if row and row[0] == 'C' and len(row) in (4, 5)]
            if len(configs) > 1:
                raise ValueError('duplicate recorded layer configuration')
            config = configs[0] if configs else (1, 17, 18, 0)
            layers, executed, record_count = config[:3]
            decode = len(config) == 4 and config[3] == 1
            if len(config) == 4 and config[3] not in (0, 1):
                raise ValueError('invalid recorded decode flag')
            full_prompt = layers == 32
            prompt_steps = 529 if args.flash_hybrid else 521
            decode_steps = prompt_steps + 3
            if (not 1 <= layers <= 32 or not 1 <= executed <= prompt_steps
                    or (decode and not full_prompt)
                    or bool(args.recorded_token_reference) != decode
                    or record_count != (decode_steps if decode else executed + (0 if full_prompt else 1))
                    or (full_prompt and executed != prompt_steps) or len(command) != 128
                    or len(records) != 120 * record_count * 2048):
                raise ValueError('unexpected recorded layer fixture size')
            weights = [tuple(map(int, row[1:])) for row in entries if row and row[0] == 'W' and len(row) == 5]
            streams = [tuple(map(int, row[1:])) for row in entries if row and row[0] == 'S' and len(row) == 4]
            outputs = [tuple(map(int, row[1:])) for row in entries if row and row[0] == 'O' and len(row) == 5]
            if len(configs) + len(weights) + len(streams) + len(outputs) != len(entries) or len(streams) != 120 or not outputs:
                raise ValueError('invalid recorded layer layout')
            routes = (786, 978, 1170, 785, 977, 1169, 1361)

            def select_bank(bank, address):
                if not 0 <= bank < len(routes) or not 0x04000000 <= address < 0xc4000000:
                    raise ValueError('invalid recorded bank address')
                route = routes[bank]
                select(2, address, route & 63, route >> 6)

            def write_bank(bank, address, payload):
                offset = 0
                while offset < len(payload):
                    current = address + offset
                    count = min(0x10000, len(payload) - offset, 0x200000 - (current & 0x1fffff))
                    select_bank(bank, current)
                    write(bar + 0x400000 + (current & 0x1fffff), payload[offset:offset + count])
                    offset += count

            def read_bank(bank, address, size):
                result = bytearray()
                while len(result) < size:
                    current = address + len(result)
                    count = min(0x10000, size - len(result), 0x200000 - (current & 0x1fffff))
                    select_bank(bank, current)
                    result.extend(read(bar + 0x400000 + (current & 0x1fffff), count))
                return result

            with (directory / 'weights.bin').open('rb') as weight_file:
                weight_size = weight_file.seek(0, 2)
                for bank, address, size, offset in weights:
                    if not size or offset + size > weight_size or address + size > 0xc4000000:
                        raise ValueError('invalid recorded weight range')
                    weight_file.seek(offset)
                    at = 0
                    while at < size:
                        current = address + at
                        count = min(0x10000, size - at, 0x200000 - (current & 0x1fffff))
                        payload = weight_file.read(count)
                        if len(payload) != count:
                            raise ValueError('truncated recorded weights')
                        select_bank(bank, current)
                        write(bar + 0x400000 + (current & 0x1fffff), payload)
                        at += count
            for expected_core, (core, bank, address) in enumerate(streams):
                if core != expected_core:
                    raise ValueError('recorded stream core order mismatch')
                write_bank(bank, address, records[core * record_count * 2048:(core + 1) * record_count * 2048])
            runs = 2 if args.recorded_warm_layer_reference else 1
            for run_number in range(runs):
                trigger, epoch = struct.unpack_from('<II', command, 8)
                layer_reference = (args.recorded_warm_layer_reference if run_number
                                   else args.recorded_layer_reference)
                token_reference = (args.recorded_warm_token_reference if run_number
                                   else args.recorded_token_reference)
                select_worker(0)
                write(bar + 0xab080, command[:64])
                write(bar + 0xab0c0, command[64:])
                steps = record_count if full_prompt else executed
                print(f'starting recorded {layers}-layer simulation ({steps} graph steps)', flush=True)
                for attempt in range(20000):
                    lib.libttsim_clock(100000)
                    snapshots = []
                    for core in range(120):
                        select_worker(core)
                        snapshots.append(struct.unpack('<16I', read(mailbox_address, 64)))
                    if full_prompt:
                        if any(words[5] in (4, 5) for words in snapshots):
                            raise RuntimeError('full recorded prompt retained a worker fault')
                        select_worker(0)
                        receipt = struct.unpack('<8I', read(bar + 0xab310, 32))
                        if receipt[7] == trigger:
                            token = audit_resident_candidate(receipt, command, snapshots, select_worker, read, bar, decode=decode)
                            if decode:
                                expected_token = int(token_reference.read_text().strip())
                                if token != expected_token:
                                    raise RuntimeError(f'recorded decode token {token} differs from reference {expected_token}')
                                print(f'recorded decode token {token} matches reference', flush=True)
                            break
                    elif all(words[5] == 4 for words in snapshots):
                        break
                    if not full_prompt and any(words[5] not in (1, 2, 4) for words in snapshots):
                        raise RuntimeError('unexpected worker state during recorded layer')
                    if attempt and attempt % 50 == 0:
                        select_worker(0)
                        progress = struct.unpack('<I', read(bar + 0xab300, 4))[0]
                        print(f'recorded graph: polling attempt {attempt}, progress=0x{progress:08x}', flush=True)
                else:
                    raise RuntimeError('recorded layer did not reach its expected terminal state')
                if not full_prompt:
                    for core, words in enumerate(snapshots):
                        if (words[2:8] != (1, 0, 1, 4, 4, 5)
                                or words[9:15] != (0x31464735, 1, executed, 120, 0x80000004, 0)
                                or words[15] != core):
                            raise RuntimeError(f'core {core}: recorded layer failed before {executed} barriers: {words}')
                result = bytearray(2560 * 4)
                next_row = 0
                for bank, address, first, rows in outputs:
                    if first != next_row or not rows:
                        raise ValueError('recorded output fragments not contiguous')
                    count = min(rows, max(0, 2560 - first))
                    if count:
                        result[first * 4:(first + count) * 4] = read_bank(bank, address, count * 4)
                    next_row += rows
                if next_row != 9216:
                    raise ValueError('incomplete recorded output layout')
                reference = layer_reference.read_bytes()
                if len(reference) != 2560 * 4:
                    raise ValueError('unexpected verified layer reference size')
                maximum = 0.0
                for row, ((got,), (want,)) in enumerate(zip(struct.iter_unpack('<f', result), struct.iter_unpack('<f', reference))):
                    error = abs(got - want)
                    if not math.isfinite(got) or not math.isfinite(want) or error > 2e-5 * max(1.0, abs(want)):
                        raise RuntimeError(f'recorded final row {row}: {got} differs from verified reference {want}')
                    maximum = max(maximum, error)
                select_worker(0)
                if not full_prompt and struct.unpack('<8I', read(bar + 0xab310, 32))[7] != 0:
                    raise RuntimeError('intentionally faulted recorded layer published a candidate')
                terminal = 'candidate audited' if full_prompt else f'record {record_count} intentionally faults'
                print(f'PASS actual recorded {layers}-layer prefix: 2560 values match verified reference; '
                      f'maximum absolute error {maximum:.9g}; {terminal}; NO HARDWARE', flush=True)
                if run_number == 0 and runs == 2:
                    # The result and all sender receipts have been audited above.
                    # Acknowledge only after comparing the layer and token, then
                    # publish a fresh ticket using the actual selected token.
                    command = resident_next_decode_command(command, token)
                    select_worker(0)
                    write(bar + 0xab330, struct.pack('<4I', trigger, (~trigger) & 0xffffffff,
                                                    epoch, (~epoch) & 0xffffffff))
                    print('acknowledged first decode; retaining model, fleet and state for position one', flush=True)
            if runs == 2:
                print('PASS two consecutive recorded full decodes on one simulated resident fleet; NO HARDWARE', flush=True)
            return

        if args.dense_attention_fixture:
            directory = args.dense_attention_fixture
            command = (directory / 'command.bin').read_bytes()
            records = (directory / 'records.bin').read_bytes()
            initial = (directory / 'initial.bin').read_bytes()
            expected_kv = (directory / 'expected-kv.bin').read_bytes()
            expected_output = (directory / 'expected-output.bin').read_bytes()
            metadata = (directory / 'metadata.bin').read_bytes()
            if len(metadata) != 12:
                raise ValueError('unexpected dense attention metadata size')
            position, context, shard_flag = struct.unpack('<3I', metadata)
            if not 0 <= position < context <= 64:
                raise ValueError('invalid dense attention position or context')
            if shard_flag != (2 if dense_hybrid else int(dense_sharded)):
                raise ValueError('dense attention fixture changed during startup')
            record_count = 6 if dense_hybrid else 5
            completed_steps = record_count - 1
            layouts = ((0x10000000, 8192 * 4), (0x11000000, 4096 * 4),
                       (0x17000000, 9216 * 4),
                       (0x14000000, 256 * 4), (0x15000000, 256 * 4),
                       (0x16000000, context * 64 * 4), (0x13000000, 8 * context * 256 * 2))
            if (len(command) != 128 or len(records) != 120 * record_count * 2048
                    or len(initial) != sum(size for _, size in layouts)
                    or len(expected_kv) != 8 * context * 256 * 2 or len(expected_output) != 9216 * 4):
                raise ValueError('unexpected dense attention fixture size')
            cursor = 0
            for address, size in layouts:
                for offset in range(0, size, 0x10000):
                    current = address + offset
                    select(2, current, 18, 12)
                    write(bar + 0x400000 + (current & 0x1fffff),
                          initial[cursor + offset:cursor + min(offset + 0x10000, size)])
                cursor += size
            select(2, 0x12000000, 18, 12)
            write(bar + 0x400000, bytes(9216 * 4))
            for core in range(120):
                address = 0x04000000 + core * 0x100000
                select(2, address, 18, 12)
                chunk = record_count * 2048
                write(bar + 0x400000 + (address & 0x1fffff), records[core * chunk:(core + 1) * chunk])
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
                    raise RuntimeError('unexpected worker state during dense attention')
            else:
                raise RuntimeError('dense attention did not reach its intentional terminal fault')
            for core, words in enumerate(snapshots):
                if (words[2:8] != (1, 0, 1, 4, 4, 5)
                        or words[9:15] != (0x31464735, 1, completed_steps, 120, 0x80000004, 0)
                        or words[15] != core):
                    raise RuntimeError(f'core {core}: dense attention failed before {completed_steps} barriers: {words}')
            select(2, 0x13000000, 18, 12)
            actual_kv = b''.join(read(bar + 0x400000 + offset, min(0x10000, len(expected_kv) - offset))
                                 for offset in range(0, len(expected_kv), 0x10000))
            if actual_kv != expected_kv:
                index = next(i for i, (got, want) in enumerate(zip(actual_kv, expected_kv)) if got != want)
                raise RuntimeError(f'dense KV byte {index}: differs from host reference')
            select(2, 0x12000000, 18, 12)
            actual_output = read(bar + 0x400000, len(expected_output))
            maximum = 0.0
            for index, ((got,), (want,)) in enumerate(zip(struct.iter_unpack('<f', actual_output),
                                                          struct.iter_unpack('<f', expected_output))):
                error = abs(got - want)
                if not math.isfinite(got) or error > 2e-5 * max(1.0, abs(want)):
                    raise RuntimeError(f'dense output {index}: {got} differs from host reference {want}')
                maximum = max(maximum, error)
            select_worker(0)
            if struct.unpack('<8I', read(bar + 0xab310, 32))[7] != 0:
                raise RuntimeError('intentionally faulted dense sequence published a candidate')
            print(f'PASS dense attention position {position}/{context}: all KV values and 9216 outputs checked; '
                      f'maximum absolute error {maximum:.9g}; record {record_count} intentionally faults; '
                      f'active key/value/query workers 4/4/{120 if dense_sharded or dense_hybrid else 16}; NO HARDWARE', flush=True)
            return

        if args.resident_sequence_fixture:
            directory = args.resident_sequence_fixture
            command = (directory / 'command.bin').read_bytes()
            records = (directory / 'records.bin').read_bytes()
            weights = (directory / 'weights.bin').read_bytes()
            norm = (directory / 'norm.bin').read_bytes()
            projection = (directory / 'projection.bin').read_bytes()
            gate = (directory / 'gate.bin').read_bytes()
            conv = (directory / 'conv.bin').read_bytes()
            history_input = (directory / 'history.bin').read_bytes()
            position_bytes = (directory / 'position.bin').read_bytes()
            delta_params_path, delta_state_path = directory / 'delta-params.bin', directory / 'delta-state.bin'
            if delta_params_path.exists() != delta_state_path.exists():
                raise ValueError('incomplete sharded recurrence fixture')
            sharded = delta_params_path.exists()
            if args.resident_export_layer_output and (not sharded or len(records) != 120 * 22 * 2048
                    or position_bytes != bytes(4)):
                raise ValueError('layer export requires the position-zero 22-record fixture')
            if (args.resident_import_state or args.resident_export_state) and not sharded:
                raise ValueError('state handoff requires the complete sharded layer fixture')
            full_prompt = (directory / 'full-prompt.marker').is_file()
            if full_prompt and (not sharded or args.resident_import_state or args.resident_export_state):
                raise ValueError('full-prompt fixture requires the standalone sharded path')
            record_count = 521 if full_prompt else 22 if sharded else 7
            if (len(command) != 128 or len(records) != 120 * record_count * 2048 or len(weights) != 33920
                    or hashlib.sha256(weights).hexdigest()
                    != 'd784c0b7fe8df8aab8d4797a8ea5a2c25b5db49a84c60a3ad93819086355871d'
                    or len(norm) != 10240 or hashlib.sha256(norm).hexdigest()
                    != 'fd26aeb9265ba4588eca258e4c7fb27d2b82198eef72bbca4d8d46136b90c104'
                    or len(projection) != 14604288 or len(gate) != 5898240
                    or hashlib.sha256(gate).hexdigest()
                    != '9e3596cdad68531922f9a5e8775e576d4254ba45f66281c203c3baa64d0cadbb'
                    or len(conv) != 131072 or hashlib.sha256(conv).hexdigest()
                    != 'e0ff35e774d276fa65d1ccde5e6667345ed74da153f906d27b688fda88f155cd'
                    or position_bytes not in (b'\x00\x00\x00\x00', b'\x01\x00\x00\x00')
                    or len(history_input) != 24576 * 4):
                raise ValueError('unexpected authenticated resident sequence fixture')
            position = position_bytes[0]
            warm = bool(args.resident_import_state)
            if args.resident_export_state and position != 0:
                raise ValueError('only position-zero results may be exported')
            if warm:
                if position != 0:
                    raise ValueError('state handoff must reuse position-zero templates and records')
                command = resident_position_one_command(command, args.resident_position_one_command.read_bytes())
                history_input, warm_delta_state = resident_handoff_payload(args.resident_import_state.read_bytes())
                position = 1
            history_prior = struct.unpack('<24576f', history_input)
            expected_history = tuple(((i * 7) % 29 - 14) * 0.0625 for i in range(24576))
            if not warm and history_prior != expected_history:
                raise ValueError('unexpected synthetic convolution history')
            if sharded:
                delta_params, delta_state = delta_params_path.read_bytes(), delta_state_path.read_bytes()
                if warm:
                    delta_state = warm_delta_state
                output_weight = (directory / 'recurrent-output-weight.bin').read_bytes()
                ffn_norm = (directory / 'ffn-norm.bin').read_bytes()
                ffn_weights = (directory / 'ffn-weights.bin').read_bytes()
                if (len(ffn_norm) != 10240 or hashlib.sha256(ffn_norm).hexdigest()
                        != '37ec1c3c62e961a58c845390743988f5e35a33aaa667a3fec5e60c43d01508ee'
                        or len(ffn_weights) != 46080000
                        or len(output_weight) != 7208960 or hashlib.sha256(output_weight).hexdigest()
                        != '25d73bfd7e111e9a0f06f78ed11e4531cf645c3909aa7fa954d414181404dd13'
                        or len(delta_params) != 768 or len(delta_state) != 32 * 128 * 128 * 4
                        or hashlib.sha256(delta_params[:128]).hexdigest()
                        != '9d3fe9c2ddd97e58d62c12f113d62456850618bbd9117d584064bf23eaa0c9e7'
                        or hashlib.sha256(delta_params[128:256]).hexdigest()
                        != '54314db72054ddcf664d15dd95ac238b9fac69e07d593b2d2531267e9edadaee'
                        or hashlib.sha256(delta_params[256:]).hexdigest()
                        != 'fecbf8328e83d0ed32ba85dcc096a922381da16fd22e430752a336bae198b701'):
                    raise ValueError('unexpected recurrent parameters or state size')
                for first, size, digest in ((0, 13271040, 'a1edf952279392b0bb90de57ffe3da54196cd68ecf33ae84ca262ac46801ac3a'),
                                            (13271040, 13271040, '5ec20ef4178d2397995cb5631c446bad2cc007142e6c9cb90e243c8727e83182'),
                                            (26542080, 19537920, 'dd954f3958a3c1db4f8f23b87fe4769f4811f4b16ea954c2da9b91de08038dd7')):
                    if hashlib.sha256(ffn_weights[first:first + size]).hexdigest() != digest:
                        raise ValueError('feed-forward weights do not match pinned model')
                old_state = struct.unpack('<524288f', delta_state)
                if not warm and any(value != ((i * 11) % 97 - 48) / 1024.0 for i, value in enumerate(old_state)):
                    raise ValueError('unexpected synthetic recurrent state')
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
            history_read, history_write = (0x22060000, 0x22040000) if warm else (0x22040000, 0x22060000)
            state_read, state_write = (0x25000000, 0x24000000) if warm else (0x24000000, 0x25000000)
            for address, payload in ((0x22000000, conv),
                                     (history_read, history_input),
                                     (history_write, struct.pack('<24576I', *([0x7fc00000] * 24576)))):
                select(2, address, 18, 12)
                write(bar + 0x400000 + (address & 0x1fffff), payload)
            if sharded:
                for address, payload in ((0x23000000, delta_params[:128]),
                                         (0x23001000, delta_params[128:256]),
                                         (0x23002000, delta_params[256:]),
                                         (state_read, delta_state),
                                         (state_write, struct.pack('<524288I', *([0x7fc00000] * 524288))),
                                         (0x2a000000, output_weight),
                                         (0x20050000, ffn_norm),
                                         (0x30000000, ffn_weights[:13271040]),
                                         (0x31000000, ffn_weights[13271040:26542080]),
                                         (0x32000000, ffn_weights[26542080:])):
                    offset = 0
                    while offset < len(payload):
                        current = address + offset
                        select(2, current, 18, 12)
                        count = min(0x10000, len(payload) - offset, 0x200000 - (current & 0x1fffff))
                        write(bar + 0x400000 + (current & 0x1fffff), payload[offset:offset + count])
                        offset += count
            for address in (0x20000000, 0x20010000, 0x20020000):
                select(2, address, 18, 12)
                write(bar + 0x400000 + (address & 0x1fffff),
                      struct.pack('<2560I', *([0x7fc00000] * 2560)))
            for address, rows in ((0x21000000, 8192), (0x21010000, 32), (0x21020000, 32),
                                  (0x21030000, 4096), (0x21040000, 2560), (0x21050000, 9216),
                                  (0x21060000, 2560), (0x21070000, 2560), (0x21080000, 2560)):
                select(2, address, 18, 12)
                write(bar + 0x400000 + (address & 0x1fffff),
                      struct.pack(f'<{rows}I', *([0x7fc00000] * rows)))
            for core in range(120):
                address = (0x40000000 + core * 0x200000) if full_prompt else (0x04000000 + core * 0x100000)
                select(2, address, 18, 12)
                write(bar + 0x400000 + (address & 0x1fffff), records[core * record_count * 2048:(core + 1) * record_count * 2048])
            commands = [command]
            if full_prompt:
                commands.append((directory / 'command-next.bin').read_bytes())
                validate_resident_prompt_commands(*commands)
            for run_number, command in enumerate(commands):
                trigger, epoch = struct.unpack_from('<II', command, 8)
                print(f'starting simulated prompt command {trigger}, epoch {epoch}', flush=True)
                select_worker(0)
                write(bar + 0xab080, command[:64])
                write(bar + 0xab0c0, command[64:])
                for attempt in range(10000 if full_prompt else 1000 if sharded else 200):
                    lib.libttsim_clock(100000)
                    snapshots = []
                    for core in range(120):
                        select_worker(core)
                        snapshots.append(struct.unpack('<16I', read(mailbox_address, 64)))
                    if full_prompt:
                        if any(words[5] in (4, 5) for words in snapshots):
                            raise RuntimeError('resident prompt retained a worker fault')
                        select_worker(0)
                        receipt = struct.unpack('<8I', read(bar + 0xab310, 32))
                        if receipt[7] == trigger:
                            audit_resident_candidate(receipt, command, snapshots, select_worker, read, bar)
                            break
                        if attempt and attempt % 50 == 0:
                            progress = struct.unpack('<I', read(bar + 0xab300, 4))[0]
                            print(f'prompt {trigger}: simulator polling attempt {attempt}, progress=0x{progress:08x}', flush=True)
                    else:
                        if all(words[5] == 4 for words in snapshots):
                            break
                        if any(words[5] not in (1, 2, 4) for words in snapshots):
                            raise RuntimeError('unexpected resident state during sequence')
                else:
                    raise RuntimeError('resident sequence did not reach its expected terminal state')
                if not full_prompt:
                    for core, words in enumerate(snapshots):
                        if (words[2:8] != (1, 0, 1, 4, 4, 5)
                                or words[9:15] != (0x31464735, 1, record_count - 1, 120, 0x80000004, 0)
                                or words[15] != core):
                            raise RuntimeError(f'core {core}: sequence failed before all {record_count - 1} barriers: {words}')
                for name, address in ((('embedding checkpoint', 0x21080000),) if sharded else
                                      (('embedding', 0x20000000), ('residual save', 0x20010000))):
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
                # The sharded graph copies RMS aside before the recurrent output
                # projection reuses the normalized activation buffer.
                address = 0x21040000 if sharded else 0x20020000
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
                # Each convolution history row ends with the original QKV input;
                # QKV itself is overwritten by the convolution output.
                select(2, history_write, 18, 12)
                history_bytes = read(bar + 0x400000 + (history_write & 0x1fffff), 24576 * 4)
                history = struct.unpack('<24576f', history_bytes)
                for i in range(8192):
                    old1 = history_prior[3 * i + 1] if position else 0.0
                    old2 = history_prior[3 * i + 2] if position else 0.0
                    if history[3 * i] != old1 or history[3 * i + 1] != old2:
                        raise RuntimeError(f'convolution history shift mismatch at row {i}')
                qkv_original = history[2::3]
                specs = (('QKV', 8192, 0x21000000, 'q5', 2816, 0),
                         ('Alpha', 32, 0x21010000, 'q8', 576, 14417920),
                         ('Beta', 32, 0x21020000, 'q8', 576, 14512128))
                for name, rows, address, quant, tile, weight_offset in specs:
                    values, norms = projection_reference(reference_fixtures[quant], quant, len(projection),
                                                         tile, rows, weight_offset)
                    select(2, address, 18, 12)
                    actual = (qkv_original if name == 'QKV' else
                              struct.unpack(f'<{rows}f', read(bar + 0x400000 + (address & 0x1fffff), rows * 4)))
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
                select(2, 0x21000000, 18, 12)
                conv_actual = struct.unpack('<8192f', read(bar + 0x400000 + (0x21000000 & 0x1fffff), 8192 * 4))
                conv_weights = struct.unpack('<32768f', conv)
                maximum = 0.0
                conv_reference = []
                for row, (got, current) in enumerate(zip(conv_actual, qkv_original)):
                    terms = [history_prior[3 * row + i] * conv_weights[4 * row + i] if position else 0.0
                             for i in range(3)]
                    value = sum(terms) + current * conv_weights[4 * row + 3]
                    want = value / (1.0 + math.exp(-value))
                    conv_reference.append(want)
                    # Sharded recurrence overwrites the V portion after convolution.
                    if not sharded or row < 4096:
                        error = abs(got - want)
                        maximum = max(maximum, error)
                        if not math.isfinite(got) or error > 2e-6 * max(1.0, abs(want)):
                            raise RuntimeError(f'convolution row {row}: {got} differs from {want}; error {error}')
                checked = 4096 if sharded else 8192
                print(f'convolution: {checked} retained values and 24576 history entries match independent '
                      f'position-{position} reference; maximum absolute error {maximum:.9g}', flush=True)
                if sharded:
                    negative_a = struct.unpack('<32f', delta_params[:128])
                    bias = struct.unpack('<32f', delta_params[128:256])
                    delta_norm = struct.unpack('<128f', delta_params[256:])
                    select(2, 0x21010000, 18, 12)
                    alpha = struct.unpack('<32f', read(bar + 0x400000 + (0x21010000 & 0x1fffff), 128))
                    select(2, 0x21020000, 18, 12)
                    beta_logit = struct.unpack('<32f', read(bar + 0x400000 + (0x21020000 & 0x1fffff), 128))
                    state_bytes = bytearray()
                    for offset in range(0, len(delta_state), 0x10000):
                        current = state_write + offset
                        select(2, current, 18, 12)
                        state_bytes.extend(read(bar + 0x400000 + (current & 0x1fffff),
                                                min(0x10000, len(delta_state) - offset)))
                    state_actual = struct.unpack('<524288f', state_bytes)
                    state_max = output_max = 0.0
                    for head in range(32):
                        key_head = head % 16
                        query = conv_reference[key_head * 128:(key_head + 1) * 128]
                        key = conv_reference[2048 + key_head * 128:2048 + (key_head + 1) * 128]
                        query_scale = 1.0 / max(math.sqrt(sum(x * x for x in query)), 1e-6) / math.sqrt(128)
                        key_scale = 1.0 / max(math.sqrt(sum(x * x for x in key)), 1e-6)
                        query = [x * query_scale for x in query]
                        key = [x * key_scale for x in key]
                        shifted = alpha[head] + bias[head]
                        softplus = max(shifted, 0.0) + math.log1p(math.exp(-abs(shifted)))
                        decay = math.exp(negative_a[head] * softplus)
                        beta = 1.0 / (1.0 + math.exp(-beta_logit[head]))
                        raw_output = []
                        for lane in range(128):
                            base = (head * 128 + lane) * 128
                            prior = old_state[base:base + 128] if position else [0.0] * 128
                            prediction = sum(prior[k] * decay * key[k] for k in range(128))
                            correction = (conv_reference[4096 + head * 128 + lane] - prediction) * beta
                            expected_state = [prior[k] * decay + key[k] * correction for k in range(128)]
                            raw_output.append(sum(expected_state[k] * query[k] for k in range(128)))
                            for k, want in enumerate(expected_state):
                                got = state_actual[base + k]
                                error = abs(got - want)
                                state_max = max(state_max, error)
                                if not math.isfinite(got) or error > 2e-5 * max(1.0, abs(want)):
                                    raise RuntimeError(f'recurrent state {head}:{lane}:{k}: {got} differs from {want}')
                        rms_scale = 1.0 / math.sqrt(sum(x * x for x in raw_output) / 128 + 1e-6)
                        for lane, raw in enumerate(raw_output):
                            gate_value = gate_actual[head * 128 + lane]
                            want = raw * rms_scale * delta_norm[lane] * gate_value / (1.0 + math.exp(-gate_value))
                            got = conv_actual[4096 + head * 128 + lane]
                            error = abs(got - want)
                            output_max = max(output_max, error)
                            if not math.isfinite(got) or error > 2e-4 * max(1.0, abs(want)):
                                raise RuntimeError(f'recurrent output {head}:{lane}: {got} differs from {want}')
                    print(f'sharded recurrence: 524288 state values and 4096 outputs match independent '
                          f'position-{position} reference; max absolute errors {state_max:.9g}, {output_max:.9g}', flush=True)
                    recurrent_input = struct.pack('<4096f',
                        *quantized_activation_reference(conv_actual[4096:], 'q5'))
                    reference, norms = projection_reference(output_weight + recurrent_input, 'q5',
                        len(output_weight), 2816, 2560, input_rows=4096)
                    select(2, 0x21060000, 18, 12)
                    output = struct.unpack('<2560f', read(bar + 0x400000 + (0x21060000 & 0x1fffff), 2560 * 4))
                    maximum = worst = 0.0
                    for row, (got, want, magnitude) in enumerate(zip(output, reference, norms)):
                        error = abs(got - want)
                        ratio = error / max(1.0, magnitude)
                        maximum, worst = max(maximum, error), max(worst, ratio)
                        if not math.isfinite(got) or ratio > 2e-6:
                            raise RuntimeError(f'recurrent projection row {row}: {got} differs from {want}')
                    print(f'recurrent output projection: 2560 values match independent Q5_K reference; '
                          f'maximum absolute error {maximum:.9g}, normalized error {worst:.9g}', flush=True)
                    select(2, 0x20010000, 18, 12)
                    residual_saved = struct.unpack('<2560f', read(bar + 0x400000 + (0x20010000 & 0x1fffff), 10240))
                    for row, (got, embedding_value, projected) in enumerate(zip(residual_saved, expected, output)):
                        want = embedding_value + projected
                        if not math.isfinite(got) or abs(got - want) > 2e-6 * max(1.0, abs(want)):
                            raise RuntimeError(f'attention residual row {row}: {got} differs from {want}')
                    print('attention residual: 2560 values match independent sum', flush=True)
                    select(2, 0x21070000, 18, 12)
                    ffn_rms = struct.unpack('<2560f', read(bar + 0x400000 + (0x21070000 & 0x1fffff), 10240))
                    post_weights = struct.unpack('<2560f', ffn_norm)
                    rms_scale = 1.0 / math.sqrt(sum(value * value for value in residual_saved) / 2560 + 1e-6)
                    maximum = 0.0
                    for row, (got, value, weight) in enumerate(zip(ffn_rms, residual_saved, post_weights)):
                        want = value * rms_scale * weight
                        maximum = max(maximum, abs(got - want))
                        if not math.isfinite(got) or abs(got - want) > 2e-5 * max(1.0, abs(want)):
                            raise RuntimeError(f'post-attention RMS row {row}: {got} differs from {want}')
                    print(f'post-attention RMS: 2560 values match independent reference; maximum error {maximum:.9g}', flush=True)
                    ffn_input = struct.pack('<2560f', *quantized_activation_reference(ffn_rms, 'q4'))
                    ffn_fixture = ffn_weights + ffn_input
                    gate_ref, gate_norms = projection_reference(ffn_fixture, 'q4', len(ffn_weights), 2304, 9216, 0)
                    up_ref, up_norms = projection_reference(ffn_fixture, 'q4', len(ffn_weights), 2304, 9216, 13271040)
                    select(2, 0x21050000, 18, 12)
                    up_actual = struct.unpack('<9216f', read(bar + 0x400000 + (0x21050000 & 0x1fffff), 9216 * 4))
                    select(2, 0x20020000, 18, 12)
                    silu_actual = struct.unpack('<9216f', read(bar + 0x400000 + (0x20020000 & 0x1fffff), 9216 * 4))
                    up_worst = silu_worst = 0.0
                    for row, (up, want_up, up_magnitude, ffn_gate, gate_magnitude, result) in enumerate(
                            zip(up_actual, up_ref, up_norms, gate_ref, gate_norms, silu_actual)):
                        ratio = abs(up - want_up) / max(1.0, up_magnitude)
                        up_worst = max(up_worst, ratio)
                        want = ffn_gate / (1.0 + math.exp(-ffn_gate)) * up
                        scale = max(1.0, abs(want), gate_magnitude * abs(up))
                        error = abs(result - want) / scale
                        silu_worst = max(silu_worst, error)
                        if not math.isfinite(up) or ratio > 2e-6 or not math.isfinite(result) or error > 2e-5:
                            raise RuntimeError(f'FFN gate/up/SiLU row {row}: output {result}, expected {want}; '
                                               f'up normalized error {ratio}, activation normalized error {error}')
                    print(f'FFN up and gated SiLU: 9216 values match independent references; '
                          f'maximum normalized errors {up_worst:.9g}, {silu_worst:.9g}', flush=True)
                    down_input = struct.pack('<9216f', *quantized_activation_reference(silu_actual, 'q6'))
                    down_ref, down_norms = projection_reference(ffn_weights + down_input, 'q6',
                        len(ffn_weights), 3392, 2560, 26542080, input_rows=9216)
                    select(2, 0x20000000, 18, 12)
                    final = struct.unpack('<2560f', read(bar + 0x400000 + (0x20000000 & 0x1fffff), 10240))
                    worst = maximum = 0.0
                    for row, (got, down, magnitude, skip) in enumerate(zip(final, down_ref, down_norms, residual_saved)):
                        want = down + skip
                        error = abs(got - want)
                        ratio = error / max(1.0, magnitude, abs(want))
                        worst, maximum = max(worst, ratio), max(maximum, error)
                        if not math.isfinite(got) or ratio > 2e-6:
                            raise RuntimeError(f'FFN down/residual row {row}: {got} differs from {want}')
                    print(f'FFN down and final residual: 2560 values match independent Q6_K reference; '
                          f'maximum absolute error {maximum:.9g}, normalized error {worst:.9g}', flush=True)
                    if args.resident_export_layer_output:
                        payload = struct.pack('<2560f', *final)
                        with args.resident_export_layer_output.open('xb') as stream:
                            stream.write(payload)
                        print('exported independently checked layer output: sha256=' + hashlib.sha256(payload).hexdigest(), flush=True)
                select_worker(0)
                if not full_prompt and struct.unpack('<8I', read(bar + 0xab310, 32))[7] != 0:
                    raise RuntimeError('intentionally faulted sequence published a candidate')
                if warm:
                    if history_bytes == history_input or bytes(state_bytes) == delta_state:
                        raise RuntimeError('second position did not change the imported state')
                    source = 'resident' if full_prompt else 'imported'
                    print(f'position-one graph reused the same templates and updated the {source} simulator state', flush=True)
                if args.resident_export_state:
                    payload = history_bytes + bytes(state_bytes)
                    with args.resident_export_state.open('xb') as stream:
                        stream.write(b'Q35SIMW1' + hashlib.sha256(payload).digest() + payload)
                    print(f'exported verified position-zero simulator state: sha256={hashlib.sha256(payload).hexdigest()}', flush=True)
                if full_prompt:
                    print(f'PASS prompt {trigger}: checked 120 worker mailboxes and sender receipts plus layer outputs; NO HARDWARE', flush=True)
                    if run_number == 0:
                        history_input, delta_state = history_bytes, bytes(state_bytes)
                        history_prior = struct.unpack('<24576f', history_input)
                        old_state = struct.unpack('<524288f', delta_state)
                        history_read, history_write = history_write, history_read
                        state_read, state_write = state_write, state_read
                        position, warm = 1, True
                        select_worker(0)
                        write(bar + 0xab330, struct.pack('<4I', trigger, (~trigger)&0xffffffff,
                                                        epoch, (~epoch)&0xffffffff))
                    continue
                print(f'PASS resident graph completed {record_count - 1} barriers on 120 simulated workers; '
                      f'record {record_count} intentionally faults; NO HARDWARE')
            if full_prompt:
                print('PASS two consecutive prompt commands on one simulated resident fleet; '
                      '500 diagnostic-copy steps per command, NOT A FULL MODEL', flush=True)
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
