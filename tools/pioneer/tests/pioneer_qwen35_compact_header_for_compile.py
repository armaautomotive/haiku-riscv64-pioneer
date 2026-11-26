#!/usr/bin/env python3
"""Emit a strictly non-loadable header for compiling against a compact image."""
import argparse
from pathlib import Path
from struct import iter_unpack, unpack


def generate(directory: Path, qualification: bool = False) -> str:
    required = ('profiled-compile-only.marker', 'compact-two-region.marker',
                'activation-sharing.marker', 'delta-shard.marker', 'flash-hybrid.marker',
                'ffn-pair.marker', 'fused-rms-save.marker')
    if any(not (directory / marker).is_file() for marker in required):
        raise ValueError('missing compact compile-only image marker')
    if any((directory / name).exists() for name in ('ttsim-unprofiled.marker', 'ttsim-qualification.marker',
                                                     'dense-shard.marker', 'flash.bin')):
        raise ValueError('simulator or extra-region image is not a compact driver compile input')
    marker = directory / 'compact-qualification.marker'
    if (marker.is_symlink() or (qualification and not marker.is_file())
            or (not qualification and marker.exists())):
        raise ValueError('compact qualification marker does not match image type')
    boot = (directory / 'boot.bin').read_bytes()
    text = (directory / 'text.bin').read_bytes()
    dispatch = (directory / 'dispatch.bin').read_bytes()
    if boot != bytes.fromhex('6fc00600'):
        raise ValueError('unexpected compact boot jump')
    if not 0 < len(text) <= 0xe000 or len(text) % 4:
        raise ValueError('compact text exceeds its aligned L1 range')
    if not 0 < len(dispatch) <= 0x9c00 or len(dispatch) % 4:
        raise ValueError('compact dispatch exceeds its aligned L1 range')
    symbols = {}
    for row in (directory / 'symbols.txt').read_text(encoding='ascii').splitlines():
        parts = row.split()
        if len(parts) == 3 and parts[1] == 'T':
            value, _, name = parts
            if name in symbols:
                raise ValueError('duplicate text symbol: ' + name)
            symbols[name] = int(value, 16)
    if symbols.get('qwen35_dataflow_entry') != 0x72000:
        raise ValueError('unexpected compact entry address')
    park_name = 'qwen35_dataflow_stopped_park' if qualification else 'qwen35_resident_graph_stopped_park'
    main_name = 'qwen35_dataflow_service_main' if qualification else 'qwen35_resident_graph_service_main'
    other_main = 'qwen35_resident_graph_service_main' if qualification else 'qwen35_dataflow_service_main'
    if other_main in symbols:
        raise ValueError('compact image contains the wrong service entry point')
    park = symbols.get(park_name, 0)
    offset = park - 0x72000
    if offset < 0 or offset % 4 or offset + 4 > len(text) or text[offset:offset + 4] != bytes.fromhex('6f000000'):
        raise ValueError('unexpected compact stopped-park instruction')
    for name in (main_name, 'qwen35_graph_run_descriptor',
                 'qwen35_shared_compute_coordinate', 'qwen35_graph_execute'):
        address = symbols.get(name, 0)
        if address % 4 or not (0x72000 <= address < 0x72000 + len(text)
                                or 0xa0400 <= address < 0xa0400 + len(dispatch)):
            raise ValueError('missing or out-of-range compact symbol: ' + name)
    required_defines = ('PIONEER_QWEN35_COMPACT_DRIVER_COMPILE_ONLY', 'PIONEER_QWEN35_DATAFLOW_REWRITE',
                        'PIONEER_QWEN35_DEVICE_DISPATCH',
                        'PIONEER_QWEN35_RESIDENT_SERVICE_WITNESS', 'PIONEER_QWEN35_RECOVERY_COMPILE_ONLY',
                        'PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS', 'PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS',
                        'PIONEER_QWEN35_SHARED_ROLE_GRAPH_WITNESS', 'PIONEER_QWEN35_ACTIVATION_SHARING_WITNESS',
                        'PIONEER_QWEN35_PAIRED_RECURRENCE', 'PIONEER_QWEN35_FUSED_RECURRENCE',
                        'PIONEER_QWEN35_DELTA_SHARD_COMPILE_ONLY', 'PIONEER_QWEN35_FLASH_DECODE_COMPILE_ONLY',
                        'PIONEER_QWEN35_FFN_PAIR_COMPILE_ONLY', 'PIONEER_QWEN35_FUSED_RMS_SAVE_COMPILE_ONLY',
                        'PIONEER_QWEN35_TREE_SETTLEMENT')
    forbidden = ('PIONEER_QWEN35_RECOVERY_QUALIFY', 'PIONEER_QWEN35_DEVICE_DISPATCH_TRIAL',
                 'PIONEER_QWEN35_TTSIM_UNPROFILED_COMPILE_ONLY', 'PIONEER_QWEN35_DENSE_SHARD_COMPILE_ONLY')
    condition = ' || '.join([f'!defined({name})' for name in required_defines]
                            + [f'defined({name})' for name in forbidden])

    def words(data):
        return ''.join(f'\t0x{word:08x}U,\n' for word, in iter_unpack('<I', data))

    name_prefix = 'DataflowQualificationService' if qualification else 'DataflowService'
    return ('#pragma once\n#if ' + condition + '\n'
            '#error "compact image is restricted to the non-loadable driver compile test"\n'
            '#endif\nnamespace PioneerQwen35 {\n'
            f'inline constexpr unsigned {name_prefix}{"BootWord" if qualification else "ImageBootWord"}=0x{unpack("<I", boot)[0]:08x}U;\n'
            f'inline constexpr unsigned {name_prefix}StoppedParkAddress=0x{park:08x}U;\n'
            f'inline constexpr unsigned {name_prefix}TextWords[]={{\n' + words(text) + '};\n'
            f'inline constexpr unsigned {name_prefix}DispatchWords[]={{\n' + words(dispatch) + '};\n}\n')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('image', type=Path)
    parser.add_argument('header', type=Path)
    parser.add_argument('--qualification', action='store_true')
    args = parser.parse_args()
    header = generate(args.image, args.qualification)
    with args.header.open('x', encoding='ascii', newline='\n') as output:
        output.write(header)


if __name__ == '__main__':
    main()
