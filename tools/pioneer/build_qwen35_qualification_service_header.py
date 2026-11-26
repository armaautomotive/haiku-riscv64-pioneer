#!/usr/bin/env python3
"""Package the standalone recovery qualification image for a guarded build.

This packages linked bytes only. It does not authorize execution or establish
physical ownership, matrix recovery, or correctness of the qualification.
"""

import argparse
from pathlib import Path
from struct import iter_unpack, unpack


def generate(directory: Path) -> str:
    if (directory / 'compact-two-region.marker').exists():
        raise ValueError('compact two-region simulator image is not packageable')
    if (directory / 'fused-rms-save.marker').exists():
        raise ValueError('fused RMS/save simulator image is not packageable')
    if (directory / 'ffn-pair.marker').exists():
        raise ValueError('paired FFN simulator image is not packageable')
    if (directory / 'flash-hybrid.marker').exists():
        raise ValueError('flash hybrid witness must not be packaged')
    if (directory / 'ttsim-unprofiled.marker').exists():
        raise ValueError('unprofiled simulator image cannot be packaged')
    if (directory / 'dense-shard.marker').exists():
        raise ValueError('dense shard image is an isolated simulator witness and cannot be packaged')
    for marker in ('activation-sharing.marker', 'fused-recurrence.marker'):
        if not (directory / marker).is_file():
            raise ValueError('missing qualification image marker: ' + marker)
    sharded = (directory / 'delta-shard.marker').is_file()
    boot = (directory / 'boot.bin').read_bytes()
    text = (directory / 'text.bin').read_bytes()
    dispatch = (directory / 'dispatch.bin').read_bytes()
    if boot != bytes.fromhex('6fc00600'):
        raise ValueError('unexpected qualification boot jump')
    if not 0 < len(text) <= 0xe000 or len(text) % 4:
        raise ValueError('qualification text does not fit its aligned L1 range')
    if not 0 < len(dispatch) <= 0x5c00 or len(dispatch) % 4:
        raise ValueError('qualification dispatch does not fit its aligned L1 range')

    symbols = {}
    for row in (directory / 'symbols.txt').read_text(encoding='ascii').splitlines():
        parts = row.split()
        if len(parts) == 3 and parts[1] == 'T':
            value, _, name = parts
            if name in symbols:
                raise ValueError('duplicate text symbol: ' + name)
            symbols[name] = int(value, 16)
    if symbols.get('qwen35_dataflow_entry') != 0x72000:
        raise ValueError('unexpected qualification entry address')
    if 'qwen35_resident_graph_service_main' in symbols:
        raise ValueError('resident entry point in qualification image')
    park = symbols.get('qwen35_dataflow_stopped_park', 0)
    offset = park - 0x72000
    if offset < 0 or offset % 4 or offset + 4 > len(text) or text[offset:offset + 4] != bytes.fromhex('6f000000'):
        raise ValueError('unexpected qualification stopped-park instruction')
    for name in ('qwen35_dataflow_service_main', 'qwen35_shared_compute_coordinate', 'qwen35_graph_execute'):
        address = symbols.get(name, 0)
        if address % 4 or not (0x72000 <= address < 0x72000 + len(text)
                                or 0xa0400 <= address < 0xa0400 + len(dispatch)):
            raise ValueError('missing or out-of-range qualification symbol: ' + name)

    def words(data: bytes) -> str:
        return ''.join(f'\t0x{word:08x}U,\n' for word, in iter_unpack('<I', data))

    return (
        '#pragma once\n'
        '#if !defined(PIONEER_QWEN35_RESIDENT_SERVICE_WITNESS) || '
        '!defined(PIONEER_QWEN35_RECOVERY_COMPILE_ONLY) || '
        '!defined(PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS) || '
        '!defined(PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS) || '
        '!defined(PIONEER_QWEN35_ACTIVATION_SHARING_WITNESS) || '
        + ('!defined(PIONEER_QWEN35_DELTA_SHARD_COMPILE_ONLY) || ' if sharded
           else 'defined(PIONEER_QWEN35_DELTA_SHARD_COMPILE_ONLY) || ')
        + 'defined(PIONEER_QWEN35_RECOVERY_QUALIFY) || '
        'defined(PIONEER_QWEN35_DEVICE_DISPATCH_TRIAL)\n'
        '#error "qualification image is restricted to the non-loadable sharing witness"\n'
        '#endif\nnamespace PioneerQwen35 {\n'
        f'inline constexpr unsigned DataflowQualificationServiceBootWord=0x{unpack("<I", boot)[0]:08x}U;\n'
        f'inline constexpr unsigned DataflowQualificationServiceStoppedParkAddress=0x{park:08x}U;\n'
        'inline constexpr unsigned DataflowQualificationServiceTextWords[]={\n' + words(text) + '};\n'
        'inline constexpr unsigned DataflowQualificationServiceDispatchWords[]={\n' + words(dispatch) + '};\n}\n'
    )


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('image', type=Path)
    parser.add_argument('header', type=Path)
    args = parser.parse_args()
    header = generate(args.image)
    with args.header.open('x', encoding='ascii', newline='\n') as output:
        output.write(header)


if __name__ == '__main__':
    main()
